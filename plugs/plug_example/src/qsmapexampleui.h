#ifndef QSMAPEXAMPLEUI__02DB4B0AC424_H
#define QSMAPEXAMPLEUI__02DB4B0AC424_H

#include <QObject>
#include <QMainWindow>

class QsMapExampleUi : public QObject
{
	Q_OBJECT
public:
    QsMapExampleUi(QObject* parent = nullptr);

	void registerGui(QMainWindow* widget);

private slots:
	void onOpenAmapActionTriggered();
    void onOpenActionTriggered();
    void onGotoChinaActionActionTriggered();

private:
    void addAmapAction(const QString& url, const QString& name);
	void initToolbar(QMainWindow* widget);
};

#endif // QSMAPEXAMPLEUI__02DB4B0AC424_H
