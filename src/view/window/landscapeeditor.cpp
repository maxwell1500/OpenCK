#include "landscapeeditor.hpp"

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>

#include "../../libs/files/esm/cellrecord.hpp"
#include "../../libs/files/esm/landrecord.hpp"
#include "../../model/world/data.hpp"
#include "../../model/tools/undostack.hpp"
#include "../../model/tools/landscapeeditcommand.hpp"
#include "logger.hpp"

#include <QFile>
#include <QDataStream>

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>

LandscapeEditor::LandscapeEditor(QWidget* parent) :
    QWidget(parent),
    glWidget(nullptr),
    terrainSize(257),
    minHeight(-1000.0f),
    maxHeight(1000.0f),
    brushSize(5),
    brushStrength(10),
    brushType(0),
    heightLimit(100),
    currentCell(nullptr),
    viewRotX(0),
    viewRotY(0),
    viewZoom(1.0f),
    dragging(false),
    hasOriginalState(false),
    waterHeight(0.0),
    waterTypeIndex(0),
    depthAttenuation(0.5),
    reflectionAmount(0.5),
    hasCopiedHeightmap(false),
    propertyTabWidget(nullptr),
    textureLayerTable(nullptr),
    addLayerButton(nullptr),
    removeLayerButton(nullptr),
    moveLayerUpButton(nullptr),
    moveLayerDownButton(nullptr),
    vegetationTable(nullptr),
    addPlantButton(nullptr),
    removePlantButton(nullptr),
    waterHeightSpinBox(nullptr),
    waterTypeCombo(nullptr),
    depthAttenuationSpinBox(nullptr),
    reflectionAmountSpinBox(nullptr),
    mData(nullptr),
    currentLand(nullptr)
{
    setupUI();
}

LandscapeEditor::~LandscapeEditor()
{
    if (glWidget && glWidget->context()) {
        glWidget->makeCurrent();
        delete shaderProgram;
        vertexVbo.destroy();
        normalVbo.destroy();
        vao.destroy();
        glWidget->doneCurrent();
    }
    delete glWidget;
}

void LandscapeEditor::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* controlLayout = new QHBoxLayout();

    controlLayout->addWidget(new QLabel("Brush Size:"));
    brushSizeSlider = new QSlider(Qt::Horizontal);
    brushSizeSlider->setRange(1, 20);
    brushSizeSlider->setValue(brushSize);
    brushSizeSlider->setTickPosition(QSlider::TicksBelow);
    controlLayout->addWidget(brushSizeSlider);

    controlLayout->addWidget(new QLabel("Strength:"));
    brushStrengthSlider = new QSlider(Qt::Horizontal);
    brushStrengthSlider->setRange(1, 100);
    brushStrengthSlider->setValue(brushStrength);
    brushStrengthSlider->setTickPosition(QSlider::TicksBelow);
    controlLayout->addWidget(brushStrengthSlider);

    controlLayout->addWidget(new QLabel("Type:"));
    brushTypeCombo = new QComboBox();
    brushTypeCombo->addItem("Raise");
    brushTypeCombo->addItem("Lower");
    brushTypeCombo->addItem("Smooth");
    brushTypeCombo->addItem("Flat");
    brushTypeCombo->setCurrentIndex(brushType);
    controlLayout->addWidget(brushTypeCombo);

    controlLayout->addWidget(new QLabel("Height:"));
    heightLimitSpin = new QSpinBox();
    heightLimitSpin->setRange(-5000, 5000);
    heightLimitSpin->setValue(100);
    controlLayout->addWidget(heightLimitSpin);

    saveButton = new QPushButton("Save");
    loadButton = new QPushButton("Load");
    controlLayout->addWidget(saveButton);
    controlLayout->addWidget(loadButton);

    controlLayout->addSpacing(20);
    controlLayout->addWidget(new QLabel("Copy/Paste:"));
    
    auto* copyPasteLayout = new QHBoxLayout();
    copyHeightmapButton = new QPushButton("Copy Heightmap");
    pasteHeightmapButton = new QPushButton("Paste Heightmap");
    copyPasteLayout->addWidget(copyHeightmapButton);
    copyPasteLayout->addWidget(pasteHeightmapButton);
    controlLayout->addLayout(copyPasteLayout);

    mainLayout->addLayout(controlLayout);

    statusLabel = new QLabel("Ready");
    mainLayout->addWidget(statusLabel);

    glWidget = new QOpenGLWidget(this);
    glWidget->setMinimumHeight(400);
    mainLayout->addWidget(glWidget);

    propertyTabWidget = new QTabWidget();

    auto* textureLayersTab = new QWidget();
    setupTextureLayersTab(textureLayersTab);
    propertyTabWidget->addTab(textureLayersTab, "Texture Layers");

    auto* vegetationTab = new QWidget();
    setupVegetationTab(vegetationTab);
    propertyTabWidget->addTab(vegetationTab, "Vegetation");

    auto* waterTab = new QWidget();
    setupWaterTab(waterTab);
    propertyTabWidget->addTab(waterTab, "Water");

    mainLayout->addWidget(propertyTabWidget);

    connect(brushSizeSlider, &QSlider::valueChanged, this, &LandscapeEditor::onBrushSizeChanged);
    connect(brushStrengthSlider, &QSlider::valueChanged, this, &LandscapeEditor::onBrushStrengthChanged);
    connect(brushTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LandscapeEditor::onBrushTypeChanged);
    connect(heightLimitSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &LandscapeEditor::onHeightLimitChanged);
    connect(saveButton, &QPushButton::clicked, this, &LandscapeEditor::onSaveClicked);
    connect(loadButton, &QPushButton::clicked, this, &LandscapeEditor::onLoadClicked);
    connect(copyHeightmapButton, &QPushButton::clicked, this, &LandscapeEditor::onCopyHeightmapClicked);
    connect(pasteHeightmapButton, &QPushButton::clicked, this, &LandscapeEditor::onPasteHeightmapClicked);
}

void LandscapeEditor::setupTextureLayersTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);

    textureLayerTable = new QTableWidget(0, 3);
    textureLayerTable->setHorizontalHeaderLabels({"Layer Index", "Texture Path", "Opacity"});
    textureLayerTable->horizontalHeader()->setStretchLastSection(true);
    textureLayerTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    textureLayerTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    textureLayerTable->setAlternatingRowColors(true);
    layout->addWidget(textureLayerTable);

    auto* buttonLayout = new QHBoxLayout();

    addLayerButton = new QPushButton("Add Layer");
    removeLayerButton = new QPushButton("Remove Layer");
    moveLayerUpButton = new QPushButton("Move Up");
    moveLayerDownButton = new QPushButton("Move Down");

    buttonLayout->addWidget(addLayerButton);
    buttonLayout->addWidget(removeLayerButton);
    buttonLayout->addWidget(moveLayerUpButton);
    buttonLayout->addWidget(moveLayerDownButton);
    buttonLayout->addStretch();

    layout->addLayout(buttonLayout);

    connect(addLayerButton, &QPushButton::clicked, this, &LandscapeEditor::onAddLayer);
    connect(removeLayerButton, &QPushButton::clicked, this, &LandscapeEditor::onRemoveLayer);
    connect(moveLayerUpButton, &QPushButton::clicked, this, &LandscapeEditor::onMoveLayerUp);
    connect(moveLayerDownButton, &QPushButton::clicked, this, &LandscapeEditor::onMoveLayerDown);
}

void LandscapeEditor::setupVegetationTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);

    vegetationTable = new QTableWidget(0, 4);
    vegetationTable->setHorizontalHeaderLabels({"FormID", "Density", "Min Height", "Max Height"});
    vegetationTable->horizontalHeader()->setStretchLastSection(true);
    vegetationTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    vegetationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    vegetationTable->setAlternatingRowColors(true);
    layout->addWidget(vegetationTable);

    auto* buttonLayout = new QHBoxLayout();

    addPlantButton = new QPushButton("Add Plant");
    removePlantButton = new QPushButton("Remove Plant");

    buttonLayout->addWidget(addPlantButton);
    buttonLayout->addWidget(removePlantButton);
    buttonLayout->addStretch();

    layout->addLayout(buttonLayout);

    connect(addPlantButton, &QPushButton::clicked, this, &LandscapeEditor::onAddPlant);
    connect(removePlantButton, &QPushButton::clicked, this, &LandscapeEditor::onRemovePlant);
}

void LandscapeEditor::setupWaterTab(QWidget* tab)
{
    auto* layout = new QVBoxLayout(tab);
    layout->setContentsMargins(8, 8, 8, 8);

    auto* waterGroup = new QGroupBox("Water Settings");
    auto* waterLayout = new QFormLayout(waterGroup);

    waterHeightSpinBox = new QDoubleSpinBox();
    waterHeightSpinBox->setRange(-100000.0, 100000.0);
    waterHeightSpinBox->setValue(waterHeight);
    waterHeightSpinBox->setDecimals(2);
    waterHeightSpinBox->setSingleStep(1.0);
    waterLayout->addRow("Water Height:", waterHeightSpinBox);

    waterTypeCombo = new QComboBox();
    waterTypeCombo->addItems({"Default Water", "Calm Water", "Ocean Water", "River Water", "Lava", "None"});
    waterTypeCombo->setCurrentIndex(waterTypeIndex);
    waterLayout->addRow("Water Type:", waterTypeCombo);

    depthAttenuationSpinBox = new QDoubleSpinBox();
    depthAttenuationSpinBox->setRange(0.0, 1.0);
    depthAttenuationSpinBox->setValue(depthAttenuation);
    depthAttenuationSpinBox->setDecimals(3);
    depthAttenuationSpinBox->setSingleStep(0.01);
    waterLayout->addRow("Depth Attenuation:", depthAttenuationSpinBox);

    reflectionAmountSpinBox = new QDoubleSpinBox();
    reflectionAmountSpinBox->setRange(0.0, 1.0);
    reflectionAmountSpinBox->setValue(reflectionAmount);
    reflectionAmountSpinBox->setDecimals(3);
    reflectionAmountSpinBox->setSingleStep(0.01);
    waterLayout->addRow("Reflection Amount:", reflectionAmountSpinBox);

    layout->addWidget(waterGroup);
    layout->addStretch();
}

void LandscapeEditor::loadCell(CellRecord* cell)
{
    currentCell = cell;
    currentLand = nullptr;

    if (cell) {
        loadHeightmap();
        setupOpenGL();
        glWidget->update();
        statusLabel->setText(QString("Loaded cell: %1").arg(cell->editorId));
    }
}

void LandscapeEditor::clear()
{
    currentCell = nullptr;
    currentLand = nullptr;
    heightmap.clear();
    statusLabel->setText("Cleared");
    glWidget->update();
}

void LandscapeEditor::setData(Data* data)
{
    mData = data;
    if (data) {
        mUndoStack = data->getUndoStack();
    }
}

void LandscapeEditor::setUndoStack(UndoStack* stack)
{
    mUndoStack = stack;
}

void LandscapeEditor::onUndo()
{
    if (mUndoStack && mUndoStack->canUndo()) {
        mUndoStack->undo();
        glWidget->update();
    }
}

void LandscapeEditor::onRedo()
{
    if (mUndoStack && mUndoStack->canRedo()) {
        mUndoStack->redo();
        glWidget->update();
    }
}

void LandscapeEditor::loadLand(LandRecord* land)
{
    if (!land || !land->hasHeightData) {
        return;
    }

    const int landSize = 33;
    const int mapSize = 257;

    for (int y = 0; y < mapSize; ++y) {
        for (int x = 0; x < mapSize; ++x) {
            float fx = (static_cast<float>(x) / static_cast<float>(mapSize - 1)) * (landSize - 1);
            float fy = (static_cast<float>(y) / static_cast<float>(mapSize - 1)) * (landSize - 1);

            int x0 = static_cast<int>(floor(fx));
            int y0 = static_cast<int>(floor(fy));
            int x1 = qMin(x0 + 1, landSize - 1);
            int y1 = qMin(y0 + 1, landSize - 1);

            float dx = fx - x0;
            float dy = fy - y0;

            float h00 = static_cast<float>(land->heightData[y0][x0]) * 8.0f + land->baseHeight;
            float h10 = static_cast<float>(land->heightData[y0][x1]) * 8.0f + land->baseHeight;
            float h01 = static_cast<float>(land->heightData[y1][x0]) * 8.0f + land->baseHeight;
            float h11 = static_cast<float>(land->heightData[y1][x1]) * 8.0f + land->baseHeight;

            float h0 = h00 + (h10 - h00) * dx;
            float h1 = h01 + (h11 - h01) * dx;
            float height = h0 + (h1 - h0) * dy;

            heightmap[y * mapSize + x] = height;
        }
    }
}

void LandscapeEditor::saveToLand(LandRecord* land)
{
    if (!land) {
        return;
    }

    const int landSize = 33;
    const int mapSize = 257;

    float minH = minHeight;
    float maxH = maxHeight;
    if (!heightmap.isEmpty()) {
        minH = heightmap.first();
        maxH = heightmap.first();
        for (float h : heightmap) {
            if (h < minH) minH = h;
            if (h > maxH) maxH = h;
        }
    }
    land->baseHeight = (minH + maxH) / 2.0f;

    for (int y = 0; y < landSize; ++y) {
        for (int x = 0; x < landSize; ++x) {
            float fx = (static_cast<float>(x) / static_cast<float>(landSize - 1)) * (mapSize - 1);
            float fy = (static_cast<float>(y) / static_cast<float>(landSize - 1)) * (mapSize - 1);

            int x0 = static_cast<int>(floor(fx));
            int y0 = static_cast<int>(floor(fy));
            int x1 = qMin(x0 + 1, mapSize - 1);
            int y1 = qMin(y0 + 1, mapSize - 1);

            float dx = fx - x0;
            float dy = fy - y0;

            float h00 = heightmap[y0 * mapSize + x0];
            float h10 = heightmap[y0 * mapSize + x1];
            float h01 = heightmap[y1 * mapSize + x0];
            float h11 = heightmap[y1 * mapSize + x1];

            float h0 = h00 + (h10 - h00) * dx;
            float h1 = h01 + (h11 - h01) * dx;
            float height = h0 + (h1 - h0) * dy;

            float offset = height - land->baseHeight;
            qint8 byteVal = static_cast<qint8>(qBound(-128.0f, offset / 8.0f, 127.0f));
            land->heightData[y][x] = byteVal;
        }
    }

    land->hasHeightData = true;
}

void LandscapeEditor::loadHeightmap()
{
    if (!currentCell) {
        statusLabel->setText("No cell loaded");
        return;
    }

    heightmap.resize(terrainSize * terrainSize);
    for (int i = 0; i < terrainSize * terrainSize; i++) {
        heightmap[i] = minHeight;
    }

    statusLabel->setText(QString("Loaded heightmap (%1x%2)").arg(terrainSize).arg(terrainSize));
}

void LandscapeEditor::saveHeightmap()
{
    if (!currentCell) {
        statusLabel->setText("No cell loaded");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Save Heightmap", "", "Heightmap Files (*.hgt)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        statusLabel->setText("Failed to save");
        return;
    }

    QDataStream out(&file);
    out.setByteOrder(QDataStream::LittleEndian);

    out << terrainSize;
    out << minHeight;
    out << maxHeight;

    for (int i = 0; i < terrainSize * terrainSize; i++) {
        out << heightmap[i];
    }

    file.close();

    if (currentLand) {
        saveToLand(currentLand);
        statusLabel->setText(QString("Saved to %1 (also persisted to LandRecord)").arg(fileName));
    } else {
        statusLabel->setText(QString("Saved to %1").arg(fileName));
    }
}

void LandscapeEditor::setupOpenGL()
{
    if (!glWidget || !glWidget->context()) {
        return;
    }

    glWidget->makeCurrent();

    if (!shaderProgram) {
        shaderProgram = new QOpenGLShaderProgram();
    }

    const QString vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aNormal;

        uniform mat4 mModel;
        uniform mat4 mView;
        uniform mat4 mProjection;

        out vec3 vNormal;

        void main()
        {
            gl_Position = mProjection * mView * mModel * vec4(aPosition, 1.0);
            vNormal = aNormal;
        }
    )";

    const QString fragmentShaderSource = R"(
        #version 330 core
        in vec3 vNormal;
        out vec4 FragColor;

        uniform vec3 lightDir;
        uniform vec3 objectColor;

        void main()
        {
            vec3 normal = normalize(vNormal);
            vec3 lightDirection = normalize(lightDir);
            float diffuse = max(dot(normal, lightDirection), 0.0f);
            vec3 result = (0.3 + diffuse) * objectColor;
            FragColor = vec4(result, 1.0);
        }
    )";

    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    shaderProgram->link();
    shaderProgram->bind();

    shaderProgram->setUniformValue("lightDir", QVector3D(0.5f, 1.0f, 0.3f));
    shaderProgram->setUniformValue("objectColor", QVector3D(0.4f, 0.6f, 0.4f));
    shaderProgram->release();

    vertexVbo.create();
    normalVbo.create();
    vao.create();

    glWidget->doneCurrent();
}

void LandscapeEditor::renderTerrain()
{
    if (heightmap.isEmpty()) {
        return;
    }

    glWidget->makeCurrent();

    shaderProgram->bind();
    vao.bind();

    QVector<QVector3D> terrainVertices;
    QVector<QVector3D> terrainNormals;
    QVector<unsigned int> terrainIndices;

    float scale = 10.0f;
    float heightScale = 0.1f;

    for (int y = 0; y < terrainSize - 1; y++) {
        for (int x = 0; x < terrainSize - 1; x++) {
            unsigned int idx = (y * terrainSize + x) * 4;

            terrainVertices.append(QVector3D(x * scale, heightmap[y * terrainSize + x] * heightScale, y * scale));
            terrainVertices.append(QVector3D((x + 1) * scale, heightmap[y * terrainSize + x + 1] * heightScale, y * scale));
            terrainVertices.append(QVector3D((x + 1) * scale, heightmap[(y + 1) * terrainSize + x + 1] * heightScale, (y + 1) * scale));
            terrainVertices.append(QVector3D(x * scale, heightmap[(y + 1) * terrainSize + x] * heightScale, (y + 1) * scale));

            terrainNormals.append(QVector3D(0.0f, 1.0f, 0.0f));
            terrainNormals.append(QVector3D(0.0f, 1.0f, 0.0f));
            terrainNormals.append(QVector3D(0.0f, 1.0f, 0.0f));
            terrainNormals.append(QVector3D(0.0f, 1.0f, 0.0f));

            terrainIndices.push_back(idx);
            terrainIndices.push_back(idx + 1);
            terrainIndices.push_back(idx + 2);
            terrainIndices.push_back(idx);
            terrainIndices.push_back(idx + 2);
            terrainIndices.push_back(idx + 3);
        }
    }

    vertexVbo.bind();
    vertexVbo.allocate(terrainVertices.constData(), terrainVertices.size() * sizeof(QVector3D));
    shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(QVector3D));
    shaderProgram->enableAttributeArray(0);
    vertexVbo.release();

    normalVbo.bind();
    normalVbo.allocate(terrainNormals.constData(), terrainNormals.size() * sizeof(QVector3D));
    shaderProgram->setAttributeBuffer(1, GL_FLOAT, 0, 3, sizeof(QVector3D));
    shaderProgram->enableAttributeArray(1);
    normalVbo.release();

    QMatrix4x4 model;
    QMatrix4x4 view;
    view.rotate(viewRotX, 1.0f, 0.0f, 0.0f);
    view.rotate(viewRotY, 0.0f, 1.0f, 0.0f);
    view.scale(viewZoom);

    shaderProgram->setUniformValue("mModel", model);
    shaderProgram->setUniformValue("mView", view);
    shaderProgram->setUniformValue("mProjection", QMatrix4x4());

    glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, nullptr);

    vao.release();
    shaderProgram->release();

    glWidget->doneCurrent();
}

void LandscapeEditor::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    if (!shaderProgram) {
        setupOpenGL();
    }

    glWidget->makeCurrent();
    glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    renderTerrain();
    glWidget->doneCurrent();
}

void LandscapeEditor::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (currentCell && !heightmap.isEmpty()) {
            if (!hasOriginalState) {
                originalHeightmap = heightmap;
                hasOriginalState = true;
            }
            int x = event->pos().x() / terrainSize;
            int y = event->pos().y() / terrainSize;
            applyBrush(x, y);
            glWidget->update();
        }
        dragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void LandscapeEditor::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging) {
        if (currentCell && !heightmap.isEmpty() && event->buttons() & Qt::LeftButton) {
            if (!hasOriginalState) {
                originalHeightmap = heightmap;
                hasOriginalState = true;
            }
            int x = event->pos().x() / terrainSize;
            int y = event->pos().y() / terrainSize;
            applyBrush(x, y);
            glWidget->update();
        } else {
            QPoint delta = event->pos() - lastMousePos;
            viewRotX += delta.y() * 0.5f;
            viewRotY += delta.x() * 0.5f;
            lastMousePos = event->pos();
            glWidget->update();
        }
    }
}

void LandscapeEditor::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && hasOriginalState && mUndoStack) {
        if (heightmap != originalHeightmap) {
            LandscapeEditCommand* cmd = new LandscapeEditCommand(
                &heightmap, terrainSize, originalHeightmap, heightmap);
            mUndoStack->push(cmd);
        }
        hasOriginalState = false;
        originalHeightmap.clear();
    }
    dragging = false;
    setCursor(Qt::ArrowCursor);
    QWidget::mouseReleaseEvent(event);
}

void LandscapeEditor::wheelEvent(QWheelEvent* event)
{
    viewZoom *= (event->angleDelta().y() > 0) ? 1.1f : 0.9f;
    viewZoom = qBound(0.1f, viewZoom, 10.0f);
    glWidget->update();
}

void LandscapeEditor::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (glWidget) {
        glWidget->resize(event->size());
    }
}

void LandscapeEditor::onBrushSizeChanged(int size)
{
    brushSize = size;
}

void LandscapeEditor::onBrushStrengthChanged(int strength)
{
    brushStrength = strength;
}

void LandscapeEditor::onBrushTypeChanged(int type)
{
    brushType = type;
}

void LandscapeEditor::onHeightLimitChanged(int height)
{
    heightLimit = height;
}

void LandscapeEditor::onSaveClicked()
{
    saveHeightmap();
}

void LandscapeEditor::onLoadClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Load Heightmap", "", "Heightmap Files (*.hgt)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        statusLabel->setText("Failed to load");
        return;
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    int loadedSize;
    float loadedMin, loadedMax;
    in >> loadedSize >> loadedMin >> loadedMax;

    terrainSize = loadedSize;
    minHeight = loadedMin;
    maxHeight = loadedMax;
    heightmap.resize(terrainSize * terrainSize);

    for (int i = 0; i < terrainSize * terrainSize; i++) {
        in >> heightmap[i];
    }

    file.close();
    statusLabel->setText(QString("Loaded from %1").arg(fileName));
    glWidget->update();
}

void LandscapeEditor::onCopyHeightmapClicked()
{
    if (heightmap.isEmpty()) {
        statusLabel->setText("No heightmap to copy");
        return;
    }

    copiedHeightmap = heightmap;
    hasCopiedHeightmap = true;
    statusLabel->setText(QString("Heightmap copied (%1x%1)").arg(terrainSize));
}

void LandscapeEditor::onPasteHeightmapClicked()
{
    if (!hasCopiedHeightmap) {
        statusLabel->setText("No heightmap in clipboard");
        return;
    }

    if (copiedHeightmap.size() != heightmap.size()) {
        statusLabel->setText(QString("Size mismatch: copied %1x%1, current %2x%2")
            .arg(static_cast<int>(std::sqrt(copiedHeightmap.size())))
            .arg(terrainSize));
        return;
    }

    if (mData && currentCell && currentLand && mUndoStack) {
        QVector<float>* heightmapPtr = &heightmap;
        mUndoStack->push(new LandscapeEditCommand(heightmapPtr, terrainSize, originalHeightmap, copiedHeightmap));
    } else {
        heightmap = copiedHeightmap;
    }

    hasCopiedHeightmap = false;
    statusLabel->setText("Heightmap pasted");
    glWidget->update();
}

float LandscapeEditor::getHeightAt(int x, int y) const
{
    if (x < 0 || x >= terrainSize || y < 0 || y >= terrainSize) {
        return 0.0f;
    }
    return heightmap[y * terrainSize + x];
}

void LandscapeEditor::setHeightAt(int x, int y, float height)
{
    if (x < 0 || x >= terrainSize || y < 0 || y >= terrainSize) {
        return;
    }
    heightmap[y * terrainSize + x] = qBound(minHeight, height, maxHeight);
}

void LandscapeEditor::applyBrush(int x, int y)
{
    int radius = brushSize / 2;

    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int nx = x + dx;
            int ny = y + dy;

            float dist = sqrt(dx * dx + dy * dy);
            if (dist > radius) {
                continue;
            }

            float factor = 1.0f - (dist / radius);
            factor = factor * factor;

            float currentHeight = getHeightAt(nx, ny);
            float newHeight = currentHeight;

            switch (brushType) {
            case 0: // Raise
                newHeight += brushStrength * factor * 0.1f;
                break;
            case 1: // Lower
                newHeight -= brushStrength * factor * 0.1f;
                break;
            case 2: // Smooth
            {
                float avg = 0.0f;
                int count = 0;
                for (int sy = -1; sy <= 1; sy++) {
                    for (int sx = -1; sx <= 1; sx++) {
                        avg += getHeightAt(nx + sx, ny + sy);
                        count++;
                    }
                }
                avg /= count;
                newHeight = currentHeight + (avg - currentHeight) * factor * 0.5f;
                break;
            }
            case 3: // Flat
                newHeight = currentHeight + (heightLimit - currentHeight) * factor;
                break;
            }

            // Apply height limit clamping for Raise/Lower brush types
            if (brushType == 0) { // Raise
                newHeight = qMin(newHeight, static_cast<float>(heightLimit));
            } else if (brushType == 1) { // Lower
                newHeight = qMax(newHeight, static_cast<float>(heightLimit));
            }

            setHeightAt(nx, ny, newHeight);
        }
    }
}

void LandscapeEditor::refreshTextureLayerTable()
{
    textureLayerTable->setRowCount(0);
    for (int i = 0; i < textureLayers.size(); i++) {
        const TextureLayer& layer = textureLayers[i];
        int row = textureLayerTable->rowCount();
        textureLayerTable->insertRow(row);

        auto* indexItem = new QTableWidgetItem(QString::number(layer.index));
        indexItem->setFlags(indexItem->flags() & ~Qt::ItemIsEditable);
        textureLayerTable->setItem(row, 0, indexItem);

        auto* pathItem = new QTableWidgetItem(layer.texturePath);
        textureLayerTable->setItem(row, 1, pathItem);

        auto* opacitySpin = new QDoubleSpinBox();
        opacitySpin->setRange(0.0, 1.0);
        opacitySpin->setValue(layer.opacity);
        opacitySpin->setDecimals(2);
        opacitySpin->setSingleStep(0.01);
        textureLayerTable->setCellWidget(row, 2, opacitySpin);

        QObject::connect(opacitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this, i](double value) {
                if (i < textureLayers.size()) {
                    textureLayers[i].opacity = value;
                }
            });
    }
}

void LandscapeEditor::refreshVegetationTable()
{
    vegetationTable->setRowCount(0);
    for (int i = 0; i < vegetationEntries.size(); i++) {
        const VegetationEntry& entry = vegetationEntries[i];
        int row = vegetationTable->rowCount();
        vegetationTable->insertRow(row);

        auto* formIDItem = new QTableWidgetItem(entry.formID);
        vegetationTable->setItem(row, 0, formIDItem);

        auto* densitySpin = new QSpinBox();
        densitySpin->setRange(0, 100);
        densitySpin->setValue(entry.density);
        vegetationTable->setCellWidget(row, 1, densitySpin);

        auto* minHeightSpin = new QDoubleSpinBox();
        minHeightSpin->setRange(-10000.0, 100000.0);
        minHeightSpin->setValue(entry.minHeight);
        minHeightSpin->setDecimals(2);
        vegetationTable->setCellWidget(row, 2, minHeightSpin);

        auto* maxHeightSpin = new QDoubleSpinBox();
        maxHeightSpin->setRange(-10000.0, 100000.0);
        maxHeightSpin->setValue(entry.maxHeight);
        maxHeightSpin->setDecimals(2);
        vegetationTable->setCellWidget(row, 3, maxHeightSpin);

        QObject::connect(densitySpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [this, i](int value) {
                if (i < vegetationEntries.size()) {
                    vegetationEntries[i].density = value;
                }
            });

        QObject::connect(minHeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this, i](double value) {
                if (i < vegetationEntries.size()) {
                    vegetationEntries[i].minHeight = value;
                }
            });

        QObject::connect(maxHeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this, i](double value) {
                if (i < vegetationEntries.size()) {
                    vegetationEntries[i].maxHeight = value;
                }
            });
    }
}

void LandscapeEditor::onAddLayer()
{
    TextureLayer layer;
    layer.index = textureLayers.size();
    layer.texturePath = QString("Textures\\Landscape\\Layer%1.dds").arg(layer.index);
    layer.opacity = 1.0;
    textureLayers.append(layer);
    refreshTextureLayerTable();
    statusLabel->setText(QString("Added texture layer %1").arg(layer.index));
}

void LandscapeEditor::onRemoveLayer()
{
    int currentRow = textureLayerTable->currentRow();
    if (currentRow < 0 || currentRow >= textureLayers.size()) {
        return;
    }

    textureLayers.removeAt(currentRow);
    for (int i = 0; i < textureLayers.size(); i++) {
        textureLayers[i].index = i;
    }
    refreshTextureLayerTable();
    statusLabel->setText(QString("Removed texture layer"));
}

void LandscapeEditor::onMoveLayerUp()
{
    int currentRow = textureLayerTable->currentRow();
    if (currentRow <= 0 || currentRow >= textureLayers.size()) {
        return;
    }

    textureLayers.swapItemsAt(currentRow, currentRow - 1);
    for (int i = 0; i < textureLayers.size(); i++) {
        textureLayers[i].index = i;
    }
    refreshTextureLayerTable();
    textureLayerTable->selectRow(currentRow - 1);
    statusLabel->setText("Moved layer up");
}

void LandscapeEditor::onMoveLayerDown()
{
    int currentRow = textureLayerTable->currentRow();
    if (currentRow < 0 || currentRow >= textureLayers.size() - 1) {
        return;
    }

    textureLayers.swapItemsAt(currentRow, currentRow + 1);
    for (int i = 0; i < textureLayers.size(); i++) {
        textureLayers[i].index = i;
    }
    refreshTextureLayerTable();
    textureLayerTable->selectRow(currentRow + 1);
    statusLabel->setText("Moved layer down");
}

void LandscapeEditor::onAddPlant()
{
    VegetationEntry entry;
    entry.formID = "00000000";
    entry.density = 50;
    entry.minHeight = 0.0;
    entry.maxHeight = 100.0;
    vegetationEntries.append(entry);
    refreshVegetationTable();
    statusLabel->setText(QString("Added vegetation entry (%1 total)").arg(vegetationEntries.size()));
}

void LandscapeEditor::onRemovePlant()
{
    int currentRow = vegetationTable->currentRow();
    if (currentRow < 0 || currentRow >= vegetationEntries.size()) {
        return;
    }

    vegetationEntries.removeAt(currentRow);
    refreshVegetationTable();
    statusLabel->setText("Removed vegetation entry");
}
