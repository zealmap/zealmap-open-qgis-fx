#ifndef QSMAPPROJECTVIEW_H
#define QSMAPPROJECTVIEW_H

#include <qsmapapi.h>

#include <QWidget>
#include <QTreeView>
#include <qgslayertreeviewindicator.h>

class QsMapProjectTreeModel;
class QsMapProjectTreeView;
class QsMapProjectDataSources;
class QsMapProject;

class QSMAP_APP_API IfaceQsMapProjectTreeViewMenuProvider
{
public:
	virtual ~IfaceQsMapProjectTreeViewMenuProvider() = default;

	//! Returns a newly created menu instance (or NULLPTR on error)
	virtual QMenu* createContextMenu() = 0;
};

class QSMAP_APP_API QsMapProjectTreeNode : public QObject {
	Q_OBJECT
public:
	//! Constructor for QsMapProjectTreeNode
	explicit QsMapProjectTreeNode(QObject* parent = nullptr);
	~QsMapProjectTreeNode() override;

public:
	virtual QString text() const;
	virtual QString toolTip() const;
	virtual QIcon getIcon() const;

	virtual void onCurrentChanged();

	virtual void createContextMenu(QMenu* rootMenu) const;

	virtual void updateTreeView();

	virtual QsMapProjectTreeNode* getRoot();
	virtual const QsMapProjectTreeNode* getRoot() const;

	virtual void collapse();
	virtual void expand();
protected:
	virtual void collapse(QsMapProjectTreeNode* the);
	virtual void expand(QsMapProjectTreeNode* the);
private:
};

class QSMAP_APP_API QsMapProjectTreeGroup : public QsMapProjectTreeNode {
	Q_OBJECT
public:
	explicit QsMapProjectTreeGroup(QObject* parent = nullptr);
	~QsMapProjectTreeGroup() override;
};

class QSMAP_APP_API QsMapProjectTreeRoot : public QsMapProjectTreeGroup {
	Q_OBJECT
public:
	explicit QsMapProjectTreeRoot(QObject* parent = nullptr);
	~QsMapProjectTreeRoot() override;

	QsMapProjectTreeRoot* getRoot() override;
	const QsMapProjectTreeRoot* getRoot() const override;
private:
};
// ==============================================================================

class QSMAP_APP_API QsMapProject : public QsMapProjectTreeGroup {
	Q_OBJECT

private:
	explicit QsMapProject(QObject* parent = nullptr);
	~QsMapProject() override;
	QsMapProject(QsMapProject const&) = delete;
	QsMapProject& operator=(QsMapProject const&) = delete;

public:
	static QsMapProject* instance();
	QsMapProjectTreeRoot* projectTreeRoot() const;

	void newProject(QsMapProjectTreeRoot* root);
	void closeProject();
	void updateTreeView() override;

protected:
	void collapse(QsMapProjectTreeNode* the) override;
	void expand(QsMapProjectTreeNode* the) override;

signals:
	void requestUpdateTreeView();
	void requestCloseTreeView();
	void requestCollapse(QsMapProjectTreeNode* the);
	void requestExpand(QsMapProjectTreeNode* the);

private:
	static QsMapProject* sProject;
	QsMapProjectTreeRoot* mRootGroup = nullptr;
};

class QSMAP_APP_API QsMapProjectTreeModel : public QAbstractItemModel {
	Q_OBJECT
public:
	explicit QsMapProjectTreeModel(QsMapProject* treeRoot, QObject* parent = nullptr);
	~QsMapProjectTreeModel() override;

	// Implementation of virtual functions from QAbstractItemModel

	int rowCount(const QModelIndex& parent = QModelIndex()) const override;
	int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex& child) const override;
	QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	Qt::ItemFlags flags(const QModelIndex& index) const override;
	bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
	Qt::DropActions supportedDropActions() const override;
	QStringList mimeTypes() const override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;
	bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
	bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

public:
	QsMapProjectTreeNode* index2node(const QModelIndex& index) const;
	QList<QsMapProjectTreeNode*> indexes2nodes(const QModelIndexList& list, bool skipInternal = false) const;

	QModelIndex indexFromItem(const QsMapProjectTreeNode* item) const;

signals:
	void requestUpdateTreeView();
	void requestCloseTreeView();
	void requestCollapse(const QModelIndex& idx);
	void requestExpand(const QModelIndex& idx);

private:
	QModelIndex indexOfParentLayerTreeNode(QObject* parentNode) const;
	bool _isChildOfNodes(QObject* child, const QList<QsMapProjectTreeNode*>& nodes) const;
	bool _isChildOfNode(QObject* child, QsMapProjectTreeNode* node) const;

private slots:
	void onRequestUpdateTreeView();
	void onRequestCloseTreeView();
	void onRequestCollapse(QsMapProjectTreeNode* the);
	void onRequestExpand(QsMapProjectTreeNode* the);

private:
	QsMapProject* mRootNode = nullptr;
};

class QSMAP_APP_API QsMapProjectTreeView : public QTreeView {
	Q_OBJECT
public:
	//! Constructor for QgsLayerTreeView
	explicit QsMapProjectTreeView(QWidget* parent = nullptr);
	~QsMapProjectTreeView() override;

public:
	QList<QgsLayerTreeViewIndicator*> indicators(QsMapProjectTreeNode* node) const;
	QsMapProjectTreeNode* index2node(const QModelIndex& index) const;
	void setLayerMarkWidth(int width) { mLayerMarkWidth = width; }
	int layerMarkWidth() const { return mLayerMarkWidth; }
	QsMapProjectTreeModel* layerTreeModel() const;
	const QPoint& getLastReleaseMousePos() const {
		return mLastReleaseMousePos;
	}

	QStyleOptionViewItem getViewOptions() const {
		return viewOptions();
	}
	void setMenuProvider(IfaceQsMapProjectTreeViewMenuProvider* menuProvider);
	void setModel(QAbstractItemModel* model) override;

private slots:
	void onHorizontalScroll(int value);
	void onDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
	void onRequestUpdateTreeView();
	void onRequestCloseTreeView();
	void onCurrentChanged();
	void onRequestCollapse(const QModelIndex& idx);
	void onRequestExpand(const QModelIndex& idx);
protected:
	void contextMenuEvent(QContextMenuEvent* event) override;

	void mouseReleaseEvent(QMouseEvent* event) override;

	void resizeEvent(QResizeEvent* event) override;

private:
	IfaceQsMapProjectTreeViewMenuProvider* mMenuProvider = nullptr;
	int mLayerMarkWidth = 1;
	QHash< QsMapProjectTreeNode*, QList<QgsLayerTreeViewIndicator*> > mIndicators;
	//! Used by the item delegate for identification of which indicator has been clicked
	QPoint mLastReleaseMousePos;
};

class QSMAP_APP_API QsMapProjectTreeViewMenuProvider : public QObject, public IfaceQsMapProjectTreeViewMenuProvider {
	Q_OBJECT
public:
	QsMapProjectTreeViewMenuProvider(QsMapProjectTreeView* view);

	QMenu* createContextMenu() override;

private:
	QsMapProjectTreeView* mView;
};

#endif
