#include "qsmapexampleui.h"
#include <qsmapapp.h>
#include <QToolBar>
#include <QAction>
#include <QMenu>
#include <QIcon>
#include <QFileDialog>
#include <QMessageBox>
#include <qgsrasterlayer.h>
#include "QgsXyzConnectionUtils.h"
#include <cmath>

QsMapExampleUi::QsMapExampleUi(QObject* parent)
    : QObject(parent)
{
}

void QsMapExampleUi::registerGui(QMainWindow* widget)
{
    initToolbar(widget);
}


void QsMapExampleUi::initToolbar(QMainWindow* widget) {
    QToolBar* toolbar = new QToolBar(tr("Example工具栏"));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    QsMapApp::instance()->addToolBar(toolbar);

    QMenu* menuFile = QsMapApp::instance()->getFileMenu();
    QIcon openIcon(QString::fromUtf8(":/images/themes/default/mActionAddLayer.svg"));

    QAction* newAction = new QAction(openIcon, "添加矢量文件", widget);
    toolbar->addAction(newAction);
    menuFile->addAction(newAction);
    connect(newAction, &QAction::triggered, this, &QsMapExampleUi::onOpenActionTriggered);

    QAction* openAction = new QAction(openIcon, "添加高德在线地图", widget);
    toolbar->addAction(openAction);
    menuFile->addAction(openAction);
    connect(openAction, &QAction::triggered, this, &QsMapExampleUi::onOpenAmapActionTriggered);

    QAction* gotoChinaAction = new QAction(openIcon, "跳转到Mercator中国范围", widget);
    toolbar->addAction(gotoChinaAction);
    menuFile->addAction(gotoChinaAction);
    connect(gotoChinaAction, &QAction::triggered, this, &QsMapExampleUi::onGotoChinaActionActionTriggered);
}

void QsMapExampleUi::onOpenAmapActionTriggered() {
    addAmapAction("http://webst01.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scale=1&style=6", "Amap影像");
    addAmapAction("http://webst01.is.autonavi.com/appmaptile?x={x}&y={y}&z={z}&lang=zh_cn&size=1&scale=1&style=8", "Amap影像注记");

    QsMapApp::instance()->mapCanvas()->refresh();
}

void QsMapExampleUi::addAmapAction(const QString& url, const QString& name)
{
    QgsXyzConnection xyz;
    xyz.url = url;
    xyz.zMin = 1;
    xyz.zMax = 18;
    xyz.tilePixelRatio = 1.0;

    QgsXyzConnectionUtils::addConnection(xyz);

    QString url2 = xyz.encodedUri();
    qDebug() << "url2 = " << url2;

    QgsRasterLayer* amapLayer = new QgsRasterLayer(url2, name, QStringLiteral("wms"));
    if (!amapLayer->isValid())
    {
        QMessageBox::critical(QsMapApp::instance()->window(), "error", QString("layer is invalid"));
        return;
    }

    QgsProject::instance()->addMapLayer(amapLayer);
}

void QsMapExampleUi::onOpenActionTriggered()
{
    QString fileName = QFileDialog::getOpenFileName(QsMapApp::instance()->window(), tr("Open shape file"), "", "*.shp");
    if (fileName.isEmpty())
    {
        return;
    }

    // 创建 QFileInfo 对象
    QFileInfo fileInfo(fileName);

    // 获取不带后缀的文件名
    QString baseName = fileInfo.baseName();

    QgsVectorLayer* vecLayer = new QgsVectorLayer(fileName, baseName, "ogr");

    if (!vecLayer->isValid())
    {
        QMessageBox::critical(QsMapApp::instance()->window(), "error", QString("layer is invalid: \n") + fileName);
        return;
    }

    QgsProject::instance()->addMapLayer(vecLayer);
    QsMapApp::instance()->mapCanvas()->setExtent(vecLayer->extent());
    QsMapApp::instance()->mapCanvas()->refresh();
}

void QsMapExampleUi::onGotoChinaActionActionTriggered()
{
    // 定义源和目标坐标参考系统
    QgsCoordinateReferenceSystem sourceCrs(4326); // WGS84
    QgsCoordinateReferenceSystem targetCrs(3857); // Web Mercator

    // 创建坐标转换对象
    QgsCoordinateTransform transform(sourceCrs, targetCrs, QgsProject::instance());

    // 定义要转换的 WGS84 坐标
    QgsPointXY wgs84Point1(113, 29);
    QgsPointXY wgs84Point2(115, 32);

    try
    {
        // 进行坐标转换
        QgsPointXY mercatorPoint1 = transform.transform(wgs84Point1);
        QgsPointXY mercatorPoint2 = transform.transform(wgs84Point2);
        QgsRectangle chinaExtent2(mercatorPoint1, mercatorPoint2);
        QsMapApp::instance()->mapCanvas()->setExtent(chinaExtent2);

    }
    catch (const QgsCsException& e)
    {
        QMessageBox::critical(QsMapApp::instance()->window(), "error", QString("坐标转换出错"));
    }

    QsMapApp::instance()->mapCanvas()->refresh();
}
