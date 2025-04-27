#include "dllqgsproviderguimetadata.h"
#include <qsmapapp.h>
#include <QToolBar>
#include <QAction>
#include <QMenu>

#include "qsmapexampleui.h"

QsMapExampleProviderGuiMetadata::QsMapExampleProviderGuiMetadata()
	: QgsProviderGuiMetadata(QSMAP_EXAMPLE_GUI_PROVIDER_KEY)
{

}

void QsMapExampleProviderGuiMetadata::registerGui(QMainWindow* widget)
{
	(new QsMapExampleUi(widget))->registerGui(widget);
}
