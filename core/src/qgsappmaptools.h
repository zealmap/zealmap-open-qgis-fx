/***************************************************************************
  qgsappmaptools.h
  --------------------------------------
  Date                 : March 2021
  Copyright            : (C) 2021 by Nyall Dawson
  Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSAPPMAPTOOLS_H
#define QGSAPPMAPTOOLS_H

#include <QList>
#include <QHash>
#include <QPointer>

class QgsMapTool;
class QgsMapCanvas;

class QgsAppMapTools
{
  public:
    enum Tool
    {
      ZoomIn,
      ZoomOut,
      Pan
    };

    QgsAppMapTools( QgsMapCanvas *canvas);
    ~QgsAppMapTools();

    /**
     * Returns the specified \a tool.
     */
    QgsMapTool *mapTool( Tool tool );

  private:

    QHash< Tool, QPointer< QgsMapTool > > mTools;

    // Disable copying as we have pointer members.
    QgsAppMapTools( const QgsAppMapTools & ) = delete;
    QgsAppMapTools &operator= ( const QgsAppMapTools & ) = delete;
};

#endif // QGSAPPMAPTOOLS_H
