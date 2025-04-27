#ifndef QGSXYZCONNECTIONUTILS_H
#define QGSXYZCONNECTIONUTILS_H

#include <QStringList>

struct QgsXyzConnection
{
    QString name;
    QString url;
    int zMin = -1;
    int zMax = -1;
    // Authentication configuration id
    QString authCfg;
    // HTTP Basic username
    QString username;
    // HTTP Basic password
    QString password;
    // Referer
    QString referer;
    // tile pixel ratio (0 = unknown (not scaled), 1.0 = 256x256, 2.0 = 512x512)
    double tilePixelRatio = 0;
    bool hidden = false;

    QString encodedUri() const;
};

//! Utility class for handling list of connections to XYZ tile layers
class QgsXyzConnectionUtils
{
public:
    //! Returns list of existing connections, unless the hidden ones
    static QStringList connectionList();

    //! Returns last used connection
    static QString selectedConnection();

    //! Saves name of the last used connection
    static void setSelectedConnection(const QString& connName);

    //! Returns connection details
    static QgsXyzConnection connection(const QString& name);

    //! Removes a connection from the list
    static void deleteConnection(const QString& name);

    //! Adds a new connection to the list
    static void addConnection(const QgsXyzConnection& conn);
};


#endif // QGSXYZCONNECTIONUTILS_H

