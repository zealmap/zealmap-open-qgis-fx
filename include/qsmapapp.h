#ifndef QsMapAPP_H
#define QsMapAPP_H

#include <qsmapapi.h>

#include <QWidget>
#include <QToolBar>
#include <QDockWidget>
#include <QVector>
#include <qgsmapcanvas.h>
#include <qgsattributetablefiltermodel.h>
#include <qgslayertreeview.h>
#include <qgsprovidersublayerdetails.h>
#include <qtvariantproperty.h>
#include <qttreepropertybrowser.h>

class QSMAP_APP_API PropertyBrowserItem {
public:
	PropertyBrowserItem();
	PropertyBrowserItem(int _propertyType, const QString& _name);
	PropertyBrowserItem(int _propertyType, const QString& _name, const QVariant& _value);
	int propertyType = QVariant::String;
	QString name;
	QVariant value;
};

class QSMAP_APP_API QsMapApp : public QObject
{
	Q_OBJECT

public:
	explicit QsMapApp(QMainWindow* parent = 0);
	~QsMapApp();

	QsMapApp(QsMapApp const&) = delete;
	QsMapApp& operator=(QsMapApp const&) = delete;

public:
	static QsMapApp* instance();

	static int startApp(QString appName, int argc, char* argv[]);

	QWidget* window() const;

	QgsMapCanvas* mapCanvas() const;
	void addToolBar(QToolBar* toolbar) const;
	void insertToolBar(QToolBar* before, QToolBar* toolbar) const;
	void addDockWidget(Qt::DockWidgetArea area, QDockWidget* thepDockWidget);

	void addToPropertyBrowser(int propertyType, const QString& name, const QVariant& value) const;
	void addToPropertyBrowser(const QVector< PropertyBrowserItem>& subItems) const;
	void addToPropertyBrowser(const QString& groupName, const QVector< PropertyBrowserItem>& subItems) const;
	void clearPropertyBrowser() const;

	QMenu* getFileMenu() const;
	QToolBar* getMapNavToolBar() const;

	void pan() const;
	void selectFeatures() const;
	void deselectAll() const;
	/* layer will be removed - changed from removingLayer to removingLayers
	 in 1.8.
   */
	void removingLayers(const QStringList&);

	//! starts/stops editing mode of the current layer
	void toggleEditing();

	//! starts/stops editing mode of a layer
	bool toggleEditing(QgsMapLayer* layer, bool allowCancel = true);

	//! Save edits for active vector layer and start new transactions
	void saveActiveLayerEdits();

	//! Sets the active layer
	bool setActiveLayer(QgsMapLayer*);

	//! Identify feature(s) on the currently selected layer
	void identify();

	//! activates the add feature tool
	void addFeature();

	//! Deletes the selected attributes for the currently selected vector layer
	void deleteSelected(QgsMapLayer* layer = nullptr
		, QWidget* parent = nullptr
		, bool checkFeaturesVisible = false);

	//! provides operations with nodes on active layer only
	void vertexToolActiveLayer();

	//! returns pointer to map legend
	QgsLayerTreeView* layerTreeView();

	QgsVectorLayer* addVectorLayer(const QString& vectorLayerPath
		, const QString& baseName
		, const QString& providerKey = QLatin1String("ogr"));

	QList< QgsMapLayer* > openFile(const QString& fileName
		, const QString& fileTypeHint = QString()
		, bool suppressBulkLayerPostProcessing = false);
	//! show the attribute table for the currently selected layer
	void attributeTable(QgsAttributeTableFilterModel::FilterMode filter = QgsAttributeTableFilterModel::ShowAll
		, const QString& filterExpression = QString());


	QList< QgsMapLayer* > addSublayers(const QList< QgsProviderSublayerDetails>& layers
		, const QString& baseName, const QString& groupName);

	bool askUserForDatumTransform(const QgsCoordinateReferenceSystem& sourceCrs
		, const QgsCoordinateReferenceSystem& destinationCrs, const QgsMapLayer* layer = nullptr);

private:
	void postProcessAddedLayer(QgsMapLayer* layer);
	QgsMapLayer* toLayer(const QgsProviderSublayerDetails& sublayer
		, QgsProviderSublayerDetails::LayerOptions& lopt);

signals:
	//! Emitted when QGIS' initialization is complete
	void initializationCompleted();
	/**
   * Emitted when the active layer is changed.
   */
	void activeLayerChanged(QgsMapLayer* layer);

private:
	static QsMapApp* sInstance;

	QWidget* m_parent;
};

#endif //QsMapAPP_H
