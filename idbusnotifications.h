#ifndef IDBUSNOTIFICATIONS_H
#define IDBUSNOTIFICATIONS_H

#include <QString>

#define DBUSPOPUPHANDLER_UUID "{63685fbc-5a2d-4e8c-b9d5-d69ea8fbdb4e}"
#define PEP_USERMOOD 4010

struct ServerInfo
{
	QString name;
	QString vendor;
	QString version;
	QString spec_version;
	QString prety_name;
	QStringList capabilities;
};


class IDBusNotifications
{
public:
		virtual QObject *instance() = 0;
};

Q_DECLARE_INTERFACE(IDBusNotifications,"Vacuum.ExternalPlugin.IDBusNotifications/0.1")


#endif // IDBUSNOTIFICATIONS_H
