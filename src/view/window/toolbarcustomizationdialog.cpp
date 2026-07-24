#include "toolbarcustomizationdialog.hpp"
#include "filepaths.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QCoreApplication>
#include <QToolBar>
#include <QAction>

ToolbarCustomizationDialog::ToolbarCustomizationDialog(QToolBar* toolbar, QWidget* parent)
    : QDialog(parent)
    , mToolbar(toolbar)
{
    setWindowTitle("Customize Toolbar");
    setMinimumSize(400, 350);

    auto* layout = new QVBoxLayout(this);

    auto* infoLabel = new QLabel("Drag actions to reorder. Double-click to toggle visibility.");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    mListWidget = new QListWidget();
    mListWidget->setDragDropMode(QAbstractItemView::InternalMove);
    mListWidget->setDefaultDropAction(Qt::MoveAction);
    populateList();
    layout->addWidget(mListWidget);

    auto* btnLayout = new QHBoxLayout();
    mUpBtn = new QPushButton("Move Up");
    mDownBtn = new QPushButton("Move Down");
    mToggleBtn = new QPushButton("Toggle Visibility");
    mSaveBtn = new QPushButton("Save");

    btnLayout->addWidget(mUpBtn);
    btnLayout->addWidget(mDownBtn);
    btnLayout->addWidget(mToggleBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(mSaveBtn);

    layout->addLayout(btnLayout);

    connect(mUpBtn, &QPushButton::clicked, this, &ToolbarCustomizationDialog::moveUp);
    connect(mDownBtn, &QPushButton::clicked, this, &ToolbarCustomizationDialog::moveDown);
    connect(mToggleBtn, &QPushButton::clicked, this, &ToolbarCustomizationDialog::toggleVisibility);
    connect(mSaveBtn, &QPushButton::clicked, this, &ToolbarCustomizationDialog::saveAndClose);
    connect(mListWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        toggleVisibility();
    });
}

void ToolbarCustomizationDialog::populateList()
{
    mListWidget->clear();

    for (QAction* action : mToolbar->actions())
    {
        if (action->isSeparator()) continue;

        auto* item = new QListWidgetItem();
        QString name = action->objectName();
        QString text = action->text();
        if (text.isEmpty()) text = name;

        bool visible = mToolbar->widgetForAction(action) != nullptr;
        QString prefix = visible ? "[+] " : "[-] ";
        item->setText(prefix + text);
        item->setData(Qt::UserRole, name);
        item->setData(Qt::UserRole + 1, visible);
        item->setData(Qt::UserRole + 2, text);
        item->setForeground(visible ? Qt::black : Qt::gray);
        mListWidget->addItem(item);
    }
}

void ToolbarCustomizationDialog::moveUp()
{
    int row = mListWidget->currentRow();
    if (row <= 0) return;

    auto* item = mListWidget->takeItem(row);
    mListWidget->insertItem(row - 1, item);
    mListWidget->setCurrentRow(row - 1);
}

void ToolbarCustomizationDialog::moveDown()
{
    int row = mListWidget->currentRow();
    if (row < 0 || row >= mListWidget->count() - 1) return;

    auto* item = mListWidget->takeItem(row);
    mListWidget->insertItem(row + 1, item);
    mListWidget->setCurrentRow(row + 1);
}

void ToolbarCustomizationDialog::toggleVisibility()
{
    int row = mListWidget->currentRow();
    if (row < 0) return;

    auto* item = mListWidget->item(row);
    bool visible = item->data(Qt::UserRole + 1).toBool();
    visible = !visible;
    item->setData(Qt::UserRole + 1, visible);

    QString displayName = item->data(Qt::UserRole + 2).toString();
    QString prefix = visible ? "[+] " : "[-] ";
    item->setText(prefix + displayName);
    item->setForeground(visible ? Qt::black : Qt::gray);
}

void ToolbarCustomizationDialog::saveAndClose()
{
    // Build ordered list of (name, visible) pairs
    QStringList order;
    QStringList hiddenActions;

    for (int i = 0; i < mListWidget->count(); ++i)
    {
        auto* item = mListWidget->item(i);
        QString name = item->data(Qt::UserRole).toString();
        bool visible = item->data(Qt::UserRole + 1).toBool();
        order.append(name);
        if (!visible) hiddenActions.append(name);
    }

    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("Toolbar");
    conf.setValue("Order", order.join(","));
    conf.setValue("Hidden", hiddenActions.join(","));
    conf.endGroup();
    conf.sync();

    // Rebuild the toolbar
    QList<QAction*> actions = mToolbar->actions();
    for (QAction* action : actions)
    {
        if (action->isSeparator()) continue;
        QString name = action->objectName();
        bool shouldShow = !hiddenActions.contains(name);
        mToolbar->removeAction(action);
        action->setVisible(shouldShow);
    }

    // Re-add in the specified order
    for (const QString& name : order)
    {
        for (QAction* action : actions)
        {
            if (action->objectName() == name)
            {
                mToolbar->addAction(action);
                action->setVisible(!hiddenActions.contains(name));
                break;
            }
        }
    }

    accept();
}

void ToolbarCustomizationDialog::saveToolbarConfig(QToolBar* toolbar)
{
    QStringList order;
    QStringList hiddenActions;

    for (QAction* action : toolbar->actions())
    {
        if (action->isSeparator())
        {
            order.append("separator");
            continue;
        }
        QString name = action->objectName();
        order.append(name);
        if (!action->isVisible())
            hiddenActions.append(name);
    }

    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("Toolbar");
    conf.setValue("Order", order.join(","));
    conf.setValue("Hidden", hiddenActions.join(","));
    conf.endGroup();
    conf.sync();
}

void ToolbarCustomizationDialog::restoreToolbarConfig(QToolBar* toolbar)
{
    QString configPath = FilePaths::configFilePath();
    QSettings conf(configPath, QSettings::IniFormat);
    conf.beginGroup("Toolbar");

    QString orderStr = conf.value("Order", QString()).toString();
    QString hiddenStr = conf.value("Hidden", QString()).toString();
    conf.endGroup();

    if (orderStr.isEmpty()) return;

    QStringList order = orderStr.split(",", Qt::SkipEmptyParts);
    QStringList hiddenActions = hiddenStr.split(",", Qt::SkipEmptyParts);

    // Take all actions from the toolbar (without deleting them)
    QList<QAction*> currentActions = toolbar->actions();
    for (QAction* action : currentActions)
        toolbar->removeAction(action);

    // Re-add in the specified order
    for (const QString& name : order)
    {
        if (name == "separator")
        {
            toolbar->addSeparator();
            continue;
        }

        for (QAction* action : currentActions)
        {
            if (action->objectName() == name)
            {
                action->setVisible(!hiddenActions.contains(name));
                toolbar->addAction(action);
                currentActions.removeOne(action);
                break;
            }
        }
    }
}
