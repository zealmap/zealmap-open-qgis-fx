#include "CQRulerBarGraphicsView.h"
#include "qgsmapcanvas.h"

//内存泄露，但是没有提示是哪一句内存泄露
//在你相应的的cpp文件中加上如下代码
//就可以追踪是哪一个new操作引起该问题的
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


QRulerBarGraphicsView::QRulerBarGraphicsView(QgsMapCanvas* mapCanvas, QWidget *parent, int hor)
	:QGraphicsView(parent), m_mapCanvas(mapCanvas), m_hor(hor)
{
	QObject::connect(mapCanvas, &QgsMapCanvas::extentsChanged, this, &QRulerBarGraphicsView::onMapExtentChanged);
}

QRulerBarGraphicsView::~QRulerBarGraphicsView()
{
	if (m_mapCanvas)
	{
		m_mapCanvas->disconnect(this);
	}

	// delete by caller
	m_mapCanvas = NULL;
}

void QRulerBarGraphicsView::paintEvent(QPaintEvent* event)
{
	qDebug("************MyView::paintEvent*****************");
	// QPainter painter;
	// QWidget::paintEngine: Should no longer be called
	QPainter painter(this->viewport());  //关键这一句

	//QLinearGradient ling(QPointF(70, 70), QPoint(140, 140));  //从起点到终点的渐变
	//ling.setColorAt(0, Qt::blue);  //起点到中心要显示的颜色
	//ling.setColorAt(1, Qt::green);
	//ling.setSpread(QGradient::PadSpread);   //默认显示模式

	//QBrush brush(ling);
	//painter.setBrush(brush);
	//painter.drawRect(0, 0, 200, 200);
	if (m_hor)
	{
		paintHorEvent(event, painter);
	}
	else
	{
		paintVerEvent(event, painter);
	}

	painter.end();
	QGraphicsView::paintEvent(event);
}

void QRulerBarGraphicsView::paintHorEvent(QPaintEvent* event, QPainter& painter)
{
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(Qt::black);
	painter.drawLine(0, height(), width(), height());
	painter.drawLine(0, 0, 0, height());
	painter.drawLine(width(), 0, width(), height());

	QFont font;
	font.setFamily("Microsoft YaHei");
	font.setPointSize(8);
	painter.setFont(font);

	const double startX = m_mapCanvas->extent().xMinimum();
	const double scale = m_mapCanvas->extent().width() / width();

	for (int i = 0; i < width(); i += 50)
	{
		painter.drawLine(i, 0, i, (2 - (i % 100) / 50) * height() / 4);
		if (i % 200 == 0)
		{
			const double viewX = startX + i * scale;
			char sz[255] = { 0 };
			sprintf_s(sz, 255, "%.3f", viewX);
			painter.drawText(i, height() * 3 / 4, QString::fromLocal8Bit(sz));
		}
	}
}

void QRulerBarGraphicsView::paintVerEvent(QPaintEvent* event, QPainter& painter)
{
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(Qt::black);
	painter.drawLine(width(), 0, width(), height());
	painter.drawLine(0, 0, width(), 0);
	painter.drawLine(0, height(), width(), height());

	QFont font;
	font.setFamily("Microsoft YaHei");
	font.setPointSize(8);
	painter.setFont(font);

	const double startY = m_mapCanvas->extent().yMaximum();
	const double scale = m_mapCanvas->extent().height() / height();
	for (int i = 0; i < height(); i += 50)
	{
		painter.resetTransform();
		painter.drawLine(0, i, (2 - (i % 100) / 50) * width() / 4, i);
		if (i % 200 == 0)
		{
			const double viewY = startY - i * scale;
			painter.translate(0, i); //设置旋转中心为文字中心
			painter.rotate(-90);
			char sz[255] = { 0 };
			sprintf_s(sz, 255, "%.3f", viewY);
			painter.drawText(0, width() * 3 / 4, QString::fromLocal8Bit(sz));
		}
	}
}

void QRulerBarGraphicsView::onMapExtentChanged()
{
	repaint();
}
