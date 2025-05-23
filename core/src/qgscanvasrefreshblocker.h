/***************************************************************************
    qgscanvasrefreshblocker.h
    -------------------------
    begin                : July 2022
    copyright            : (C) 2022 by Nyall Dawson
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSCANVASREFRESHBLOCKER_H
#define QGSCANVASREFRESHBLOCKER_H

#include <qgsmapcanvas.h>

class QgsCanvasRefreshBlocker
{
  public:

    QgsCanvasRefreshBlocker(QgsMapCanvas* qgsMapCanvas);
    QgsCanvasRefreshBlocker( const QgsCanvasRefreshBlocker &other ) = delete;
    QgsCanvasRefreshBlocker &operator=( const QgsCanvasRefreshBlocker &other ) = delete;

    void release();

    ~QgsCanvasRefreshBlocker();

  private:
    QgsMapCanvas* mQgsMapCanvas = nullptr;
    bool mReleased = false;
};

#endif // QGSCANVASREFRESHBLOCKER_H
