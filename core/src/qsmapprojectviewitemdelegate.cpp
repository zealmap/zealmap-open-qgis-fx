#include "qsmapprojectviewitemdelegate.h"

#include "qgslayertreemodel.h"
#include "qsmapprojectview.h"
#include "qgslayertreeviewindicator.h"

#include <QBrush>
#include <QHelpEvent>
#include <QMenu>
#include <QPen>
#include <QToolTip>

/// @cond PRIVATE

QsMapProjectTreeViewProxyStyle::QsMapProjectTreeViewProxyStyle(QsMapProjectTreeView* treeView)
	: QgsProxyStyle(treeView)
	, mLayerTreeView(treeView)
{
}


QRect QsMapProjectTreeViewProxyStyle::subElementRect(QStyle::SubElement element, const QStyleOption* option, const QWidget* widget) const
{
	if (element == SE_LayerTreeItemIndicator)
	{
		if (const QStyleOptionViewItem* vopt = qstyleoption_cast<const QStyleOptionViewItem*>(option))
		{
			if (QsMapProjectTreeNode* node = mLayerTreeView->index2node(vopt->index))
			{
				const int count = mLayerTreeView->indicators(node).count();
				if (count)
				{
					const QRect vpr = mLayerTreeView->viewport()->rect();
					const QRect r = QProxyStyle::subElementRect(SE_ItemViewItemText, option, widget);
					const int indiWidth = r.height() * count;
					const int spacing = r.height() / 10;
					const int vpIndiWidth = vpr.width() - indiWidth - spacing - mLayerTreeView->layerMarkWidth();
					return QRect(vpIndiWidth, r.top(), indiWidth, r.height());
				}
			}
		}
	}
	return QProxyStyle::subElementRect(element, option, widget);
}


// -----


QsMapProjectTreeViewItemDelegate::QsMapProjectTreeViewItemDelegate(QsMapProjectTreeView* parent)
	: QStyledItemDelegate(parent)
	, mLayerTreeView(parent)
{
	connect(mLayerTreeView, &QsMapProjectTreeView::clicked, this, &QsMapProjectTreeViewItemDelegate::onClicked);
}


void QsMapProjectTreeViewItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyledItemDelegate::paint(painter, option, index);

	QsMapProjectTreeNode* node = mLayerTreeView->index2node(index);
	if (!node)
		return;

	QStyleOptionViewItem opt = option;
	initStyleOption(&opt, index);

	const QColor baseColor = opt.palette.base().color();
	const QRect tRect = mLayerTreeView->style()->subElementRect(QStyle::SE_ItemViewItemText, &opt, mLayerTreeView);

	const bool shouldShowLayerMark = tRect.left() < 0;  // Layer/group node icon not visible anymore?
	if (shouldShowLayerMark)
	{
		const int tPadding = tRect.height() / 10;
		const QRect mRect(mLayerTreeView->viewport()->rect().right() - mLayerTreeView->layerMarkWidth(), tRect.top() + tPadding, mLayerTreeView->layerMarkWidth(), tRect.height() - tPadding * 2);
		const QBrush pb = painter->brush();
		const QPen pp = painter->pen();
		painter->setPen(QPen(Qt::NoPen));
		QBrush b = QBrush(opt.palette.mid());
		QColor bc = b.color();
		// mix mid color with base color for a less dominant, yet still opaque, version of the color
		bc.setRed(static_cast<int>(bc.red() * 0.3 + baseColor.red() * 0.7));
		bc.setGreen(static_cast<int>(bc.green() * 0.3 + baseColor.green() * 0.7));
		bc.setBlue(static_cast<int>(bc.blue() * 0.3 + baseColor.blue() * 0.7));
		b.setColor(bc);
		painter->setBrush(b);
		painter->drawRect(mRect);
		painter->setBrush(pb);
		painter->setPen(pp);
	}

	const QList<QgsLayerTreeViewIndicator*> indicators = mLayerTreeView->indicators(node);
	if (indicators.isEmpty())
		return;

	const QRect indRect = mLayerTreeView->style()->subElementRect(static_cast<QStyle::SubElement>(QsMapProjectTreeViewProxyStyle::SE_LayerTreeItemIndicator), &opt, mLayerTreeView);
	const int spacing = indRect.height() / 10;
	const int h = indRect.height();
	int x = indRect.left();

	for (QgsLayerTreeViewIndicator* indicator : indicators)
	{
		const QRect rect(x + spacing, indRect.top() + spacing, h - spacing * 2, h - spacing * 2);
		// Add a little more padding so the icon does not look misaligned to background
		const QRect iconRect(x + spacing * 2, indRect.top() + spacing * 2, h - spacing * 4, h - spacing * 4);
		x += h;

		QIcon::Mode mode = QIcon::Normal;
		if (!(opt.state & QStyle::State_Enabled))
			mode = QIcon::Disabled;
		else if (opt.state & QStyle::State_Selected)
			mode = QIcon::Selected;

		// Draw indicator background, for when floating over text content
		const qreal bradius = spacing;
		const QBrush pb = painter->brush();
		const QPen pp = painter->pen();
		QBrush b = QBrush(opt.palette.midlight());
		QColor bc = b.color();
		bc.setRed(static_cast<int>(bc.red() * 0.3 + baseColor.red() * 0.7));
		bc.setGreen(static_cast<int>(bc.green() * 0.3 + baseColor.green() * 0.7));
		bc.setBlue(static_cast<int>(bc.blue() * 0.3 + baseColor.blue() * 0.7));
		b.setColor(bc);
		painter->setBrush(b);
		painter->setPen(QPen(QBrush(opt.palette.mid()), 0.25));
		painter->drawRoundedRect(rect, bradius, bradius);
		painter->setBrush(pb);
		painter->setPen(pp);

		indicator->icon().paint(painter, iconRect, Qt::AlignCenter, mode);
	}
}

static void _fixStyleOption(QStyleOptionViewItem& opt)
{
	// This makes sure our delegate behaves correctly across different styles. Unfortunately there is inconsistency
	// in how QStyleOptionViewItem::showDecorationSelected is prepared for paint() vs what is returned from view's viewOptions():
	// - viewOptions() returns it based on style's SH_ItemView_ShowDecorationSelected hint
	// - for paint() there is extra call to QTreeViewPrivate::adjustViewOptionsForIndex() which makes it
	//   always true if view's selection behavior is SelectRows (which is the default and our case with layer tree view)
	// So for consistency between different calls we override it to what we get in paint() method ... phew!
	opt.showDecorationSelected = true;
}

bool QsMapProjectTreeViewItemDelegate::helpEvent(QHelpEvent* event, QAbstractItemView* view, const QStyleOptionViewItem& option, const QModelIndex& index)
{
	if (event && event->type() == QEvent::ToolTip)
	{
		QsMapProjectTreeNode* node = mLayerTreeView->index2node(index);
		if (node)
		{
			const QList<QgsLayerTreeViewIndicator*> indicators = mLayerTreeView->indicators(node);
			if (!indicators.isEmpty())
			{
				QStyleOptionViewItem opt = option;
				initStyleOption(&opt, index);
				_fixStyleOption(opt);

				const QRect indRect = mLayerTreeView->style()->subElementRect(static_cast<QStyle::SubElement>(QsMapProjectTreeViewProxyStyle::SE_LayerTreeItemIndicator), &opt, mLayerTreeView);

				if (indRect.contains(event->pos()))
				{
					const int indicatorIndex = (event->pos().x() - indRect.left()) / indRect.height();
					if (indicatorIndex >= 0 && indicatorIndex < indicators.count())
					{
						const QString tooltip = indicators[indicatorIndex]->toolTip();
						if (!tooltip.isEmpty())
						{
							QToolTip::showText(event->globalPos(), tooltip, view);
							return true;
						}
					}
				}
			}
		}
	}
	return QStyledItemDelegate::helpEvent(event, view, option, index);
}


void QsMapProjectTreeViewItemDelegate::onClicked(const QModelIndex& index)
{
	QsMapProjectTreeNode* node = mLayerTreeView->index2node(index);
	if (!node)
		return;

	const QList<QgsLayerTreeViewIndicator*> indicators = mLayerTreeView->indicators(node);
	if (indicators.isEmpty())
		return;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QStyleOptionViewItem opt(mLayerTreeView->getViewOptions());
#else
	QStyleOptionViewItem opt;
	mLayerTreeView->initViewItemOption(&opt);
#endif

	opt.rect = mLayerTreeView->visualRect(index);
	initStyleOption(&opt, index);
	_fixStyleOption(opt);

	const QRect indRect = mLayerTreeView->style()->subElementRect(static_cast<QStyle::SubElement>(QsMapProjectTreeViewProxyStyle::SE_LayerTreeItemIndicator), &opt, mLayerTreeView);

	const QPoint& pos = mLayerTreeView->getLastReleaseMousePos();
	if (indRect.contains(pos))
	{
		const int indicatorIndex = (pos.x() - indRect.left()) / indRect.height();
		if (indicatorIndex >= 0 && indicatorIndex < indicators.count())
			emit indicators[indicatorIndex]->clicked(index);
	}
}

/// @endcond
