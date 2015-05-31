#include <definitions/notificationdataroles.h>
#include <definitions/optionvalues.h>
#include <utils/options.h>

#include "dbuspopuphandler.h"
#include "definitions.h"

DbusPopupHandler::DbusPopupHandler()
{
	FAvatars = NULL;
	FNotifications = NULL;
	FNotifyInterface = NULL;

#ifdef DEBUG_RESOURCES_DIR
		FileStorage::setResourcesDirs(FileStorage::resourcesDirs() << DEBUG_RESOURCES_DIR);
#endif
}


DbusPopupHandler::~DbusPopupHandler()
{
	delete FNotifyInterface;
}

void DbusPopupHandler::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Dbus Popup Notifications Handler");
	APluginInfo->description = tr("Allows other modules use DBus to show notifications");
	APluginInfo->version = "1.3";
	APluginInfo->author = "Crying Angel";
	APluginInfo->homePage = "http://code.google.com/p/vacuum-plugins/";
	APluginInfo->dependences.append(NOTIFICATIONS_UUID);
}

bool DbusPopupHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{

	IPlugin *plugin = APluginManager->pluginInterface("IAvatars").value(0, NULL);
	if(plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0, NULL);
	if(plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
	}

	connect(APluginManager->instance(),SIGNAL(aboutToQuit()),this,SLOT(onApplicationQuit()));

	return FNotifications !=NULL;
}

bool DbusPopupHandler::initObjects()
{
	FNotifyInterface = new QDBusInterface("org.freedesktop.Notifications",
										  "/org/freedesktop/Notifications",
										  "org.freedesktop.Notifications",
										  QDBusConnection::sessionBus(), this);
	if(FNotifyInterface->lastError().type() != QDBusError::NoError)
		return false;

	connect(FNotifyInterface,SIGNAL(ActionInvoked(uint,QString)),this,SLOT(onActionInvoked(uint,QString)));
	connect(FNotifyInterface,SIGNAL(NotificationClosed(uint,uint)),this,SLOT(onNotificationClosed(uint,uint)));

	FNotifications->insertNotificationHandler(NHO_DBUSPOPUP, this);

	return true;
}

bool DbusPopupHandler::showNotification(int AOrder, ushort AKind, int ANotifyId, const INotification &ANotification)
{
	if (AOrder != NHO_DBUSPOPUP || !(AKind & INotification::PopupWindow))
		return false;

	Jid contactJid = ANotification.data.value(NDR_CONTACT_JID).toString();
	QString imgPath;
	QString iconPath;

	if (FAvatars)
		imgPath = FAvatars->avatarFileName(FAvatars->avatarHash(contactJid));

	QList<QVariant> notifyArgs;
	notifyArgs.append("Vacuum-IM");
	notifyArgs.append((uint)0);
	notifyArgs.append(iconPath);
	notifyArgs.append(ANotification.data.value(NDR_TOOLTIP).toString());

	QString popupBody = ANotification.data.value(NDR_POPUP_TEXT).toString();
	notifyArgs.append(popupBody);

	QStringList actions;
	actions << "action_show" << tr("Show");
	actions << "action_ignore" << tr("Ignore");
	notifyArgs.append(actions);

	QVariantMap hints;
	if (!imgPath.isEmpty())
	{
		hints.insert("image_path", imgPath);
		hints.insert("suppress-sound", true);
	}
	notifyArgs.append(hints);
	notifyArgs.append(Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).value().toInt()*1000);

	QDBusReply<uint> reply = FNotifyInterface->callWithArgumentList(QDBus::Block, "Notify", notifyArgs);
	FNotifies.insert(reply.value(), ANotifyId);

	return true;
}

void DbusPopupHandler::onActionInvoked(uint dbusNotifyId, QString action)
{
	if (action == "action_show")
		FNotifications->activateNotification(FNotifies.value(dbusNotifyId));
	else
		FNotifications->removeNotification(FNotifies.value(dbusNotifyId));

	FNotifyInterface->call("CloseNotification", dbusNotifyId);
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

Q_EXPORT_PLUGIN2(plg_notifications, DbusPopupHandler)
