#include "navmesheditor.hpp"
#include "navmesheditordialog.hpp"
#include "nifviewportwidget.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/collection.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../model/tools/navmeshgenerator.hpp"
#include "logger.hpp"

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

NavmeshEditor::NavmeshEditor(Data* data, QWidget* parent, NifViewportWidget* viewport)
    : QDialog(parent),
      mData(data),
      mTree(nullptr),
      mDetailEdit(nullptr),
      mAddCellButton(nullptr),
      mEditButton(nullptr),
      mDeleteButton(nullptr),
      mSaveButton(nullptr),
      mEditDetailsButton(nullptr),
      mGenerateButton(nullptr),
      mStatusLabel(nullptr),
      mSelectedCell(nullptr),
      mDetailDialog(nullptr),
      mViewport(viewport)
{
    LOG_INFO("NavmeshEditor created");
    setupUI();
    loadCells();
}

NavmeshEditor::~NavmeshEditor()
{
}

void NavmeshEditor::setupUI()
{
    setWindowTitle("Navmesh Editor");
    setMinimumSize(1200, 800);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    auto* topBar = new QHBoxLayout();
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search cells...");
    topBar->addWidget(new QLabel("Search:"));
    topBar->addWidget(searchEdit, 1);
    mainLayout->addLayout(topBar);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    mTree = new QTreeWidget();
    mTree->setHeaderLabels(QStringList() << "Cell" << "Type" << "Details");
    mTree->setColumnWidth(0, 350);
    mTree->setColumnWidth(1, 80);
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
    mAddCellButton = new QPushButton("Add Cell");
    buttonBar->addWidget(mAddCellButton);

    mEditButton = new QPushButton("Edit");
    mEditButton->setEnabled(false);
    buttonBar->addWidget(mEditButton);

    mDeleteButton = new QPushButton("Delete");
    mDeleteButton->setEnabled(false);
    buttonBar->addWidget(mDeleteButton);

    buttonBar->addStretch();

    mEditDetailsButton = new QPushButton("Edit Details...");
    mEditDetailsButton->setEnabled(false);
    buttonBar->addWidget(mEditDetailsButton);

    mGenerateButton = new QPushButton("Generate Navmesh");
    mGenerateButton->setEnabled(mViewport != nullptr);
    buttonBar->addWidget(mGenerateButton);

    mSaveButton = new QPushButton("Save Changes");
    buttonBar->addWidget(mSaveButton);

    mainLayout->addLayout(buttonBar);

    mStatusLabel = new QLabel("Ready");
    mainLayout->addWidget(mStatusLabel);

    connect(mTree, &QTreeWidget::itemClicked, this, &NavmeshEditor::onNodeSelected);
    connect(mAddCellButton, &QPushButton::clicked, this, &NavmeshEditor::onAddCell);
    connect(mEditButton, &QPushButton::clicked, this, &NavmeshEditor::onEditCell);
    connect(mDeleteButton, &QPushButton::clicked, this, &NavmeshEditor::onDeleteCell);
    connect(mSaveButton, &QPushButton::clicked, this, &NavmeshEditor::onSave);
    connect(mEditDetailsButton, &QPushButton::clicked, this, &NavmeshEditor::onEditDetails);
    connect(mGenerateButton, &QPushButton::clicked, this, &NavmeshEditor::onGenerateNavmesh);
}

void NavmeshEditor::loadCells()
{
    mTree->clear();
    mSelectedCell = nullptr;

    auto& cellCollection = mData->getCellCollection();

    for (int i = 0; i < cellCollection.size(); i++) {
        Record<CellRecord>& record = cellCollection.getRecord(i);
        if (record.state == State_Erased) continue;

        CellRecord& cell = record.get();
        QTreeWidgetItem* cellItem = new QTreeWidgetItem(mTree);
        cellItem->setText(0, cell.cellName.isEmpty() ? cell.editorId : cell.cellName);
        cellItem->setText(1, "CELL");
        cellItem->setText(2, QString("X: %1 | Y: %2")
            .arg(cell.cellX)
            .arg(cell.cellY));
        cellItem->setData(0, Qt::UserRole, QVariant::fromValue<CellRecord*>(&cell));
    }

    mTree->expandAll();
    mStatusLabel->setText(QString("Loaded %1 cells").arg(cellCollection.size()));
    LOG_INFO(QString("Loaded %1 cells").arg(cellCollection.size()));
}

void NavmeshEditor::refreshTree()
{
    loadCells();
}

void NavmeshEditor::onNodeSelected(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);

    if (!item) return;

    mEditButton->setEnabled(true);
    mDeleteButton->setEnabled(true);
    mEditDetailsButton->setEnabled(true);

    QString type = item->text(1);

    if (type == "CELL") {
        CellRecord* cell = static_cast<CellRecord*>(item->data(0, Qt::UserRole).value<CellRecord*>());
        if (cell) {
            mSelectedCell = cell;
            showCellDetails(cell);
        }
    }
}

void NavmeshEditor::showCellDetails(const CellRecord* cell)
{
    QString text;
    text += QString("<h2>%1</h2>").arg(cell->cellName.isEmpty() ? cell->editorId : cell->cellName);
    text += QString("<p><b>Editor ID:</b> %1</p>").arg(cell->editorId);
    text += QString("<p><b>FormID:</b> 0x%1</p>").arg(cell->formId, 8, 16, QChar('0')).toUpper();
    text += QString("<p><b>Cell X:</b> %1</p>").arg(cell->cellX);
    text += QString("<p><b>Cell Y:</b> %1</p>").arg(cell->cellY);

    mDetailEdit->setHtml(text);
}

void NavmeshEditor::onAddCell()
{
    bool ok = false;
    QString editorId = QInputDialog::getText(this, "Add Cell",
        "Enter Editor ID for new cell:", QLineEdit::Normal, "", &ok);

    if (!ok || editorId.isEmpty()) return;

    CellRecord newCell;
    newCell.editorId = editorId;
    newCell.formId = 0;
    newCell.cellName = editorId;
    newCell.cellX = 0;
    newCell.cellY = 0;

    if (mData->addCell(newCell)) {
        LOG_INFO(QString("Added cell '%1'").arg(editorId));
        mStatusLabel->setText(QString("Added cell '%1'").arg(editorId));
        refreshTree();
    } else {
        QMessageBox::critical(this, "Error",
            QString("Failed to add cell '%1'. ID may already exist.").arg(editorId));
    }
}

void NavmeshEditor::onEditCell()
{
    if (!mSelectedCell) return;

    bool ok = false;
    QString newName = QInputDialog::getText(this, "Edit Cell",
        QString("Enter new name for '%1':").arg(mSelectedCell->cellName.isEmpty() ? mSelectedCell->editorId : mSelectedCell->cellName),
        QLineEdit::Normal,
        mSelectedCell->cellName.isEmpty() ? mSelectedCell->editorId : mSelectedCell->cellName,
        &ok);

    if (!ok) return;

    LOG_INFO(QString("Renamed cell from '%1' to '%2'")
        .arg(mSelectedCell->editorId).arg(newName));
    mSelectedCell->cellName = newName;
    refreshTree();
}

void NavmeshEditor::onDeleteCell()
{
    QTreeWidgetItem* item = mTree->currentItem();
    if (!item) return;

    QString type = item->text(1);

    if (type == "CELL") {
        const CellRecord* cell = static_cast<const CellRecord*>(item->data(0, Qt::UserRole).value<const CellRecord*>());
        if (!cell) return;

        auto reply = QMessageBox::question(this, "Delete Cell",
            QString("Are you sure you want to delete cell '%1'?\n\nThis action cannot be undone.")
                .arg(cell->cellName.isEmpty() ? cell->editorId : cell->cellName),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            mData->removeRecord(CkId::Type_Cel_, cell->editorId);
            LOG_INFO(QString("Deleted cell '%1'").arg(cell->editorId));
            refreshTree();
        }
    }
}

void NavmeshEditor::onSave()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save Navmesh Data", "",
        "ESM Files (*.esm);;All Files (*)");

    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Failed to open file for writing.");
        return;
    }

    ESMWriter writer;
    writer.setVersion(1.0f);
    writer.save(file);

    auto& cellCollection = mData->getCellCollection();
    auto cellRecords = cellCollection.getRecords();

    int totalCells = 0;

    for (const auto& record : cellRecords) {
        if (record.state == State_Erased) continue;

        const CellRecord& cell = record.get();
        RecHeader recHeader;
        recHeader.id = cell.formId;
        writer.startRecord('CEL_', recHeader);
        cell.save(writer);
        writer.endRecord();

        totalCells++;
    }

    file.close();

    LOG_INFO(QString("Saved %1 cells to %2")
        .arg(totalCells).arg(filePath));

    QMessageBox::information(this, "Saved",
        QString("Cell data saved.\n\n"
                "Total cells: %1\n\n"
                "File: %2")
            .arg(totalCells)
            .arg(filePath));
}

void NavmeshEditor::onEditDetails()
{
    if (!mDetailDialog) {
        mDetailDialog = new NavmeshEditorDialog(this);
    }
    mDetailDialog->show();
    mDetailDialog->raise();
    mDetailDialog->activateWindow();
}

void NavmeshEditor::onGenerateNavmesh()
{
    if (!mViewport) {
        QMessageBox::information(this, "Navmesh Generation",
            "No 3D viewport available.\n\n"
            "Open the 3D viewport first and load a NIF file.");
        return;
    }

    const Nif::NifParser* parser = mViewport->getNifParser();
    if (!parser) {
        QMessageBox::information(this, "Navmesh Generation",
            "No NIF file is loaded in the viewport.\n\n"
            "Load a NIF file first.");
        return;
    }

    mStatusLabel->setText("Generating navmesh...");
    LOG_INFO("Starting navmesh generation");

    NavMeshGenerator generator;
    NavMeshGenerator::NavMesh genMesh = generator.generate(*parser);

    if (genMesh.triangles.isEmpty()) {
        mStatusLabel->setText("Navmesh generation produced no triangles");
        QMessageBox::information(this, "Navmesh Generation",
            "Navmesh generation produced no triangles.\n\n"
            "The loaded mesh may have no walkable surfaces.");
        return;
    }

    NavMeshData navData;
    navData.vertices = genMesh.vertices;

    for (const auto& tri : genMesh.triangles) {
        NavTriangle navTri;

        int i0 = -1, i1 = -1, i2 = -1;
        for (int v = 0; v < navData.vertices.size(); ++v) {
            if (i0 == -1 && (navData.vertices[v] - tri.v0).lengthSquared() < 0.001f) i0 = v;
            if (i1 == -1 && (navData.vertices[v] - tri.v1).lengthSquared() < 0.001f) i1 = v;
            if (i2 == -1 && (navData.vertices[v] - tri.v2).lengthSquared() < 0.001f) i2 = v;
        }

        if (i0 < 0) { i0 = navData.vertices.size(); navData.vertices.append(tri.v0); }
        if (i1 < 0) { i1 = navData.vertices.size(); navData.vertices.append(tri.v1); }
        if (i2 < 0) { i2 = navData.vertices.size(); navData.vertices.append(tri.v2); }

        navTri.v0 = i0;
        navTri.v1 = i1;
        navTri.v2 = i2;
        navTri.normal = tri.normal;
        navTri.walkable = true;
        navData.triangles.append(navTri);
    }

    for (const auto& edge : genMesh.edges) {
        NavEdge navEdge;
        navEdge.startVertex = edge.first;
        navEdge.endVertex = edge.second;
        navEdge.blocked = false;
        navData.edges.append(navEdge);
    }

    if (!mDetailDialog) {
        mDetailDialog = new NavmeshEditorDialog(this);
    }
    mDetailDialog->setNavMesh(navData);
    mDetailDialog->show();
    mDetailDialog->raise();
    mDetailDialog->activateWindow();

    QVector<QVector3D> displayTriangles;
    for (const auto& tri : navData.triangles) {
        if (tri.v0 >= 0 && tri.v0 < navData.vertices.size() &&
            tri.v1 >= 0 && tri.v1 < navData.vertices.size() &&
            tri.v2 >= 0 && tri.v2 < navData.vertices.size()) {
            displayTriangles.append(navData.vertices[tri.v0]);
            displayTriangles.append(navData.vertices[tri.v1]);
            displayTriangles.append(navData.vertices[tri.v2]);
        }
    }
    mViewport->setNavmeshData(displayTriangles);

    if (mViewport) {
        connect(mDetailDialog, &NavmeshEditorDialog::triangleSelected,
                mViewport, &NifViewportWidget::highlightNavmeshTriangle);
        connect(mDetailDialog, &NavmeshEditorDialog::pathChanged,
                mViewport, &NifViewportWidget::setPathData);
    }

    mStatusLabel->setText(QString("Generated navmesh: %1 vertices, %2 triangles")
        .arg(navData.vertices.size())
        .arg(navData.triangles.size()));
    LOG_INFO(QString("Navmesh generated: %1 vertices, %2 triangles")
        .arg(navData.vertices.size()).arg(navData.triangles.size()));
}
