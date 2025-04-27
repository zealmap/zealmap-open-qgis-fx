#include <qgis.h>
#include <qsmapapp.h>
#include "dllqgsproviderguimetadata.h"

//QGISEXTERN bool providerEnableForApp(QString appName)
//{
//	return SUB_APP_NAME_DBMS == appName;
//}

QGISEXTERN QgsProviderGuiMetadata* providerGuiMetadataFactory()
{
	return new QsMapExampleProviderGuiMetadata();
}
