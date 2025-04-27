#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qgsappmaptools.h"
#include <qsmapapp.h>

#include <QMenu>
#include <QToolBar>
#include <QToolButton>
#include <QMainWindow>
#include <QStackedWidget>

#include <math.h>

#include <qgis.h>
#include <qgsmapcanvas.h>
#include <qgsdockwidget.h>
#include <qgsmaplayer.h>
#include <qgsmessagebar.h>
#include <qgsmessagelogviewer.h>
#include <qgslayertreeview.h>
#include <qgslayertreemapcanvasbridge.h>
#include <qgslayertreeregistrybridge.h>
#include <qgsuserinputwidget.h>
#include <qttreepropertybrowser.h>
#include <qtvariantproperty.h>
#include <qtpropertymanager.h>
#include <qgsstatusbar.h>
#include <qgsadvanceddigitizingdockwidget.h>

class QgsStatusBarCoordinatesWidget;
class QgsStatusBarScaleWidget;

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = 0);
  ~MainWindow();

public:
  QgsMessageBar* messageBar();
  void addToolBar(QToolBar* toolBar, Qt::ToolBarArea area);
  QgsLayerTreeView* layerTreeView();
  QSize iconSize(bool dockedToolbar) const;
  void activeDockWidget(QDockWidget* thepDockWidget);
  void addDockWidget(Qt::DockWidgetArea area, QDockWidget* thepDockWidget, bool tabify = false);
  void removeDockWidget(QDockWidget* thepDockWidget);
  QgsMapCanvas* mapCanvas() const;
  /**
   * Freezes all map canvases (or thaws them if the \a frozen argument is FALSE).
   */
  void freezeCanvases(bool frozen = true);
  QList<QgsMapCanvas*> mapCanvases();
  QgsAdvancedDigitizingDockWidget* cadDockWidget()
  {
    return mAdvancedDigitizingDockWidget;
  }

  QMenu* getFileMenu() const;
  QToolBar* getMapNavToolBar() const;
  QMenu* getViewMenu() const;
  QMenuBar* getQMenuBar();

  void refreshMapCanvas(bool redrawAllLayers = false);

  //! Sets map tool to Zoom out
  void zoomOut();
  //! Sets map tool to Zoom in
  void zoomIn();
  //! Zoom to full extent
  void zoomFull();
  //! Zoom to the previous extent
  void zoomToPrevious();
  //! Zoom to the forward extent
  void zoomToNext();
  //! Zoom to selected features
  void zoomToSelected();
  //! zoom to extent of layer
  void zoomToLayerExtent();
  //! Pan map to selected features
  void pan();
  void panToSelected();
  void zoomActualSize();

  void setMapTool(QgsMapTool* tool);
  void clearMapTool();

  void legendLayerZoomNative();

  QtVariantPropertyManager* propertyManager() const;
  QtTreePropertyBrowser* propertyBrowser() const;


private:
  void initLayerTreeView();
  void initPropertyBrowserView();
  QgsLayerTreeRegistryBridge::InsertionPoint layerTreeInsertionPoint() const;
  void createActions();
  void createStatusBar();
  void functionProfile(void (MainWindow::* fnc)(), MainWindow* instance, const QString& name);
  void showScale(double scale);

  void updateCrsStatusBar();
  //! Sets project properties, including map untis
  void projectProperties(const QString& currentPage = QString());
  template <class WidgetType>
  WidgetType* findAncestorWidget(QWidget* widget) {
    WidgetType* findWidget = nullptr;
    QWidget* parentWidget = widget;
    while (parentWidget != nullptr) {
      parentWidget = parentWidget->parentWidget();
      qDebug() << "Ancestor:" << parentWidget;
      findWidget = qobject_cast<WidgetType*>(parentWidget);
      if (findWidget) {
        return findWidget;
      }
    }

    return nullptr;
  }

  void updateMouseCoordinatePrecision();

private slots:
  void on_actionExit_triggered();
  void slotAddVector();
  void slotOpenS101();
  void displayMessage(const QString& title, const QString& message, Qgis::MessageLevel level);
  void layerTreeViewDoubleClicked(const QModelIndex& index);
  void onActiveLayerChanged(QgsMapLayer* layer);
  void updateNewLayerInsertionPoint();
  void autoSelectAddedLayer(QList<QgsMapLayer*> layers);
  void projectPropertiesProjections();

  void projectCrsChanged();
  void mapCanvasScaleChanged(double);
private:
  Ui::MainWindow* ui;
  QsMapApp* m_QgisApp = nullptr;
  QMenu* mPanelMenu = nullptr;
  QMenu* mToolbarMenu = nullptr;
  QgsMapCanvas* mMapCanvas = nullptr;
  QgsMessageBar* mInfoBar = nullptr;
  QStackedWidget* mCentralContainer = nullptr;
  QgsMessageLogViewer* mLogViewer = nullptr;
  QgsLayerTreeView* mLayerTreeView = nullptr;
  QtTreePropertyBrowser* mPropertyBrowser = nullptr;
  QtVariantPropertyManager* m_pVarManager = nullptr;
  QgsDockWidget* mLogDock = nullptr;
  QgsDockWidget* mLayerTreeDock = nullptr;
  QgsDockWidget* mPropertyBrowserDock = nullptr;
  QgsLayerTreeMapCanvasBridge* mLayerTreeCanvasBridge = nullptr;
  QgsStatusBar* mStatusBar = nullptr;
  QToolButton* mOnTheFlyProjectionStatusButton = nullptr;
  QToolButton* mMessageButton = nullptr;
  QgsStatusBarCoordinatesWidget* mCoordsEdit = nullptr;
  QgsStatusBarScaleWidget* mScaleWidget = nullptr;
  QList<QgsMapLayer*> mapCanvasLayerSet; // 地图画布所用的图层集合
  QActionGroup* mToolsActionGroup = nullptr;
  std::unique_ptr< QgsAppMapTools > mMapTools;
  QgsAdvancedDigitizingDockWidget* mAdvancedDigitizingDockWidget = nullptr;
};

#endif // MAINWINDOW_H
