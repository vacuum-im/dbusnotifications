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

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>

#include <interfaces/ipluginmanager.h>
#include <interfaces/inotifications.h>
#include <interfaces/ioptionsmanager.h>
#include <interfaces/iavatars.h>
#include "idbusnotifications.h"

class DbusPopupHandler :
		public QObject,
		public IPlugin,
		public INotificationHandler,
		public IOptionsHolder,
		public IDBusNotifications
{
	Q_OBJECT
	Q_INTERFACES(IPlugin INotificationHandler IOptionsHolder IDBusNotifications)
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
	//IOptionsHolder
	virtual QMultiMap<int, IOptionsWidget *> optionsWidgets(const QString &ANodeId, QWidget *AParent);
	//INotificationHandler
	virtual bool showNotification(int AOrder, ushort AKind, int ANotifyId, const INotification &ANotification);

protected:
	QString filter(const QString &text);
	void closeNotification(const unsigned int notifyId);

protected slots:
	void onOptionsOpened();
	void onOptionsChanged(const OptionsNode &ANode);
	void onActionInvoked(unsigned int notifyId, QString action);
	void onApplicationQuit();

private:
	IAvatars *FAvatars;
	INotifications *FNotifications;
	IOptionsManager *FOptionsManager;
	QDBusInterface *FNotifyInterface;

	ServerInfo *FServerInfo;

	QMap <int, int> FNotifies;
	bool FAllowActions;
};

#endif // DBUSPOPUPHANDLER_H
