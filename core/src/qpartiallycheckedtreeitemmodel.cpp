#include "qpartiallycheckedtreeitemmodel.h"

QPartiallyCheckedTreeItemModel::QPartiallyCheckedTreeItemModel(QObject* parent)
    : QAbstractItemModel(parent)
    , _root(new PartiallyCheckedTreeItemRoot(nullptr))
{
    connect(_root, &PartiallyCheckedTreeItemRoot::updateTreeViewRequest
        , this, &QPartiallyCheckedTreeItemModel::requestUpdateTreeView);
}

QPartiallyCheckedTreeItemModel::~QPartiallyCheckedTreeItemModel()
{

}

int QPartiallyCheckedTreeItemModel::rowCount(const QModelIndex& parent) const
{
    PartiallyCheckedTreeItem* n = index2node(parent);
    if (!n)
        return 0;

    return n->m_children.count();
}

int QPartiallyCheckedTreeItemModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
        return 1;
}

QModelIndex QPartiallyCheckedTreeItemModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column < 0 || column >= columnCount(parent) ||
        row < 0 || row >= rowCount(parent))
        return QModelIndex();

    PartiallyCheckedTreeItem* n = index2node(parent);
    if (!n)
        return QModelIndex(); // have no children

    return createIndex(row, column, (n->m_children.at(row)));
}

QModelIndex QPartiallyCheckedTreeItemModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return QModelIndex();
    PartiallyCheckedTreeItem* n = index2node(child);
    if (n)
    {
        return indexOfParentLayerTreeNode(n->pparent()); // must not be null
    }
    else
    {
        Q_ASSERT(false); // no other node types!
        return QModelIndex();
    }
}

QVariant QPartiallyCheckedTreeItemModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.column() > 1)
        return QVariant();

    PartiallyCheckedTreeItem* node = index2node(index);
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
    else if (role == Qt::CheckStateRole) {
        if (isCheckable()) {
            return node->checkState();
        }
    }
    return QVariant();
}


Qt::ItemFlags QPartiallyCheckedTreeItemModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        Qt::ItemFlags rootFlags = Qt::ItemFlags();
        return rootFlags;
    }

    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    if (isCheckable()) {
        f = f | Qt::ItemIsUserCheckable;
    }

    return f;
}

bool QPartiallyCheckedTreeItemModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    PartiallyCheckedTreeItem* node = index2node(index);
    if (!node)
        return QAbstractItemModel::setData(index, value, role);

    if (role == Qt::CheckStateRole)
    {
        node->setCheckState((Qt::CheckState)value.toInt());
        node->updateTreeView();
        return true;
    }
    else if (role == Qt::EditRole)
    {
    }

    return QAbstractItemModel::setData(index, value, role);
}

bool QPartiallyCheckedTreeItemModel::isCheckable() const
{
    return _isCheckable;
}

PartiallyCheckedTreeItem* QPartiallyCheckedTreeItemModel::invisibleRootItem()
{
    return _root;
}

void QPartiallyCheckedTreeItemModel::clear()
{
    beginResetModel();
    qDeleteAll(_root->m_children);
    _root->m_children.clear();
    endResetModel();

    requestUpdateTreeView();
}

void QPartiallyCheckedTreeItemModel::requestUpdateTreeView()
{
    emit layoutChanged();
}

PartiallyCheckedTreeItem* QPartiallyCheckedTreeItemModel::index2node(const QModelIndex& index) const
{
    if (!index.isValid())
        return _root;

    auto obj = reinterpret_cast<PartiallyCheckedTreeItem*>(index.internalPointer());
    return obj;
}

QModelIndex QPartiallyCheckedTreeItemModel::indexOfParentLayerTreeNode(PartiallyCheckedTreeItem* parentNode) const
{
    if (!parentNode)
        return QModelIndex();  // root node -> invalid index

    Q_ASSERT(parentNode);

    PartiallyCheckedTreeItem* grandParentNode = parentNode->pparent();
    if (!grandParentNode)
        return QModelIndex();  // root node -> invalid index

    int row = grandParentNode->m_children.indexOf(parentNode);
    Q_ASSERT(row >= 0);

    return createIndex(row, 0, (parentNode));
}

PartiallyCheckedTreeItem::PartiallyCheckedTreeItem(QObject* parent) : QObject(parent) {
}

PartiallyCheckedTreeItem::PartiallyCheckedTreeItem(QString text, QObject* parent)
    : QObject(parent), m_text(text)
{
}

PartiallyCheckedTreeItem::~PartiallyCheckedTreeItem()
{
    qDeleteAll(m_children);
}

QString PartiallyCheckedTreeItem::text() const {
    return m_text;
}

QString PartiallyCheckedTreeItem::toolTip() const {
    return QString();
}
Qt::CheckState PartiallyCheckedTreeItem::checkState() const
{
    Qt::CheckState checked = m_checked;
    int check_count = 0;
    int uncheck_count = 0;
    int m_rowCount = m_children.count();
    if (m_rowCount > 0) {
        for (PartiallyCheckedTreeItem* node_child : m_children)
        {
            if (Qt::CheckState::PartiallyChecked == node_child->checkState()) {
                checked = Qt::CheckState::PartiallyChecked;
                break;
            }
            else if (Qt::CheckState::Checked == node_child->checkState()) {
                ++check_count;
            }
            else if (Qt::CheckState::Unchecked == node_child->checkState()) {
                ++uncheck_count;
            }
        }

        if (check_count == m_rowCount)
            return Qt::Checked;
        else if (uncheck_count == m_rowCount)
            return Qt::Unchecked;
        else
            return Qt::PartiallyChecked;
    }

    return checked;
}

void PartiallyCheckedTreeItem::setCheckState(Qt::CheckState checked)
{
    m_checked = checked;

    if (checked != Qt::CheckState::PartiallyChecked) {
        for (PartiallyCheckedTreeItem* node_child : m_children)
        {
            node_child->setCheckState(checked);
        }
    }
}

QIcon PartiallyCheckedTreeItem::getIcon() {
    return QIcon();
}

void PartiallyCheckedTreeItem::updateTreeView()
{
    PartiallyCheckedTreeItem* treeModelParent = (pparent());
    if (treeModelParent) {
        treeModelParent->updateTreeView();
    }
}

int PartiallyCheckedTreeItem::rowCount() const
{
    return m_children.size();
}

PartiallyCheckedTreeItem* PartiallyCheckedTreeItem::child(int i)
{
    return (m_children.at(i));
}

bool PartiallyCheckedTreeItem::hasChildren() const
{
    return rowCount() > 0;
}

void PartiallyCheckedTreeItem::appendRow(PartiallyCheckedTreeItem* child)
{
    m_children.append(child);
    child->m_pparent = this;
}

void PartiallyCheckedTreeItem::setData(QVariant val, int role)
{
    m_datas[role] = val;
}

QVariant PartiallyCheckedTreeItem::data(int role)
{
    if (m_datas.contains(role)) {
        return m_datas[role];
    }

    return QVariant();
}

PartiallyCheckedTreeItem* PartiallyCheckedTreeItem::pparent()
{
    return m_pparent;
}

PartiallyCheckedTreeItemRoot
::PartiallyCheckedTreeItemRoot(QObject* parent) : PartiallyCheckedTreeItem(parent)
{
}

void PartiallyCheckedTreeItemRoot::updateTreeView()
{
    emit updateTreeViewRequest();
}
