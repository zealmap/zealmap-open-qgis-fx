#pragma once

#include <QMenu>
#include <QAbstractItemModel>

class PartiallyCheckedTreeItem : public QObject {
    Q_OBJECT
public:
    PartiallyCheckedTreeItem(QObject* parent = nullptr);
    PartiallyCheckedTreeItem(QString text, QObject* parent = nullptr);
    ~PartiallyCheckedTreeItem() override;

public:
    virtual QString text() const;
    virtual QString toolTip() const;
    virtual Qt::CheckState checkState() const;
    virtual void setCheckState(Qt::CheckState checked);
    virtual QIcon getIcon();
    virtual void updateTreeView();

    virtual int rowCount() const;

    PartiallyCheckedTreeItem* child(int i);

    virtual bool hasChildren() const;

    void appendRow(PartiallyCheckedTreeItem* child);

    void setData(QVariant val, int role = Qt::UserRole + 1);
    QVariant data(int role = Qt::UserRole + 1);

    PartiallyCheckedTreeItem* pparent();

private:
    Qt::CheckState m_checked = Qt::CheckState::Unchecked;
    QString m_text;

    PartiallyCheckedTreeItem* m_pparent = nullptr;
    QList<PartiallyCheckedTreeItem*> m_children;
    QMap<int, QVariant> m_datas;

    friend class QPartiallyCheckedTreeItemModel;
};

class PartiallyCheckedTreeItemRoot : public PartiallyCheckedTreeItem {
    Q_OBJECT
public:
    PartiallyCheckedTreeItemRoot(QObject* parent = nullptr);

    virtual void updateTreeView() override;
signals:
    void updateTreeViewRequest();
};

class QPartiallyCheckedTreeItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    QPartiallyCheckedTreeItemModel(QObject* parent);
    ~QPartiallyCheckedTreeItemModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

public:
    bool isCheckable() const;
    PartiallyCheckedTreeItem* invisibleRootItem();
    void clear();

public slots:
    void requestUpdateTreeView();

private:
    PartiallyCheckedTreeItem* index2node(const QModelIndex& index) const;
    QModelIndex indexOfParentLayerTreeNode(PartiallyCheckedTreeItem* parentNode) const;

private:
    bool _isCheckable = true;

    PartiallyCheckedTreeItemRoot* _root = nullptr;
};
