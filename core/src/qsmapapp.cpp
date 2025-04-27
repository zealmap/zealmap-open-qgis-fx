#include <qsmapapp.h>
#include "mainwindow.h"
#include <QApplication>
#include <QBitmap>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QPixmap>
#include <QLocale>
#include <QSplashScreen>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>
#include <QImageReader>
#include <QMessageBox>
#include <QStandardPaths>
#include <QScreen>

#include <fcntl.h>

#ifdef WIN32  // Windows
#include <windows.h>
#endif

#include "qgscanvasrefreshblocker.h"
#include "qsmappluginmanager.h"

#include <qgsapplication.h>
#include <qgsproviderregistry.h>
#include <qgsproviderutils.h>
#include <qgsgui.h>

// declaration only, link qgis_gui
class GUI_EXPORT QgsDatumTransformDialog {
public:
	// declaration only, link qgis_gui
	static bool run(const QgsCoordinateReferenceSystem& sourceCrs = QgsCoordinateReferenceSystem(),
		const QgsCoordinateReferenceSystem& destinationCrs = QgsCoordinateReferenceSystem(),
		QWidget* parent = nullptr,
		QgsMapCanvas* mapCanvas = nullptr,
		const QString& windowTitle = QString());
};

#ifdef WIN32  // Windows
void EnableDrag(QMainWindow& w) {
	ChangeWindowMessageFilter(WM_DROPFILES, 1);

	w.winId() << w.effectiveWinId();
	ChangeWindowMessageFilterEx((HWND)w.effectiveWinId(), WM_DROPFILES, MSGFLT_ALLOW, NULL);
	ChangeWindowMessageFilterEx((HWND)w.effectiveWinId(), WM_COPYDATA, MSGFLT_ALLOW, NULL);
	ChangeWindowMessageFilterEx((HWND)w.effectiveWinId(), 0x0049, MSGFLT_ALLOW, NULL);

	DragAcceptFiles((HWND)w.effectiveWinId(), true);
	RevokeDragDrop((HWND)w.winId());
}
#else
//参数：Fun为传入的函数地址，这个函数是你想获取的进程中的函数。
//     sFilePath 为传出参数，传出的是当前进程路径，也就是动态库.so自身路径。

int GetModuleFileName(void* Fun, char*& sFilePath, int sz)
{
	int ret = -1;
	Dl_info dl_info;        //特定结构体
	if (dladdr(Fun, &dl_info))        //第二个参数就是获取的结果
	{
		ret = 0;
		sFilePath = strdup(dl_info.dli_fname);
		char* pName = strrchr(sFilePath, '/');    //找到绝对路径中最后一个"/"
		*pName = '\0';                            //舍弃掉最后“/”之后的文件名，只需要路径
	}

	return ret;
}

#endif  // WIN32

#define D_MAINWIN(x) MainWindow* p_mw = dynamic_cast<MainWindow*>(x);

QsMapApp* QsMapApp::sInstance = nullptr;

PropertyBrowserItem::PropertyBrowserItem() {}
PropertyBrowserItem::PropertyBrowserItem(int _propertyType, const QString& _name)
	: propertyType(_propertyType), name(_name) {}
PropertyBrowserItem::PropertyBrowserItem(int _propertyType, const QString& _name, const QVariant& _value)
	: propertyType(_propertyType), name(_name), value(_value) {}

QsMapApp::QsMapApp(QMainWindow* parent) : QObject(parent), m_parent(parent) {
	if (sInstance)
	{
		QMessageBox::critical(
			parent,
			tr("Multiple Instances of QsMapApp"),
			tr("Multiple instances of QGIS application object detected.\nPlease contact the developers.\n"));
		abort();
	}

	sInstance = this;
}

QsMapApp::~QsMapApp() {

}

QsMapApp* QsMapApp::instance()
{
	return sInstance;
}

QString findInstallQgisCorePath(QString& currentPath) {

	char dllFullPath[MAX_PATH] = { 0 };
	//获取指定文件路径路径
	HMODULE hm = NULL;
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCSTR)&qgsPermissiveToDouble, &hm) == 0)
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}
	if (GetModuleFileName(hm, dllFullPath, sizeof(dllFullPath)) == 0)
	{
		int ret = GetLastError();
		fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
		// Return or however you want to handle an error.
	}

	// The path variable should now contain the full filepath for this DLL.
	QFileInfo fileInfo(dllFullPath);
	QString parentPath = fileInfo.dir().path(); // 获取上一级路径

	QFileInfo fileInfoBin(parentPath);
	QString parentPathRoot = fileInfoBin.path();

	return parentPathRoot;
}

int QsMapApp::startApp(QString appName, int argc, char* argv[]) {

	QgsDebugMsgLevel(QStringLiteral("Starting qgis main"));
	QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
	QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

	QgsApplication a(argc, argv, true);
    a.setOrganizationName("zealmap");
    a.setOrganizationDomain("www.zealmap.com");
    a.setApplicationName(appName);

#ifdef WIN32  // Windows
#ifdef _MSC_VER
	_set_fmode(_O_BINARY);
#else //MinGW
	_fmode = _O_BINARY;
#endif  // _MSC_VER
#endif  // WIN32

#ifdef WIN32
	QString appPath(QApplication::applicationDirPath());
	QString prefixPath = findInstallQgisCorePath(appPath);
	if (!prefixPath.isEmpty()) {
		QgsApplication::setPrefixPath(prefixPath, true);
	}
	else {
		QgsApplication::setPrefixPath(appPath, true);
	}
	// For non static builds on win (static builds are not supported)
	// we need to be sure we can find the qt image plugins.
	QCoreApplication::addLibraryPath(appPath + QDir::separator() + "qtplugins");
#endif
	// Set locale to emit QgsApplication's localeChanged signal
	QgsApplication::setLocale(QLocale());
	QgsApplication::setWindowIcon(QIcon(QgsApplication::appIconPath()));
	
	QgsApplication::setTranslation("zh-Hans");
	QString i18 = QgsApplication::i18nPath();

	//set up splash screen
	QString mySplashPath(QgsApplication::splashPath());
	QPixmap myPixmap(mySplashPath + QStringLiteral("splash.png"));

	double screenDpi = 96;
	if (QScreen* screen = QGuiApplication::primaryScreen())
	{
		screenDpi = screen->physicalDotsPerInch();
	}

	int w = 600 * screenDpi / 96;
	int h = 300 * screenDpi / 96;

	QSplashScreen* mypSplash = new QSplashScreen(myPixmap.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation));

	// Force splash screen to start on primary screen
	if (QScreen* screen = QGuiApplication::primaryScreen())
	{
		const QPoint currentDesktopsCenter = screen->availableGeometry().center();
		mypSplash->move(currentDesktopsCenter - mypSplash->rect().center());
	}
	//for win and linux we can just automask and png transparency areas will be used
	mypSplash->setMask(myPixmap.mask());
	mypSplash->show();

	MainWindow wMain;

	wMain.setAcceptDrops(true);
	wMain.show();
	mypSplash->hide();
	mypSplash->finish(&wMain);
	delete mypSplash;

	//a.completeInitialization();

	auto exit_code = a.exec();

	// This function *MUST* be the last one called, as it destroys in
  // particular GDAL. As above objects can hold GDAL/OGR objects, it is not
  // safe destroying them afterwards
	QgsApplication::exitQgis();

	return exit_code;
}

QWidget* QsMapApp::window() const {
	return m_parent;
}

QgsMapCanvas* QsMapApp::mapCanvas() const {
	D_MAINWIN(m_parent);
	return p_mw->mapCanvas();
}

QMenu* QsMapApp::getFileMenu() const {
	D_MAINWIN(m_parent);
	return p_mw->getFileMenu();
}

QToolBar* QsMapApp::getMapNavToolBar() const {
	D_MAINWIN(m_parent);
	return p_mw->getMapNavToolBar();
}

void QsMapApp::insertToolBar(QToolBar* before, QToolBar* toolbar) const {
	D_MAINWIN(m_parent);
	return p_mw->insertToolBar(before, toolbar);
}

void QsMapApp::addToolBar(QToolBar* toolbar) const {
	D_MAINWIN(m_parent);
	return p_mw->addToolBar(toolbar, Qt::ToolBarArea::TopToolBarArea);
}

void QsMapApp::addDockWidget(Qt::DockWidgetArea area, QDockWidget* thepDockWidget) {
	D_MAINWIN(m_parent);
	return p_mw->addDockWidget(area, thepDockWidget);
}

void QsMapApp::pan() const {
	D_MAINWIN(m_parent);
	return p_mw->pan();
}
void QsMapApp::selectFeatures() const {

}
void QsMapApp::deselectAll() const {

}
/* layer will be removed - changed from removingLayer to removingLayers
 in 1.8.
*/
void QsMapApp::removingLayers(const QStringList&) {

}

//! starts/stops editing mode of the current layer
void QsMapApp::toggleEditing() {

}

//! starts/stops editing mode of a layer
bool QsMapApp::toggleEditing(QgsMapLayer* layer, bool allowCancel) {
	return false;
}

//! Save edits for active vector layer and start new transactions
void QsMapApp::saveActiveLayerEdits() {

}

//! Sets the active layer
bool QsMapApp::setActiveLayer(QgsMapLayer*) {
	return false;
}

//! Identify feature(s) on the currently selected layer
void QsMapApp::identify() {

}

//! activates the add feature tool
void QsMapApp::addFeature() {

}

//! Deletes the selected attributes for the currently selected vector layer
void QsMapApp::deleteSelected(QgsMapLayer* layer
	, QWidget* parent
	, bool checkFeaturesVisible) {

}

//! provides operations with nodes on active layer only
void QsMapApp::vertexToolActiveLayer() {

}

//! returns pointer to map legend
QgsLayerTreeView* QsMapApp::layerTreeView() {
	D_MAINWIN(m_parent);
	return p_mw->layerTreeView();
}

QgsVectorLayer* QsMapApp::addVectorLayer(const QString& vectorLayerPath
	, const QString& baseName
	, const QString& providerKey) {
	return nullptr;
}

QList< QgsMapLayer* > QsMapApp::openFile(const QString& fileName
	, const QString& fileTypeHint
	, bool suppressBulkLayerPostProcessing) {

	QList< QgsMapLayer* > openedLayers;

	QgsDebugMsgLevel("Adding " + fileName + " to the map canvas", 2);
	bool ok = false;

	//const QList< QgsProviderRegistry::ProviderCandidateDetails > candidateProviders
	//  = QgsProviderRegistry::instance()->preferredProvidersForUri(fileName);

	//res = QgsAppLayerHandling::openLayer(fileName, ok, true, suppressBulkLayerPostProcessing);

	  // query sublayers
	QList< QgsProviderSublayerDetails > sublayers
		= QsMapPluginManager::instance()->querySublayers(fileName, Qgis::SublayerQueryFlag::IncludeSystemTables);
	if (!sublayers.empty()) {
		const bool detailsAreIncomplete = QgsProviderUtils::sublayerDetailsAreIncomplete(sublayers
			, QgsProviderUtils::SublayerCompletenessFlag::IgnoreUnknownFeatureCount);

		if (detailsAreIncomplete)
		{
			// requery sublayers, resolving geometry types
			sublayers = QsMapPluginManager::instance()->querySublayers(fileName, Qgis::SublayerQueryFlag::ResolveGeometryType);
		}
	}

	// now add sublayers
	if (!sublayers.empty())
	{
		QgsCanvasRefreshBlocker refreshBlocker(mapCanvas());
		QgsSettings settings;

		QString base = QgsProviderUtils::suggestLayerNameFromFilePath(fileName);
		if (settings.value(QStringLiteral("qgis/formatLayerName"), false).toBool())
		{
			base = QgsMapLayer::formatLayerName(base);
		}

		QString groupName;
		openedLayers.append(addSublayers(sublayers, base, groupName));
	}

	return openedLayers;
}

QgsMapLayer* QsMapApp::toLayer(const QgsProviderSublayerDetails& sublayer
	, QgsProviderSublayerDetails::LayerOptions& lopt) {
	QgsVectorLayer::LayerOptions vectorOptions;
	vectorOptions.transformContext = lopt.transformContext;
	vectorOptions.loadDefaultStyle = lopt.loadDefaultStyle;
	return new QgsVectorLayer(sublayer.uri(), sublayer.name(), sublayer.providerKey(), vectorOptions);
}

QList< QgsMapLayer* > QsMapApp::addSublayers(const QList< QgsProviderSublayerDetails>& layers
	, const QString& baseName, const QString& groupName) {
	QgsSettings settings;
	const bool formatLayerNames = settings.value(QStringLiteral("qgis/formatLayerName"), false).toBool();

	// if we aren't adding to a group, we need to add the layers in reverse order so that they maintain the correct
	// order in the layer tree!
	QList<QgsProviderSublayerDetails> sortedLayers = layers;
	if (groupName.isEmpty())
	{
		std::reverse(sortedLayers.begin(), sortedLayers.end());
	}

	QList< QgsMapLayer* > result;
	result.reserve(sortedLayers.size());

	for (const QgsProviderSublayerDetails& sublayer : std::as_const(sortedLayers))
	{

		QgsProviderSublayerDetails::LayerOptions options(QgsProject::instance()->transformContext());
		options.loadDefaultStyle = false;

		std::unique_ptr<QgsMapLayer> layer(sublayer.toLayer(options));
		if (!layer)
			continue;

		QgsMapLayer* ml = layer.get();
		// if we aren't adding to a group, then we're iterating the layers in the reverse order
		// so account for that in the returned list of layers
		if (groupName.isEmpty())
			result.insert(0, ml);
		else
			result << ml;

		QString layerName = layer->name();
		if (formatLayerNames)
		{
			layerName = QgsMapLayer::formatLayerName(layerName);
		}

		const bool projectWasEmpty = QgsProject::instance()->mapLayers().empty();


		if (layerName != baseName && !layerName.isEmpty() && !baseName.isEmpty())
			layer->setName(QStringLiteral("%1 — %2").arg(baseName, layerName));
		else if (!layerName.isEmpty())
			layer->setName(layerName);
		else if (!baseName.isEmpty())
			layer->setName(baseName);
		QgsProject::instance()->addMapLayer(layer.release());

		// Some of the logic relating to matching a new project's CRS to the first layer added CRS is deferred to happen when the event loop
	// next runs -- so in those cases we can't assume that the project's CRS has been matched to the actual desired CRS yet.
	// In these cases we don't need to show the coordinate operation selection choice, so just hardcode an exception in here to avoid that...
		QgsCoordinateReferenceSystem projectCrsAfterLayerAdd = QgsProject::instance()->crs();
		const QgsGui::ProjectCrsBehavior projectCrsBehavior = QgsSettings().enumValue(QStringLiteral("/projections/newProjectCrsBehavior"), QgsGui::UseCrsOfFirstLayerAdded, QgsSettings::App);
		switch (projectCrsBehavior)
		{
		case QgsGui::UseCrsOfFirstLayerAdded:
		{
			if (projectWasEmpty)
				projectCrsAfterLayerAdd = ml->crs();
			break;
		}

		case QgsGui::UsePresetCrs:
			break;
		}

		askUserForDatumTransform(ml->crs(), projectCrsAfterLayerAdd, ml);
	}

	// Post process all added layers
	for (QgsMapLayer* ml : std::as_const(result))
	{
		postProcessAddedLayer(ml);
	}

	return result;
}
//! show the attribute table for the currently selected layer
void QsMapApp::attributeTable(QgsAttributeTableFilterModel::FilterMode filter
	, const QString& filterExpression) {

}

bool QsMapApp::askUserForDatumTransform(const QgsCoordinateReferenceSystem& sourceCrs, const QgsCoordinateReferenceSystem& destinationCrs, const QgsMapLayer* layer)
{
	QString title;
	if (layer)
	{
		// try to make a user-friendly (short!) identifier for the layer
		QString layerIdentifier;
		if (!layer->name().isEmpty())
		{
			layerIdentifier = layer->name();
		}
		else
		{
			const QVariantMap parts = QgsProviderRegistry::instance()->decodeUri(layer->providerType(), layer->source());
			if (parts.contains(QStringLiteral("path")))
			{
				const QFileInfo fi(parts.value(QStringLiteral("path")).toString());
				layerIdentifier = fi.fileName();
			}
			else if (layer->dataProvider())
			{
				const QgsDataSourceUri uri(layer->source());
				layerIdentifier = uri.table();
			}
		}
		if (!layerIdentifier.isEmpty())
			title = tr("Select Transformation for %1").arg(layerIdentifier);
	}

	return QgsDatumTransformDialog::run(sourceCrs, destinationCrs, window(), mapCanvas(), title);
}

void QsMapApp::postProcessAddedLayer(QgsMapLayer* layer) {
	switch (layer->type()) {
	case Qgis::LayerType::Vector:
	case Qgis::LayerType::Raster:
	{
		bool ok = false;
		layer->loadDefaultStyle(ok);
		layer->loadDefaultMetadata(ok);
		break;
	}
	}
}

void QsMapApp::clearPropertyBrowser() const {
	D_MAINWIN(m_parent);

	p_mw->propertyBrowser()->clear();
	p_mw->propertyManager()->clear();
}

void QsMapApp::addToPropertyBrowser(int propertyType, const QString& name, const QVariant& value) const {
	D_MAINWIN(m_parent);
	QtTreePropertyBrowser* propertyBrowser = p_mw->propertyBrowser();
	QtVariantPropertyManager* pm = p_mw->propertyManager();
	QtVariantProperty* itemLocation = pm->addProperty(propertyType, name);
	itemLocation->setValue(value);
	propertyBrowser->addProperty(itemLocation);
}

void QsMapApp::addToPropertyBrowser(const QVector< PropertyBrowserItem>& subItems) const {
	D_MAINWIN(m_parent);
	QtTreePropertyBrowser* propertyBrowser = p_mw->propertyBrowser();
	QtVariantPropertyManager* pm = p_mw->propertyManager();
	for (auto prop : subItems)
	{
		QtVariantProperty* itemSub = pm->addProperty(prop.propertyType, prop.name);
		itemSub->setValue(prop.value);
		propertyBrowser->addProperty(itemSub);
	}
}

void QsMapApp::addToPropertyBrowser(const QString& groupName, const QVector< PropertyBrowserItem>& subItems) const {
	D_MAINWIN(m_parent);
	QtTreePropertyBrowser* propertyBrowser = p_mw->propertyBrowser();
	QtVariantPropertyManager* pm = p_mw->propertyManager();

	QtProperty* groupItem = pm->addProperty(QtVariantPropertyManager::groupTypeId()
		, groupName);

	for (auto prop : subItems)
	{
		QtVariantProperty* itemSub = pm->addProperty(prop.propertyType, prop.name);
		itemSub->setValue(prop.value);
		groupItem->addSubProperty(itemSub);

		propertyBrowser->addProperty(groupItem);
	}
}
