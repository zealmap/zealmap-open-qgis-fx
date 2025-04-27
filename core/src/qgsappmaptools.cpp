/***************************************************************************
  qgsappmaptools.cpp
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

#include "qgsappmaptools.h"
#include "qgsmaptool.h"
#include "qgsmaptoolzoom.h"
#include "qgsmaptoolpan.h"

//
// QgsAppMapTools
//

QgsAppMapTools::QgsAppMapTools( QgsMapCanvas *canvas )
{
  mTools.insert( Tool::ZoomIn, new QgsMapToolZoom( canvas, false /* zoomIn */ ) );
  mTools.insert( Tool::ZoomOut, new QgsMapToolZoom( canvas, true /* zoomOut */ ) );
  mTools.insert( Tool::Pan, new QgsMapToolPan( canvas ) );
}

QgsAppMapTools::~QgsAppMapTools()
{
  for ( auto it = mTools.constBegin(); it != mTools.constEnd(); ++it )
  {
    delete it.value();
  }
}

QgsMapTool *QgsAppMapTools::mapTool( QgsAppMapTools::Tool tool )
{
  return mTools.value( tool );
}
