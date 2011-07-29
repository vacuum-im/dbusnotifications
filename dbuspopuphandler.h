//   Plugin popup notifications for vacuum-im (c) Crying Angel, 2010
//   This plugin uses DBus to show notifications.

//   This library is free software; you can redistribute it and/or
//   modify it under the terms of the GNU Library General Public
//   License version 2 or later as published by the Free Software Foundation.
//
//   This library is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//   Library General Public License for more details.
//
//   You should have received a copy of the GNU Library General Public License
//   along with this library; see the file COPYING.LIB.  If not, write to
//   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//   Boston, MA 02110-1301, USA.

#ifndef DBUSPOPUPHANDLER_H
#define DBUSPOPUPHANDLER_H

#include <QDebug>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <definitions/notificationdataroles.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/iavatars.h>
#include <definitions/optionvalues.h>
#include <utils/options.h>

#ifdef SVNINFO
#  include "svninfo.h"
#else
#  define SVN_REVISION              "0"
#endif

#define DBUSPOPUPHANDLER_UUID  "{63685fbc-5a2d-4e8c-b9d5-d69ea8fbdb4e}"
#define NHO_DBUSPOPUP              3000

class DbusPopupHandler :
                        public QObject,
                        public IPlugin,
                        public INotificationHandler
{
        Q_OBJECT;
        Q_INTERFACES(IPlugin INotificationHandler);
public:
        DbusPopupHandler();
        ~DbusPopupHandler();
        //IPlugin
        virtual QObject *instance() { return this; }
        virtual QUuid pluginUuid() const { return DBUSPOPUPHANDLER_UUID; }
        virtual void pluginInfo(IPluginInfo *APluginInfo);
        virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
        virtual bool initObjects();
        virtual bool initSettings();
        virtual bool startPlugin() { return true; }
        //INotificationHandler
        virtual bool showNotification(int AOrder, ushort AKind, int ANotifyId, const INotification &ANotification);

protected slots:
        void onActionInvoked(unsigned int notifyId, QString action);
        void onWindowNotifyRemoved(/*int ANotifyId*/);
        void onApplicationQuit ();

private:
        IAvatars *FAvatars;
        INotifications *FNotifications;
        QDBusInterface *FNotify;

        int FTimeout;
        bool FUpdateNotify;
        bool FUseFreedesktopSpec;
        bool FRemoveTags;
};

#endif // DBUSPOPUPHANDLER_H

