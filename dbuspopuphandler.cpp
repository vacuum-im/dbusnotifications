#include <QDebug>
#include <definitions/notificationdataroles.h>
#include <definitions/optionvalues.h>
#include <utils/options.h>

#include "dbuspopuphandler.h"
#include "definitions.h"

DbusPopupHandler::DbusPopupHandler()
{
	FAvatars = NULL;
	FNotifications = NULL;
	FOptionsManager = NULL;
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
	APluginInfo->version = "1.2.2";
	APluginInfo->author = "Crying Angel";
	APluginInfo->homePage = "http://code.google.com/p/vacuum-plugins/";
	APluginInfo->dependences.append(NOTIFICATIONS_UUID);
	APluginInfo->dependences.append(OPTIONSMANAGER_UUID);
}

bool DbusPopupHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{

	IPlugin *plugin = APluginManager->pluginInterface("IOptionsManager").value(0);
	if(plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
			connect(Options::instance(),SIGNAL(optionsChanged(OptionsNode)),SLOT(onOptionsChanged(OptionsNode)));
		}
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0, NULL);
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

	return FOptionsManager != NULL && FNotifications !=NULL;
}

bool DbusPopupHandler::initObjects()
{
	FNotifyInterface = new QDBusInterface("org.freedesktop.Notifications",
										  "/org/freedesktop/Notifications",
										  "org.freedesktop.Notifications",
										  QDBusConnection::sessionBus()/*, this*/);
	if(FNotifyInterface->lastError().type() != QDBusError::NoError)
	{
		qWarning() << "DBusNotifications: Unable to create interface.";
		return false;
	}
	else
		qDebug() << "DBusNotifications: DBus interface created successfully.";

	FServerInfo = new ServerInfo;

	QDBusMessage replyCaps = FNotifyInterface->call(QDBus::Block, "GetCapabilities");
	if (QDBusMessage::ErrorMessage != replyCaps.type())
	{
		qDebug() << "GetCapabilities:" << replyCaps.arguments().at(0).toStringList();
		FServerInfo->capabilities = replyCaps.arguments().at(0).toStringList();
		FAllowActions = FServerInfo->capabilities.contains("actions");
	}
	else
		qWarning() << "GetCapabilities: DBus Error: " << replyCaps.errorMessage();

	QDBusMessage reply = FNotifyInterface->call(QDBus::Block,"GetServerInformation");
	if (QDBusMessage::ErrorMessage != reply.type())
	{
		for (int i=0; i<reply.arguments().count(); i++)
		{
			qDebug() << "GetServerInformation:" << reply.arguments().at(i).toString();
		};
		FServerInfo->name = reply.arguments().at(0).toString();
		FServerInfo->vendor = reply.arguments().at(1).toString();
		FServerInfo->version = reply.arguments().at(2).toString();
		FServerInfo->spec_version = reply.arguments().at(3).toString();
		FServerInfo->prety_name = QString("%1 %2 %3").arg(FServerInfo->name).arg(FServerInfo->version).arg(FServerInfo->vendor);
	}
	else
		qWarning() << "GetServerInformation: DBus Error: " << reply.errorMessage();

	if (FAllowActions)
		connect(FNotifyInterface,SIGNAL(ActionInvoked(uint,QString)),this,SLOT(onActionInvoked(uint,QString)));

	FNotifications->insertNotificationHandler(NHO_DBUSPOPUP, this);

	return true;
}

bool DbusPopupHandler::initSettings()
{
	Options::setDefaultValue(OPV_DP_ALLOW_ACTIONS, FAllowActions);

	if (FOptionsManager)
	{
		IOptionsDialogNode dnode = { ONO_DBUSPOPUP, OPN_DBUSPOPUP, tr("DBus Popup"), MNI_DBUSPOPUP };
		FOptionsManager->insertOptionsDialogNode(dnode);
		FOptionsManager->insertOptionsDialogHolder(this);
	}
	return true;
}

void DbusPopupHandler::onOptionsOpened()
{
	onOptionsChanged(Options::node(OPV_DP_ALLOW_ACTIONS));
}

void DbusPopupHandler::onOptionsChanged(const OptionsNode &ANode)
{
	if (ANode.path() == OPV_DP_ALLOW_ACTIONS)
	{
		FAllowActions = Options::node(OPV_DP_ALLOW_ACTIONS).value().toBool();
	}
}

QMultiMap<int, IOptionsDialogWidget *> DbusPopupHandler::optionsDialogWidgets(const QString &ANodeId, QWidget *AParent)
{
	QMultiMap<int, IOptionsDialogWidget *> widgets;
	if (FOptionsManager && ANodeId==OPN_DBUSPOPUP)
	{
		widgets.insertMulti(OWO_DBUSPOPUP, FOptionsManager->newOptionsDialogHeader(tr("Notifications show provider: %1").arg(FServerInfo->prety_name),AParent));
		widgets.insertMulti(OWO_DBUSPOPUP, FOptionsManager->newOptionsDialogWidget(Options::node(OPV_DP_ALLOW_ACTIONS), tr("Allow actions in notifications"), AParent));
	}
	return widgets;
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
	notifyArgs.append((uint)ANotifyId);
	notifyArgs.append(iconPath);
	notifyArgs.append(ANotification.data.value(NDR_TOOLTIP).toString());

	QString popupBody = ANotification.data.value(NDR_POPUP_TEXT).toString();
	notifyArgs.append(popupBody);

	QStringList actions;
	if (FAllowActions)
	{
		actions << "action_show" << tr("Show");
		actions << "action_ignore" << tr("Ignore");
	}
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
	if (reply.isValid())
		FNotifies.insert(reply.value(), ANotifyId);
	else
		qWarning() << "DBusNotifications Error: " << reply.error();

	return true;
}

void DbusPopupHandler::onActionInvoked(unsigned int notifyId, QString action)
{
	if (action == "action_show")
		FNotifications->activateNotification(FNotifies.value((int)notifyId));
	else
		FNotifications->removeNotification((int)notifyId);

	FNotifyInterface->call("CloseNotification", notifyId);
}

void DbusPopupHandler::onApplicationQuit()
{
	FNotifications->removeNotificationHandler(NHO_DBUSPOPUP, this);
}

Q_EXPORT_PLUGIN2(plg_notifications, DbusPopupHandler)
