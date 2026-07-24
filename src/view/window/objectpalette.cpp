#include "objectpalette.hpp"

#include <QListView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QPainter>
#include <QMouseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QFile>
#include <QDataStream>
#include <QStandardItem>
#include <cmath>

#include "../../libs/files/esm/cellrecord.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idcollection.hpp"
#include "../../libs/files/esm/weaprecord.hpp"
#include "../../libs/files/esm/armorrecord.hpp"
#include "../../libs/files/esm/spellrecord.hpp"
#include "../../libs/files/esm/gmst.hpp"
#include "../../libs/files/esm/Statrecord.hpp"
#include "../../libs/files/esm/records.hpp"
#include "logger.hpp"

ObjectFilterProxyModel::ObjectFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
}

void ObjectFilterProxyModel::setSearchPattern(const QString& pattern)
{
    searchPattern = pattern;
    invalidateFilter();
}

bool ObjectFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QString itemText = sourceModel()->data(index).toString();
    
    if (searchPattern.isEmpty()) {
        return true;
    }
    
    return itemText.contains(searchPattern, Qt::CaseInsensitive);
}

ObjectPalette::ObjectPalette(Data* data, QWidget* parent) :
    QWidget(parent),
    mData(data),
    currentCell(nullptr),
    placementMode(false),
    nextRefFormId(0x80000000)
{
    setupUI();
    populateObjectList();
}

ObjectPalette::~ObjectPalette()
{
}

void ObjectPalette::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("Search:"));
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Search objects...");
    topLayout->addWidget(searchEdit, 1);
    mainLayout->addLayout(topLayout);

    auto* listLayout = new QHBoxLayout();

    objectModel = new QStandardItemModel(this);
    filterProxyModel = new ObjectFilterProxyModel(this);
    filterProxyModel->setSourceModel(objectModel);
    filterProxyModel->setDynamicSortFilter(true);
    filterProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    filterProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    objectListView = new QListView();
    objectListView->setModel(filterProxyModel);
    objectListView->setViewMode(QListView::IconMode);
    objectListView->setIconSize(QSize(32, 32));
    objectListView->setMaximumWidth(200);
    objectListView->setUniformItemSizes(true);
    listLayout->addWidget(objectListView, 1);

    auto* settingsLayout = new QVBoxLayout();

    auto* positionGroup = new QGroupBox("Position");
    auto* positionLayout = new QVBoxLayout(positionGroup);
    positionLayout->addWidget(new QLabel("X:"));
    xSpin = new QDoubleSpinBox();
    xSpin->setRange(-100000.0, 100000.0);
    xSpin->setDecimals(3);
    positionLayout->addWidget(xSpin);
    positionLayout->addWidget(new QLabel("Y:"));
    ySpin = new QDoubleSpinBox();
    ySpin->setRange(-100000.0, 100000.0);
    ySpin->setDecimals(3);
    positionLayout->addWidget(ySpin);
    positionLayout->addWidget(new QLabel("Z:"));
    zSpin = new QDoubleSpinBox();
    zSpin->setRange(-100000.0, 100000.0);
    zSpin->setDecimals(3);
    positionLayout->addWidget(zSpin);
    settingsLayout->addWidget(positionGroup);

    auto* rotationGroup = new QGroupBox("Rotation");
    auto* rotationLayout = new QVBoxLayout(rotationGroup);
    rotationLayout->addWidget(new QLabel("RotX:"));
    rotXSpin = new QDoubleSpinBox();
    rotXSpin->setRange(-360, 360);
    rotXSpin->setSingleStep(1.0);
    rotationLayout->addWidget(rotXSpin);
    rotationLayout->addWidget(new QLabel("RotY:"));
    rotYSpin = new QDoubleSpinBox();
    rotYSpin->setRange(-360, 360);
    rotYSpin->setSingleStep(1.0);
    rotationLayout->addWidget(rotYSpin);
    rotationLayout->addWidget(new QLabel("RotZ:"));
    rotZSpin = new QDoubleSpinBox();
    rotZSpin->setRange(-360, 360);
    rotZSpin->setSingleStep(1.0);
    rotationLayout->addWidget(rotZSpin);
    settingsLayout->addWidget(rotationGroup);

    auto* transformGroup = new QGroupBox("Transform");
    auto* transformLayout = new QVBoxLayout(transformGroup);
    transformLayout->addWidget(new QLabel("Scale:"));
    scaleSpin = new QDoubleSpinBox();
    scaleSpin->setRange(0.01, 100.0);
    scaleSpin->setSingleStep(0.1);
    scaleSpin->setDecimals(2);
    transformLayout->addWidget(scaleSpin);
    activeCheckBox = new QCheckBox("Active");
    activeCheckBox->setChecked(true);
    transformLayout->addWidget(activeCheckBox);
    settingsLayout->addWidget(transformGroup);

    auto* gridGroup = new QGroupBox("Grid Snap");
    auto* gridLayout = new QVBoxLayout(gridGroup);
    gridLayout->addWidget(new QLabel("Grid Size:"));
    gridSizeSpin = new QSpinBox();
    gridSizeSpin->setRange(1, 1000);
    gridSizeSpin->setValue(64);
    gridSizeSpin->setSingleStep(16);
    gridLayout->addWidget(gridSizeSpin);
    snapPositionCheck = new QCheckBox("Snap Position");
    snapPositionCheck->setChecked(false);
    gridLayout->addWidget(snapPositionCheck);
    snapRotationCheck = new QCheckBox("Snap Rotation (15° increments)");
    snapRotationCheck->setChecked(false);
    gridLayout->addWidget(snapRotationCheck);
    settingsLayout->addWidget(gridGroup);

    listLayout->addLayout(settingsLayout, 1);
    mainLayout->addLayout(listLayout, 3);

    auto* buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton("Save Placement");
    loadButton = new QPushButton("Load Placement");
    placementModeButton = new QPushButton("Toggle Placement Mode");
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(placementModeButton, 1);
    mainLayout->addLayout(buttonLayout);

    objectCountLabel = new QLabel("Objects: 0");
    mainLayout->addWidget(objectCountLabel);

    statusLabel = new QLabel("Ready");
    mainLayout->addWidget(statusLabel);

    connect(searchEdit, &QLineEdit::textChanged, this, &ObjectPalette::onSearchTextChanged);
    connect(objectListView, &QListView::clicked, this, &ObjectPalette::onObjectSelected);
    connect(saveButton, &QPushButton::clicked, this, &ObjectPalette::onSavePlacementClicked);
    connect(loadButton, &QPushButton::clicked, this, &ObjectPalette::onLoadPlacementClicked);
    connect(placementModeButton, &QPushButton::clicked, this, &ObjectPalette::onTogglePlacementMode);
}

void ObjectPalette::loadCell(CellRecord* cell)
{
    currentCell = cell;
    if (cell) {
        statusLabel->setText(QString("Loaded cell: %1").arg(cell->editorId));
    } else {
        statusLabel->setText("No cell loaded");
    }
}

void ObjectPalette::clear()
{
    currentCell = nullptr;
    placements.clear();
    nextRefFormId = 0x80000000;
    objectCountLabel->setText("Objects: 0");
    statusLabel->setText("Cleared");
}

void ObjectPalette::populateObjectList()
{
    objectModel->removeRows(0, objectModel->rowCount());

    if (!mData) {
        objectCountLabel->setText("Objects: 0");
        return;
    }

    auto addItems = [this](const auto& collection) {
        int startRow = objectModel->rowCount();
        objectModel->insertRows(startRow, collection.size());

        for (int i = 0; i < collection.size(); i++) {
            const auto& record = collection.getRecord(i).get();
            QModelIndex index = objectModel->index(startRow + i, 0);
            objectModel->setData(index, record.editorId);
            objectModel->setData(index, static_cast<quint32>(record.formId), Qt::UserRole);
        }
    };

    addItems(mData->getWeaponCollection());
    addItems(mData->getArmorCollection());
    addItems(mData->getSpellCollection());
    addItems(mData->getActiCollection());
    addItems(mData->getAlchCollection());
    addItems(mData->getBookCollection());
    addItems(mData->getContCollection());
    addItems(mData->getIngrCollection());
    addItems(mData->getMiscCollection());
    addItems(mData->getTreeCollection());
    addItems(mData->getGameSettings());
    addItems(mData->getStatCollection());

    filterProxyModel->sort(0);

    int totalCount = objectModel->rowCount();
    objectCountLabel->setText(QString("Objects: %1").arg(totalCount));
    LOG_INFO(QString("ObjectPalette populated with %1 objects").arg(totalCount));
}

void ObjectPalette::onSearchTextChanged(const QString& text)
{
    filterProxyModel->setSearchPattern(text);
    filterProxyModel->sort(0);
}

void ObjectPalette::onObjectSelected(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    QModelIndex sourceIndex = filterProxyModel->mapToSource(index);
    QString objectName = filterProxyModel->data(index).toString();
    quint32 formId = filterProxyModel->data(index, Qt::UserRole).toUInt();

    if (!objectName.isEmpty()) {
        selectedObject = objectName;
        statusLabel->setText(QString("Selected: %1 (0x%2)")
            .arg(objectName)
            .arg(formId, 8, 16, QChar('0')));
    }
}

void ObjectPalette::onSavePlacementClicked()
{
    if (!currentCell) {
        statusLabel->setText("No cell loaded");
        return;
    }

    QModelIndex currentIndex = objectListView->currentIndex();
    if (!currentIndex.isValid()) {
        statusLabel->setText("No object selected");
        return;
    }

    QModelIndex sourceIndex = filterProxyModel->mapToSource(currentIndex);
    QString objectName = filterProxyModel->data(currentIndex).toString();
    quint32 formId = filterProxyModel->data(currentIndex, Qt::UserRole).toUInt();

    if (objectName.isEmpty()) {
        statusLabel->setText("No object selected");
        return;
    }

    Placement placement;
    placement.baseObjectFormId = formId;
    placement.baseObjectName = objectName;
    
    float xPos = static_cast<float>(xSpin->value());
    float yPos = static_cast<float>(ySpin->value());
    float zPos = static_cast<float>(zSpin->value());
    
    if (snapPositionCheck->isChecked()) {
        int gridSize = gridSizeSpin->value();
        xPos = snapToGrid(xPos, gridSize);
        yPos = snapToGrid(yPos, gridSize);
        zPos = snapToGrid(zPos, gridSize);
    }
    
    placement.x = xPos;
    placement.y = yPos;
    placement.z = zPos;
    
    float rotX = rotXSpin->value();
    float rotY = rotYSpin->value();
    float rotZ = rotZSpin->value();
    
    if (snapRotationCheck->isChecked()) {
        rotX = snapToGrid(rotX, 15);
        rotY = snapToGrid(rotY, 15);
        rotZ = snapToGrid(rotZ, 15);
    }
    
    placement.rotX = rotX;
    placement.rotY = rotY;
    placement.rotZ = rotZ;
    placement.scale = scaleSpin->value();
    placement.active = activeCheckBox->isChecked();

    placements.append(placement);
    objectCountLabel->setText(QString("Objects: %1").arg(placements.size()));
    statusLabel->setText(QString("Added %1 (FormID: 0x%2)")
        .arg(placement.baseObjectName)
        .arg(placement.baseObjectFormId, 8, 16, QChar('0')));

    // Sync to CellRecord
    syncPlacementsToCell();
}

void ObjectPalette::onLoadPlacementClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Placement", "", "Placement Files (*.json)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        statusLabel->setText("Failed to load");
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QDataStream in(&fileData, QIODevice::ReadOnly);
    in.setByteOrder(QDataStream::LittleEndian);

    int count;
    in >> count;

    placements.clear();
    for (int i = 0; i < count; i++) {
        Placement p;
        QString name;
        in >> name >> p.x >> p.y >> p.z >> p.rotX >> p.rotY >> p.rotZ >> p.scale >> p.active;
        p.baseObjectName = name;
        p.baseObjectFormId = 0;
        placements.append(p);
    }

    objectCountLabel->setText(QString("Objects: %1").arg(placements.size()));
    statusLabel->setText(QString("Loaded %1 placements").arg(count));

    // Sync to CellRecord
    syncPlacementsToCell();
}

void ObjectPalette::onTogglePlacementMode()
{
    placementMode = !placementMode;
    placementModeButton->setText(placementMode ? "Exit Placement Mode" : "Toggle Placement Mode");
    statusLabel->setText(placementMode ? "Placement mode active" : "Placement mode inactive");
}

void ObjectPalette::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(40, 40, 40));

    painter.setPen(QColor(200, 200, 200));
    painter.drawText(10, 20, QString("Placements: %1").arg(placements.size()));

    int y = 40;
    for (const auto& p : placements) {
        QString text = QString("%1 at (%2, %3, %4)").arg(p.baseObjectName).arg(p.x).arg(p.y).arg(p.z);
        painter.drawText(10, y, text);
        y += 20;
    }
}

void ObjectPalette::syncPlacementsToCell()
{
    if (!currentCell) {
        return;
    }

    // Remove existing REFR subrecords from cell
    for (int i = currentCell->rawSubRecords.size() - 1; i >= 0; i--) {
        if (currentCell->rawSubRecords[i].name == 'REFR') {
            currentCell->rawSubRecords.removeAt(i);
        }
    }

    // Also remove old PLOC for cleanup
    for (int i = currentCell->rawSubRecords.size() - 1; i >= 0; i--) {
        if (currentCell->rawSubRecords[i].name == 'PLOC') {
            currentCell->rawSubRecords.removeAt(i);
        }
    }

    // Add new REFR placement data
    if (!placements.isEmpty()) {
        for (const auto& p : placements) {
            RawSubRecord refRecord;
            refRecord.name = 'REFR';

            QByteArray data;
            QDataStream out(&data, QIODevice::WriteOnly);
            out.setByteOrder(QDataStream::LittleEndian);

            quint32 refFormId = nextRefFormId++;
            quint32 baseObj = p.baseObjectFormId;
            float px = p.x;
            float py = p.y;
            float pz = p.z;
            float rx = p.rotX;
            float ry = p.rotY;
            float rz = p.rotZ;
            float sc = p.scale;
            quint32 flags = p.active ? 0 : 0x01;

            out << refFormId << baseObj << px << py << pz << rx << ry << rz << sc << flags;

            refRecord.data = data;
            currentCell->rawSubRecords.append(refRecord);
        }
    }

    LOG_INFO(QString("Synced %1 REFR placements to cell %2").arg(placements.size()).arg(currentCell->editorId));
}

float ObjectPalette::snapToGrid(float value, int gridSize) const
{
    return std::round(value / gridSize) * gridSize;
}
