#include "qsmappluginmanager.h"

#include <QDir>
#include <QRegularExpression>
#include <QApplication>
#include <QCoreApplication>
#include <qgis.h>
#include <qgsruntimeprofiler.h>
#include <qgslogger.h>

#include <qgsproviderregistry.h>

QsMapPluginManager* QsMapPluginManager::sInstance = nullptr;
QsMapPluginManager* QsMapPluginManager::instance()
{
	if (!sInstance)
	{
		static QMutex sMutex;
		const QMutexLocker locker(&sMutex);
		if (!sInstance)
		{
			sInstance = new QsMapPluginManager();
		}
	}
	return sInstance;
}

void QsMapPluginManager::loadCppPlugin(const QString& mDirPath)
{
	QApplication* app = qobject_cast<QApplication*>(QCoreApplication::instance());
	QString appName = app->applicationName();

	QDir mLibraryDirectory;
	const QgsScopedRuntimeProfile profile(QObject::tr("Initialize data providers"));
	mLibraryDirectory.setPath(mDirPath);

	mLibraryDirectory.setSorting(QDir::Name | QDir::IgnoreCase);
	mLibraryDirectory.setFilter(QDir::Files | QDir::NoSymLinks);

#if defined(_MSC_VER) || defined(__CYGWIN__)
	mLibraryDirectory.setNameFilters(QStringList("qsmap_plug_*.dll"));
#elif defined(ANDROID)
	mLibraryDirectory.setNameFilters(QStringList("*qsmap_plug_*.so"));
#else
	mLibraryDirectory.setNameFilters(QStringList(QStringLiteral("libqsmap_plug_*.so")));
#endif

	QgsDebugMsgLevel(QStringLiteral("Checking %1 for provider plugins").arg(mLibraryDirectory.path()), 2);

	if (mLibraryDirectory.count() == 0)
	{
		QgsDebugMsgLevel(QStringLiteral("No dynamic QGIS data provider plugins found in:\n%1\n").arg(mLibraryDirectory.path()));
	}

	const QString filePattern;
	QRegularExpression fileRegexp;
	if (!filePattern.isEmpty())
	{
		fileRegexp.setPattern(filePattern);
	}

	const auto constEntryInfoList = mLibraryDirectory.entryInfoList();
	for (const QFileInfo& fi : constEntryInfoList) {
		if (!filePattern.isEmpty())
		{
			if (fi.fileName().indexOf(fileRegexp) == -1)
			{
				QgsDebugMsgLevel("provider " + fi.fileName() + " skipped because doesn't match pattern " + filePattern);
				continue;
			}
		}

		// Always skip authentication methods
		std::string ssfileName = fi.fileName().toStdString();
		if (fi.fileName().contains(QStringLiteral("authmethod"), Qt::CaseSensitivity::CaseInsensitive))
		{
			continue;
		}

		const QgsScopedRuntimeProfile profile(QObject::tr("Load %1").arg(fi.fileName()));
		QLibrary myLib(fi.filePath());
		if (!myLib.load())
		{
			QString sserror = myLib.errorString();
			QgsDebugMsgLevel(QStringLiteral("Checking %1: ...invalid (lib not loadable): %2").arg(myLib.fileName(), myLib.errorString()));
			continue;
		}

		typedef bool app_enable_function(QString);

		QFunctionPointer func = myLib.resolve(QStringLiteral("providerEnableForApp").toLatin1().data());
		app_enable_function* function = reinterpret_cast<app_enable_function*>(cast_to_fptr(func));
		if (function) {
			if (!function(appName)) {
				continue; //skip
			}
		}

		resolve_providerMetadataFactory(myLib);
		resolve_providerGuiMetadataFactory(myLib);
	}
}

void QsMapPluginManager::resolve_providerGuiMetadataFactory(QLibrary& myLib) {

	typedef QgsProviderGuiMetadata* factory_function();

	bool libraryLoaded{ false };
	QFunctionPointer func = myLib.resolve(QStringLiteral("providerGuiMetadataFactory").toLatin1().data());
	factory_function* function = reinterpret_cast<factory_function*>(cast_to_fptr(func));
	if (function)
	{
		QgsProviderGuiMetadata* meta = function();
		if (meta)
		{
			if (findMetadata_gui(meta->key()))
			{
				QgsDebugMsgLevel(QStringLiteral("Checking %1: ...invalid (key %2 already registered)").arg(myLib.fileName()).arg(meta->key()));
				delete meta;
				return;
			}
			// add this provider to the provider map
			m_guiProviders[meta->key()] = meta;
			libraryLoaded = true;
		}
	}

	if (!libraryLoaded)
	{
		QgsDebugMsgLevel(QStringLiteral("Checking %1: ...invalid (no providerMetadataFactory method)").arg(myLib.fileName()), 2);
	}
}

void QsMapPluginManager::resolve_providerMetadataFactory(QLibrary& myLib) {
	typedef QgsProviderMetadata* factory_function();

	bool libraryLoaded{ false };
	QFunctionPointer func = myLib.resolve(QStringLiteral("providerMetadataFactory").toLatin1().data());
	factory_function* function = reinterpret_cast<factory_function*>(cast_to_fptr(func));
	if (function)
	{
		QgsProviderMetadata* meta = function();
		if (meta)
		{
			if (findMetadata_(meta->key()))
			{
				QgsDebugMsgLevel(QStringLiteral("Checking %1: ...invalid (key %2 already registered)").arg(myLib.fileName()).arg(meta->key()));
				delete meta;
				return;
			}
			// add this provider to the provider map
			QgsProviderRegistry::instance()->registerProvider(meta);
			libraryLoaded = true;
		}
	}

	if (!libraryLoaded)
	{
		QgsDebugMsgLevel(QStringLiteral("Checking %1: ...invalid (no providerMetadataFactory method)").arg(myLib.fileName()), 2);
	}
}

const QgsProviderMetadata* QsMapPluginManager::findMetadata_(const QString& providerKey)
{
	return QgsProviderRegistry::instance()->providerMetadata(providerKey);
}

const QgsProviderGuiMetadata* QsMapPluginManager::findMetadata_gui(const QString& providerKey)
{
	// first do case-sensitive match
	const std::map<QString, QgsProviderGuiMetadata*>::const_iterator i =
		m_guiProviders.find(providerKey);

	if (i != m_guiProviders.end())
	{
		return i->second;
	}

	// fallback to case-insensitive match
	for (auto it = m_guiProviders.begin(); it != m_guiProviders.end(); ++it)
	{
		if (providerKey.compare(it->first, Qt::CaseInsensitive) == 0)
			return it->second;
	}

	return nullptr;
}


void QsMapPluginManager::registerGuis(QMainWindow* parent)
{
	for (auto it = m_guiProviders.begin(); it != m_guiProviders.end(); ++it)
	{
		it->second->registerGui(parent);
	}
}


QList<QgsProviderSublayerDetails> QsMapPluginManager::querySublayers(const QString& uri
	, Qgis::SublayerQueryFlags flags, QgsFeedback* feedback) const
{
	return QgsProviderRegistry::instance()->querySublayers(uri, flags, feedback);
}
