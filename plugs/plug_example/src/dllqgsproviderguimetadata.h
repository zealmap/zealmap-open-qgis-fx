#ifndef DLL_QSMAPEXAMPLEPROVIDERGUIMETADATA_H
#define DLL_QSMAPEXAMPLEPROVIDERGUIMETADATA_H

#include "qgsproviderguimetadata.h"

#define QSMAP_EXAMPLE_GUI_PROVIDER_KEY "qsmap_example_gui_provider"

class QsMapExampleProviderGuiMetadata : public QgsProviderGuiMetadata
{
public:
    QsMapExampleProviderGuiMetadata();

	void registerGui(QMainWindow* widget) override;
};

#endif // DLL_QSMAPEXAMPLEPROVIDERGUIMETADATA_H
