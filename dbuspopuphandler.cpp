#include "dbuspopuphandler.h"

DbusPopupHandler::DbusPopupHandler()
{
        FAvatars = NULL;
        FNotifications = NULL;
        FNotify = NULL;
}

DbusPopupHandler::~DbusPopupHandler()
{
        delete FNotify;
}

void DbusPopupHandler::pluginInfo(IPluginInfo *APluginInfo)
{
        APluginInfo->name = tr("Dbus Popup Notifications Handler");
        APluginInfo->description = tr("Allows other modules use DBus to show notifications");
        APluginInfo->version = "1.0";
        APluginInfo->author = "Crying Angel";
        APluginInfo->homePage = "http://www.vacuum-im.org";
        APluginInfo->dependences.append(NOTIFICATIONS_UUID);
        APluginInfo->dependences.append(AVATARTS_UUID);
}

bool DbusPopupHandler::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
        IPlugin *plugin;

        plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
        if (!plugin) return false;
                FAvatars = qobject_cast<IAvatars *>(plugin->instance());

        plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
        if (!plugin) return false;
                FNotifications = qobject_cast<INotifications *>(plugin->instance());

//        connect (FNotifications->instance(), SIGNAL(notificationRemoved(int)),this,SLOT(onWindowNotifyRemoved(int)));

        return true;
}

bool DbusPopupHandler::initObjects()
{
        FNotify = new QDBusInterface("org.freedesktop.Notifications",
                                     "/org/freedesktop/Notifications","org.freedesktop.Notifications",
                                     QDBusConnection::sessionBus()/*, this*/);
        if(FNotify->lastError().type() != QDBusError::NoError)
        {
               qWarning() << "DBus Notifys: Unable to create interface.";
               return false;
        }
        FUseFreedesktopSpec = true;
        qDebug() << "DBus Notifys: DBus interface created successfully.";

//        connect(FNotify,SIGNAL(NotificationClosed(uint,uint)),this,SLOT(onNotifyClosed(uint,uint)));
        connect(FNotify,SIGNAL(ActionInvoked(uint,QString)),this,SLOT(onActionInvoked(uint,QString)));

        FNotifications->insertNotificationHandler(NHO_DBUSPOPUP, this);

        return true;
}

bool DbusPopupHandler::initSettings()
{
        FTimeout = 6000;
        FUpdateNotify = false;
        return true;
}

bool DbusPopupHandler::showNotification(int AOrder, uchar AKind, int ANotifyId, const INotification &ANotification)
{
    if (AOrder!=NHO_DBUSPOPUP||!(AKind&INotification::PopupWindow)) return false;

    Jid contactJid = ANotification.data.value(NDR_CONTACT_JID).toString();
    QString toolTip = ANotification.data.value(NDR_TOOLTIP).toString();
    QString imgPath;
    QString iconPath;

    if (FAvatars)
    {
        imgPath = FAvatars->avatarFileName(FAvatars->avatarHash(contactJid));
    }

    QList<QVariant> notifyArgs;
    notifyArgs.append("Vacuum IM");                                             //app-name
    notifyArgs.append((uint)ANotifyId);                                         //id;
    if(!FUseFreedesktopSpec)
        notifyArgs.append("");                                                  //event-id
    notifyArgs.append(iconPath);                                                //app-icon(path to icon on disk)
    notifyArgs.append(ANotification.data.value(NDR_POPUP_CAPTION).toString());  //summary (notification title)
    if(FUseFreedesktopSpec)
        notifyArgs.append(ANotification.data.value(NDR_TOOLTIP).toString()+":\n"+ANotification.data.value(NDR_POPUP_TEXT).toString());   //body
    QStringList acts;
    acts  << "actOne" << tr("Show");
    acts  << "actTwo" << tr("Ignore");
    notifyArgs.append(acts);                                                    //actions
    QVariantMap hints;
    if (!imgPath.isEmpty())
    {
        hints.insert("image_path", imgPath);
    }
    notifyArgs.append(hints);                                                   //hints
    notifyArgs.append(FTimeout);                                                //timeout

    QDBusReply<uint> reply = FNotify->callWithArgumentList(QDBus::Block,"Notify",notifyArgs);
    if(reply.isValid())
    {
        if (FUpdateNotify)
        {
            ANotifyId = reply.value();
        }
    }
    else
    {
        qWarning() << "DBus Notifys Error: " << reply.error();
    };

    return true;
}

void DbusPopupHandler::onActionInvoked(unsigned int notifyId, QString action)
{
        qDebug() << "DBus Notifys: action "<<action<<" invoked";
        if (action=="actOne")
            FNotifications->activateNotification(notifyId);
        else
            FNotifications->removeNotification(notifyId);
}

void DbusPopupHandler::onWindowNotifyRemoved(/*int ANotifyId*/)
{

}

//void DbusPopupHandler::onWindowNotifyDestroyed()
//{

//}

Q_EXPORT_PLUGIN2(plg_notifications, DbusPopupHandler)

