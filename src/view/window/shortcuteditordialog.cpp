#include "shortcuteditordialog.hpp"
#include "../../model/world/shortcutmanager.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QKeyEvent>
#include <QApplication>

ShortcutEditorDialog::ShortcutEditorDialog(QWidget* parent)
    : QDialog(parent)
    , mRecordingRow(-1)
    , mRecording(false)
{
    setWindowTitle("Keyboard Shortcuts");
    setMinimumSize(600, 500);

    auto* layout = new QVBoxLayout(this);

    auto* infoLabel = new QLabel("Double-click a shortcut and press a new key combination to change it.");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    mTable = new QTableWidget();
    mTable->setColumnCount(4);
    mTable->setHorizontalHeaderLabels({"Action", "Category", "Shortcut", ""});
    mTable->horizontalHeader()->setStretchLastSection(true);
    mTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTable->verticalHeader()->setVisible(false);

    connect(mTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        Q_UNUSED(col);
        mRecordingRow = row;
        mRecording = true;
        mTable->item(row, 2)->setText("Press keys...");
        mTable->item(row, 2)->setBackground(QColor(100, 150, 255, 60));
        updateButtonStates();
    });

    mTable->installEventFilter(this);

    layout->addWidget(mTable);

    auto* btnLayout = new QHBoxLayout();
    mRecordBtn = new QPushButton("Record Shortcut");
    mResetBtn = new QPushButton("Reset Selected");
    mResetAllBtn = new QPushButton("Reset All");
    mSaveBtn = new QPushButton("Save");

    mRecordBtn->setVisible(false);

    btnLayout->addWidget(mResetBtn);
    btnLayout->addWidget(mResetAllBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(mSaveBtn);

    layout->addLayout(btnLayout);

    connect(mRecordBtn, &QPushButton::clicked, this, &ShortcutEditorDialog::recordShortcut);
    connect(mResetBtn, &QPushButton::clicked, this, &ShortcutEditorDialog::resetSelected);
    connect(mResetAllBtn, &QPushButton::clicked, this, &ShortcutEditorDialog::resetAll);
    connect(mSaveBtn, &QPushButton::clicked, this, &ShortcutEditorDialog::saveAndClose);

    populateTable();
    updateButtonStates();
}

void ShortcutEditorDialog::populateTable()
{
    auto& mgr = ShortcutManager::instance();
    auto entries = mgr.entries();

    mTable->setRowCount(entries.size());

    int row = 0;
    for (const auto& entry : entries)
    {
        auto* nameItem = new QTableWidgetItem(entry.name);
        nameItem->setData(Qt::UserRole, entry.name);
        mTable->setItem(row, 0, nameItem);

        mTable->setItem(row, 1, new QTableWidgetItem(entry.category));

        QString shortcutText = entry.currentKey.isEmpty() ? "(none)" : entry.currentKey.toString();
        auto* shortcutItem = new QTableWidgetItem(shortcutText);
        shortcutItem->setBackground(entry.currentKey != entry.defaultKey ? QColor(255, 255, 200) : Qt::white);
        mTable->setItem(row, 2, shortcutItem);

        QString defaultText = entry.defaultKey.isEmpty() ? "(none)" : entry.defaultKey.toString();
        mTable->setItem(row, 3, new QTableWidgetItem(defaultText));

        row++;
    }

    mTable->sortByColumn(1, Qt::AscendingOrder);
    mTable->resizeColumnsToContents();
}

void ShortcutEditorDialog::updateButtonStates()
{
    bool hasSelection = mTable->currentRow() >= 0;
    mResetBtn->setEnabled(hasSelection && !mRecording);
    mResetAllBtn->setEnabled(!mRecording);
    mSaveBtn->setEnabled(!mRecording);
}

void ShortcutEditorDialog::recordShortcut()
{
    if (mRecordingRow < 0) return;
    mRecording = true;
    mTable->item(mRecordingRow, 2)->setText("Press keys...");
    updateButtonStates();
}

void ShortcutEditorDialog::resetSelected()
{
    int row = mTable->currentRow();
    if (row < 0) return;

    QString name = mTable->item(row, 0)->data(Qt::UserRole).toString();
    auto& mgr = ShortcutManager::instance();

    // Find the default key from the entries
    auto entries = mgr.entries();
    for (const auto& entry : entries)
    {
        if (entry.name == name)
        {
            mgr.set(name, entry.defaultKey);
            mTable->item(row, 2)->setText(entry.defaultKey.isEmpty() ? "(none)" : entry.defaultKey.toString());
            mTable->item(row, 2)->setBackground(Qt::white);
            break;
        }
    }
}

void ShortcutEditorDialog::resetAll()
{
    auto ret = QMessageBox::question(this, "Reset All Shortcuts",
        "Reset all shortcuts to their defaults?",
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    ShortcutManager::instance().resetToDefaults();
    populateTable();
}

void ShortcutEditorDialog::saveAndClose()
{
    ShortcutManager::instance().saveToIni();
    emit shortcutsChanged();
    accept();
}

bool ShortcutEditorDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == mTable && mRecording && event->type() == QEvent::KeyPress)
    {
        auto* ke = static_cast<QKeyEvent*>(event);

        if (ke->key() == Qt::Key_Escape)
        {
            mRecording = false;
            auto& mgr = ShortcutManager::instance();
            QString name = mTable->item(mRecordingRow, 0)->data(Qt::UserRole).toString();
            QKeySequence current = mgr.get(name);
            mTable->item(mRecordingRow, 2)->setText(current.isEmpty() ? "(none)" : current.toString());
            mTable->item(mRecordingRow, 2)->setBackground(Qt::white);
            mRecordingRow = -1;
            updateButtonStates();
            return true;
        }

        QKeySequence seq(ke->key() | ke->modifiers());
        QString name = mTable->item(mRecordingRow, 0)->data(Qt::UserRole).toString();

        // Check for conflicts
        auto& mgr = ShortcutManager::instance();
        for (const auto& entry : mgr.entries())
        {
            if (entry.name != name && entry.currentKey == seq && !seq.isEmpty())
            {
                QMessageBox::warning(this, "Conflict",
                    QString("'%1' is already assigned to '%2'.").arg(seq.toString(), entry.name));
                return true;
            }
        }

        mgr.set(name, seq);

        mTable->item(mRecordingRow, 2)->setText(seq.isEmpty() ? "(none)" : seq.toString());
        mTable->item(mRecordingRow, 2)->setBackground(QColor(200, 255, 200));
        mRecording = false;
        mRecordingRow = -1;
        updateButtonStates();
        return true;
    }
    return QDialog::eventFilter(obj, event);
}
