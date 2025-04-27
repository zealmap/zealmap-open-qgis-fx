#ifndef QSMAPPROJECTVIEWITEMDELEGATE_H
#define QSMAPPROJECTVIEWITEMDELEGATE_H

class QsMapProjectTreeView;

#include "qgsproxystyle.h"
#include <QStyledItemDelegate>

/**
 * Proxy style for layer items with indicators
 */
class QsMapProjectTreeViewProxyStyle : public QgsProxyStyle
{
	Q_OBJECT

public:
	explicit QsMapProjectTreeViewProxyStyle(QsMapProjectTreeView* treeView);

	QRect subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const override;

	static const auto SE_LayerTreeItemIndicator = SE_CustomBase + 1;

private:
	QsMapProjectTreeView* mLayerTreeView;
};


/**
 * Item delegate that adds drawing of indicators
 */
class QsMapProjectTreeViewItemDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit QsMapProjectTreeViewItemDelegate(QsMapProjectTreeView* parent);

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	bool helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private slots:
	void onClicked(const QModelIndex& index);

private:
	QsMapProjectTreeView* mLayerTreeView;
};

/// @endcond

#endif // QS101PROJECTVIEWITEMDELEGATE_H
