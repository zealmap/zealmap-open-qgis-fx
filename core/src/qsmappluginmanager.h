#ifndef QsMapPLUGINMANAGER_H
#define QsMapPLUGINMANAGER_H

#include <map>

#include <QString>
#include <QLibrary>

#include <qgsproviderguimetadata.h>
#include <qgsprovidermetadata.h>

#include <qgsfeedback.h>
#include <qgsprovidersublayerdetails.h>

class QsMapPluginManager {
public:
  static QsMapPluginManager* instance();

  void loadCppPlugin(const QString& mDirPath);
  void registerGuis(QMainWindow* parent);

  QList< QgsProviderSublayerDetails > querySublayers(const QString& uri
    , Qgis::SublayerQueryFlags flags = Qgis::SublayerQueryFlags()
    , QgsFeedback* feedback = nullptr) const;
protected:
  //! protected constructor
  QsMapPluginManager() = default;
  QsMapPluginManager(QsMapPluginManager const&) = delete;
  QsMapPluginManager& operator=(QsMapPluginManager const&) = delete;

private:
  const QgsProviderMetadata* findMetadata_(const QString& providerKey);
  const QgsProviderGuiMetadata* findMetadata_gui(const QString& providerKey);

  void resolve_providerMetadataFactory(QLibrary& myLib);
  void resolve_providerGuiMetadataFactory(QLibrary& myLib);

private:
  static QsMapPluginManager* sInstance;

  //std::map<QString, QgsProviderMetadata*> m_qProviders;
  std::map<QString, QgsProviderGuiMetadata*> m_guiProviders;
};

#endif
