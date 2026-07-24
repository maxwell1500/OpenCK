#ifndef LOADORDERDIALOG_H
#define LOADORDERDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>

class LoadOrderDialog : public QDialog
{
    Q_OBJECT

public:
    LoadOrderDialog(const QStringList& currentOrder, QWidget* parent = nullptr);
    ~LoadOrderDialog();

    QStringList getLoadOrder() const;

private slots:
    void moveUp();
    void moveDown();
    void removePlugin();
    void addPlugin();

private:
    QListWidget* loadOrderList;
    QPushButton* moveUpButton;
    QPushButton* moveDownButton;
    QPushButton* removeButton;
    QPushButton* addButton;
    QStringList currentOrder;
};

#endif // LOADORDERDIALOG_H
