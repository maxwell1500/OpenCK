#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class Data;

class UseInfoDialog : public QDialog
{
    Q_OBJECT

public:
    UseInfoDialog(Data* data, quint32 targetFormId, const QString& targetEditorId, QWidget* parent = nullptr);
    ~UseInfoDialog() override = default;

    int totalUsesCount() const { return mTotalCount; }

signals:
    void referenceDoubleClicked(quint32 refrFormId);

private slots:
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onOpenSelected();

private:
    void populateUses();
    QString cellNameForFormId(quint32 cellFormId) const;

    Data* mData;
    quint32 mTargetFormId;
    QString mTargetEditorId;
    QTreeWidget* mTreeWidget;
    QLabel* mHeaderLabel;
    QPushButton* mOpenButton;
    QPushButton* mCloseButton;
    int mTotalCount = 0;
};
