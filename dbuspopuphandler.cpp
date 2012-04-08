#ifndef QT_NO_DEBUG
#  include <QDebug>
#endif

#include <definitions/notificationdataroles.h>
#include <definitions/optionvalues.h>
#include <utils/options.h>

#include "dbuspopuphandler.h"

DbusPopupHandler::DbusPopupHandler() :
    FAvatars(NULL),
    FNotifications(NULL),
    FOptionsManager(NULL),
    FNotify(NULL)
{

}

DbusPopupHandler::~DbusPopupHandler()
{
    delete FNotify;
}

void DbusPopupHandler::pluginInfo(IPluginInfo *APluginInfo)
{
    APluginInfo->name = tr("Dbus Popup Notifications Handler");
    APluginInfo->description = tr("Allows other modules use DBus to show notifications");
    APluginInfo->version = "1.0.2";
    APluginInfo->author = "Crying Angel";
    APluginInfo->homePage = "http://www.vacuum-im.org";
    APluginInfo->dependences.append(NOTIFICATIONS_UUID);
    APluginInfo->dependences.append(AVATARTS_UUID);
    APluginInfo->dependences.append(OPTIONSMANAGER_UUID);
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

    plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
    if (!plugin) return false;
    FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());

    connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
    connect(Options::instance(),SIGNAL(optionsChanged(const OptionsNode &)),SLOT(onOptionsChanged(const OptionsNode &)));

    connect (APluginManager->instance(), SIGNAL(aboutToQuit()), this, SLOT(onApplicationQuit()));
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
#ifndef QT_NO_DEBUG
        qWarning() << "DBus Notifys: Unable to create interface.";
#endif
        return false;
    }
    FUseFreedesktopSpec = true;
    FRemoveTags = false;
    FAllowActions = true;
#ifndef QT_NO_DEBUG
    qDebug() << "DBus Notifys: DBus interface created successfully.";
#endif

    QDBusMessage reply = FNotify->call(QDBus::Block,"GetServerInformation");
    if (QDBusMessage::ErrorMessage != reply.type())
    {
#ifndef QT_NO_DEBUG
        for (int i=0; i<reply.arguments().count(); i++)
        {
            qDebug() << reply.arguments().at(i).toString();
        };
#endif
        FServerName = reply.arguments().at(0).toString();
        FServerVendor = reply.arguments().at(1).toString();
        FServerVersion = reply.arguments().at(2).toString();

        if (FServerVendor == "GNOME")
        {
            FRemoveTags = true;
        }
        else if (FServerName == "naughty")
        {
            FRemoveTags = true;
        }

        if (FServerName == "notify-osd")
        {
            FAllowActions = false;
        }
    }
#ifndef QT_NO_DEBUG
    else
    {
        qWarning() << "DBus Error: " << reply.errorMessage();
    }
#endif

    //        connect(FNotify,SIGNAL(NotificationClosed(uint,uint)),this,SLOT(onNotifyClosed(uint,uint)));
    connect(FNotify,SIGNAL(ActionInvoked(uint,QString)),this,SLOT(onActionInvoked(uint,QString)));

    FNotifications->insertNotificationHandler(NHO_DBUSPOPUP, this);

    return true;
}

bool DbusPopupHandler::initSettings()
{
    FTimeout = 6000;
    FUpdateNotify = false;

    Options::setDefaultValue(OPV_DP_ALLOW_ACTIONS, FAllowActions);
    Options::setDefaultValue(OPV_DP_REMOVE_TAGS, FRemoveTags);

    if (FOptionsManager)
    {
        IOptionsDialogNode dnode = { ONO_DBUSPOPUP, OPN_DBUSPOPUP, tr("DBus Popup"), MNI_DBUSPOPUP };
        FOptionsManager->insertOptionsDialogNode(dnode);
        FOptionsManager->insertOptionsHolder(this);
    }
    return true;
}

void DbusPopupHandler::onOptionsOpened()
{
    onOptionsChanged(Options::node(OPV_DP_ALLOW_ACTIONS));
    onOptionsChanged(Options::node(OPV_DP_REMOVE_TAGS));
}

void DbusPopupHandler::onOptionsChanged(const OptionsNode &ANode)
{
    if (ANode.path() == OPV_DP_ALLOW_ACTIONS) {
        FAllowActions = Options::node(OPV_DP_ALLOW_ACTIONS).value().toBool();
    } else if (ANode.path() == OPV_DP_REMOVE_TAGS) {
        FRemoveTags = Options::node(OPV_DP_REMOVE_TAGS).value().toBool();
    }
}

QMultiMap<int, IOptionsWidget *> DbusPopupHandler::optionsWidgets(const QString &ANodeId, QWidget *AParent)
{
    QMultiMap<int, IOptionsWidget *> widgets;
    if (FOptionsManager && ANodeId==OPN_DBUSPOPUP)
    {
        widgets.insertMulti(OWO_DBUSPOPUP,
                            FOptionsManager->optionsHeaderWidget(tr("Notifications show provider: %1").arg(FServerVendor+" "+FServerName+" "+FServerVersion),AParent));
        widgets.insertMulti(OWO_DBUSPOPUP,
                            FOptionsManager->optionsNodeWidget(Options::node(OPV_DP_ALLOW_ACTIONS),
                            tr("Allow actions in notifications"),
                            AParent));
        widgets.insertMulti(OWO_DBUSPOPUP,
                            FOptionsManager->optionsNodeWidget(Options::node(OPV_DP_REMOVE_TAGS),
                            tr("Remove html tags"),
                            AParent));
    }
    return widgets;
}

bool DbusPopupHandler::showNotification(int AOrder, ushort AKind, int ANotifyId, const INotification &ANotification)
{
    if (AOrder != NHO_DBUSPOPUP || !(AKind & INotification::PopupWindow))
        return false;

#ifndef QT_NO_DEBUG
    qDebug() << "DBus Notifys: showNotification request accepted.";
#endif

    Jid contactJid = ANotification.data.value(NDR_CONTACT_JID).toString();
    QString imgPath;
    QString iconPath;

    FTimeout = Options::node(OPV_NOTIFICATIONS_POPUPTIMEOUT).value().toInt()*1000;

#ifndef QT_NO_DEBUG
    qDebug() << "NDR_TOOLTIP" << ANotification.data.value(NDR_TOOLTIP).toString();
    qDebug() << "NDR_POPUP_CAPTION" << ANotification.data.value(NDR_POPUP_CAPTION).toString();
    qDebug() << "NDR_POPUP_TITLE" << ANotification.data.value(NDR_POPUP_TITLE).toString();
    qDebug() << "NDR_POPUP_HTML" << ANotification.data.value(NDR_POPUP_HTML).toString();
#endif

    if (FAvatars)
    {
        imgPath = FAvatars->avatarFileName(FAvatars->avatarHash(contactJid));
    }

    QList<QVariant> notifyArgs;
    notifyArgs.append("Vacuum IM");                                             //app-name
    notifyArgs.append((uint)ANotifyId);                                         //id;
    //    if(!FUseFreedesktopSpec)
    //        notifyArgs.append("");                                                  //event-id
    notifyArgs.append(iconPath);                                                //app-icon(path to icon on disk)
    notifyArgs.append(ANotification.data.value(NDR_TOOLTIP).toString());  //summary (notification title)
    //    if(FUseFreedesktopSpec)
    if (FRemoveTags)
    {
        QString popupBody = ANotification.data.value(NDR_POPUP_HTML).toString();
        popupBody = popupBody.replace("<br />", "\n");
#ifndef QT_NO_DEBUG
        qDebug() << "NDR_POPUP_HTML trimed 1" << popupBody;
#endif
        popupBody = popupBody.replace(QRegExp("<[^>]*>"), "");
#ifndef QT_NO_DEBUG
        qDebug() << "NDR_POPUP_HTML trimed 2" << popupBody;
#endif
        notifyArgs.append(popupBody);
    }
    else
    {
        notifyArgs.append(/*ANotification.data.value(NDR_TOOLTIP).toString()+":\n"+*/ANotification.data.value(NDR_POPUP_HTML).toString());
    }
    //body
    QStringList acts;
    if (FAllowActions)
    {
        acts  << "actOne" << tr("Show");
        acts  << "actTwo" << tr("Ignore");
    }
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
#ifndef QT_NO_DEBUG
    else
    {
        qWarning() << "DBus Notifys Error: " << reply.error();
    }
#endif
    return true;
}

void DbusPopupHandler::onActionInvoked(unsigned int notifyId, QString action)
{
#ifndef QT_NO_DEBUG
    qDebug() << "DBus Notifys: action "<<action<<" invoked";
#endif
    if (action=="actOne")
        FNotifications->activateNotification(notifyId);
    else
        FNotifications->removeNotification(notifyId);

    FNotify->call("CloseNotification",notifyId);
}

void DbusPopupHandler::onWindowNotifyRemoved(/*int ANotifyId*/)
{

}

void DbusPopupHandler::onApplicationQuit()
{
    FNotifications->removeNotificationHandler(NHO_DBUSPOPUP, this);
}

//void DbusPopupHandler::onWindowNotifyDestroyed()
//{

//}

Q_EXPORT_PLUGIN2(plg_notifications, DbusPopupHandler)

