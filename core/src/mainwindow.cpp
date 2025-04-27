#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QDialogButtonBox>
#include <QOpenGLWidget>

//QGIS
#include <qgsgui.h>
#include <qgsvectorlayer.h>
#include <qgslayoutitemregistry.h>
#include <qgsrasterlayer.h>
#include <qgsruntimeprofiler.h>
#include <qgslayertreemodel.h>
#include <qgslayertreeviewdefaultactions.h>
#include <qgsmessagebaritem.h>
#include <qgslayertree.h>
#include <qgslayertreeutils.h>
#include <qgsprojectviewsettings.h>

#include "qsmappluginmanager.h"
#include "qgscanvasrefreshblocker.h"
#include "qgsstatusbarcoordinateswidget.h"
#include "qgsstatusbarscalewidget.h"
#include <qgsoptionswidgetfactory.h>
#include <qgscoordinateutils.h>
#include <qgsprojectionselectiondialog.h>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent, Qt::Window),
    ui(new Ui::MainWindow),
    m_QgisApp(new QsMapApp(this)),
    mToolsActionGroup(new QActionGroup(this)),
    mMapCanvas(nullptr)
{
    ui->setupUi(this);

    // Panel and Toolbar Submenus
    mPanelMenu = new QMenu(tr("停靠面板"), this);
    mPanelMenu->setObjectName(QStringLiteral("mPanelMenu"));
    mToolbarMenu = new QMenu(tr("工具栏"), this);
    mToolbarMenu->setObjectName(QStringLiteral("mToolbarMenu"));
    // Get platform for menu layout customization (Gnome, Kde, Mac, Win)
    QDialogButtonBox::ButtonLayout layout =
        QDialogButtonBox::ButtonLayout(style()->styleHint(QStyle::SH_DialogButtonLayout, nullptr, this));
    ui->menuView->addMenu(mPanelMenu);
    ui->menuView->addMenu(mToolbarMenu);

    // add to the Toolbar submenu
    mToolbarMenu->addAction(ui->mMapNavToolBar->toggleViewAction());

    connect(ui->actionAddVector, SIGNAL(triggered(bool)), this, SLOT(slotAddVector()));

    // Do this early on before anyone else opens it and prevents us copying it
    QString dbError;
    if (!QgsApplication::createDatabase(&dbError))
    {
        QMessageBox::critical(this, tr("Private qgis.db"), dbError);
    }

    QgsApplication::initQgis();

    setDockOptions(dockOptions() | QMainWindow::GroupedDragging);

    QGridLayout* centralLayout = ui->gridLayout;
    mMapCanvas = new QgsMapCanvas(ui->centralWidget);
    mMapCanvas->setObjectName(QStringLiteral("theMapCanvas"));

    // 使用QOpenGLWidget实例作为GraphicsView的视口
    //QOpenGLWidget* w = new QOpenGLWidget(this);
    //mMapCanvas->setViewport(w);

    mMapCanvas->enableAntiAliasing(true);
    mMapCanvas->setCanvasColor(QColor(255, 255, 255));
    // before anything, let's freeze canvas redraws
    QgsCanvasRefreshBlocker refreshBlocker(mMapCanvas);
    mMapCanvas->setProject(QgsProject::instance());

    mLogViewer = new QgsMessageLogViewer(this);
    mLogDock = new QgsDockWidget(tr("输出"), this);
    mLogDock->setObjectName(QStringLiteral("MessageLog"));
    mLogDock->setAllowedAreas(Qt::AllDockWidgetAreas);
    addDockWidget(Qt::BottomDockWidgetArea, mLogDock);
    mLogDock->setWidget(mLogViewer);
    mLogDock->setUserVisible(true);

    functionProfile(&MainWindow::createStatusBar, this, QStringLiteral("Status bar"));

    connect(mMapCanvas, &QgsMapCanvas::messageEmitted, this, &MainWindow::displayMessage);
    //connect(mMapCanvas, &QgsMapCanvas::xyCoordinates, this, &QgisApp::saveLastMousePosition);
    //connect(mMapCanvas, &QgsMapCanvas::extentsChanged, this, &QgisApp::extentChanged);
    connect(mMapCanvas, &QgsMapCanvas::scaleChanged, this, &MainWindow::mapCanvasScaleChanged);
    //connect(mMapCanvas, &QgsMapCanvas::rotationChanged, this, &QgisApp::showRotation);
    //connect(mMapCanvas, &QgsMapCanvas::mapToolSet, this, &QgisApp::mapToolChanged);
    //connect(mMapCanvas, &QgsMapCanvas::selectionChanged, this, &QgisApp::selectionChanged);
    //connect(mMapCanvas, &QgsMapCanvas::layersChanged, this, &QgisApp::markDirty);
    //connect(mMapCanvas, &QgsMapCanvas::zoomLastStatusChanged, mActionZoomLast, &QAction::setEnabled);
    //connect(mMapCanvas, &QgsMapCanvas::zoomNextStatusChanged, mActionZoomNext, &QAction::setEnabled);

    // a bar to warn the user with non-blocking messages
    mInfoBar = new QgsMessageBar(ui->centralWidget);
    mInfoBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    centralLayout->addWidget(mInfoBar, 0, 0, 1, 1);

    mCentralContainer = new QStackedWidget(ui->centralWidget);
    //QGridLayout* pLayout = new QGridLayout(mCentralContainer);
    //initMapView(pLayout);
    //mCentralContainer->setLayout(pLayout);
    mCentralContainer->addWidget(mMapCanvas);

    centralLayout->addWidget(mCentralContainer, 0, 0, 2, 1);
    mInfoBar->raise();
    mCentralContainer->setCurrentIndex(0);

    mLayerTreeView = new QgsLayerTreeView(this);
    mLayerTreeView->setObjectName(QStringLiteral("theLayerTreeView")); // "theLayerTreeView" used to find this canonical instance later

    mPropertyBrowser = new QtTreePropertyBrowser(this);
    mPropertyBrowser->setHeaderVisible(false);
    mPropertyBrowser->setRootIsDecorated(true);
    m_pVarManager = new QtVariantPropertyManager(mPropertyBrowser);

    initLayerTreeView();
    initPropertyBrowserView();

    // setup drag drop
    setAcceptDrops(true);

    mAdvancedDigitizingDockWidget = new QgsAdvancedDigitizingDockWidget(mMapCanvas, this);
    mAdvancedDigitizingDockWidget->setWindowTitle(tr("Advanced Digitizing"));
    mAdvancedDigitizingDockWidget->setObjectName(QStringLiteral("AdvancedDigitizingTools"));
    addDockWidget(Qt::LeftDockWidgetArea, mAdvancedDigitizingDockWidget);
    mAdvancedDigitizingDockWidget->hide();

    createActions();

    QString appPath(QCoreApplication::applicationDirPath());
    QsMapPluginManager::instance()->loadCppPlugin(appPath);
    QsMapPluginManager::instance()->registerGuis(this);
    // Change to maximized state. Just calling showMaximized() results in
  // the window going to the normal state. Calling showNormal() then
  // showMaxmized() is a work-around. Turn off rendering for this as it
  // would otherwise cause two re-renders of the map, which can take a
  // long time.
    showNormal();
    showMaximized();

    mMapTools = std::make_unique< QgsAppMapTools >(mMapCanvas);
    mMapCanvas->setMapTool(mMapTools->mapTool(QgsAppMapTools::Pan));
    connect(QgsProject::instance(), &QgsProject::crsChanged, this, &MainWindow::projectCrsChanged);
    connect(QgsProject::instance()->viewSettings(), &QgsProjectViewSettings::mapScalesChanged, this, [=] { mScaleWidget->updateScales(); });
    // 输出消息到停靠窗
    QgsMessageLog::logMessage(tr(u8"启动成功, 程序已就绪"), QString(tr(u8"常规")), Qgis::MessageLevel::Info);
    //QgsMessageLog::logMessage(QgsApplication::showSettings(), QString(), Qgis::MessageLevel::Info);

    updateCrsStatusBar();

#if DEBUG
    auto addVector = new QAction("添加矢量数据", this);
    connect(addVector, &QAction::triggered, this, &MainWindow::slotAddVector);
    getFileMenu()->addAction(addVector);
#endif
}

void MainWindow::functionProfile(void (MainWindow::* fnc)(), MainWindow* instance, const QString& name)
{
    QgsScopedRuntimeProfile profile(name);
    (instance->*fnc)();
}

void MainWindow::createActions() {
    mToolsActionGroup->addAction(ui->mActionPan);
    mToolsActionGroup->addAction(ui->mActionZoomIn);
    mToolsActionGroup->addAction(ui->mActionZoomOut);
    connect(ui->mActionPan, &QAction::triggered, this, &MainWindow::pan);
    connect(ui->mActionPanToSelected, &QAction::triggered, this, &MainWindow::panToSelected);
    connect(ui->mActionZoomIn, &QAction::triggered, this, &MainWindow::zoomIn);
    connect(ui->mActionZoomOut, &QAction::triggered, this, &MainWindow::zoomOut);
    connect(ui->mActionZoomFullExtent, &QAction::triggered, this, &MainWindow::zoomFull);
    connect(ui->mActionZoomToLayer, &QAction::triggered, this, &MainWindow::zoomToLayerExtent);
    connect(ui->mActionZoomToLayers, &QAction::triggered, this, &MainWindow::zoomToLayerExtent);
    connect(ui->mActionZoomToSelected, &QAction::triggered, this, &MainWindow::zoomToSelected);
    connect(ui->mActionDraw, &QAction::triggered, this, [this] { refreshMapCanvas(true); });
}

MainWindow::~MainWindow()
{
    delete ui;
    delete mLayerTreeView;
    delete m_QgisApp;
}

void MainWindow::createStatusBar()
{
    //remove borders from children under Windows
    statusBar()->setStyleSheet(QStringLiteral("QStatusBar::item {border: none;}"));

    // Drop the font size in the status bar by a couple of points
    QFont statusBarFont = font();
    int fontSize = statusBarFont.pointSize();
#ifdef Q_OS_WIN
    fontSize = std::max(fontSize - 1, 8); // bit less on windows, due to poor rendering of small point sizes
#else
    fontSize = std::max(fontSize - 2, 6);
#endif
    statusBarFont.setPointSize(fontSize);
    statusBar()->setFont(statusBarFont);

    mStatusBar = new QgsStatusBar(statusBar());
    mStatusBar->setParentStatusBar(QMainWindow::statusBar());
    mStatusBar->setFont(statusBarFont);

    statusBar()->addPermanentWidget(mStatusBar, 10);

    // Add a panel to the status bar for the scale, coords and progress
    // And also rendering suppression checkbox
    //connect(mMapCanvas, &QgsMapCanvas::renderStarting, this, &MainWindow::canvasRefreshStarted);
    //connect(mMapCanvas, &QgsMapCanvas::mapCanvasRefreshed, this, &MainWindow::canvasRefreshFinished);

    //coords status bar widget
    mCoordsEdit = new QgsStatusBarCoordinatesWidget(mStatusBar);
    mCoordsEdit->setObjectName(QStringLiteral("mCoordsEdit"));
    mCoordsEdit->setMapCanvas(mMapCanvas);
    mCoordsEdit->setFont(statusBarFont);
    mStatusBar->addPermanentWidget(mCoordsEdit, 0);

    mScaleWidget = new QgsStatusBarScaleWidget(mMapCanvas, mStatusBar);
    mScaleWidget->setObjectName(QStringLiteral("mScaleWidget"));
    mScaleWidget->setFont(statusBarFont);
    mStatusBar->addPermanentWidget(mScaleWidget, 0);

    // On the fly projection status bar icon
    // Changed this to a tool button since a QPushButton is
    // sculpted on OS X and the icon is never displayed [gsherman]
    mOnTheFlyProjectionStatusButton = new QToolButton(mStatusBar);
    mOnTheFlyProjectionStatusButton->setAutoRaise(true);
    mOnTheFlyProjectionStatusButton->setFont(statusBarFont);
    mOnTheFlyProjectionStatusButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    mOnTheFlyProjectionStatusButton->setObjectName(QStringLiteral("mOntheFlyProjectionStatusButton"));
    // Maintain uniform widget height in status bar by setting button height same as labels
    // For Qt/Mac 3.3, the default toolbutton height is 30 and labels were expanding to match
    mOnTheFlyProjectionStatusButton->setMaximumHeight(mScaleWidget->height());
    mOnTheFlyProjectionStatusButton->setIcon(QgsApplication::getThemeIcon(QStringLiteral("mIconProjectionEnabled.svg")));
    mOnTheFlyProjectionStatusButton->setToolTip(tr("坐标系状态 - 点击设置坐标系"));
    connect(mOnTheFlyProjectionStatusButton, &QAbstractButton::clicked,
        this, &MainWindow::projectPropertiesProjections);//bring up the project props dialog when clicked
    mStatusBar->addPermanentWidget(mOnTheFlyProjectionStatusButton, 0);
    mStatusBar->showMessage(tr("就绪"));

    mMessageButton = new QToolButton(mStatusBar);
    mMessageButton->setAutoRaise(true);
    mMessageButton->setIcon(QgsApplication::getThemeIcon(QStringLiteral("/mMessageLogRead.svg")));
    mMessageButton->setToolTip(tr("消息"));
    mMessageButton->setObjectName(QStringLiteral("mMessageLogViewerButton"));
    mMessageButton->setMaximumHeight(mScaleWidget->height());
    mMessageButton->setCheckable(true);
    connect(mLogDock, &QgsDockWidget::visibilityChanged, mMessageButton, &QAbstractButton::setChecked);
    connect(mMessageButton, &QAbstractButton::toggled, mLogDock, &QgsDockWidget::setUserVisible);
    mStatusBar->addPermanentWidget(mMessageButton, 0);
}


void MainWindow::projectPropertiesProjections()
{
    // display the project props dialog and switch to the projections tab
    projectProperties(QStringLiteral("mProjOptsCRS"));
}

void MainWindow::mapCanvasScaleChanged(double scale)
{
    showScale(scale);
    updateMouseCoordinatePrecision();
}
void MainWindow::showScale(double scale)
{
    mScaleWidget->setScale(scale);
}


void MainWindow::projectCrsChanged()
{
    updateCrsStatusBar();
    QgsDebugMsgLevel(QStringLiteral("QgisApp::setupConnections -1- : QgsProject::instance()->crs().description[%1]ellipsoid[%2]").arg(QgsProject::instance()->crs().description(), QgsProject::instance()->crs().ellipsoidAcronym()), 3);
    mMapCanvas->setDestinationCrs(QgsProject::instance()->crs());

    // handle datum transforms
    QList<QgsCoordinateReferenceSystem> alreadyAsked;
    QMap<QString, QgsMapLayer*> layers = QgsProject::instance()->mapLayers();
    for (QMap<QString, QgsMapLayer*>::const_iterator it = layers.constBegin(); it != layers.constEnd(); ++it)
    {
        if (!alreadyAsked.contains(it.value()->crs()))
        {
            alreadyAsked.append(it.value()->crs());
            //askUserForDatumTransform(it.value()->crs(),
            //  QgsProject::instance()->crs(), it.value());
        }
    }
}
void MainWindow::updateCrsStatusBar()
{
    const QgsCoordinateReferenceSystem projectCrs = QgsProject::instance()->crs();
    if (projectCrs.isValid())
    {
        if (!projectCrs.authid().isEmpty())
            mOnTheFlyProjectionStatusButton->setText(projectCrs.authid());
        else
            mOnTheFlyProjectionStatusButton->setText(tr("未知坐标系"));

        mOnTheFlyProjectionStatusButton->setToolTip(
            tr("当前坐标系: %1").arg(projectCrs.userFriendlyIdentifier()));
        mOnTheFlyProjectionStatusButton->setIcon(QgsApplication::getThemeIcon(QStringLiteral("mIconProjectionEnabled.svg")));
    }
    else
    {
        mOnTheFlyProjectionStatusButton->setText(QString());
        mOnTheFlyProjectionStatusButton->setToolTip(tr("无坐标系"));
        mOnTheFlyProjectionStatusButton->setIcon(QgsApplication::getThemeIcon(QStringLiteral("mIconProjectionDisabled.svg")));
    }
}
void MainWindow::projectProperties(const QString& currentPage)
{
    QgsProjectionSelectionDialog pp(this, QgsGuiUtils::ModalDialogFlags);

    qApp->processEvents();

    // Display the modal dialog box.
    pp.exec();

    //mMapTools->mapTool< QgsMeasureTool >(QgsAppMapTools::MeasureDistance)->updateSettings();
    //mMapTools->mapTool< QgsMeasureTool >(QgsAppMapTools::MeasureArea)->updateSettings();
    //mMapTools->mapTool< QgsMapToolMeasureAngle >(QgsAppMapTools::MeasureAngle)->updateSettings();
    //mMapTools->mapTool< QgsMapToolMeasureBearing >(QgsAppMapTools::MeasureBearing)->updateSettings();

    // Set the window title.
    //setTitleBarText_(*this);
}

void MainWindow::updateMouseCoordinatePrecision()
{
    mCoordsEdit->setMouseCoordinatesPrecision(QgsCoordinateUtils::calculateCoordinatePrecision(mapCanvas()->mapUnitsPerPixel(), mapCanvas()->mapSettings().destinationCrs()));
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::slotOpenS101() {
    QString filename = QFileDialog::getOpenFileName(this, tr("open...")
        , "", "000 file (*.000);;S101 file (*.s101);;*.*");
    if (filename.isEmpty()) {
        return;
    }
    // TODO
}

void MainWindow::slotAddVector()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("open vector"), "", "*.shp;;*.*");
    if (filename.isEmpty()) {
        return;
    }

    QStringList temp = filename.split(QDir::separator());
    QString basename = temp.at(temp.size() - 1);
    QgsVectorLayer* vecLayer = new QgsVectorLayer(filename, basename, "ogr");// , false);
    if (!vecLayer->isValid())
    {
        QMessageBox::critical(this, "error", "layer is invalid");
        return;
    }

    //QgsMapLayerRegistry::instance()->addMapLayer(vecLayer);

    // Register this layer with the layers registry
    QList<QgsMapLayer*> myList;
    myList << vecLayer;
    QgsProject::instance()->addMapLayers(myList);

    //QgisApp::instance()->askUserForDatumTransform(mapLayer->crs(), QgsProject::instance()->crs(), mapLayer);

    //mapCanvasLayerSet.append(vecLayer);
    //mMapCanvas->setExtent(vecLayer->extent());
    //mMapCanvas->setLayers(mapCanvasLayerSet);
    //mMapCanvas->setVisible(true);
    //mMapCanvas->freeze(false);
    //mMapCanvas->refresh();
}


void MainWindow::displayMessage(const QString& title, const QString& message, Qgis::MessageLevel level)
{
    messageBar()->pushMessage(title, message, level);
}

QgsMapCanvas* MainWindow::mapCanvas() const
{
    return mMapCanvas;
}

QgsMessageBar* MainWindow::messageBar()
{
    // Q_ASSERT( mInfoBar );
    return mInfoBar;
}

void MainWindow::addToolBar(QToolBar* toolBar, Qt::ToolBarArea area)
{
    QMainWindow::addToolBar(area, toolBar);
    // add to the Toolbar submenu
    mToolbarMenu->addAction(toolBar->toggleViewAction());
}

QgsLayerTreeView* MainWindow::layerTreeView()
{
    return mLayerTreeView;
}

void MainWindow::initPropertyBrowserView() {
    mPropertyBrowserDock = new QgsDockWidget(tr("属性"), this);
    mPropertyBrowserDock->setObjectName(QStringLiteral("PropertyBrowserDock"));
    mPropertyBrowserDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    mPropertyBrowserDock->setWidget(mPropertyBrowser);
    addDockWidget(Qt::RightDockWidgetArea, mPropertyBrowserDock);
}

void MainWindow::initLayerTreeView()
{
    mLayerTreeDock = new QgsDockWidget(tr("图层"), this);
    mLayerTreeDock->setObjectName(QStringLiteral("Layers"));
    mLayerTreeDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QgsLayerTreeModel* model = new QgsLayerTreeModel(QgsProject::instance()->layerTreeRoot(), this);

    model->setFlag(QgsLayerTreeModel::AllowNodeReorder);
    model->setFlag(QgsLayerTreeModel::AllowNodeRename);
    model->setFlag(QgsLayerTreeModel::AllowNodeChangeVisibility);
    model->setFlag(QgsLayerTreeModel::ShowLegendAsTree);
    model->setFlag(QgsLayerTreeModel::UseEmbeddedWidgets);
    model->setFlag(QgsLayerTreeModel::UseTextFormatting);
    model->setAutoCollapseLegendNodes(10);

    mLayerTreeView->setModel(model);
    mLayerTreeView->setMessageBar(mInfoBar);

    //mLayerTreeView->setMenuProvider(new QgsAppLayerTreeViewMenuProvider(mLayerTreeView, mMapCanvas));
    connect(mLayerTreeView, &QAbstractItemView::doubleClicked, this, &MainWindow::layerTreeViewDoubleClicked);
    connect(mLayerTreeView, &QgsLayerTreeView::currentLayerChanged, this, &MainWindow::onActiveLayerChanged);
    connect(mLayerTreeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::updateNewLayerInsertionPoint);
    connect(QgsProject::instance()->layerTreeRegistryBridge(), &QgsLayerTreeRegistryBridge::addedLayersToLayerTree,
        this, &MainWindow::autoSelectAddedLayer);

    // add group action
    QAction* actionAddGroup = new QAction(tr("添加分组"), this);
    actionAddGroup->setIcon(QgsApplication::getThemeIcon(QStringLiteral("/mActionAddGroup.svg")));
    actionAddGroup->setToolTip(tr("添加分组"));
    connect(actionAddGroup, &QAction::triggered, mLayerTreeView->defaultActions()
        , &QgsLayerTreeViewDefaultActions::addGroup);

    // expand / collapse tool buttons
    QAction* actionExpandAll = new QAction(tr("全部展开"), this);
    actionExpandAll->setIcon(QgsApplication::getThemeIcon(QStringLiteral("/mActionExpandTree.svg")));
    actionExpandAll->setToolTip(tr("全部展开"));
    connect(actionExpandAll, &QAction::triggered, mLayerTreeView, &QgsLayerTreeView::expandAllNodes);
    QAction* actionCollapseAll = new QAction(tr("全部折叠"), this);
    actionCollapseAll->setIcon(QgsApplication::getThemeIcon(QStringLiteral("/mActionCollapseTree.svg")));
    actionCollapseAll->setToolTip(tr("全部折叠"));
    connect(actionCollapseAll, &QAction::triggered, mLayerTreeView, &QgsLayerTreeView::collapseAllNodes);

    QToolBar* toolbar = new QToolBar();
    toolbar->setIconSize(iconSize(true));
    toolbar->addAction(actionAddGroup);
    toolbar->addAction(actionExpandAll);
    toolbar->addAction(actionCollapseAll);

    QVBoxLayout* vboxLayout = new QVBoxLayout;
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    vboxLayout->setSpacing(0);
    vboxLayout->addWidget(toolbar);
    vboxLayout->addWidget(mLayerTreeView);

    QWidget* w = new QWidget;
    w->setLayout(vboxLayout);
    mLayerTreeDock->setWidget(w);
    addDockWidget(Qt::LeftDockWidgetArea, mLayerTreeDock);

    mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge(QgsProject::instance()->layerTreeRoot(), mMapCanvas, this);

    connect(mMapCanvas, &QgsMapCanvas::renderErrorOccurred, mInfoBar, [this](const QString& error, QgsMapLayer* layer)
        {
            mInfoBar->pushItem(new QgsMessageBarItem(layer->name(), QgsStringUtils::insertLinks(error), Qgis::MessageLevel::Warning));
        });
}


QSize MainWindow::iconSize(bool dockedToolbar) const
{
    return QgsGuiUtils::iconSize(dockedToolbar);
}


void MainWindow::layerTreeViewDoubleClicked(const QModelIndex& index) {

}

void MainWindow::onActiveLayerChanged(QgsMapLayer* layer)
{
    mMapCanvas->setCurrentLayer(layer);
}

void MainWindow::autoSelectAddedLayer(QList<QgsMapLayer*> layers)
{
    if (!layers.isEmpty())
    {
        QgsLayerTreeLayer* nodeLayer = QgsProject::instance()->layerTreeRoot()->findLayer(layers[0]->id());

        if (!nodeLayer)
            return;

        QModelIndex index = mLayerTreeView->node2index(nodeLayer);
        mLayerTreeView->setCurrentIndex(index);
    }
}
void MainWindow::updateNewLayerInsertionPoint()
{
    QgsLayerTreeRegistryBridge::InsertionPoint insertionPoint = layerTreeInsertionPoint();
    QgsProject::instance()->layerTreeRegistryBridge()->setLayerInsertionPoint(insertionPoint);
}

QgsLayerTreeRegistryBridge::InsertionPoint MainWindow::layerTreeInsertionPoint() const
{
    // defaults
    QgsLayerTreeGroup* insertGroup = mLayerTreeView->layerTreeModel()->rootGroup();
    QModelIndex current = mLayerTreeView->currentIndex();

    int index = 0;

    if (current.isValid())
    {
        index = current.row();

        QgsLayerTreeNode* currentNode = mLayerTreeView->currentNode();
        if (currentNode)
        {
            // if the insertion point is actually a group, insert new layers into the group
            if (QgsLayerTree::isGroup(currentNode))
            {
                // if the group is embedded go to the first non-embedded group, at worst the top level item
                QgsLayerTreeGroup* insertGroup = QgsLayerTreeUtils::firstGroupWithoutCustomProperty(QgsLayerTree::toGroup(currentNode), QStringLiteral("embedded"));

                return QgsLayerTreeRegistryBridge::InsertionPoint(insertGroup, 0);
            }

            // otherwise just set the insertion point in front of the current node
            QgsLayerTreeNode* parentNode = currentNode->parent();
            if (QgsLayerTree::isGroup(parentNode))
            {
                // if the group is embedded go to the first non-embedded group, at worst the top level item
                QgsLayerTreeGroup* parentGroup = QgsLayerTree::toGroup(parentNode);
                insertGroup = QgsLayerTreeUtils::firstGroupWithoutCustomProperty(parentGroup, QStringLiteral("embedded"));
                if (parentGroup != insertGroup)
                    index = 0;
            }
        }
    }
    return QgsLayerTreeRegistryBridge::InsertionPoint(insertGroup, index);
}

void MainWindow::activeDockWidget(QDockWidget* thepDockWidget) {
    thepDockWidget->show();
    thepDockWidget->raise();
}

void MainWindow::addDockWidget(Qt::DockWidgetArea area, QDockWidget* thepDockWidget, bool tabify)
{
    QMainWindow::addDockWidget(area, thepDockWidget);

    if (tabify) {
        QList<QDockWidget*> docks = findChildren< QDockWidget*>();
        for each(QDockWidget * dock in docks) {
            if (dock == thepDockWidget) {
                continue;
            }

            Qt::DockWidgetArea da = dockWidgetArea(dock);
            if (da == area) {
                QMainWindow::tabifyDockWidget(dock, thepDockWidget);
            }
        }
    }

    // Make the right and left docks consume all vertical space and top
    // and bottom docks nest between them
    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // add to the Panel submenu
    auto action2 = thepDockWidget->toggleViewAction();
    action2->setData("xxxxxxxxxxxxxxx");
    mPanelMenu->addAction(action2);

    thepDockWidget->show();

    // refresh the map canvas
    refreshMapCanvas();
}
void MainWindow::removeDockWidget(QDockWidget* thepDockWidget)
{
    QMainWindow::removeDockWidget(thepDockWidget);
    mPanelMenu->removeAction(thepDockWidget->toggleViewAction());
}

QList<QgsMapCanvas*> MainWindow::mapCanvases()
{
    // filter out browser canvases -- they are children of app, but a different
  // kind of beast, and here we only want the main canvas or dock canvases
    QList<QgsMapCanvas*> canvases = findChildren< QgsMapCanvas* >();

    canvases.append(mMapCanvas);

    canvases.erase(std::remove_if(canvases.begin(), canvases.end(),
        [](QgsMapCanvas* canvas)
        {
            return !canvas || canvas->property("browser_canvas").toBool();
        }), canvases.end());
    return canvases;
}
void MainWindow::freezeCanvases(bool frozen)
{
    const auto canvases = mapCanvases();
    for (QgsMapCanvas* canvas : canvases)
    {
        canvas->freeze(frozen);
    }

    if (mMapCanvas)
    {
        mMapCanvas->freeze(frozen);
    }
}

QMenu* MainWindow::getFileMenu() const
{
    return ui->menuFile;
}
QToolBar* MainWindow::getMapNavToolBar() const
{
    return ui->mMapNavToolBar;
}
QMenu* MainWindow::getViewMenu() const
{
    return ui->menuView;
}
QMenuBar* MainWindow::getQMenuBar() {
    return ui->menuBar;
}

void MainWindow::refreshMapCanvas(bool redrawAllLayers)
{
    //stop any current rendering
    mMapCanvas->stopRendering();
    if (redrawAllLayers)
        mMapCanvas->refreshAllLayers();
    else
        mMapCanvas->refresh();
}


void MainWindow::zoomIn()
{
    QgsDebugMsgLevel(QStringLiteral("Setting map tool to zoomIn"), 2);

    mMapCanvas->setMapTool(mMapTools->mapTool(QgsAppMapTools::ZoomIn));
}


void MainWindow::zoomOut()
{
    mMapCanvas->setMapTool(mMapTools->mapTool(QgsAppMapTools::ZoomOut));
}

void MainWindow::zoomToSelected()
{
    const QList<QgsMapLayer*> layers = mLayerTreeView->selectedLayers();

    if (layers.size() > 1)
        mMapCanvas->zoomToSelected(layers);
    else
        mMapCanvas->zoomToSelected();
}

void MainWindow::panToSelected()
{
    const QList<QgsMapLayer*> layers = mLayerTreeView->selectedLayers();

    if (layers.size() > 1)
        mMapCanvas->panToSelected(layers);
    else
        mMapCanvas->panToSelected();
}

void MainWindow::pan()
{
    mMapCanvas->setMapTool(mMapTools->mapTool(QgsAppMapTools::Pan));
}

void MainWindow::zoomFull()
{
    mMapCanvas->zoomToProjectExtent();
}

void MainWindow::zoomToPrevious()
{
    mMapCanvas->zoomToPreviousExtent();
}

void MainWindow::zoomToNext()
{
    mMapCanvas->zoomToNextExtent();
}
void MainWindow::zoomToLayerExtent()
{
    mLayerTreeView->defaultActions()->zoomToLayers(mMapCanvas);
}
void MainWindow::zoomActualSize()
{
    legendLayerZoomNative();
}

void MainWindow::setMapTool(QgsMapTool* tool)
{
    mMapCanvas->setMapTool(tool, true);
}
void MainWindow::clearMapTool()
{
    mMapCanvas->setMapTool(mMapTools->mapTool(QgsAppMapTools::Pan));
}
void MainWindow::legendLayerZoomNative()
{
    if (!mLayerTreeView)
        return;

    //find current Layer
    QgsMapLayer* currentLayer = mLayerTreeView->currentLayer();
    if (!currentLayer)
        return;

    if (QgsRasterLayer* layer = qobject_cast<QgsRasterLayer*>(currentLayer))
    {
        QgsDebugMsgLevel("Raster units per pixel  : " + QString::number(layer->rasterUnitsPerPixelX()), 2);
        QgsDebugMsgLevel("MapUnitsPerPixel before : " + QString::number(mMapCanvas->mapUnitsPerPixel()), 2);

        QList< double >nativeResolutions;
        if (layer->dataProvider())
        {
            nativeResolutions = layer->dataProvider()->nativeResolutions();
        }

        // get length of central canvas pixel width in source raster crs
        QgsRectangle e = mMapCanvas->extent();
        QSize s = mMapCanvas->mapSettings().outputSize();
        QgsPointXY p1(e.center().x(), e.center().y());
        QgsPointXY p2(e.center().x() + e.width() / s.width(), e.center().y() + e.height() / s.height());
        QgsCoordinateTransform ct(mMapCanvas->mapSettings().destinationCrs(), layer->crs(), QgsProject::instance());
        p1 = ct.transform(p1);
        p2 = ct.transform(p2);
        const double diagonalSize = std::sqrt(p1.sqrDist(p2)); // width (actually the diagonal) of reprojected pixel
        if (!nativeResolutions.empty())
        {
            // find closest native resolution
            QList< double > diagonalNativeResolutions;
            diagonalNativeResolutions.reserve(nativeResolutions.size());
            for (double d : std::as_const(nativeResolutions))
                diagonalNativeResolutions << std::sqrt(2 * d * d);

            int i;
            for (i = 0; i < diagonalNativeResolutions.size() && diagonalNativeResolutions.at(i) < diagonalSize; i++)
            {
                QgsDebugMsgLevel(QStringLiteral("test resolution %1: %2").arg(i).arg(diagonalNativeResolutions.at(i)), 2);
            }
            if (i == nativeResolutions.size() ||
                (i > 0 && ((diagonalNativeResolutions.at(i) - diagonalSize) > (diagonalSize - diagonalNativeResolutions.at(i - 1)))))
            {
                QgsDebugMsgLevel(QStringLiteral("previous resolution"), 2);
                i--;
            }

            mMapCanvas->zoomByFactor(nativeResolutions.at(i) / mMapCanvas->mapUnitsPerPixel());
        }
        else
        {
            mMapCanvas->zoomByFactor(std::sqrt(layer->rasterUnitsPerPixelX() * layer->rasterUnitsPerPixelX() + layer->rasterUnitsPerPixelY() * layer->rasterUnitsPerPixelY()) / diagonalSize);
        }

        mMapCanvas->refresh();
        QgsDebugMsgLevel("MapUnitsPerPixel after  : " + QString::number(mMapCanvas->mapUnitsPerPixel()), 2);
    }
}

QtVariantPropertyManager* MainWindow::propertyManager() const
{
    return m_pVarManager;
}
QtTreePropertyBrowser* MainWindow::propertyBrowser() const
{
    return mPropertyBrowser;
}
