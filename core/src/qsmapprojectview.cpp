#include "qsmapprojectview.h"
#include "qsmapprojectviewitemdelegate.h"
#include <QHeaderView>
#include <QScrollBar>
#include <QMimeData>
#include <QMenu>
#include <qgis.h>

#include <qttreepropertybrowser.h>
#include <qsmapapp.h>
#include <QStandardItemModel>

QsMapProjectTreeNode::QsMapProjectTreeNode(QObject* parent) : QObject(parent) {

}

QsMapProjectTreeNode::~QsMapProjectTreeNode() {

}

QString QsMapProjectTreeNode::text() const {
	return QString();
}

QString QsMapProjectTreeNode::toolTip() const {
	return QString();
}

QIcon QsMapProjectTreeNode::getIcon() const {
	return QIcon();
}

void QsMapProjectTreeNode::onCurrentChanged()
{
	QsMapApp::instance()->clearPropertyBrowser();
}

void QsMapProjectTreeNode::createContextMenu(QMenu* rootMenu) const
{
}

void QsMapProjectTreeNode::updateTreeView()
{
	QsMapProjectTreeNode* treeModelParent = qobject_cast<QsMapProjectTreeNode*>(parent());
	if (treeModelParent) {
		treeModelParent->updateTreeView();
	}
}

const QsMapProjectTreeNode* QsMapProjectTreeNode::getRoot() const {
	QsMapProjectTreeNode* treeModelParent = qobject_cast<QsMapProjectTreeNode*>(parent());
	if (treeModelParent) {
		return treeModelParent->getRoot();
	}
	return this;
}

void QsMapProjectTreeNode::collapse()
{
	collapse(this);
}

void QsMapProjectTreeNode::expand()
{
	expand(this);
}

void QsMapProjectTreeNode::collapse(QsMapProjectTreeNode* the)
{
	QsMapProjectTreeNode* treeModelParent = qobject_cast<QsMapProjectTreeNode*>(parent());
	if (treeModelParent) {
		treeModelParent->collapse(the);
	}
}

void QsMapProjectTreeNode::expand(QsMapProjectTreeNode* the)
{
	QsMapProjectTreeNode* treeModelParent = qobject_cast<QsMapProjectTreeNode*>(parent());
	if (treeModelParent) {
		treeModelParent->expand(the);
	}
}

QsMapProjectTreeNode* QsMapProjectTreeNode::getRoot()
{
	QsMapProjectTreeNode* treeModelParent = qobject_cast<QsMapProjectTreeNode*>(parent());
	if (treeModelParent) {
		return treeModelParent->getRoot();
	}
	return this;
}

//============================QsMapProjectTreeGroup============================
QsMapProjectTreeGroup::QsMapProjectTreeGroup(QObject* parent) : QsMapProjectTreeNode(parent) {

}

QsMapProjectTreeGroup::~QsMapProjectTreeGroup() {

}

//============================QsMapProjectTree============================
QsMapProjectTreeRoot::QsMapProjectTreeRoot(QObject* parent)
	: QsMapProjectTreeGroup(parent) {
}

QsMapProjectTreeRoot::~QsMapProjectTreeRoot() {

}
QsMapProjectTreeRoot* QsMapProjectTreeRoot::getRoot()
{
	return this;
}
const QsMapProjectTreeRoot* QsMapProjectTreeRoot::getRoot() const
{
	return this;
}
// =============================QsMapProject=============================
QsMapProject* QsMapProject::sProject = nullptr;
QsMapProject::QsMapProject(QObject* parent)
	: QsMapProjectTreeGroup(parent)
	, mRootGroup(nullptr) {

}

QsMapProject::~QsMapProject() {

}

QsMapProject* QsMapProject::instance()
{
	if (!sProject)
	{
		sProject = new QsMapProject;
	}
	return sProject;
}

QsMapProjectTreeRoot* QsMapProject::projectTreeRoot() const
{
	return mRootGroup;
}

void QsMapProject::newProject(QsMapProjectTreeRoot* root)
{
	closeProject();

	mRootGroup = root;
	mRootGroup->setParent(this);

	updateTreeView();
}

void QsMapProject::closeProject()
{
	if (mRootGroup) {
		delete mRootGroup;
	}

	mRootGroup = nullptr;

	emit requestCloseTreeView();
}

void QsMapProject::updateTreeView()
{
	emit requestUpdateTreeView();
}

void QsMapProject::collapse(QsMapProjectTreeNode* the)
{
	emit requestCollapse(the);
}

void QsMapProject::expand(QsMapProjectTreeNode* the)
{
	emit requestExpand(the);
}

//============================QsMapProjectTreeModel============================

QsMapProjectTreeModel::QsMapProjectTreeModel(QsMapProject* treeRoot, QObject* parent)
	: QAbstractItemModel(parent)
	, mRootNode(treeRoot) {
	connect(treeRoot, &QsMapProject::requestUpdateTreeView, this, &QsMapProjectTreeModel::onRequestUpdateTreeView);
	connect(treeRoot, &QsMapProject::requestCloseTreeView, this, &QsMapProjectTreeModel::onRequestCloseTreeView);
	connect(treeRoot, &QsMapProject::requestCollapse, this, &QsMapProjectTreeModel::onRequestCollapse);
	connect(treeRoot, &QsMapProject::requestExpand, this, &QsMapProjectTreeModel::onRequestExpand);
}

QsMapProjectTreeModel::~QsMapProjectTreeModel() {

}

void QsMapProjectTreeModel::onRequestUpdateTreeView() {
	emit requestUpdateTreeView();
}

void QsMapProjectTreeModel::onRequestCloseTreeView() {
	emit requestCloseTreeView();
}

QsMapProjectTreeNode* QsMapProjectTreeModel::index2node(const QModelIndex& index) const
{
	if (!index.isValid())
		return mRootNode;

	QObject* obj = reinterpret_cast<QObject*>(index.internalPointer());
	return qobject_cast<QsMapProjectTreeNode*>(obj);
}

int QsMapProjectTreeModel::rowCount(const QModelIndex& parent) const
{
	QsMapProjectTreeNode* n = index2node(parent);
	if (!n)
		return 0;

	return n->children().count();
}

int QsMapProjectTreeModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
		return 1;
}

QModelIndex QsMapProjectTreeModel::index(int row, int column, const QModelIndex& parent) const
{
	if (column < 0 || column >= columnCount(parent) ||
		row < 0 || row >= rowCount(parent))
		return QModelIndex();

	QsMapProjectTreeNode* n = index2node(parent);
	if (!n)
		return QModelIndex(); // have no children

	return createIndex(row, column, static_cast<QObject*>(n->children().at(row)));
}

QModelIndex QsMapProjectTreeModel::parent(const QModelIndex& child) const
{
	if (!child.isValid())
		return QModelIndex();

	if (QsMapProjectTreeNode* n = index2node(child))
	{
		return indexOfParentLayerTreeNode(n->parent()); // must not be null
	}
	else
	{
		Q_ASSERT(false); // no other node types!
		return QModelIndex();
	}
}

QModelIndex QsMapProjectTreeModel::indexOfParentLayerTreeNode(QObject* parentNode) const
{
	Q_ASSERT(parentNode);

	QObject* grandParentNode = parentNode->parent();
	if (!grandParentNode)
		return QModelIndex();  // root node -> invalid index

	int row = grandParentNode->children().indexOf(parentNode);
	Q_ASSERT(row >= 0);

	return createIndex(row, 0, static_cast<QObject*>(parentNode));
}

QVariant QsMapProjectTreeModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || index.column() > 1)
		return QVariant();

	QsMapProjectTreeNode* node = index2node(index);
	if (role == Qt::DisplayRole || role == Qt::EditRole)
	{
		return node->text();
	}
	else if (role == Qt::DecorationRole && index.column() == 0)
	{
		return node->getIcon();
	}
	else if (role == Qt::ToolTipRole)
	{
		return node->toolTip();
	}

	return QVariant();
}


Qt::ItemFlags QsMapProjectTreeModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		Qt::ItemFlags rootFlags = Qt::ItemFlags();
		return rootFlags;
	}

	Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	QsMapProjectTreeNode* node = index2node(index);
	return f;
}

bool QsMapProjectTreeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	QsMapProjectTreeNode* node = index2node(index);
	if (!node)
		return QAbstractItemModel::setData(index, value, role);

	if (role == Qt::CheckStateRole)
	{
		return true;
	}
	else if (role == Qt::EditRole)
	{
	}

	return QAbstractItemModel::setData(index, value, role);
}


Qt::DropActions QsMapProjectTreeModel::supportedDropActions() const
{
	return Qt::CopyAction | Qt::MoveAction;
}

QStringList QsMapProjectTreeModel::mimeTypes() const
{
	QStringList types;
	types << QStringLiteral("application/qgis.layertreemodeldata");
	return types;
}

QMimeData* QsMapProjectTreeModel::mimeData(const QModelIndexList& indexes) const
{
	// Sort the indexes. Depending on how the user selected the items, the indexes may be unsorted.
	QModelIndexList sortedIndexes = indexes;
	std::sort(sortedIndexes.begin(), sortedIndexes.end(), std::less<QModelIndex>());

	QList<QsMapProjectTreeNode*> nodesFinal = indexes2nodes(sortedIndexes, true);

	if (nodesFinal.isEmpty())
		return nullptr;

	QMimeData* mimeData = new QMimeData();
	return mimeData;
}

bool QsMapProjectTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
	if (action == Qt::IgnoreAction)
		return true;

	if (!data->hasFormat(QStringLiteral("application/qgis.layertreemodeldata")))
		return false;

	if (column >= columnCount(parent))
		return false;

	// don't accept drops from some layer tree subclasses to non-matching subclasses
	const QString restrictTypes(data->data(QStringLiteral("application/qgis.restrictlayertreemodelsubclass")));
	if (!restrictTypes.isEmpty() && restrictTypes != QString(metaObject()->className()))
		return false;

	QsMapProjectTreeNode* nodeParent = index2node(parent);
	return true;
}

bool QsMapProjectTreeModel::removeRows(int row, int count, const QModelIndex& parent)
{
	QsMapProjectTreeNode* parentNode = index2node(parent);
	return false;
}

QList<QsMapProjectTreeNode*> QsMapProjectTreeModel::indexes2nodes(const QModelIndexList& list, bool skipInternal) const
{
	QList<QsMapProjectTreeNode*> nodes;
	const auto constList = list;
	for (const QModelIndex& index : constList)
	{
		QsMapProjectTreeNode* node = index2node(index);
		if (!node)
			continue;

		nodes << node;
	}

	if (!skipInternal)
		return nodes;

	// remove any children of nodes if both parent node and children are selected
	QList<QsMapProjectTreeNode*> nodesFinal;
	for (QsMapProjectTreeNode* node : std::as_const(nodes))
	{
		if (!_isChildOfNodes(node, nodes))
			nodesFinal << node;
	}

	return nodesFinal;
}

QModelIndex QsMapProjectTreeModel::indexFromItem(const QsMapProjectTreeNode* item) const
{
	if (item && item->parent())
	{
		return indexOfParentLayerTreeNode((QObject*)item);
	}

	return QModelIndex();
}

void QsMapProjectTreeModel::onRequestCollapse(QsMapProjectTreeNode* the)
{
	QModelIndex idx = indexFromItem(the);
	emit requestCollapse(idx);
}

void QsMapProjectTreeModel::onRequestExpand(QsMapProjectTreeNode* the)
{
	QModelIndex idx = indexFromItem(the);
	emit requestExpand(idx);
}

bool QsMapProjectTreeModel::_isChildOfNode(QObject* child, QsMapProjectTreeNode* node) const
{
	if (!child->parent())
		return false;

	if (child->parent() == node)
		return true;

	return _isChildOfNode(child->parent(), node);
}

bool QsMapProjectTreeModel::_isChildOfNodes(QObject* child, const QList<QsMapProjectTreeNode*>& nodes) const
{
	for (QsMapProjectTreeNode* n : nodes)
	{
		if (_isChildOfNode(child, n))
			return true;
	}

	return false;
}

// =============================QsMapProjectTreeView=============================
QsMapProjectTreeView::QsMapProjectTreeView(QWidget* parent)
	: QTreeView(parent)
{
	setHeaderHidden(true);

	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setEditTriggers(EditKeyPressed);
	//setExpandsOnDoubleClick(false); // normally used for other actions

	// Ensure legend graphics are scrollable
	header()->setStretchLastSection(false);
	header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	// If vertically scrolling by item, legend graphics can get clipped
	setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	setSelectionMode(ExtendedSelection);
	setDefaultDropAction(Qt::MoveAction);

	// we need a custom item delegate in order to draw indicators
	setItemDelegate(new QsMapProjectTreeViewItemDelegate(this));
	setStyle(new QsMapProjectTreeViewProxyStyle(this));

	setLayerMarkWidth(static_cast<int>(QFontMetricsF(font()).horizontalAdvance('l') * Qgis::UI_SCALE_FACTOR));

	connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &QsMapProjectTreeView::onHorizontalScroll);
}

QsMapProjectTreeView::~QsMapProjectTreeView()
{
	delete mMenuProvider;
}

void QsMapProjectTreeView::setMenuProvider(IfaceQsMapProjectTreeViewMenuProvider* menuProvider)
{
	delete mMenuProvider;
	mMenuProvider = menuProvider;
}

void QsMapProjectTreeView::setModel(QAbstractItemModel* model) {
	QTreeView::setModel(model);

	QsMapProjectTreeModel* treeModel = qobject_cast<QsMapProjectTreeModel*>(model);

	connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &QsMapProjectTreeView::onCurrentChanged);
	connect(treeModel, &QsMapProjectTreeModel::requestUpdateTreeView, this, &QsMapProjectTreeView::onRequestUpdateTreeView);
	connect(treeModel, &QsMapProjectTreeModel::requestCloseTreeView, this, &QsMapProjectTreeView::onRequestCloseTreeView);
	connect(treeModel, &QsMapProjectTreeModel::requestCollapse, this, &QsMapProjectTreeView::onRequestCollapse);
	connect(treeModel, &QsMapProjectTreeModel::requestExpand, this, &QsMapProjectTreeView::onRequestExpand);
	connect(treeModel, &QAbstractItemModel::dataChanged, this, &QsMapProjectTreeView::onDataChanged);

	this->expandAll();
}

void QsMapProjectTreeView::onCurrentChanged()
{
	QsMapProjectTreeNode* node = index2node(currentIndex());

	if (node) {
		node->onCurrentChanged();
	}
}

void QsMapProjectTreeView::onRequestCollapse(const QModelIndex& idx)
{
	this->collapse(idx);
}

void QsMapProjectTreeView::onRequestExpand(const QModelIndex& idx)
{
	this->expand(idx);
}

void QsMapProjectTreeView::onHorizontalScroll(int value)
{
	Q_UNUSED(value)
		viewport()->update();
}

QList<QgsLayerTreeViewIndicator*> QsMapProjectTreeView::indicators(QsMapProjectTreeNode* node) const
{
	return mIndicators.value(node);
}

QsMapProjectTreeNode* QsMapProjectTreeView::index2node(const QModelIndex& index) const
{
	return layerTreeModel()->index2node(index);
}

QsMapProjectTreeModel* QsMapProjectTreeView::layerTreeModel() const
{
	QsMapProjectTreeModel* treeModel = qobject_cast<QsMapProjectTreeModel*>(model());
	return treeModel;
}

void QsMapProjectTreeView::onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
	Q_UNUSED(topLeft)
		Q_UNUSED(bottomRight)

		// If an item is resized asynchronously (e.g. wms legend)
		// The items below will need to be shifted vertically.
		// This doesn't happen automatically, unless the viewport update is triggered.

		if (roles.contains(Qt::SizeHintRole))
			viewport()->update();

	//checkModel();
}

void QsMapProjectTreeView::onRequestUpdateTreeView()
{
	doItemsLayout();
}

void QsMapProjectTreeView::onRequestCloseTreeView()
{
	setCurrentIndex(QModelIndex());

	doItemsLayout();
}

void QsMapProjectTreeView::resizeEvent(QResizeEvent* event)
{
	// Since last column is resized to content (instead of stretched), the active
	// selection rectangle ends at width of widest visible item in tree,
	// regardless of which item is selected. This causes layer indicators to
	// become 'inactive' (not clickable and no tool tip) unless their rectangle
	// enters the view item's selection (active) rectangle.
	// Always resetting the minimum section size relative to the viewport ensures
	// the view item's selection rectangle extends to the right edge of the
	// viewport, which allows indicators to become active again.
	header()->setMinimumSectionSize(viewport()->width());
	QTreeView::resizeEvent(event);
}

void QsMapProjectTreeView::mouseReleaseEvent(QMouseEvent* event)
{
	// we need to keep last mouse position in order to know whether to emit an indicator's clicked() signal
	// (the item delegate needs to know which indicator has been clicked)
	mLastReleaseMousePos = event->pos();
	QTreeView::mouseReleaseEvent(event);
}
void QsMapProjectTreeView::contextMenuEvent(QContextMenuEvent* event)
{
	if (!mMenuProvider)
		return;

	const QModelIndex idx = indexAt(event->pos());
	if (!idx.isValid())
		setCurrentIndex(QModelIndex());

	QMenu* menu = mMenuProvider->createContextMenu();
	if (menu)
	{
		//emit contextMenuAboutToShow(menu);

		if (menu->actions().count() != 0)
			menu->exec(mapToGlobal(event->pos()));
		delete menu;
	}
}


// =============================QsMapProjectTreeViewMenuProvider=============================
QsMapProjectTreeViewMenuProvider::QsMapProjectTreeViewMenuProvider(QsMapProjectTreeView* view)
	: mView(view)
{
}

QMenu* QsMapProjectTreeViewMenuProvider::createContextMenu()
{
	QMenu* menu = new QMenu;
	const QModelIndex idx = mView->currentIndex();
	if (!idx.isValid()) {

	}
	else if (QsMapProjectTreeNode* node = mView->index2node(idx)) {
		node->createContextMenu(menu);
	}
	return menu;
}