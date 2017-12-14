#include <definitions/notificationdataroles.h>
#include <definitions/notificationtypes.h>
#include <definitions/optionvalues.h>
#include <utils/options.h>

#include "dbuspopuphandler.h"
#include "definitions.h"
#include <utils/logger.h>

DbusPopupHandler::DbusPopupHandler()
{
	FAvatars = NULL;
	FNotifications = NULL;
	FNotifyInterface = NULL;

#ifdef DEBUG_RESOURCES_DIR
	FileStorage::setResourcesDirs(FileStorage::resourcesDirs() << DEBUG_RESOURCES_DIR);
#endif

	QDBusConnection::sessionBus().connect("org.freedesktop.DBus",
										  "/org/freedesktop/DBus",
										  "org.freedesktop.DBus",
										  "NameOwnerChanged",
										  this,
										  SLOT(onNameOwnerChanged(QString, QString, QString)));
}

DbusPopupHandler::~DbusPopupHandler()
{
	QDBusConnection::sessionBus().disconnect("org.freedesktop.DBus",
											 "/org/freedesktop/DBus",
											 "org.freedesktop.DBus",
											 "NameOwnerChanged",
											 this,
											 SLOT(onNameOwnerChanged(QString, QString, QString)));
	delete FNotifyInterface;
}

void DbusPopupHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Dbus Popup Notifications Handler");
	APluginInfo->description = tr("Allows other modules use DBus to show notifications");
	APluginInfo->version = "1.5.2";
	APluginInfo->author = "Crying Angel";
	APluginInfo->homePage = "https://github.com/Vacuum-IM/dbusnotifications";
	APluginInfo->dependences.append(NOTIFICATIONS_UUID);
}

bool DbusPopupHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IAvatars").value(0, NULL);
	if(plugin)
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());

	plugin = APluginManager->pluginInterface("INotifications").value(0, NULL);
	if(plugin)
		FNotifications = qobject_cast<INotifications *>(plugin->instance());

	connect(APluginManager->instance(),SIGNAL(aboutToQuit()),this,SLOT(onApplicationQuit()));

	return FNotifications!=NULL && FAvatars!=NULL;
}

bool DbusPopupHandler::initObjects()
{
	if (!initNotifyInterface())
		return false;

	FServerInfo = new ServerInfo;

	QDBusMessage replyCaps = FNotifyInterface->call(QDBus::Block, "GetCapabilities");
	if (replyCaps.type() != QDBusMessage::ErrorMessage)
	{
		for (int i=0; i<replyCaps.arguments().at(0).toStringList().count(); i++)
			LOG_INFO(QString("Capabilities: %1").arg(replyCaps.arguments().at(0).toStringList().at(i)));
		FServerInfo->capabilities = replyCaps.arguments().at(0).toStringList();
		FAllowActions = FServerInfo->capabilities.contains("actions");
	}
	else
		LOG_WARNING(QString("Capabilities: DBus Error: %1").arg(replyCaps.errorMessage()));

	QDBusMessage replyInfo = FNotifyInterface->call(QDBus::Block, "GetServerInformation");
	if (replyInfo.type() != QDBusMessage::ErrorMessage)
	{
		for (int i=0; i<replyInfo.arguments().count(); i++)
			LOG_INFO(QString("Server Information: %1").arg(replyInfo.arguments().at(i).toString()));

		FServerInfo->name = replyInfo.arguments().at(0).toString();
		FServerInfo->vendor = replyInfo.arguments().at(1).toString();
		FServerInfo->version = replyInfo.arguments().at(2).toString();
		FServerInfo->spec_version = replyInfo.arguments().at(3).toString();

	}
	else
		LOG_WARNING(QString("Server Information: DBus Error: %1").arg(replyInfo.errorMessage()));

	if (FAllowActions)
	{
		connect(FNotifyInterface,SIGNAL(ActionInvoked(uint,QString)),this,SLOT(onActionInvoked(uint,QString)));
		connect(FNotifyInterface,SIGNAL(NotificationClosed(uint,uint)),this,SLOT(onNotificationClosed(uint,uint)));
		LOG_INFO(QLatin1String("Actions supported."));
	}
	else
		LOG_INFO(QLatin1String("Actions not supported."));

	FNotifications->insertNotificationHandler(NHO_DBUSPOPUP, this);

	return true;
}

bool DbusPopupHandler::initNotifyInterface() {
	FNotifyInterface = new QDBusInterface("org.freedesktop.Notifications",
										  "/org/freedesktop/Notifications",
										  "org.freedesktop.Notifications",
										  QDBusConnection::sessionBus(),
										  this);
	if(FNotifyInterface->lastError().type() != QDBusError::NoError)
	{
		LOG_WARNING(QString("Unable to create QDBusInterface: %1.").arg(FNotifyInterface->lastError().message()));
		return false;
	}
	else
	{
		LOG_INFO(QLatin1String("QDBusInterface created successfully."));
		return true;
	}
}

bool DbusPopupHandler::showNotification(int AOrder, ushort AKind, int ANotifyId, const INotification &ANotification)
{
	if (!FNotifyInterface)
		return false;
	if (AOrder != NHO_DBUSPOPUP || !(AKind & INotification::PopupWindow))
		return false;

	Jid contactJid = ANotification.data.value(NDR_CONTACT_JID).toString();
	QString imgPath;
	QString iconPath;

	if (FAvatars)
		imgPath = FAvatars->avatarFileName(FAvatars->avatarHash(contactJid));

	QList<QVariant> notifyArgs;
	notifyArgs.append(QLatin1String("Vacuum-IM"));
	notifyArgs.append(static_cast<uint>(0));
	notifyArgs.append(iconPath);
	notifyArgs.append(ANotification.data.value(NDR_TOOLTIP).toString());

	QString popupBody = ANotification.data.value(NDR_POPUP_TEXT).toString();
	notifyArgs.append(popupBody.toHtmlEscaped());

	QStringList actions;
	if (FAllowActions && NNT_CONNECTION_ERROR != ANotification.typeId)
	{
		actions << QLatin1String("action_show") << tr("Show");
		actions << QLatin1String("action_ignore") << tr("Ignore");
	}
	notifyArgs.append(actions);

	QVariantMap hints;
	if (!imgPath.isEmpty())
	{
		hints.insert(QLatin1String("image_path"), imgPath);
		hints.insert(QLatin1String("suppress-sound"), true);
	}
	hints.insert(QLatin1String("category"), QLatin1String("im"));
	notifyArgs.append(hints);
	notifyArgs.append(Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).value().toInt()*1000);

	QDBusReply<uint> reply = FNotifyInterface->callWithArgumentList(QDBus::Block, QLatin1String("Notify"), notifyArgs);
	FNotifies.insert(reply.value(), ANotifyId);

	return true;
}

void DbusPopupHandler::onActionInvoked(uint dbusNotifyId, QString action)
{
	if (action == QLatin1String("action_show"))
		FNotifications->activateNotification(FNotifies.value(dbusNotifyId));
	else
		FNotifications->removeNotification(FNotifies.value(dbusNotifyId));

	FNotifyInterface->call(QLatin1String("CloseNotification"), dbusNotifyId);
}

void DbusPopupHandler::onNotificationClosed(uint dbusNotifyId, uint reason)
{
	if (reason == 2) /*  2 - The notification was dismissed by the user. */
		FNotifications->removeNotification(FNotifies.value(dbusNotifyId));
}

void DbusPopupHandler::onApplicationQuit()
{
	FNotifications->removeNotificationHandler(NHO_DBUSPOPUP, this);
}

void DbusPopupHandler::onNameOwnerChanged(QString AName, QString /*empty*/, QString /*ANewOwner*/) {
	if (AName != QLatin1Literal("org.freedesktop.Notifications")) {
		return;
	}

	initNotifyInterface();
}
