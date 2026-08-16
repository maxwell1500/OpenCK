#include "celltransitionseditor.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "logger.hpp"

#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/esmwriter.hpp"

#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFile>

CellTransitionsEditor::CellTransitionsEditor(Data* data, QWidget* parent)
    : QDialog(parent),
      mData(data),
      mTree(nullptr),
      mDetailEdit(nullptr),
      mAddTransitionButton(nullptr),
      mEditButton(nullptr),
      mDeleteButton(nullptr),
      mSaveButton(nullptr),
      mStatusLabel(nullptr),
      mSelectedWorldspace(nullptr),
      mSelectedCell(nullptr),
      mSelectedFromCell(-1),
      mSelectedToCell(-1)
{
    LOG_INFO("CellTransitionsEditor created");
    setupUI();
    loadWorldspaces();
}

CellTransitionsEditor::~CellTransitionsEditor()
{
}

void CellTransitionsEditor::setupUI()
{
    setWindowTitle("Cell Transitions Editor");
    setMinimumSize(1200, 800);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* topBar = new QHBoxLayout();
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search worldspaces...");
    topBar->addWidget(new QLabel("Search:"));
    topBar->addWidget(searchEdit, 1);
    mainLayout->addLayout(topBar);

    auto* infoLabel = new QLabel("Read-only overview. Cell-to-cell transitions are not yet editable.");
    mainLayout->addWidget(infoLabel);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    mTree = new QTreeWidget();
    mTree->setHeaderLabels(QStringList() << "Worldspace/Cell" << "Type" << "Details");
    mTree->setColumnWidth(0, 350);
    mTree->setColumnWidth(1, 100);
    mTree->setColumnWidth(2, 400);
    mTree->setAlternatingRowColors(true);
    mTree->setRootIsDecorated(true);
    splitter->addWidget(mTree);

    mDetailEdit = new QTextEdit();
    mDetailEdit->setReadOnly(true);
    mDetailEdit->setFontPointSize(10);
    splitter->addWidget(mDetailEdit);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    mainLayout->addWidget(splitter, 1);

    auto* buttonBar = new QHBoxLayout();
    mAddTransitionButton = new QPushButton("Add Transition");
    mEditButton = new QPushButton("Edit");
    mDeleteButton = new QPushButton("Delete");

    // Hidden, not just disabled: the plugin format stores no cell-connection
    // data (see docs/REMAINING_WORK_PLAN.md Phase D), so these actions are dead.
    mAddTransitionButton->setVisible(false);
    mEditButton->setVisible(false);
    mDeleteButton->setVisible(false);
    buttonBar->addWidget(mAddTransitionButton);
    buttonBar->addWidget(mEditButton);
    buttonBar->addWidget(mDeleteButton);

    buttonBar->addStretch();

    mSaveButton = new QPushButton("Export Worldspaces...");
    buttonBar->addWidget(mSaveButton);

    mainLayout->addLayout(buttonBar);

    mStatusLabel = new QLabel("Ready");
    mainLayout->addWidget(mStatusLabel);

    connect(mTree, &QTreeWidget::itemClicked, this, &CellTransitionsEditor::onNodeSelected);
    connect(mSaveButton, &QPushButton::clicked, this, &CellTransitionsEditor::onSave);
}

void CellTransitionsEditor::loadWorldspaces()
{
    mTree->clear();
    mSelectedWorldspace = nullptr;
    mSelectedCell = nullptr;
    mSelectedFromCell = -1;
    mSelectedToCell = -1;

    auto& wsCollection = mData->getWorldspaceCollection();

    for (int i = 0; i < wsCollection.size(); i++) {
        Record<WorldspaceRecord>& wsRecord = wsCollection.getRecord(i);
        if (wsRecord.state == State_Erased) continue;

        WorldspaceRecord& ws = wsRecord.get();
        QTreeWidgetItem* wsItem = new QTreeWidgetItem(mTree);
        wsItem->setText(0, ws.editorId);
        wsItem->setText(1, "WRLD");
        wsItem->setText(2, QString("Name: %1 | Water: %2")
            .arg(ws.name)
            .arg(ws.waterType));
        wsItem->setData(0, Qt::UserRole, QVariant::fromValue<WorldspaceRecord*>(&ws));
    }

    mTree->expandAll();
    mStatusLabel->setText(QString("Loaded %1 worldspaces").arg(wsCollection.size()));
    LOG_INFO(QString("Loaded %1 worldspaces").arg(wsCollection.size()));
}

void CellTransitionsEditor::refreshTree()
{
    loadWorldspaces();
}

void CellTransitionsEditor::onNodeSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    QString type = item->text(1);

    if (type == "WRLD") {
        WorldspaceRecord* ws = static_cast<WorldspaceRecord*>(item->data(0, Qt::UserRole).value<WorldspaceRecord*>());
        if (ws) {
            mSelectedWorldspace = ws;
            showWorldspaceDetails(ws);
        }
    }
}

void CellTransitionsEditor::showWorldspaceDetails(const WorldspaceRecord* ws)
{
    if (!ws) return;

    QString text;
    text += QString("<h2>%1</h2>").arg(ws->editorId);
    text += QString("<p><b>FormID:</b> 0x%1</p>").arg(ws->formId, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Name:</b> %1</p>").arg(ws->name);
    text += QString("<p><b>Icon:</b> %1</p>").arg(ws->iconPath);
    text += QString("<p><b>Water Type:</b> 0x%1</p>").arg(ws->waterType, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Climate:</b> 0x%1</p>").arg(ws->climateId, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Lighting:</b> 0x%1</p>").arg(ws->lightingId, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Map Size:</b> %1 x %2</p>").arg(ws->mapWidth).arg(ws->mapHeight);
    text += QString("<p><b>Map Cells:</b> NW (%1, %2) - SE (%3, %4)</p>")
        .arg(ws->mapNwX).arg(ws->mapNwY).arg(ws->mapSeX).arg(ws->mapSeY);
    text += QString("<p><b>Map Scale:</b> %1</p>").arg(ws->mapScale(), 0, 'f', 2);
    text += QString("<p><b>LOD Bias:</b> %1</p>").arg(ws->mapLodBias, 0, 'f', 2);
    text += QString("<p><b>Stored Cells:</b> %1</p>")
        .arg(mData ? static_cast<int>(mData->cellsInWorldspace(ws->formId).size()) : 0);
    text += "<p><b>Note:</b> Add/Edit/Delete transition actions are permanently disabled - the "
            "plugin format stores no cell-connection data "
            "(see docs/REMAINING_WORK_PLAN.md Phase D).</p>";

    mDetailEdit->setHtml(text);
}

void CellTransitionsEditor::showCellDetails(const CellRecord* cell)
{
    QString text;
    text += "<h2>Cell Transitions</h2>";
    text += "<p>Select a worldspace to view its details.</p>";
    text += "<p><b>Note:</b> Full transition editing requires detailed cell connection data.</p>";

    mDetailEdit->setHtml(text);
}

void CellTransitionsEditor::showTransitionDetails(int fromCell, int toCell)
{
    QString text;
    text += "<h2>Cell Transition</h2>";
    text += QString("<p><b>From Cell:</b> %1</p>").arg(fromCell);
    text += QString("<p><b>To Cell:</b> %1</p>").arg(toCell);
    text += "<p><b>Transition Type:</b> Standard</p>";
    text += "<p><b>Loading Screen:</b> Enabled</p>";

    mDetailEdit->setHtml(text);
}

void CellTransitionsEditor::onAddTransition()
{
    Q_UNUSED(mSelectedWorldspace);
    // Unreachable: Add Transition is permanently disabled because the plugin
    // format stores no cell-connection data (see docs/REMAINING_WORK_PLAN.md
    // Phase D).
}

void CellTransitionsEditor::onEditTransition()
{
    Q_UNUSED(mSelectedWorldspace);
    // Unreachable: Edit is permanently disabled because the plugin format
    // stores no cell-connection data (see docs/REMAINING_WORK_PLAN.md
    // Phase D).
}

void CellTransitionsEditor::onDeleteTransition()
{
    Q_UNUSED(mSelectedWorldspace);
    // Unreachable: Delete is permanently disabled because the plugin format
    // stores no cell-connection data (see docs/REMAINING_WORK_PLAN.md
    // Phase D).
}

void CellTransitionsEditor::onSave()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Cell Transitions", "",
        "ESM Files (*.esm);;All Files (*)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Failed to open file for writing.");
        return;
    }

    ESMWriter writer;
    writer.setVersion(1.0f);

    auto& wsCollection = mData->getWorldspaceCollection();
    auto wsRecords = wsCollection.getRecords();

    int totalWorldspaces = 0;

    for (const auto& wsRecord : wsRecords) {
        if (wsRecord.state == State_Erased) continue;

        const WorldspaceRecord& ws = wsRecord.get();
        RecHeader recHeader;
        recHeader.id = ws.formId;
        writer.startRecord('WRLD', recHeader);
        ws.save(writer);
        writer.endRecord();
        totalWorldspaces++;
    }

    file.close();

    LOG_INFO(QString("Saved %1 worldspaces to %2").arg(totalWorldspaces).arg(filePath));

    QMessageBox::information(this, "Saved",
        QString("Worldspace data saved.\n\n"
                "Total worldspaces: %1\n\n"
                "File: %2")
            .arg(totalWorldspaces)
            .arg(filePath));
}
