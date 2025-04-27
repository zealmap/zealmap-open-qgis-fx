#ifndef QRULERBARGRAPHICSVIEW_H
#define QRULERBARGRAPHICSVIEW_H

#include <QtWidgets/qgraphicsview.h>

class QgsMapCanvas;

class QRulerBarGraphicsView :
	public QGraphicsView
{
	Q_OBJECT
public:
	QRulerBarGraphicsView(QgsMapCanvas* mapCanvas, QWidget *parent = Q_NULLPTR, int hor = 1);
	~QRulerBarGraphicsView();

protected:
	void paintEvent(QPaintEvent *) override;
	void paintHorEvent(QPaintEvent* event, QPainter& painter);
	void paintVerEvent(QPaintEvent* event, QPainter& painter);

private slots:
	void onMapExtentChanged();

private:
	QgsMapCanvas* m_mapCanvas;
	int m_hor;
};

#endif