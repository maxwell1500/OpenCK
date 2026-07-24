#ifndef LOADORDEROPTIMIZERDIALOG_HPP
#define LOADORDEROPTIMIZERDIALOG_HPP

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QTextEdit>
#include <QAbstractItemView>
#include <QStringList>
#include <QMap>
#include <QSet>

class Data;

class LoadOrderOptimizerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoadOrderOptimizerDialog(Data* data, QWidget* parent = nullptr);
    ~LoadOrderOptimizerDialog();

private slots:
    void onAnalyze();
    void onOptimize();
    void onApply();
    void onMoveUp();
    void onMoveDown();
    void onSortWithLoot();
    void onValidate();
    void onAutoFix();

private:
    void setupUI();
    void analyzeDependencies();
    void optimizeLoadOrder();
    void validate();
    void autoFix();

    bool detectCircularDepsDFS(const QString& node, QSet<QString>& visited, QSet<QString>& recursionStack, QStringList& cyclePath);
    void buildDependencyTreeWidget();
    QString findPluginPath(const QString& pluginName) const;

    Data* mData;
    QListWidget* pluginList;
    QTextBrowser* reportBrowser;
    QTreeWidget* mDependencyTree;
    QTextEdit* mValidationReport;
    QPushButton* analyzeButton;
    QPushButton* optimizeButton;
    QPushButton* applyButton;
    QPushButton* lootSortButton;
    QPushButton* validateButton;
    QPushButton* autoFixButton;
    QLabel* statusLabel;

    QStringList currentOrder;
    QMap<QString, QStringList> dependencyGraph;
    QMap<QString, QStringList> mPluginMasters;
    QStringList optimizedOrder;
    bool analyzed;
    bool hasCircularDeps;
    bool hasMissingMasters;
};

#endif // LOADORDEROPTIMIZERDIALOG_HPP
