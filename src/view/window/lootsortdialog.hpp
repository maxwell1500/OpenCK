#ifndef LOOTSORTDIALOG_HPP
#define LOOTSORTDIALOG_HPP

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QProgressBar>
#include <QStringList>

class Data;
class LootWrapper;

class LootSortDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LootSortDialog(Data* data, QWidget* parent = nullptr);
    ~LootSortDialog();

    QStringList getSortedPlugins() const;

private slots:
    void onSortWithLoot();
    void onApplySortedOrder();
    void onRefreshStatus();

private:
    void setupUI();
    void loadCurrentPlugins();
    void updateStatus();
    void logMessage(const QString& message);

    Data* mData;
    LootWrapper* m_lootWrapper;

    QListWidget* currentOrderList;
    QListWidget* sortedOrderList;
    QTextBrowser* logOutput;

    QPushButton* sortButton;
    QPushButton* applyButton;
    QPushButton* refreshButton;
    QPushButton* closeButton;

    QLabel* statusLabel;
    QLabel* lootStatusLabel;
    QProgressBar* progressBar;

    QStringList m_currentPlugins;
    QStringList m_sortedPlugins;
    bool m_sortingComplete;
};

#endif // LOOTSORTDIALOG_HPP
