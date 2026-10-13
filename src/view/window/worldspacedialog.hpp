#ifndef WORLDSACEDIALOG_HPP
#define WORLDSACEDIALOG_HPP

#include <QDialog>
#include <QTreeView>
#include <QAbstractItemModel>

class Data;

class WorldspaceTableModel;

class WorldspacesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WorldspacesDialog(Data* data, QWidget* parent = nullptr);

signals:
    void worldspaceSelected(const QString& editorId);

private slots:
    void onOpenInCellView();
    void onEditDetails();
    void onItemDoubleClicked(const QModelIndex& index);

private:
    Data* mData;
    QTreeView* mTreeView;
    WorldspaceTableModel* mModel;
};

#endif // WORLDSACEDIALOG_HPP