#include "nifviewportwidget.hpp"

#include "gizmomath.hpp"

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QFileDialog>
#include <QDebug>
#include <QPainter>
#include <QFont>

#include "../../libs/files/nif/nifparser.hpp"
#include "../../libs/files/nif/ddsdecoder.hpp"
#include "../../libs/files/nifanim/nifanimation.hpp"
#include "../../libs/files/nif/particle/particleeffects.hpp"
#include "model/tools/nifanimationstate.hpp"
#include "particlesystem.hpp"
#include "particlerenderer.hpp"
#include "logger.hpp"

#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLContext>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>
#include <QToolBar>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QOpenGLTexture>
#include <QSlider>
#include <QHBoxLayout>
#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <QTreeWidget>
#include <QSplitter>
#include <QActionGroup>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include "mesheditordialog.hpp"

#include <cmath>
#include <cstddef>
#include <functional>

namespace {

// Minimal TGA loader (uncompressed RGB types 2, and RLE types 10), 24/32-bit.
// Qt's QImage cannot decode TGA, a common Bethesda asset format.
QImage loadTga(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return QImage();

    QByteArray data = file.readAll();
    file.close();
    if (data.size() < 18) return QImage();

    const quint8 imageType = static_cast<quint8>(data[2]);
    if (imageType != 2 && imageType != 10) return QImage();

    const quint16 width = *reinterpret_cast<const quint16*>(data.constData() + 12);
    const quint16 height = *reinterpret_cast<const quint16*>(data.constData() + 14);
    const quint8 pixelDepth = static_cast<quint8>(data[16]);
    if (width == 0 || height == 0 || (pixelDepth != 24 && pixelDepth != 32)) {
        return QImage();
    }

    const int bytesPerPixel = pixelDepth / 8;
    const bool originTopLeft = (static_cast<quint8>(data[17]) & 0x20) != 0;
    const int idLength = static_cast<quint8>(data[0]);
    int pos = 18 + idLength;

    QImage img(width, height, QImage::Format_ARGB32);
    const int pixelCount = width * height;

    QVector<quint8> pixels;
    pixels.reserve(pixelCount * bytesPerPixel);

    if (imageType == 10) {
        // RLE
        int decoded = 0;
        while (decoded < pixelCount && pos + 1 < data.size()) {
            const quint8 packet = static_cast<quint8>(data[pos++]);
            const int count = (packet & 0x7F) + 1;
            if (packet & 0x80) {
                for (int i = 0; i < count && pos + bytesPerPixel <= data.size(); ++i) {
                    for (int b = 0; b < bytesPerPixel; ++b) pixels.append(data[pos + b]);
                }
                pos += bytesPerPixel;
            } else {
                for (int i = 0; i < count && pos + count * bytesPerPixel <= data.size(); ++i) {
                    for (int b = 0; b < bytesPerPixel; ++b) pixels.append(data[pos + i * bytesPerPixel + b]);
                }
                pos += count * bytesPerPixel;
            }
            decoded += count;
        }
    } else {
        const int needed = pixelCount * bytesPerPixel;
        if (data.size() - pos < needed) return QImage();
        pixels.resize(needed);
        memcpy(pixels.data(), data.constData() + pos, needed);
    }

    int p = 0;
    for (int y = 0; y < height; ++y) {
        const int row = originTopLeft ? y : (height - 1 - y);
        for (int x = 0; x < width; ++x) {
            if (p + bytesPerPixel > pixels.size()) break;
            const quint8 b = pixels[p + 0];
            const quint8 g = pixels[p + 1];
            const quint8 r = pixels[p + 2];
            const quint8 a = (bytesPerPixel == 4) ? pixels[p + 3] : 255;
            img.setPixel(x, row, qRgba(r, g, b, a));
            p += bytesPerPixel;
        }
    }
    return img;
}


static const char* overlayVertexShaderSrc = R"(
    #version 330 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aColor;
    uniform mat4 mvp;
    out vec3 vColor;
    void main() {
        gl_Position = mvp * vec4(aPos, 1.0);
        vColor = aColor;
    }
)";

static const char* overlayFragmentShaderSrc = R"(
    #version 330 core
    in vec3 vColor;
    out vec4 FragColor;
    void main() {
        FragColor = vec4(vColor, 1.0);
    }
)";

} // namespace

void OverlayVBO::build(const QVector<OverlayVertex>& vertices, unsigned int mode) {
    if (!buffer.isCreated()) buffer.create();
    buffer.bind();
    buffer.allocate(vertices.constData(), vertices.size() * sizeof(OverlayVertex));
    buffer.release();
    vertexCount = vertices.size();
    primitiveMode = mode;
    dirty = false;
}

void OverlayVBO::draw(QOpenGLShaderProgram* shader, const QMatrix4x4& mvp) {
    if (vertexCount == 0 || !shader) return;
    shader->bind();
    shader->setUniformValue("mvp", mvp);
    buffer.bind();
    shader->setAttributeBuffer(0, GL_FLOAT, offsetof(OverlayVertex, position), 3, sizeof(OverlayVertex));
    shader->enableAttributeArray(0);
    shader->setAttributeBuffer(1, GL_FLOAT, offsetof(OverlayVertex, color), 3, sizeof(OverlayVertex));
    shader->enableAttributeArray(1);
    glDrawArrays(primitiveMode, 0, vertexCount);
    shader->disableAttributeArray(0);
    shader->disableAttributeArray(1);
    buffer.release();
    shader->release();
}

void OverlayVBO::clear() {
    if (buffer.isCreated()) buffer.destroy();
    vertexCount = 0;
    dirty = true;
}

QImage NifViewportWidget::loadTextureImage(const QString& path)
{
    QImage img(path);
    if (!img.isNull()) return img;
    img = loadTga(path);
    if (!img.isNull()) return img;
    img = DdsDecoder::decodeFile(path);
    return img;
}

NifViewportWidget::NifViewportWidget(QWidget* parent) :
    QWidget(parent),
    glWidget(nullptr),
    shaderProgram(nullptr),
    vbo(QOpenGLBuffer::VertexBuffer),
    ibo(QOpenGLBuffer::IndexBuffer),
    meshBuilt(false),
    rotationX(0),
    rotationY(0),
    zoom(1.0f),
    cameraPos(0.0f, 0.0f, 0.0f),
    dragging(false),
    selectedShape(-1),
    highlightEnabled(true),
    hierarchyVisible(false),
    selectedNode(nullptr),
    animState(nullptr),
    nifAnimData(nullptr),
    animTimeline(nullptr),
    animTimeLabel(nullptr),
    animClipLabel(nullptr),
    animSpeedCombo(nullptr),
    animClipCombo(nullptr),
    animPlayBtn(nullptr),
    animStopBtn(nullptr),
    animLoopBtn(nullptr),
    animToolbar(nullptr),
    animDraggingSlider(false)
{
    setMinimumSize(400, 300);
    setMouseTracking(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));

    mEditModeGroup = new QActionGroup(this);
    mEditModeGroup->setExclusive(true);
    mActionSelect = mEditModeGroup->addAction(tr("Select"));
    mActionMove = mEditModeGroup->addAction(tr("Move"));
    mActionRotate = mEditModeGroup->addAction(tr("Rotate"));
    mActionScale = mEditModeGroup->addAction(tr("Scale"));
    mActionSelect->setCheckable(true);
    mActionMove->setCheckable(true);
    mActionRotate->setCheckable(true);
    mActionScale->setCheckable(true);
    mActionSelect->setChecked(true);
    toolbar->addAction(mActionSelect);
    toolbar->addAction(mActionMove);
    toolbar->addAction(mActionRotate);
    toolbar->addAction(mActionScale);
    connect(mEditModeGroup, &QActionGroup::triggered, this, [this](QAction* a) {
        if (a == mActionSelect) setEditMode(EditMode::Select);
        else if (a == mActionMove) setEditMode(EditMode::Move);
        else if (a == mActionRotate) setEditMode(EditMode::Rotate);
        else if (a == mActionScale) setEditMode(EditMode::Scale);
    });

    toolbar->addSeparator();
    mActionSnapGrid = toolbar->addAction(tr("Snap to Grid"));
    mActionSnapGrid->setCheckable(true);
    connect(mActionSnapGrid, &QAction::toggled, this, [this](bool on) {
        mSnapToGrid = on;
    });
    mActionSnapAngle = toolbar->addAction(tr("Snap to Angle"));
    mActionSnapAngle->setCheckable(true);
    connect(mActionSnapAngle, &QAction::toggled, this, [this](bool on) {
        mSnapToAngle = on;
    });

    mSnapAngleSpin = new QSpinBox(toolbar);
    mSnapAngleSpin->setRange(1, 180);
    mSnapAngleSpin->setValue(mSnapAngleIncrement);
    mSnapAngleSpin->setSuffix(QStringLiteral("\u00b0"));
    toolbar->addWidget(mSnapAngleSpin);
    connect(mSnapAngleSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
        [this](int v) { mSnapAngleIncrement = v; });

    mSnapGridSpin = new QDoubleSpinBox(toolbar);
    mSnapGridSpin->setRange(0.01, 4096.0);
    mSnapGridSpin->setDecimals(3);
    mSnapGridSpin->setValue(mSnapGridSize);
    toolbar->addWidget(mSnapGridSpin);
    connect(mSnapGridSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
        [this](double v) { mSnapGridSize = v; });

    toolbar->addSeparator();

    auto* exportAction = toolbar->addAction(tr("Export NIF..."));
    connect(exportAction, &QAction::triggered, this, &NifViewportWidget::exportNif);

    auto* editAction = toolbar->addAction(tr("Edit Mesh..."));
    connect(editAction, &QAction::triggered, this, [this]() {
        if (!nifParser) {
            LOG_ERROR("No NIF loaded to edit");
            return;
        }
        MeshEditorDialog dialog(nifParser.get(), this, this);
        dialog.exec();
    });

    toolbar->addSeparator();
    auto* filterLabel = new QLabel(tr("Filter:"));
    filterLabel->setObjectName("filterLabel");
    toolbar->addWidget(filterLabel);

    auto* filterCombo = new QComboBox();
    filterCombo->setObjectName("filterCombo");
    filterCombo->addItems({tr("Linear (default)"), tr("Nearest"), tr("Mipmap")});
    filterCombo->setCurrentIndex(0);
    toolbar->addWidget(filterCombo);

    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NifViewportWidget::onFilterChanged);

    toolbar->addSeparator();
    auto* wireframeBtn = new QPushButton(tr("Wireframe"));
    wireframeBtn->setCheckable(true);
    wireframeBtn->setObjectName("wireframeBtn");
    toolbar->addWidget(wireframeBtn);
    connect(wireframeBtn, &QPushButton::toggled, this, [this](bool checked) {
        wireframeMode = checked;
        glWidget->update();
    });

    auto* gridBtn = new QPushButton(tr("Grid"));
    gridBtn->setCheckable(true);
    gridBtn->setObjectName("gridBtn");
    toolbar->addWidget(gridBtn);
    connect(gridBtn, &QPushButton::toggled, this, [this](bool checked) {
        gridEnabled = checked;
        glWidget->update();
    });

    auto* axisBtn = new QPushButton(tr("Axis"));
    axisBtn->setCheckable(true);
    axisBtn->setObjectName("axisBtn");
    toolbar->addWidget(axisBtn);
    connect(axisBtn, &QPushButton::toggled, this, [this](bool checked) {
        axisEnabled = checked;
        glWidget->update();
    });

    auto* boundsBtn = new QPushButton(tr("Bounds"));
    boundsBtn->setCheckable(true);
    boundsBtn->setObjectName("boundsBtn");
    toolbar->addWidget(boundsBtn);
    connect(boundsBtn, &QPushButton::toggled, this, [this](bool checked) {
        boundsEnabled = checked;
        m_bboxVBO.dirty = true;
        glWidget->update();
    });

    auto* collisionBtn = new QPushButton(tr("Collision"));
    collisionBtn->setCheckable(true);
    collisionBtn->setObjectName("collisionBtn");
    toolbar->addWidget(collisionBtn);
    connect(collisionBtn, &QPushButton::toggled, this, [this](bool checked) {
        collisionEnabled = checked;
        glWidget->update();
    });

    auto* cellGridBtn = new QPushButton(tr("Cell Grid"));
    cellGridBtn->setCheckable(true);
    cellGridBtn->setObjectName("cellGridBtn");
    toolbar->addWidget(cellGridBtn);
    connect(cellGridBtn, &QPushButton::toggled, this, [this](bool checked) {
        cellGridEnabled = checked;
        glWidget->update();
    });

    toolbar->addSeparator();
    auto* hierarchyBtn = new QPushButton(tr("Hierarchy"));
    hierarchyBtn->setCheckable(true);
    hierarchyBtn->setObjectName("hierarchyBtn");
    toolbar->addWidget(hierarchyBtn);
    connect(hierarchyBtn, &QPushButton::toggled, this, &NifViewportWidget::toggleHierarchy);

    layout->addWidget(toolbar);

    animState = new NifAnimationState(this);
    setupAnimToolbar();
    setupParticleToolbar();

    mainSplitter = new QSplitter(Qt::Horizontal, this);

    glWidget = new QOpenGLWidget(this);
    glWidget->setMouseTracking(true);
    glWidget->setFocusPolicy(Qt::StrongFocus);
    glWidget->installEventFilter(this);
    mainSplitter->addWidget(glWidget);

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 4, 4, 4);

    hierarchyTree = new QTreeWidget(rightPanel);
    hierarchyTree->setHeaderLabel(tr("Node Hierarchy"));
    rightLayout->addWidget(hierarchyTree, 1);

    nodeInfoLabel = new QLabel(tr("No node selected"), rightPanel);
    nodeInfoLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    rightLayout->addWidget(nodeInfoLabel);

    mainSplitter->addWidget(rightPanel);
    mainSplitter->setSizes({400, 200});
    rightPanel->setVisible(false);

    layout->addWidget(mainSplitter);

    setLayout(layout);

    connect(hierarchyTree, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* item, int column) {
        Q_UNUSED(column);
        if (!item) return;
        quint64 ptr = item->data(0, Qt::UserRole).toULongLong();
        if (ptr == 0) return;
        Nif::Node* node = reinterpret_cast<Nif::Node*>(static_cast<quintptr>(ptr));
        if (selectedNode == node) {
            selectedNode = nullptr;
        } else {
            selectedNode = node;
        }
        updateNodeMetadata();
        glWidget->update();
    });
}

NifViewportWidget::~NifViewportWidget()
{
    if (m_particleSystem) {
        m_particleSystem->stop();
    }

    if (glWidget && glWidget->context()) {
        glWidget->makeCurrent();

        delete m_particleRenderer;
        m_particleRenderer = nullptr;

        delete m_overlayShader;
        m_overlayShader = nullptr;
        delete shaderProgram;
        shaderProgram = nullptr;

        vbo.destroy();
        ibo.destroy();
        vao.destroy();
    m_navmeshVBO.clear();
    m_pathVBO.clear();
    m_bboxVBO.clear();
    m_gridVBO.clear();
    m_highlightTriVBO.clear();
    m_collisionVBO.clear();
    m_cellGridVBO.clear();
    m_cellRefVBO.clear();
    m_nodeAxisVBO_R.clear();
    m_nodeAxisVBO_G.clear();
    m_nodeAxisVBO_B.clear();
    m_translateGizmoVBO.clear();
    m_rotateGizmoVBO.clear();
    m_scaleGizmoVBO.clear();
    clearTextures();
        if (defaultTexture) { delete defaultTexture; defaultTexture = nullptr; }

    glWidget->doneCurrent();
    }
    delete glWidget;
    delete animState;
    delete nifAnimData;
    delete m_particleSystem;
    m_particleSystem = nullptr;
}

void NifViewportWidget::loadNif(const QString& fileName)
{
    LOG_INFO(QString("Loading NIF: %1").arg(fileName));

    nifParser.reset(new Nif::NifParser());
    if (nifParser->load(fileName)) {
        currentNifFile = fileName;
        nifFileDir = QFileInfo(fileName).absolutePath();
        LOG_INFO("NIF loaded successfully");
        buildMesh();
        buildHierarchyTree();
        initAnimationState();
        initParticleSystems();
        glWidget->update();
    } else {
        LOG_ERROR("Failed to load NIF");
    }
}

bool NifViewportWidget::saveNif(const QString& fileName)
{
    if (!nifParser) {
        LOG_ERROR("No NIF loaded to save");
        return false;
    }
    const bool ok = nifParser->save(fileName);
    if (ok) {
        currentNifFile = fileName;
    }
    return ok;
}

void NifViewportWidget::exportNif()
{
    if (!nifParser) {
        LOG_ERROR("No NIF loaded to export");
        return;
    }

    QString suggested = currentNifFile;
    if (suggested.isEmpty()) {
        suggested = "exported.nif";
    } else {
        QFileInfo info(suggested);
        suggested = info.absolutePath() + "/" + info.baseName() + "_export.nif";
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export NIF"), suggested, tr("NIF Files (*.nif)"));
    if (fileName.isEmpty()) {
        return;
    }

    if (saveNif(fileName)) {
        LOG_INFO(QString("NIF exported to: %1").arg(fileName));
    } else {
        LOG_ERROR("NIF export failed");
    }
}

void NifViewportWidget::translateMesh(float dx, float dy, float dz)
{
    if (!nifParser) return;
    nifParser->translateAll(dx, dy, dz);
    buildMesh();
    glWidget->update();
}

void NifViewportWidget::scaleMesh(float factor)
{
    if (!nifParser) return;
    nifParser->scaleAll(factor);
    buildMesh();
    glWidget->update();
}

int NifViewportWidget::vertexCount() const
{
    return nifParser ? nifParser->totalVertexCount() : 0;
}

void NifViewportWidget::refreshMesh()
{
    if (!nifParser) return;
    buildMesh();
    glWidget->update();
}

void NifViewportWidget::showPreviewPrimitive(PrimitiveMeshGenerator::Type type, float size)
{
    const PrimitiveMeshGenerator::Mesh mesh =
        PrimitiveMeshGenerator::generate(type, size);

    QVector<OverlayVertex> verts;
    verts.reserve(mesh.vertices.size());
    const QVector3D color(0.55f, 0.75f, 0.95f);
    for (const QVector3D& v : mesh.vertices)
    {
        verts.append({ v, color });
    }
    if (m_primitiveVBO.dirty || m_primitiveVBO.vertexCount != verts.size())
    {
        m_primitiveVBO.build(verts, GL_TRIANGLES);
    }
    glWidget->update();
    LOG_INFO(QString("Showing preview primitive (%1 triangles)")
        .arg(mesh.triangleCount()));
}

void NifViewportWidget::clearPreviewPrimitive()
{
    if (m_primitiveVBO.vertexCount == 0) return;
    m_primitiveVBO.clear();
    glWidget->update();
}

bool NifViewportWidget::isShowingPreviewPrimitive() const
{
    return m_primitiveVBO.vertexCount > 0;
}

const Nif::NifParser* NifViewportWidget::getNifParser() const
{
    return nifParser.get();
}

void NifViewportWidget::setNavmeshData(const QVector<QVector3D>& triangles)
{
    navmeshTriangles = triangles;
    navmeshEnabled = !triangles.isEmpty();
    m_navmeshVBO.dirty = true;
    glWidget->update();
}

void NifViewportWidget::setPathData(const QVector<QVector3D>& waypoints)
{
    pathWaypoints = waypoints;
    pathEnabled = !waypoints.isEmpty();
    m_pathVBO.dirty = true;
    glWidget->update();
}

void NifViewportWidget::highlightNavmeshTriangle(int index)
{
    mHighlightedTriangle = index;
    m_highlightTriVBO.dirty = true;
    glWidget->update();
}

void NifViewportWidget::setCellReferences(const QVector<ViewportCellRef>& refs)
{
    cellReferences = refs;
    if (mSelectedRefIndex >= cellReferences.size()) {
        mSelectedRefIndex = -1;
    }
    m_cellRefVBO.dirty = true;
    m_cellGridVBO.dirty = true;
    glWidget->update();
}

void NifViewportWidget::setSelectedRefIndex(int index)
{
    if (index < -1 || index >= cellReferences.size()) {
        index = -1;
    }
    if (mSelectedRefIndex == index) {
        glWidget->update();
        return;
    }
    mSelectedRefIndex = index;
    mHoverAxis = -1;
    mHoverRefIndex = -1;
    m_cellRefVBO.dirty = true;
    glWidget->update();
}

void NifViewportWidget::focusOnReference(const QVector3D& gameUnitsPos)
{
    cameraPos = gameUnitsPos * 0.01f;  // cameraPos lives in view space where model scales 0.01
    updateCamera();
}

void NifViewportWidget::setSelectedRefByDataIndex(int dataIndex)
{
    for (int i = 0; i < cellReferences.size(); ++i)
    {
        if (cellReferences[i].dataIndex == dataIndex)
        {
            setSelectedRefIndex(i);
            focusOnReference(cellReferences[i].position);
            return;
        }
    }
    setSelectedRefIndex(-1);
}

void NifViewportWidget::updateParticleSystem(const ParticleSystemData* data)
{
    if (m_particleSystem) {
        m_particleSystem->setSettings(data);
    }
}

void NifViewportWidget::clear()
{
    LOG_DEBUG("Clearing viewport");
    nifParser.reset();
    currentNifFile.clear();
    nifFileDir.clear();
    vertices.clear();
    normals.clear();
    uvs.clear();
    indices.clear();
    shapeIndexRanges.clear();
    shapeBaseColors.clear();
    shapeTexturePaths.clear();
    shapeAlphaModes.clear();
    shapeOpacity.clear();
    shapeSpecularColors.clear();
    shapeSpecularExponents.clear();
    shapeEmissionColors.clear();
    shapeBounds.clear();
    navmeshTriangles.clear();
    pathWaypoints.clear();
    cellReferences.clear();
    mSelectedRefIndex = -1;
    mHoverRefIndex = -1;
    mGizmoDragging = false;
    mGizmoAxis = -1;
    mHoverAxis = -1;
    cellGridEnabled = false;
    collisionEnabled = false;
    meshBuilt = false;
    m_meshDirty = false;

        m_navmeshVBO.clear();
        m_pathVBO.clear();
        m_bboxVBO.clear();
        m_gridVBO.clear();
        m_highlightTriVBO.clear();
        m_collisionVBO.clear();
        m_axisVBO_R.clear();
        m_axisVBO_G.clear();
        m_axisVBO_B.clear();
        m_cellGridVBO.clear();
        m_cellRefVBO.clear();
        m_nodeAxisVBO_R.clear();
        m_nodeAxisVBO_G.clear();
        m_nodeAxisVBO_B.clear();
        m_translateGizmoVBO.clear();
        m_rotateGizmoVBO.clear();
        m_scaleGizmoVBO.clear();
        clearTextures();

    hierarchyTree->clear();
    nodeCumulativeTransforms.clear();
    shapeOwnerNode.clear();
    selectedNode = nullptr;
    nodeInfoLabel->setText(tr("No node selected"));

    if (animState) {
        animState->stop();
        animToolbar->setVisible(false);
        animBlendToolbar->setVisible(false);
        animClipCombo->clear();
        animTimeLabel->setText("0:00 / 0:00");
        animTimeline->setValue(0);
    }
    delete nifAnimData;
    nifAnimData = nullptr;
    restVertices.clear();
    restNormals.clear();

    if (m_particleSystem) {
        m_particleSystem->stop();
        m_particleSystem->reset();
    }
    if (m_particleToolbar) {
        m_particleToolbar->setVisible(false);
    }

    glWidget->update();
}

void NifViewportWidget::setupOpenGL()
{
    if (!glWidget || !glWidget->context()) {
        return;
    }

    glWidget->makeCurrent();

    setupShaders();

    if (!m_overlayShader) {
        m_overlayShader = new QOpenGLShaderProgram(this);
        m_overlayShader->addShaderFromSourceCode(QOpenGLShader::Vertex, overlayVertexShaderSrc);
        m_overlayShader->addShaderFromSourceCode(QOpenGLShader::Fragment, overlayFragmentShaderSrc);
        m_overlayShader->link();
    }

    vbo.create();
    ibo.create();
    vao.create();

    glWidget->doneCurrent();
    LOG_DEBUG("OpenGL initialized");
}

void NifViewportWidget::setupShaders()
{
    if (!shaderProgram) {
        shaderProgram = new QOpenGLShaderProgram();
    }

    const QString vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoord;
        layout(location = 3) in vec4 aColor;

        uniform mat4 mModel;
        uniform mat4 mView;
        uniform mat4 mProjection;

        out vec3 vNormal;
        out vec2 vTexCoord;
        out vec4 vColor;

        void main()
        {
            gl_Position = mProjection * mView * mModel * vec4(aPosition, 1.0);
            vNormal = aNormal;
            vTexCoord = aTexCoord;
            vColor = aColor;
        }
    )";

    const QString fragmentShaderSource = R"(
        #version 330 core
        in vec3 vNormal;
        in vec2 vTexCoord;
        in vec4 vColor;

        out vec4 FragColor;

        uniform vec3 lightDir;
        uniform vec3 objectColor;
        uniform float shininess;
        uniform vec3 emissionColor;
        uniform float opacity;
        uniform float ambientStrength;
        uniform float specularStrength;
        uniform vec3 specularColor;
        uniform sampler2D tex;
        uniform bool useTexture;

        void main()
        {
            vec3 normal = normalize(vNormal);
            vec3 lightDirection = normalize(lightDir);

            float ambient = ambientStrength;
            float diffuse = max(dot(normal, lightDirection), 0.0f);

            vec3 viewDirection = normalize(vec3(0.0, 0.0, 5.0) - vec3(0.0));
            vec3 reflectDirection = reflect(-lightDirection, normal);
            float spec = specularStrength * pow(max(dot(viewDirection, reflectDirection), 0.0f), shininess);

            vec4 texColor = useTexture ? texture(tex, vTexCoord) : vec4(1.0);
            vec3 materialColor = objectColor * vColor.rgb;
            vec3 lit = (ambient + diffuse) * materialColor * texColor.rgb + spec * specularColor;
            vec3 result = lit + emissionColor * materialColor;

            FragColor = vec4(result, texColor.a * vColor.a * opacity);
        }
    )";

    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    shaderProgram->link();

    shaderProgram->bind();

    shaderProgram->setUniformValue("lightDir", QVector3D(0.5f, 1.0f, 0.3f));
    shaderProgram->setUniformValue("shininess", 32.0f);
    shaderProgram->setUniformValue("objectColor", QVector3D(0.7f, 0.7f, 0.8f));
    shaderProgram->setUniformValue("emissionColor", QVector3D(0.0f, 0.0f, 0.0f));
    shaderProgram->setUniformValue("opacity", 1.0f);
    shaderProgram->setUniformValue("ambientStrength", 0.3f);
    shaderProgram->setUniformValue("specularStrength", 1.0f);
    shaderProgram->setUniformValue("specularColor", QVector3D(0.0f, 0.0f, 0.0f));

    shaderProgram->release();
}

void NifViewportWidget::buildMesh()
{
    vertices.clear();
    normals.clear();
    uvs.clear();
    indices.clear();
    shapeIndexRanges.clear();
    shapeBaseColors.clear();
    shapeTexturePaths.clear();
    shapeAlphaModes.clear();
    shapeOpacity.clear();
    shapeSpecularColors.clear();
    shapeSpecularExponents.clear();
    shapeEmissionColors.clear();
    shapeBounds.clear();
    shapeOwnerNode.clear();
    clearTextures();
    texturesBuilt = false;

    Nif::Node* root = nifParser->getRoot();
    if (!root) {
        meshBuilt = false;
        return;
    }

    QMatrix4x4 identity;
    buildMeshFromNode(root, identity);

    meshBuilt = !vertices.isEmpty();
    restVertices = vertices;
    restNormals = normals;
    m_meshDirty = true;
    m_bboxVBO.dirty = true;
    m_collisionVBO.dirty = true;
}

void NifViewportWidget::clearTextures()
{
    for (auto* tex : shapeTextures) {
        if (tex) delete tex;
    }
    shapeTextures.clear();
}

void NifViewportWidget::buildMeshFromNode(Nif::Node* node, const QMatrix4x4& parentTransform)
{
    if (!node) {
        return;
    }

    QMatrix4x4 localTransform;
    localTransform.translate(node->position.x, node->position.y, node->position.z);
    float rx = node->rotation.x * 180.0f / 3.14159265f;
    float ry = node->rotation.y * 180.0f / 3.14159265f;
    float rz = node->rotation.z * 180.0f / 3.14159265f;
    localTransform.rotate(rz, 0.0f, 0.0f, 1.0f);
    localTransform.rotate(ry, 0.0f, 1.0f, 0.0f);
    localTransform.rotate(rx, 1.0f, 0.0f, 0.0f);

    QMatrix4x4 cumulativeTransform = parentTransform * localTransform;
    nodeCumulativeTransforms[node] = cumulativeTransform;

    for (auto& shape : node->shapes) {
        const int indexStart = indices.size();
        const unsigned int vertexOffset = vertices.size();

        if (shape.normals.isEmpty()) {
            shape.recalculateNormals();
        }

        for (int vi = 0; vi < shape.vertices.size(); ++vi) {
            const auto& vertex = shape.vertices[vi];
            vertices.append(QVector3D(vertex.x, vertex.y, vertex.z));

            if (vi < shape.normals.size()) {
                normals.append(QVector3D(shape.normals[vi].x, shape.normals[vi].y, shape.normals[vi].z));
            } else {
                normals.append(QVector3D(0.0f, 1.0f, 0.0f));
            }

            if (vi < shape.uvs.size()) {
                uvs.append(QVector2D(shape.uvs[vi].u, shape.uvs[vi].v));
            } else {
                uvs.append(QVector2D(0.0f, 0.0f));
            }
        }

        if (!shape.indices.isEmpty()) {
            for (unsigned int idx : shape.indices) {
                indices.push_back(vertexOffset + idx);
            }
        } else {
            for (unsigned int i = 0; i + 2 < shape.vertices.size(); i += 3) {
                indices.push_back(vertexOffset + i);
                indices.push_back(vertexOffset + i + 1);
                indices.push_back(vertexOffset + i + 2);
            }
        }

        const int indexEnd = indices.size();
        shapeIndexRanges.append({indexStart, indexEnd});
        shapeOwnerNode.append(node);
        shapeBaseColors.append(QColor::fromRgbF(
            qBound(0.0f, shape.baseColor.r, 1.0f),
            qBound(0.0f, shape.baseColor.g, 1.0f),
            qBound(0.0f, shape.baseColor.b, 1.0f)));
        QString resolvedPath;
        if (!shape.texture.isEmpty()) {
            const QString& texPath = shape.texture;
            QFileInfo fi(texPath);
            if (fi.isAbsolute()) {
                resolvedPath = texPath;
            } else {
                QStringList candidates;
                if (texPath.startsWith("Textures\\", Qt::CaseInsensitive)
                    || texPath.startsWith("Textures/", Qt::CaseInsensitive)) {
                    candidates.append(nifFileDir + "/../../Data/" + texPath);
                }
                candidates.append(nifFileDir + "/" + QFileInfo(texPath).fileName());
                candidates.append(nifFileDir + "/../" + texPath);
                candidates.append(texPath);

                resolvedPath = texPath;
                for (const auto& cand : candidates) {
                    if (QFile::exists(cand)) {
                        resolvedPath = cand;
                        break;
                    }
                }
            }
        }
        shapeTexturePaths.append(resolvedPath);
        shapeAlphaModes.append(static_cast<int>(shape.alphaMode));
        shapeOpacity.append(shape.baseColor.a);
        shapeSpecularColors.append(QVector3D(shape.specularColor.r, shape.specularColor.g, shape.specularColor.b));
        shapeSpecularExponents.append(shape.specularExponent);
        shapeEmissionColors.append(QVector3D(shape.emissionColor.r, shape.emissionColor.g, shape.emissionColor.b));

        if (shape.vertices.isEmpty()) {
            shapeBounds.append({QVector3D(0, 0, 0), QVector3D(0, 0, 0)});
        } else {
            float minX = shape.vertices[0].x, minY = shape.vertices[0].y, minZ = shape.vertices[0].z;
            float maxX = minX, maxY = minY, maxZ = minZ;
            for (const auto& v : shape.vertices) {
                if (v.x < minX) minX = v.x;
                if (v.y < minY) minY = v.y;
                if (v.z < minZ) minZ = v.z;
                if (v.x > maxX) maxX = v.x;
                if (v.y > maxY) maxY = v.y;
                if (v.z > maxZ) maxZ = v.z;
            }
            shapeBounds.append({QVector3D(minX, minY, minZ), QVector3D(maxX, maxY, maxZ)});
        }
    }

    for (int ci = 0; ci < node->children.size(); ++ci) {
        auto child = node->children[ci];

        // LOD nodes: only render highest-detail level (first child) in editor
        if (node->isLODNode && ci > 0) {
            continue;
        }

        buildMeshFromNode(child, cumulativeTransform);
    }
}

void NifViewportWidget::ensureDefaultTexture()
{
    if (defaultTexture && defaultTexture->isCreated()) return;
    QImage img(2, 2, QImage::Format_RGBA8888);
    img.fill(QColor(255, 255, 255));
    defaultTexture = new QOpenGLTexture(img);
    applyFilterToTexture(*defaultTexture);
}

void NifViewportWidget::ensureTexture(int index, const QString& path)
{
    while (shapeTextures.size() <= index) {
        shapeTextures.append(nullptr);
    }
    if (shapeTextures[index]) return;

    QString resolvedPath = path;
    {
        QFileInfo fi(path);
        if (!fi.isAbsolute() && !nifFileDir.isEmpty()) {
            QStringList candidates;
            if (path.startsWith("Textures\\", Qt::CaseInsensitive)
                || path.startsWith("Textures/", Qt::CaseInsensitive)) {
                candidates.append(nifFileDir + "/../../Data/" + path);
            }
            candidates.append(nifFileDir + "/" + QFileInfo(path).fileName());
            candidates.append(nifFileDir + "/../" + path);
            for (const auto& cand : candidates) {
                if (QFile::exists(cand)) {
                    resolvedPath = cand;
                    break;
                }
            }
        }
    }

    QImage img(resolvedPath);
    if (img.isNull()) {
        img = loadTga(resolvedPath);
    }
    if (img.isNull()) {
        img = DdsDecoder::decodeFile(resolvedPath);
    }
    if (img.isNull()) {
        LOG_WARNING(QString("Failed to load texture: %1").arg(path));
        return;
    }
    auto* tex = new QOpenGLTexture(img);
    applyFilterToTexture(*tex);
    shapeTextures[index] = tex;
    LOG_INFO(QString("Loaded texture: %1").arg(path));
}

void NifViewportWidget::applyFilterToTexture(QOpenGLTexture& tex)
{
    switch (currentFilter) {
    case TexFilter::Nearest:
        tex.setMinificationFilter(QOpenGLTexture::Nearest);
        tex.setMagnificationFilter(QOpenGLTexture::Nearest);
        break;
    case TexFilter::Mipmap:
        tex.setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        tex.setMagnificationFilter(QOpenGLTexture::Linear);
        break;
    case TexFilter::Linear:
    default:
        tex.setMinificationFilter(QOpenGLTexture::Linear);
        tex.setMagnificationFilter(QOpenGLTexture::Linear);
        break;
    }
}

void NifViewportWidget::onFilterChanged(int index)
{
    auto* filterCombo = findChild<QComboBox*>("filterCombo");
    if (!filterCombo) return;

    switch (index) {
    case 0: currentFilter = TexFilter::Linear; break;
    case 1: currentFilter = TexFilter::Nearest; break;
    case 2: currentFilter = TexFilter::Mipmap; break;
    default: return;
    }

    clearTextures();
    ensureDefaultTexture();
    for (int i = 0; i < shapeTexturePaths.size(); ++i) {
        if (!shapeTexturePaths[i].isEmpty()) {
            ensureTexture(i, shapeTexturePaths[i]);
        }
    }
    glWidget->update();
}

void NifViewportWidget::renderMesh()
{
    if (!meshBuilt || vertices.isEmpty() || indices.isEmpty()) {
        return;
    }

    if (animState && animState->state() != NifAnimationState::Stopped) {
        applyAnimationFrame();
    }

    ensureDefaultTexture();

    shaderProgram->bind();

    if (m_meshDirty) {
    QVector<float> interleaved;
    interleaved.reserve(vertices.size() * 12);
    for (int i = 0; i < vertices.size(); ++i) {
        const QVector3D& p = vertices[i];
        const QVector3D& n = (i < normals.size()) ? normals[i] : QVector3D(0.0f, 1.0f, 0.0f);
        const QVector2D& t = (i < uvs.size()) ? uvs[i] : QVector2D(0.0f, 0.0f);
        const QColor& c = shapeBaseColors.value(i, QColor(255, 255, 255, 255));
        interleaved << p.x() << p.y() << p.z();
        interleaved << n.x() << n.y() << n.z();
        interleaved << t.x() << t.y();
        interleaved << static_cast<float>(c.redF()) << static_cast<float>(c.greenF())
                    << static_cast<float>(c.blueF()) << static_cast<float>(c.alphaF());
    }

    vao.bind();
    vbo.bind();
    vbo.allocate(interleaved.constData(), interleaved.size() * sizeof(float));

    const int stride = 12 * sizeof(float);
    shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);
    shaderProgram->enableAttributeArray(0);
    shaderProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, stride);
    shaderProgram->enableAttributeArray(1);
    shaderProgram->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 2, stride);
    shaderProgram->enableAttributeArray(2);
    shaderProgram->setAttributeBuffer(3, GL_FLOAT, 8 * sizeof(float), 4, stride);
    shaderProgram->enableAttributeArray(3);

    ibo.bind();
    ibo.allocate(indices.constData(), indices.size() * sizeof(unsigned int));
    m_meshDirty = false;
    }

    vao.bind();
    vbo.bind();
    ibo.bind();

    QMatrix4x4 model;
    model.scale(0.01f);

    float rotXRad = rotationX * 3.14159265f / 180.0f;
    float rotYRad = rotationY * 3.14159265f / 180.0f;

    QMatrix4x4 rotXMatrix;
    rotXMatrix.rotate(rotXRad, 1.0f, 0.0f, 0.0f);

    QMatrix4x4 rotYMatrix;
    rotYMatrix.rotate(rotYRad, 0.0f, 1.0f, 0.0f);

    QMatrix4x4 view = rotYMatrix * rotXMatrix;
    view.scale(zoom);
    view.translate(-cameraPos);

    shaderProgram->setUniformValue("mModel", model);
    shaderProgram->setUniformValue("mView", view);
    shaderProgram->setUniformValue("mProjection", QMatrix4x4());
    shaderProgram->setUniformValue("lightDir", QVector3D(0.5f, 1.0f, 0.3f));
    shaderProgram->setUniformValue("shininess", 32.0f);
    shaderProgram->setUniformValue("tex", 0);

    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.0f);
    }

    for (int s = 0; s < shapeIndexRanges.size(); ++s) {
        QColor col = (s < shapeBaseColors.size())
                               ? shapeBaseColors[s]
                               : QColor(180, 180, 200);

        bool isSelectedShape = (s == selectedShape && highlightEnabled);
        bool isNodeShape = (selectedNode && s < shapeOwnerNode.size() && shapeOwnerNode[s] == selectedNode);
        bool isHighlighted = isSelectedShape || isNodeShape;

        QColor renderCol = isHighlighted ? QColor(255, 255, 100) : col;

        shaderProgram->setUniformValue("objectColor",
                                       QVector3D(renderCol.redF(), renderCol.greenF(), renderCol.blueF()));

        const QString texPath = (s < shapeTexturePaths.size()) ? shapeTexturePaths[s] : QString();
        QOpenGLTexture* tex = nullptr;
        if (!texPath.isEmpty()) {
            ensureTexture(s, texPath);
            tex = (s < shapeTextures.size()) ? shapeTextures[s] : nullptr;
        }

        if (tex && tex->isCreated()) {
            tex->bind(0);
            shaderProgram->setUniformValue("useTexture", true);
        } else {
            defaultTexture->bind(0);
            shaderProgram->setUniformValue("useTexture", false);
        }

        if (isHighlighted) {
            shaderProgram->setUniformValue("emissionColor", QVector3D(0.3f, 0.3f, 0.1f));
            shaderProgram->setUniformValue("specularStrength", 2.0f);
            shaderProgram->setUniformValue("specularColor", QVector3D(1.0f, 1.0f, 1.0f));
            shaderProgram->setUniformValue("shininess", 64.0f);
        } else {
            QVector3D em = shapeEmissionColors.value(s, QVector3D(0.0f, 0.0f, 0.0f));
            shaderProgram->setUniformValue("emissionColor", em);
            shaderProgram->setUniformValue("specularStrength", 1.0f);
            QVector3D spec = shapeSpecularColors.value(s, QVector3D(0.0f, 0.0f, 0.0f));
            shaderProgram->setUniformValue("specularColor", spec);
            shaderProgram->setUniformValue("shininess", shapeSpecularExponents.value(s, 32.0f));
        }

        int alphaMode = shapeAlphaModes.value(s, 0);
        switch (alphaMode) {
        case 0: // None
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case 1: // Blend
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;
        case 2: // Additive
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            break;
        case 3: // Multiply
            glBlendFunc(GL_DST_COLOR, GL_ZERO);
            break;
        }

        shaderProgram->setUniformValue("opacity", shapeOpacity.value(s, 1.0f));

        const int start = shapeIndexRanges[s].first;
        const int count = shapeIndexRanges[s].second - shapeIndexRanges[s].first;
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT,
                       reinterpret_cast<void*>(
                           static_cast<quintptr>(start * sizeof(unsigned int))));

        if (isHighlighted) {
            glEnable(GL_POLYGON_OFFSET_LINE);
            glPolygonOffset(-0.5f, -1.0f);
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(2.0f);
            shaderProgram->setUniformValue("useTexture", false);
            shaderProgram->setUniformValue("objectColor", QVector3D(1.0f, 0.5f, 0.0f));
            shaderProgram->setUniformValue("emissionColor", QVector3D(0.5f, 0.2f, 0.0f));
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT,
                           reinterpret_cast<void*>(
                               static_cast<quintptr>(start * sizeof(unsigned int))));
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_POLYGON_OFFSET_LINE);
        }

    }

    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ibo.release();
    vbo.release();
    vao.release();
    shaderProgram->release();

    QMatrix4x4 modelView = view * model;

    if (navmeshEnabled && !navmeshTriangles.isEmpty()) {
        if (m_navmeshVBO.dirty) {
            QVector<OverlayVertex> verts;
            verts.reserve(navmeshTriangles.size());
            for (const auto& v : navmeshTriangles) {
                verts.append({v, QVector3D(0.2f, 0.4f, 1.0f)});
            }
            m_navmeshVBO.build(verts, GL_TRIANGLES);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.0f, 1.0f);

        m_navmeshVBO.draw(m_overlayShader, modelView);

        if (mHighlightedTriangle >= 0 && mHighlightedTriangle * 3 + 2 < navmeshTriangles.size()) {
            if (m_highlightTriVBO.dirty) {
                int t = mHighlightedTriangle * 3;
                QVector<OverlayVertex> verts;
                verts.append({navmeshTriangles[t], QVector3D(1.0f, 0.3f, 0.0f)});
                verts.append({navmeshTriangles[t+1], QVector3D(1.0f, 0.3f, 0.0f)});
                verts.append({navmeshTriangles[t+2], QVector3D(1.0f, 0.3f, 0.0f)});
                m_highlightTriVBO.build(verts, GL_TRIANGLES);
            }
            m_highlightTriVBO.draw(m_overlayShader, modelView);
        }

        glDisable(GL_POLYGON_OFFSET_FILL);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
    }

    if (pathEnabled && pathWaypoints.size() >= 2) {
        if (m_pathVBO.dirty) {
            QVector<OverlayVertex> verts;
            verts.reserve(pathWaypoints.size());
            for (const auto& v : pathWaypoints) {
                verts.append({v, QVector3D(1.0f, 1.0f, 0.0f)});
            }
            m_pathVBO.build(verts, GL_LINE_STRIP);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_DEPTH_TEST);
        glLineWidth(3.0f);

        m_pathVBO.draw(m_overlayShader, modelView);

        glLineWidth(1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_DEPTH_TEST);
    }

}

void NifViewportWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    if (!shaderProgram) {
        setupOpenGL();
    }

    QMatrix4x4 model;
    model.scale(0.01f);
    float rotXRad = rotationX * 3.14159265f / 180.0f;
    float rotYRad = rotationY * 3.14159265f / 180.0f;
    QMatrix4x4 rotXMatrix;
    rotXMatrix.rotate(rotXRad, 1.0f, 0.0f, 0.0f);
    QMatrix4x4 rotYMatrix;
    rotYMatrix.rotate(rotYRad, 0.0f, 1.0f, 0.0f);
    QMatrix4x4 view = rotYMatrix * rotXMatrix;
    view.scale(zoom);
    view.translate(-cameraPos);
    QMatrix4x4 modelView = view * model;
    QMatrix4x4 proj;

    glWidget->makeCurrent();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderMesh();

    drawNodeAxis(modelView);

    if (boundsEnabled && !shapeBounds.isEmpty()) {
        if (m_bboxVBO.dirty) {
            QVector<OverlayVertex> verts;
            const QVector3D bboxColor(0.2f, 0.8f, 0.2f);
            for (int s = 0; s < shapeBounds.size(); ++s) {
                if (selectedShape >= 0 && highlightEnabled && s != selectedShape) continue;

                const QVector3D& bmin = shapeBounds[s].first;
                const QVector3D& bmax = shapeBounds[s].second;

                // Bottom face
                verts.append({QVector3D(bmin.x(), bmin.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmin.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmin.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmin.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmin.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmin.x(), bmin.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmin.x(), bmin.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmin.x(), bmin.y(), bmin.z()), bboxColor});
                // Top face
                verts.append({QVector3D(bmin.x(), bmax.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmax.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmax.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmax.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmax.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmin.x(), bmax.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmin.x(), bmax.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmin.x(), bmax.y(), bmin.z()), bboxColor});
                // Vertical edges
                verts.append({QVector3D(bmin.x(), bmin.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmin.x(), bmax.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmin.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmax.y(), bmin.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmin.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmax.x(), bmax.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmin.x(), bmin.y(), bmax.z()), bboxColor});
                verts.append({QVector3D(bmin.x(), bmax.y(), bmax.z()), bboxColor});
            }
            m_bboxVBO.build(verts, GL_LINES);
        }

        m_bboxVBO.draw(m_overlayShader, modelView);
    }

    if (collisionEnabled && nifParser && nifParser->getRoot()
        && !nifParser->getRoot()->collisionShapes.isEmpty()) {
        if (m_collisionVBO.dirty) {
            QVector<QVector3D> rawVerts;
            for (const auto& cs : nifParser->getRoot()->collisionShapes) {
                QVector3D cCenter(cs.center.x, cs.center.y, cs.center.z);
                switch (cs.type) {
                case Nif::CollisionShape::Sphere: {
                    const float phi = (1.0f + sqrtf(5.0f)) / 2.0f;
                    QVector3D iv[12] = {
                        QVector3D( 0,  1,  phi).normalized(),
                        QVector3D( 0,  1, -phi).normalized(),
                        QVector3D( 0, -1,  phi).normalized(),
                        QVector3D( 0, -1, -phi).normalized(),
                        QVector3D( 1,  phi,  0).normalized(),
                        QVector3D(-1,  phi,  0).normalized(),
                        QVector3D( 1, -phi,  0).normalized(),
                        QVector3D(-1, -phi,  0).normalized(),
                        QVector3D( phi,  0,  1).normalized(),
                        QVector3D( phi,  0, -1).normalized(),
                        QVector3D(-phi,  0,  1).normalized(),
                        QVector3D(-phi,  0, -1).normalized()
                    };
                    static const int edges[60] = {
                        0,2,  0,4,  0,5,  0,8,  0,10,
                        1,3,  1,4,  1,5,  1,9,  1,11,
                        2,6,  2,7,  2,8,  2,10,
                        3,6,  3,7,  3,9,  3,11,
                        4,5,  4,8,  4,9,
                        5,10, 5,11,
                        6,7,  6,8,  6,9,
                        7,10, 7,11,
                        8,9,
                        10,11
                    };
                    for (int e = 0; e < 30; ++e) {
                        int i0 = edges[e * 2];
                        int i1 = edges[e * 2 + 1];
                        rawVerts.append(cCenter + iv[i0] * cs.radius);
                        rawVerts.append(cCenter + iv[i1] * cs.radius);
                    }
                    break;
                }
                case Nif::CollisionShape::Box: {
                    QVector3D ext(cs.extents.x, cs.extents.y, cs.extents.z);
                    QVector3D corners[8] = {
                        cCenter + QVector3D(-ext.x(), -ext.y(), -ext.z()),
                        cCenter + QVector3D( ext.x(), -ext.y(), -ext.z()),
                        cCenter + QVector3D( ext.x(),  ext.y(), -ext.z()),
                        cCenter + QVector3D(-ext.x(),  ext.y(), -ext.z()),
                        cCenter + QVector3D(-ext.x(), -ext.y(),  ext.z()),
                        cCenter + QVector3D( ext.x(), -ext.y(),  ext.z()),
                        cCenter + QVector3D( ext.x(),  ext.y(),  ext.z()),
                        cCenter + QVector3D(-ext.x(),  ext.y(),  ext.z())
                    };
                    static const int boxEdges[24] = {
                        0,1, 1,2, 2,3, 3,0,
                        4,5, 5,6, 6,7, 7,4,
                        0,4, 1,5, 2,6, 3,7
                    };
                    for (int e = 0; e < 12; ++e) {
                        rawVerts.append(corners[boxEdges[e * 2]]);
                        rawVerts.append(corners[boxEdges[e * 2 + 1]]);
                    }
                    break;
                }
                case Nif::CollisionShape::Capsule: {
                    float halfH = cs.extents.y * 0.5f;
                    float r = cs.radius;
                    const int seg = 16;
                    QVector3D topCenter = cCenter + QVector3D(0, halfH, 0);
                    QVector3D botCenter = cCenter + QVector3D(0, -halfH, 0);
                    rawVerts.append(topCenter + QVector3D(r, 0, 0));
                    rawVerts.append(botCenter + QVector3D(r, 0, 0));
                    rawVerts.append(topCenter + QVector3D(-r, 0, 0));
                    rawVerts.append(botCenter + QVector3D(-r, 0, 0));
                    rawVerts.append(topCenter + QVector3D(0, 0, r));
                    rawVerts.append(botCenter + QVector3D(0, 0, r));
                    rawVerts.append(topCenter + QVector3D(0, 0, -r));
                    rawVerts.append(botCenter + QVector3D(0, 0, -r));
                    for (int i = 0; i < seg; ++i) {
                        float a0 = (float)i / seg * 2.0f * 3.14159265f;
                        float a1 = (float)(i + 1) / seg * 2.0f * 3.14159265f;
                        rawVerts.append(topCenter + QVector3D(cosf(a0) * r, 0, sinf(a0) * r));
                        rawVerts.append(topCenter + QVector3D(cosf(a1) * r, 0, sinf(a1) * r));
                    }
                    for (int i = 0; i < seg; ++i) {
                        float a0 = (float)i / seg * 2.0f * 3.14159265f;
                        float a1 = (float)(i + 1) / seg * 2.0f * 3.14159265f;
                        rawVerts.append(botCenter + QVector3D(cosf(a0) * r, 0, sinf(a0) * r));
                        rawVerts.append(botCenter + QVector3D(cosf(a1) * r, 0, sinf(a1) * r));
                    }
                    for (int i = 0; i < seg; ++i) {
                        float a0 = (float)i / seg * 2.0f * 3.14159265f;
                        float a1 = (float)(i + 1) / seg * 2.0f * 3.14159265f;
                        rawVerts.append(topCenter + QVector3D(cosf(a0) * r, 0, sinf(a0) * r));
                        rawVerts.append(botCenter + QVector3D(cosf(a0) * r, 0, sinf(a0) * r));
                        rawVerts.append(topCenter + QVector3D(cosf(a1) * r, 0, sinf(a1) * r));
                        rawVerts.append(botCenter + QVector3D(cosf(a1) * r, 0, sinf(a1) * r));
                    }
                    break;
                }
                }
            }
            QVector<OverlayVertex> verts;
            verts.reserve(rawVerts.size());
            for (const auto& v : rawVerts) {
                verts.append({v, QVector3D(1.0f, 0.6f, 0.0f)});
            }
            m_collisionVBO.build(verts, GL_LINES);
        }

        glDisable(GL_DEPTH_TEST);
        glLineWidth(1.0f);

        m_collisionVBO.draw(m_overlayShader, modelView);

        glLineWidth(1.0f);
        glEnable(GL_DEPTH_TEST);
    }

    if (gridEnabled || axisEnabled) {
        glDisable(GL_DEPTH_TEST);

        if (gridEnabled) {
            if (m_gridVBO.dirty) {
                QVector<OverlayVertex> verts;
                for (int i = -5; i <= 5; i++) {
                    verts.append({QVector3D(-5.0f, 0.0f, static_cast<float>(i)), QVector3D(0.3f, 0.3f, 0.3f)});
                    verts.append({QVector3D(5.0f, 0.0f, static_cast<float>(i)), QVector3D(0.3f, 0.3f, 0.3f)});
                    verts.append({QVector3D(static_cast<float>(i), 0.0f, -5.0f), QVector3D(0.3f, 0.3f, 0.3f)});
                    verts.append({QVector3D(static_cast<float>(i), 0.0f, 5.0f), QVector3D(0.3f, 0.3f, 0.3f)});
                }
                m_gridVBO.build(verts, GL_LINES);
            }

            m_gridVBO.draw(m_overlayShader, modelView);
        }

        if (axisEnabled) {
            if (m_axisVBO_R.dirty) {
                QVector<OverlayVertex> v;
                v.append({QVector3D(0, 0, 0), QVector3D(1.0f, 0.0f, 0.0f)});
                v.append({QVector3D(5, 0, 0), QVector3D(1.0f, 0.0f, 0.0f)});
                m_axisVBO_R.build(v, GL_LINES);
            }
            if (m_axisVBO_G.dirty) {
                QVector<OverlayVertex> v;
                v.append({QVector3D(0, 0, 0), QVector3D(0.0f, 1.0f, 0.0f)});
                v.append({QVector3D(0, 5, 0), QVector3D(0.0f, 1.0f, 0.0f)});
                m_axisVBO_G.build(v, GL_LINES);
            }
            if (m_axisVBO_B.dirty) {
                QVector<OverlayVertex> v;
                v.append({QVector3D(0, 0, 0), QVector3D(0.0f, 0.0f, 1.0f)});
                v.append({QVector3D(0, 0, 5), QVector3D(0.0f, 0.0f, 1.0f)});
                m_axisVBO_B.build(v, GL_LINES);
            }

            glLineWidth(2.0f);

            m_axisVBO_R.draw(m_overlayShader, modelView);
            m_axisVBO_G.draw(m_overlayShader, modelView);
            m_axisVBO_B.draw(m_overlayShader, modelView);

            glLineWidth(1.0f);
        }

        glEnable(GL_DEPTH_TEST);
    }

    if (cellGridEnabled) {
        if (m_cellGridVBO.dirty) {
            QVector<OverlayVertex> verts;
            const float cellHalf = 2048.0f;
            const float subDiv = 512.0f;
            const QVector3D gridColor(1.0f, 1.0f, 1.0f);

            QVector3D corners[4] = {
                QVector3D(cellOrigin.x() - cellHalf, 0.0f, cellOrigin.z() - cellHalf),
                QVector3D(cellOrigin.x() + cellHalf, 0.0f, cellOrigin.z() - cellHalf),
                QVector3D(cellOrigin.x() + cellHalf, 0.0f, cellOrigin.z() + cellHalf),
                QVector3D(cellOrigin.x() - cellHalf, 0.0f, cellOrigin.z() + cellHalf)
            };

            for (int i = 0; i < 4; ++i) {
                int j = (i + 1) % 4;
                verts.append({corners[i], gridColor});
                verts.append({corners[j], gridColor});
            }

            for (float d = subDiv; d < cellHalf; d += subDiv) {
                verts.append({QVector3D(cellOrigin.x() - cellHalf, 0.0f, cellOrigin.z() - d), gridColor});
                verts.append({QVector3D(cellOrigin.x() + cellHalf, 0.0f, cellOrigin.z() - d), gridColor});

                verts.append({QVector3D(cellOrigin.x() - cellHalf, 0.0f, cellOrigin.z() + d), gridColor});
                verts.append({QVector3D(cellOrigin.x() + cellHalf, 0.0f, cellOrigin.z() + d), gridColor});

                verts.append({QVector3D(cellOrigin.x() - d, 0.0f, cellOrigin.z() - cellHalf), gridColor});
                verts.append({QVector3D(cellOrigin.x() - d, 0.0f, cellOrigin.z() + cellHalf), gridColor});

                verts.append({QVector3D(cellOrigin.x() + d, 0.0f, cellOrigin.z() - cellHalf), gridColor});
                verts.append({QVector3D(cellOrigin.x() + d, 0.0f, cellOrigin.z() + cellHalf), gridColor});
            }

            m_cellGridVBO.build(verts, GL_LINES);
        }

        glDisable(GL_DEPTH_TEST);

        glLineWidth(2.0f);
        m_cellGridVBO.draw(m_overlayShader, modelView);
        glLineWidth(1.0f);

        glEnable(GL_DEPTH_TEST);
    }

    if (!cellReferences.isEmpty()) {
        if (m_cellRefVBO.dirty) {
            QVector<OverlayVertex> verts;
            const QVector3D refColor(0.0f, 1.0f, 0.0f);
            for (int i = 0; i < cellReferences.size(); ++i) {
                const ViewportCellRef& ref = cellReferences[i];
                QVector3D pos = ref.position;
                float armLen = 32.0f;
                QVector3D color = refColor;
                if (i == mSelectedRefIndex) {
                    color = QVector3D(1.0f, 0.85f, 0.1f);
                    armLen = 48.0f;
                } else if (i == mHoverRefIndex) {
                    color = QVector3D(1.0f, 1.0f, 1.0f);
                }
                if (ref.enabled) {
                    QVector3D h1 = pos - QVector3D(armLen, 0, 0);
                    QVector3D h2 = pos + QVector3D(armLen, 0, 0);
                    QVector3D v1 = pos - QVector3D(0, 0, armLen);
                    QVector3D v2 = pos + QVector3D(0, 0, armLen);
                    verts.append({h1, color});
                    verts.append({h2, color});
                    verts.append({v1, color});
                    verts.append({v2, color});
                } else {
                    QVector3D d1a = pos + QVector3D(-armLen, 0, -armLen);
                    QVector3D d1b = pos + QVector3D(armLen, 0, armLen);
                    QVector3D d2a = pos + QVector3D(armLen, 0, -armLen);
                    QVector3D d2b = pos + QVector3D(-armLen, 0, armLen);
                    verts.append({d1a, color});
                    verts.append({d1b, color});
                    verts.append({d2a, color});
                    verts.append({d2b, color});
                }
            }
            m_cellRefVBO.build(verts, GL_LINES);
        }

        glDisable(GL_DEPTH_TEST);

        m_cellRefVBO.draw(m_overlayShader, modelView);

        glEnable(GL_DEPTH_TEST);
    }

    // Standalone preview primitive (cube/cylinder/plane/sphere)
    if (m_primitiveVBO.vertexCount > 0) {
        m_primitiveVBO.draw(m_overlayShader, modelView);
    }

    // Transform gizmo for the selected reference
    if (mSelectedRefIndex >= 0 && mSelectedRefIndex < cellReferences.size()
        && mEditMode != EditMode::Select)
    {
        const ViewportCellRef& ref = cellReferences[mSelectedRefIndex];
        const gizmo::ViewTransform t{ view, model, proj, glWidget->size() };
        const float len = gizmo::worldSizeForPixels(t, 90.0f);
        glDisable(GL_DEPTH_TEST);
        glLineWidth(2.5f);
        if (mEditMode == EditMode::Move)      { buildTranslateGizmo(len, ref.position, mHoverAxis); m_translateGizmoVBO.draw(m_overlayShader, modelView); }
        else if (mEditMode == EditMode::Rotate){ buildRotateGizmo(len, ref.position, mHoverAxis);   m_rotateGizmoVBO.draw(m_overlayShader, modelView); }
        else if (mEditMode == EditMode::Scale){ buildScaleGizmo(len, ref.position, mHoverAxis);     m_scaleGizmoVBO.draw(m_overlayShader, modelView); }
        glLineWidth(1.0f);
        glEnable(GL_DEPTH_TEST);
    }

    if (m_particleSystem && m_particleRenderer) {
        m_particleRenderer->render(m_particleSystem, modelView, proj);
    }

    glWidget->doneCurrent();

    if (axisEnabled) {
        QRect vp(QPoint(0, 0), glWidget->size());
        int h = glWidget->height();

        QVector3D xScreen = QVector3D(5, 0, 0).project(modelView, proj, vp);
        QVector3D yScreen = QVector3D(0, 5, 0).project(modelView, proj, vp);
        QVector3D zScreen = QVector3D(0, 0, 5).project(modelView, proj, vp);

        QPainter painter(glWidget);
        painter.setFont(QFont("Arial", 10));

        auto label = [&](const QVector3D& s, const QString& text, const QColor& color) {
            if (s.z() >= 0.0f && s.z() <= 1.0f) {
                painter.setPen(color);
                painter.drawText(QPointF(s.x() + 4, h - s.y() - 4), text);
            }
        };

        label(xScreen, "X", QColor(255, 50, 50));
        label(yScreen, "Y", QColor(50, 255, 50));
        label(zScreen, "Z", QColor(50, 50, 255));
    }
}

void NifViewportWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    glWidget->resize(event->size());
    updateCamera();
}

void NifViewportWidget::setEditMode(EditMode mode)
{
    mEditMode = mode;
    if (mActionSelect) mActionSelect->setChecked(mode == EditMode::Select);
    if (mActionMove) mActionMove->setChecked(mode == EditMode::Move);
    if (mActionRotate) mActionRotate->setChecked(mode == EditMode::Rotate);
    if (mActionScale) mActionScale->setChecked(mode == EditMode::Scale);
    mHoverAxis = -1;
    glWidget->update();
}

gizmo::ViewTransform NifViewportWidget::currentViewTransform() const
{
    QMatrix4x4 model;
    model.scale(0.01f);
    float rotXRad = rotationX * 3.14159265f / 180.0f;
    float rotYRad = rotationY * 3.14159265f / 180.0f;
    QMatrix4x4 rotXMatrix;
    rotXMatrix.rotate(rotXRad, 1.0f, 0.0f, 0.0f);
    QMatrix4x4 rotYMatrix;
    rotYMatrix.rotate(rotYRad, 0.0f, 1.0f, 0.0f);
    QMatrix4x4 view = rotYMatrix * rotXMatrix;
    view.scale(zoom);
    view.translate(-cameraPos);
    QMatrix4x4 proj;
    return { view, model, proj, glWidget ? glWidget->size() : QSize() };
}

void NifViewportWidget::buildTranslateGizmo(float len, const QVector3D& origin, int highlightAxis)
{
    static const QVector3D axisDirs[3] = {
        QVector3D(1.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)
    };
    static const QVector3D perpU[3] = {
        QVector3D(0.0f, 1.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)
    };
    static const QVector3D perpV[3] = {
        QVector3D(0.0f, 0.0f, 1.0f), QVector3D(0.0f, 0.0f, 1.0f), QVector3D(0.0f, 1.0f, 0.0f)
    };
    static const QVector3D axisColors[3] = {
        QVector3D(0.9f, 0.2f, 0.2f), QVector3D(0.2f, 0.9f, 0.2f), QVector3D(0.3f, 0.3f, 0.9f)
    };

    QVector<OverlayVertex> verts;
    verts.reserve(13 * 3);
    const float halfWidth = len * 0.014f;
    for (int i = 0; i < 3; ++i) {
        const bool hl = (i == highlightAxis);
        const float effLen = hl ? len * 1.15f : len;
        const QVector3D color = hl ? QVector3D(1.0f, 1.0f, 1.0f) : axisColors[i];
        const QVector3D tip = origin + axisDirs[i] * effLen;
        const QVector3D side = perpU[i] * halfWidth;
        verts.append({origin - side, color});
        verts.append({origin + side, color});
        verts.append({tip + side, color});
        verts.append({origin - side, color});
        verts.append({tip + side, color});
        verts.append({tip - side, color});
        const float baseR = len * 0.12f;
        const QVector3D baseCenter = tip - axisDirs[i] * baseR;
        QVector3D ring[3];
        for (int k = 0; k < 3; ++k) {
            const float a = k * 2.0f * 3.14159265f / 3.0f;
            ring[k] = baseCenter + (perpU[i] * std::cos(a) + perpV[i] * std::sin(a)) * baseR;
        }
        verts.append({tip, color});
        verts.append({ring[0], color});
        verts.append({ring[1], color});
        verts.append({tip, color});
        verts.append({ring[1], color});
        verts.append({ring[2], color});
        verts.append({tip, color});
        verts.append({ring[2], color});
        verts.append({ring[0], color});
    }
    m_translateGizmoVBO.build(verts, GL_TRIANGLES);
}

void NifViewportWidget::buildRotateGizmo(float len, const QVector3D& origin, int highlightAxis)
{
    static const QVector3D perpU[3] = {
        QVector3D(0.0f, 1.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)
    };
    static const QVector3D perpV[3] = {
        QVector3D(0.0f, 0.0f, 1.0f), QVector3D(0.0f, 0.0f, 1.0f), QVector3D(0.0f, 1.0f, 0.0f)
    };
    static const QVector3D axisColors[3] = {
        QVector3D(0.9f, 0.2f, 0.2f), QVector3D(0.2f, 0.9f, 0.2f), QVector3D(0.3f, 0.3f, 0.9f)
    };

    const int segments = 48;
    QVector<OverlayVertex> verts;
    verts.reserve(3 * segments * 2);
    for (int i = 0; i < 3; ++i) {
        const QVector3D color = (i == highlightAxis) ? QVector3D(1.0f, 1.0f, 1.0f) : axisColors[i];
        for (int s = 0; s < segments; ++s) {
            const float a0 = static_cast<float>(s) / segments * 2.0f * 3.14159265f;
            const float a1 = static_cast<float>(s + 1) / segments * 2.0f * 3.14159265f;
            const QVector3D p0 = origin + (perpU[i] * std::cos(a0) + perpV[i] * std::sin(a0)) * len;
            const QVector3D p1 = origin + (perpU[i] * std::cos(a1) + perpV[i] * std::sin(a1)) * len;
            verts.append({p0, color});
            verts.append({p1, color});
        }
    }
    m_rotateGizmoVBO.build(verts, GL_LINES);
}

void NifViewportWidget::buildScaleGizmo(float len, const QVector3D& origin, int highlightAxis)
{
    static const QVector3D axisDirs[3] = {
        QVector3D(1.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)
    };
    static const QVector3D perpU[3] = {
        QVector3D(0.0f, 1.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f), QVector3D(1.0f, 0.0f, 0.0f)
    };
    static const QVector3D axisColors[3] = {
        QVector3D(0.9f, 0.2f, 0.2f), QVector3D(0.2f, 0.9f, 0.2f), QVector3D(0.3f, 0.3f, 0.9f)
    };
    static const int cubeFaces[36] = {
        0,1,2, 0,2,3,
        5,4,7, 5,7,6,
        4,0,3, 4,3,7,
        1,5,6, 1,6,2,
        3,2,6, 3,6,7,
        4,5,1, 4,1,0
    };

    QVector<OverlayVertex> verts;
    verts.reserve(12 * 3);
    const float halfWidth = len * 0.014f;
    for (int i = 0; i < 3; ++i) {
        const bool hl = (i == highlightAxis);
        const QVector3D color = hl ? QVector3D(1.0f, 1.0f, 1.0f) : axisColors[i];
        const QVector3D tip = origin + axisDirs[i] * len;
        const QVector3D side = perpU[i] * halfWidth;
        verts.append({origin - side, color});
        verts.append({origin + side, color});
        verts.append({tip + side, color});
        verts.append({origin - side, color});
        verts.append({tip + side, color});
        verts.append({tip - side, color});
        const float h = len * 0.10f;
        const QVector3D corners[8] = {
            tip + QVector3D(-h, -h, -h),
            tip + QVector3D( h, -h, -h),
            tip + QVector3D( h,  h, -h),
            tip + QVector3D(-h,  h, -h),
            tip + QVector3D(-h, -h,  h),
            tip + QVector3D( h, -h,  h),
            tip + QVector3D( h,  h,  h),
            tip + QVector3D(-h,  h,  h)
        };
        for (int f = 0; f < 36; ++f) {
            verts.append({corners[cubeFaces[f]], color});
        }
    }
    m_scaleGizmoVBO.build(verts, GL_TRIANGLES);
}

int NifViewportWidget::pickGizmoAxis(const QPoint& pos, const gizmo::ViewTransform& t)
{
    if (mSelectedRefIndex < 0 || mSelectedRefIndex >= cellReferences.size()) return -1;
    const QVector3D origin = cellReferences[mSelectedRefIndex].position;
    const float len = gizmo::worldSizeForPixels(t, 90.0f);
    const QVector3D axisDirs[3] = {
        QVector3D(1.0f, 0.0f, 0.0f), QVector3D(0.0f, 1.0f, 0.0f), QVector3D(0.0f, 0.0f, 1.0f)
    };
    for (int i = 0; i < 3; ++i) {
        const float d = gizmo::axisPickDistance(t, QPointF(pos), origin, axisDirs[i], len);
        if (d >= 0.0f && d <= 14.0f) return i;
    }
    return -1;
}

int NifViewportWidget::pickRefMarker(const QPoint& pos, const gizmo::ViewTransform& t)
{
    int best = -1;
    float bestDist = 11.0f;
    const QPointF p(pos);
    for (int i = 0; i < cellReferences.size(); ++i) {
        const QVector3D s = gizmo::worldToScreen(t, cellReferences[i].position);
        const float dx = s.x() - p.x();
        const float dy = s.y() - p.y();
        const float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

void NifViewportWidget::applyGizmoDrag(const QPoint& currentPos)
{
    if (mSelectedRefIndex < 0 || mSelectedRefIndex >= cellReferences.size()) return;
    ViewportCellRef& ref = cellReferences[mSelectedRefIndex];
    const gizmo::ViewTransform t = currentViewTransform();
    const QVector3D axisDir = mGizmoAxis == 0 ? QVector3D(1,0,0) : mGizmoAxis == 1 ? QVector3D(0,1,0) : QVector3D(0,0,1);
    const QPointF delta = QPointF(currentPos) - QPointF(mGizmoStartScreen);
    switch (mEditMode)
    {
    case EditMode::Move:
    {
        const float d = gizmo::dragDeltaAlongAxis(t, mGizmoStartPos, axisDir, delta);
        float v = mGizmoStartPos[mGizmoAxis] + d;
        if (mSnapToGrid) v = gizmo::snapToStep(v, mSnapGridSize);
        QVector3D newPos = mGizmoStartPos;
        newPos[mGizmoAxis] = v;
        ref.position = newPos;
        break;
    }
    case EditMode::Rotate:
    {
        const float angle = gizmo::arcballRotation(t, mGizmoStartPos, QPointF(mGizmoStartScreen), QPointF(currentPos));
        float snapped = mSnapToAngle ? gizmo::snapDegrees(angle, mSnapAngleIncrement) : angle;
        ref.rotX = mGizmoStartRotRad[0];
        ref.rotY = mGizmoStartRotRad[1];
        ref.rotZ = mGizmoStartRotRad[2];
        float& target = mGizmoAxis == 0 ? ref.rotX : mGizmoAxis == 1 ? ref.rotY : ref.rotZ;
        target = mGizmoStartRotRad[mGizmoAxis] + snapped * 3.14159265f / 180.0f;
        break;
    }
    case EditMode::Scale:
    {
        const float d = gizmo::dragDeltaAlongAxis(t, mGizmoStartPos, axisDir, delta);
        const float worldLen = gizmo::worldSizeForPixels(t, 90.0f);
        float s = mGizmoStartScale * (1.0f + d / worldLen);
        ref.scale = qBound(0.01f, s, 100.0f);
        break;
    }
    case EditMode::Select: break;
    }
    emit refTransformPreview(cellReferences[mSelectedRefIndex].dataIndex,
                             ref.position, QVector3D(ref.rotX, ref.rotY, ref.rotZ), ref.scale);
    m_cellRefVBO.dirty = true;
    glWidget->update();
}

void NifViewportWidget::commitGizmoDrag()
{
    mGizmoDragging = false;
    if (mSelectedRefIndex >= 0 && mSelectedRefIndex < cellReferences.size()) {
        const ViewportCellRef& ref = cellReferences[mSelectedRefIndex];
        emit refTransformCommitted(ref.dataIndex, ref.position,
                                   QVector3D(ref.rotX, ref.rotY, ref.rotZ), ref.scale);
    }
    mGizmoAxis = -1;
    mHoverAxis = -1;
    glWidget->update();
}

void NifViewportWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        const gizmo::ViewTransform t = currentViewTransform();
        if (mSelectedRefIndex >= 0 && mSelectedRefIndex < cellReferences.size()
            && mEditMode != EditMode::Select)
        {
            const int axis = pickGizmoAxis(event->pos(), t);
            if (axis >= 0) {
                mGizmoDragging = true;
                mGizmoAxis = axis;
                mHoverAxis = axis;
                mGizmoStartScreen = event->pos();
                const ViewportCellRef& ref = cellReferences[mSelectedRefIndex];
                mGizmoStartPos = ref.position;
                mGizmoStartRotRad[0] = ref.rotX; mGizmoStartRotRad[1] = ref.rotY; mGizmoStartRotRad[2] = ref.rotZ;
                mGizmoStartScale = ref.scale;
                setCursor(Qt::SizeAllCursor);
                return; // don't orbit
            }
        }
        const int refIdx = pickRefMarker(event->pos(), t);
        if (refIdx >= 0) {
            setSelectedRefIndex(refIdx);
            emit refSelected(cellReferences[refIdx].dataIndex);
            return; // don't orbit
        }
        dragging = true;
        lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::RightButton && highlightEnabled) {
        if (shapeIndexRanges.isEmpty()) return;
        selectedShape++;
        if (selectedShape >= shapeIndexRanges.size()) {
            selectedShape = -1;
        }
        m_bboxVBO.dirty = true;
        glWidget->update();
    }
}

void NifViewportWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (mGizmoDragging && mGizmoAxis >= 0) {
        applyGizmoDrag(event->pos());
        return;
    }

    if (dragging) {
        QPoint delta = event->pos() - lastMousePos;
        rotationX += delta.y() * 0.5f;
        rotationY += delta.x() * 0.5f;
        lastMousePos = event->pos();
        updateCamera();
        return;
    }

    const gizmo::ViewTransform t = currentViewTransform();
    if (mSelectedRefIndex >= 0 && mSelectedRefIndex < cellReferences.size()
        && mEditMode != EditMode::Select)
    {
        const int axis = pickGizmoAxis(event->pos(), t);
        if (axis != mHoverAxis) {
            mHoverAxis = axis;
            glWidget->update();
        }
        if (axis >= 0) {
            setCursor(Qt::SizeAllCursor);
        } else {
            unsetCursor();
        }
    } else {
        const int refIdx = pickRefMarker(event->pos(), t);
        if (refIdx != mHoverRefIndex) {
            mHoverRefIndex = refIdx;
            m_cellRefVBO.dirty = true;
            glWidget->update();
        }
        if (refIdx >= 0) {
            emit refHovered(cellReferences[refIdx].dataIndex);
        }
    }
}

void NifViewportWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (mGizmoDragging) {
            commitGizmoDrag();
        }
        dragging = false;
        unsetCursor();
    }
    QWidget::mouseReleaseEvent(event);
}

void NifViewportWidget::wheelEvent(QWheelEvent* event)
{
    zoom *= (event->angleDelta().y() > 0) ? 1.1f : 0.9f;
    zoom = qBound(0.1f, zoom, 10.0f);
    updateCamera();
}

void NifViewportWidget::keyPressEvent(QKeyEvent* event)
{
    float moveSpeed = 0.5f / zoom;
    float rotXRad = rotationX * 3.14159265f / 180.0f;
    float rotYRad = rotationY * 3.14159265f / 180.0f;

    QVector3D forward(-sinf(rotYRad) * cosf(rotXRad),
                      sinf(rotXRad),
                      -cosf(rotYRad) * cosf(rotXRad));
    QVector3D right(cosf(rotYRad), 0.0f, -sinf(rotYRad));

    switch (event->key()) {
    case Qt::Key_Q:
        if (mSelectedRefIndex >= 0) {
            setEditMode(EditMode::Select);
            break;
        }
        QWidget::keyPressEvent(event);
        return;
    case Qt::Key_W:
        if (mSelectedRefIndex >= 0) {
            setEditMode(EditMode::Move);
            break;
        }
        cameraPos += forward * moveSpeed;
        break;
    case Qt::Key_E:
        if (mSelectedRefIndex >= 0) {
            setEditMode(EditMode::Rotate);
            break;
        }
        QWidget::keyPressEvent(event);
        return;
    case Qt::Key_R:
        if (mSelectedRefIndex >= 0) {
            setEditMode(EditMode::Scale);
            break;
        }
        QWidget::keyPressEvent(event);
        return;
    case Qt::Key_S:
        cameraPos -= forward * moveSpeed;
        break;
    case Qt::Key_A:
        cameraPos -= right * moveSpeed;
        break;
    case Qt::Key_D:
        cameraPos += right * moveSpeed;
        break;
    case Qt::Key_Home:
        cameraPos = QVector3D(0.0f, 0.0f, 0.0f);
        rotationX = 0.0f;
        rotationY = 0.0f;
        zoom = 1.0f;
        break;
    case Qt::Key_T: // Top-down view (CKShortcuts::TopDown)
        rotationX = 90.0f;
        rotationY = 0.0f;
        break;
    case Qt::Key_Y: // Cycle preset camera angles (CKShortcuts::CycleView)
        if (qFuzzyCompare(rotationX, 0.0f) && qFuzzyCompare(rotationY, 0.0f)) {
            rotationX = 45.0f;
            rotationY = 45.0f;
        } else if (rotationX == 45.0f && rotationY == 45.0f) {
            rotationX = 0.0f;
            rotationY = 90.0f;
        } else if (qFuzzyCompare(rotationX, 0.0f) && rotationY == 90.0f) {
            rotationX = 90.0f;
            rotationY = 0.0f;
        } else {
            rotationX = 0.0f;
            rotationY = 0.0f;
        }
        break;
    case Qt::Key_M: // Toggle markers (CKShortcuts::ToggleMarkers)
        hierarchyVisible = !hierarchyVisible;
        if (mainSplitter->count() > 1) {
            mainSplitter->widget(1)->setVisible(hierarchyVisible);
        }
        break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    updateCamera();
}

bool NifViewportWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == glWidget && event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_W || ke->key() == Qt::Key_A ||
            ke->key() == Qt::Key_S || ke->key() == Qt::Key_D ||
            ke->key() == Qt::Key_Home || ke->key() == Qt::Key_T ||
            ke->key() == Qt::Key_Y || ke->key() == Qt::Key_M ||
            ke->key() == Qt::Key_Q || ke->key() == Qt::Key_E ||
            ke->key() == Qt::Key_R) {
            keyPressEvent(ke);
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void NifViewportWidget::updateCamera()
{
    glWidget->update();
}

void NifViewportWidget::toggleHierarchy()
{
    hierarchyVisible = !hierarchyVisible;
    if (mainSplitter->count() > 1) {
        mainSplitter->widget(1)->setVisible(hierarchyVisible);
    }
    glWidget->update();
}

void NifViewportWidget::buildHierarchyTree()
{
    hierarchyTree->clear();
    nodeCumulativeTransforms.clear();
    selectedNode = nullptr;
    nodeInfoLabel->setText(tr("No node selected"));

    if (!nifParser || !nifParser->getRoot()) return;

    QMatrix4x4 identity;
    populateTreeItem(nullptr, nifParser->getRoot(), identity);
    hierarchyTree->expandAll();
}

void NifViewportWidget::populateTreeItem(QTreeWidgetItem* parentItem, Nif::Node* node, const QMatrix4x4& parentTransform)
{
    if (!node) return;

    QMatrix4x4 localTransform;
    localTransform.translate(node->position.x, node->position.y, node->position.z);
    float rx = node->rotation.x * 180.0f / 3.14159265f;
    float ry = node->rotation.y * 180.0f / 3.14159265f;
    float rz = node->rotation.z * 180.0f / 3.14159265f;
    localTransform.rotate(rz, 0.0f, 0.0f, 1.0f);
    localTransform.rotate(ry, 0.0f, 1.0f, 0.0f);
    localTransform.rotate(rx, 1.0f, 0.0f, 0.0f);

    QMatrix4x4 cumulativeTransform = parentTransform * localTransform;
    nodeCumulativeTransforms[node] = cumulativeTransform;

    auto* item = new QTreeWidgetItem();
    item->setText(0, node->name.isEmpty() ? tr("<unnamed>") : node->name);
    item->setData(0, Qt::UserRole, QVariant::fromValue(reinterpret_cast<quintptr>(node)));

    for (const auto& shape : node->shapes) {
        auto* shapeItem = new QTreeWidgetItem();
        shapeItem->setText(0, shape.name.isEmpty() ? tr("<shape>") : shape.name);
        shapeItem->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<quintptr>(0)));
        item->addChild(shapeItem);
    }

    for (auto* child : node->children) {
        populateTreeItem(item, child, cumulativeTransform);
    }

    if (parentItem) {
        parentItem->addChild(item);
    } else {
        hierarchyTree->addTopLevelItem(item);
    }
}

void NifViewportWidget::updateNodeMetadata()
{
    if (!selectedNode) {
        nodeInfoLabel->setText(tr("No node selected"));
        return;
    }

    int shapeCount = selectedNode->shapes.size();
    int childCount = selectedNode->children.size();
    Nif::Vector3 pos = selectedNode->position;

    QString info = QString("Name: %1\nShapes: %2\nPosition: (%3, %4, %5)\nChildren: %6")
        .arg(selectedNode->name.isEmpty() ? tr("<unnamed>") : selectedNode->name)
        .arg(shapeCount)
        .arg(pos.x, 0, 'f', 3)
        .arg(pos.y, 0, 'f', 3)
        .arg(pos.z, 0, 'f', 3)
        .arg(childCount);

    nodeInfoLabel->setText(info);
}

void NifViewportWidget::drawNodeAxis(const QMatrix4x4& modelView)
{
    if (!selectedNode || !nodeCumulativeTransforms.contains(selectedNode)) return;

    const QMatrix4x4& nodeWorld = nodeCumulativeTransforms[selectedNode];
    const float axisLen = 3.0f;

    QVector3D origin = nodeWorld * QVector3D(0, 0, 0);
    QVector3D xEnd = nodeWorld * QVector3D(axisLen, 0, 0);
    QVector3D yEnd = nodeWorld * QVector3D(0, axisLen, 0);
    QVector3D zEnd = nodeWorld * QVector3D(0, 0, axisLen);

    QVector<OverlayVertex> rVerts;
    rVerts.append({origin, QVector3D(1.0f, 0.0f, 0.0f)});
    rVerts.append({xEnd, QVector3D(1.0f, 0.0f, 0.0f)});
    m_nodeAxisVBO_R.build(rVerts, GL_LINES);

    QVector<OverlayVertex> gVerts;
    gVerts.append({origin, QVector3D(0.0f, 1.0f, 0.0f)});
    gVerts.append({yEnd, QVector3D(0.0f, 1.0f, 0.0f)});
    m_nodeAxisVBO_G.build(gVerts, GL_LINES);

    QVector<OverlayVertex> bVerts;
    bVerts.append({origin, QVector3D(0.0f, 0.0f, 1.0f)});
    bVerts.append({zEnd, QVector3D(0.0f, 0.0f, 1.0f)});
    m_nodeAxisVBO_B.build(bVerts, GL_LINES);

    glLineWidth(3.0f);
    glDisable(GL_DEPTH_TEST);

    m_nodeAxisVBO_R.draw(m_overlayShader, modelView);
    m_nodeAxisVBO_G.draw(m_overlayShader, modelView);
    m_nodeAxisVBO_B.draw(m_overlayShader, modelView);

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}

void NifViewportWidget::setupAnimToolbar()
{
    animToolbar = new QWidget(this);
    auto* animLayout = new QHBoxLayout(animToolbar);
    animLayout->setContentsMargins(4, 2, 4, 2);
    animLayout->setSpacing(4);

    animPlayBtn = new QPushButton(tr("\u25B6"), animToolbar);
    animPlayBtn->setToolTip(tr("Play / Pause"));
    animPlayBtn->setCheckable(true);
    animPlayBtn->setFixedWidth(30);
    animLayout->addWidget(animPlayBtn);

    animStopBtn = new QPushButton(tr("\u23F9"), animToolbar);
    animStopBtn->setToolTip(tr("Stop"));
    animStopBtn->setFixedWidth(30);
    animLayout->addWidget(animStopBtn);

    animLoopBtn = new QPushButton(tr("\U0001F501"), animToolbar);
    animLoopBtn->setToolTip(tr("Toggle Loop"));
    animLoopBtn->setCheckable(true);
    animLoopBtn->setChecked(true);
    animLoopBtn->setFixedWidth(30);
    animLayout->addWidget(animLoopBtn);

    animClipLabel = new QLabel(tr("Clip:"), animToolbar);
    animLayout->addWidget(animClipLabel);

    animClipCombo = new QComboBox(animToolbar);
    animClipCombo->setMinimumWidth(100);
    animLayout->addWidget(animClipCombo);

    animTimeline = new QSlider(Qt::Horizontal, animToolbar);
    animTimeline->setRange(0, 1000);
    animTimeline->setValue(0);
    animLayout->addWidget(animTimeline, 1);

    animTimeLabel = new QLabel(tr("0:00 / 0:00"), animToolbar);
    animTimeLabel->setMinimumWidth(80);
    animLayout->addWidget(animTimeLabel);

    animSpeedCombo = new QComboBox(animToolbar);
    animSpeedCombo->addItems({"0.25x", "0.5x", "1x", "2x", "4x"});
    animSpeedCombo->setCurrentIndex(2);
    animSpeedCombo->setToolTip(tr("Playback Speed"));
    animLayout->addWidget(animSpeedCombo);

    animBlendToolbar = new QWidget(this);
    auto* blendLayout = new QHBoxLayout(animBlendToolbar);
    blendLayout->setContentsMargins(4, 2, 4, 2);
    blendLayout->setSpacing(4);

    auto* blendClipLbl = new QLabel(tr("Blend:"), animBlendToolbar);
    blendLayout->addWidget(blendClipLbl);

    animBlendClipCombo = new QComboBox(animBlendToolbar);
    animBlendClipCombo->setMinimumWidth(100);
    animBlendClipCombo->addItem(tr("(none)"));
    blendLayout->addWidget(animBlendClipCombo);

    auto* weightLbl = new QLabel(tr("Weight:"), animBlendToolbar);
    blendLayout->addWidget(weightLbl);

    animBlendWeightSlider = new QSlider(Qt::Horizontal, animBlendToolbar);
    animBlendWeightSlider->setRange(0, 100);
    animBlendWeightSlider->setValue(0);
    animBlendWeightSlider->setToolTip(tr("Blend Weight"));
    blendLayout->addWidget(animBlendWeightSlider, 1);

    animBlendWeightLabel = new QLabel(tr("0%"), animBlendToolbar);
    animBlendWeightLabel->setMinimumWidth(30);
    blendLayout->addWidget(animBlendWeightLabel);

    animToolbar->setVisible(false);
    animBlendToolbar->setVisible(false);

    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        int toolbarIdx = mainLayout->indexOf(mainLayout->itemAt(0)->widget());
        mainLayout->insertWidget(toolbarIdx + 1, animToolbar);
        mainLayout->insertWidget(toolbarIdx + 2, animBlendToolbar);
    }

    connect(animPlayBtn, &QPushButton::clicked, this, [this](bool checked) {
        if (!animState) return;
        if (checked) {
            animState->play();
            animPlayBtn->setText("\u23F8");
        } else {
            animState->pause();
            animPlayBtn->setText("\u25B6");
        }
    });

    connect(animStopBtn, &QPushButton::clicked, this, [this]() {
        if (!animState) return;
        animState->stop();
        animPlayBtn->setChecked(false);
        animPlayBtn->setText("\u25B6");
        animTimeline->setValue(0);
        animTimeLabel->setText(formatAnimTime(0) + " / " + formatAnimTime(animState->duration()));
        glWidget->update();
    });

    connect(animLoopBtn, &QPushButton::clicked, this, [this](bool checked) {
        if (!animState) return;
        animState->setLooping(checked);
    });

    connect(animSpeedCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!animState) return;
        float speeds[] = {0.25f, 0.5f, 1.0f, 2.0f, 4.0f};
        animState->setSpeed(speeds[qBound(0, index, 4)]);
    });

    connect(animClipCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!animState || index < 0) return;
        animState->setClip(animClipCombo->currentText());
        float dur = animState->duration();
        animTimeLabel->setText(formatAnimTime(0) + " / " + formatAnimTime(dur));
        animTimeline->setValue(0);
        glWidget->update();
    });

    connect(animTimeline, &QSlider::sliderPressed, this, [this]() {
        animDraggingSlider = true;
    });

    connect(animTimeline, &QSlider::sliderReleased, this, [this]() {
        animDraggingSlider = false;
        if (!animState) return;
        float t = (animTimeline->value() / 1000.0f) * animState->duration();
        animState->setCurrentTime(t);
        glWidget->update();
    });

    connect(animState, &NifAnimationState::timeChanged, this, [this](float time) {
        if (animDraggingSlider) return;
        float dur = animState->duration();
        int sliderVal = (dur > 0.0f) ? static_cast<int>((time / dur) * 1000.0f) : 0;
        animTimeline->blockSignals(true);
        animTimeline->setValue(sliderVal);
        animTimeline->blockSignals(false);
        animTimeLabel->setText(formatAnimTime(time) + " / " + formatAnimTime(dur));
        glWidget->update();
    });

    connect(animState, &NifAnimationState::animationFinished, this, [this]() {
        animPlayBtn->setChecked(false);
        animPlayBtn->setText("\u25B6");
    });

    connect(animState, &NifAnimationState::stateChanged, this, [this](NifAnimationState::PlayState state) {
        if (state == NifAnimationState::Stopped) {
            animPlayBtn->setChecked(false);
            animPlayBtn->setText("\u25B6");
        }
    });

    connect(animBlendClipCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!animState) return;
        if (index <= 0) {
            animState->setBlendClip(QString());
            animState->resetCrossfade();
            animBlendWeightSlider->setValue(0);
            animBlendWeightLabel->setText(tr("0%"));
        } else {
            animState->setBlendAnimation(nifAnimData);
            animState->setBlendClip(animBlendClipCombo->currentText());
        }
        glWidget->update();
    });

    connect(animBlendWeightSlider, &QSlider::valueChanged, this, [this](int value) {
        if (!animState) return;
        float w = value / 100.0f;
        animState->setBlendWeight(w);
        animBlendWeightLabel->setText(QString("%1%").arg(value));
        glWidget->update();
    });
}

QString NifViewportWidget::formatAnimTime(float seconds) const
{
    int totalSec = static_cast<int>(seconds);
    int min = totalSec / 60;
    int sec = totalSec % 60;
    return QString("%1:%2").arg(min).arg(sec, 2, 10, QChar('0'));
}

void NifViewportWidget::initAnimationState()
{
    if (!nifParser) {
        animToolbar->setVisible(false);
        animBlendToolbar->setVisible(false);
        return;
    }

    int clipCount = nifParser->getAnimationClipCount();
    if (clipCount <= 0) {
        animToolbar->setVisible(false);
        animBlendToolbar->setVisible(false);
        return;
    }

    // Build a NifAnimation from parser data
    delete nifAnimData;
    nifAnimData = new NifAnimation();
    nifAnimData->name = QFileInfo(currentNifFile).baseName();

    // Walk the node tree to build animation clips with populated keyframe channels
    struct NodeAnimInfo {
        QString boneName;
        QVector<Nif::NiKeyframeController> controllers;
    };
    QVector<NodeAnimInfo> nodeAnims;

    std::function<void(Nif::Node*)> collectNodeAnims = [&](Nif::Node* node) {
        if (!node) return;
        if (node->hasAnimation && !node->animations.isEmpty()) {
            NodeAnimInfo info;
            info.boneName = node->name;
            info.controllers = node->animations;
            nodeAnims.append(info);
        }
        for (auto* child : node->children) {
            collectNodeAnims(child);
        }
    };

    collectNodeAnims(nifParser->getRoot());

    if (!nodeAnims.isEmpty()) {
        AnimClip clip;
        clip.name = QFileInfo(currentNifFile).baseName();
        clip.duration = nifParser->getAnimationDuration();

        for (const auto& na : nodeAnims) {
            for (const auto& ctrl : na.controllers) {
                if (ctrl.keyframes.isEmpty()) continue;

                AnimChannel channel;
                channel.boneName = na.boneName;
                channel.type = ctrl.clipName.isEmpty() ? "transform" : ctrl.clipName;
                channel.duration = 0;

                for (const auto& tk : ctrl.keyframes) {
                    AnimKeyframe kf;
                    kf.time = tk.time;
                    kf.tx = tk.translation.x;
                    kf.ty = tk.translation.y;
                    kf.tz = tk.translation.z;
                    // Convert quaternion to Euler angles
                    float qw = tk.rotation.w, qx = tk.rotation.x, qy = tk.rotation.y, qz = tk.rotation.z;
                    kf.rx = std::atan2(2.0f * (qw * qx + qy * qz), 1.0f - 2.0f * (qx * qx + qy * qy));
                    kf.ry = std::asin(qMax(-1.0f, qMin(1.0f, 2.0f * (qw * qy - qz * qx))));
                    kf.rz = std::atan2(2.0f * (qw * qz + qx * qy), 1.0f - 2.0f * (qy * qy + qz * qz));
                    channel.keyframes.append(kf);
                    if (kf.time > channel.duration) channel.duration = kf.time;
                }

                clip.channels.append(channel);
            }
        }

        nifAnimData->clips.append(clip);
    } else {
        // Fallback: create empty clips for NIF files without animation controllers
        for (int c = 0; c < clipCount; ++c) {
            AnimClip clip;
            clip.name = nifParser->getAnimationClipName(c);
            clip.duration = nifParser->getAnimationDuration();
            nifAnimData->clips.append(clip);
        }
    }

    // Populate clip combo
    animClipCombo->blockSignals(true);
    animClipCombo->clear();
    for (int c = 0; c < clipCount; ++c) {
        animClipCombo->addItem(nifParser->getAnimationClipName(c));
    }
    animClipCombo->setCurrentIndex(0);
    animClipCombo->blockSignals(false);

    animBlendClipCombo->blockSignals(true);
    animBlendClipCombo->clear();
    animBlendClipCombo->addItem(tr("(none)"));
    for (int c = 0; c < clipCount; ++c) {
        animBlendClipCombo->addItem(nifParser->getAnimationClipName(c));
    }
    animBlendClipCombo->setCurrentIndex(0);
    animBlendClipCombo->blockSignals(false);

    animBlendWeightSlider->setValue(0);
    animBlendWeightLabel->setText(tr("0%"));

    animState->setBlendAnimation(nifAnimData);
    animState->setBlendWeight(0.0f);

    float dur = animState->duration();
    animTimeLabel->setText(formatAnimTime(0) + " / " + formatAnimTime(dur));
    animTimeline->setValue(0);
    animPlayBtn->setChecked(false);
    animPlayBtn->setText("\u25B6");
    animLoopBtn->setChecked(true);
    animSpeedCombo->setCurrentIndex(2);
    animState->setLooping(true);
    animState->setSpeed(1.0f);

    animToolbar->setVisible(true);
    animBlendToolbar->setVisible(true);
}

void NifViewportWidget::applyAnimationFrame()
{
    if (!animState || animState->state() == NifAnimationState::Stopped) return;
    if (!nifParser || !nifParser->getRoot()) return;

    QVector<TransformKeyframe> frames = animState->getCurrentFrame();
    if (frames.isEmpty()) return;

    QMap<QString, TransformKeyframe> frameMap;
    for (const auto& f : frames) {
        frameMap[f.nodeName] = f;
    }

    // Recompute cumulative transforms with animation overrides
    nodeCumulativeTransforms.clear();

    std::function<void(Nif::Node*, const QMatrix4x4&)> computeAnimTransforms =
        [&](Nif::Node* node, const QMatrix4x4& parentTransform) {
        if (!node) return;

        QMatrix4x4 localTransform;
        localTransform.translate(node->position.x, node->position.y, node->position.z);
        float rx = node->rotation.x * 180.0f / 3.14159265f;
        float ry = node->rotation.y * 180.0f / 3.14159265f;
        float rz = node->rotation.z * 180.0f / 3.14159265f;
        localTransform.rotate(rz, 0.0f, 0.0f, 1.0f);
        localTransform.rotate(ry, 0.0f, 1.0f, 0.0f);
        localTransform.rotate(rx, 1.0f, 0.0f, 0.0f);

        if (frameMap.contains(node->name)) {
            const TransformKeyframe& f = frameMap[node->name];
            QMatrix4x4 animLocal;
            animLocal.translate(f.tx, f.ty, f.tz);
            animLocal.rotate(f.rz * 180.0f / 3.14159265f, 0.0f, 0.0f, 1.0f);
            animLocal.rotate(f.ry * 180.0f / 3.14159265f, 0.0f, 1.0f, 0.0f);
            animLocal.rotate(f.rx * 180.0f / 3.14159265f, 1.0f, 0.0f, 0.0f);
            animLocal.scale(f.sx, f.sy, f.sz);
            localTransform = animLocal;
        }

        QMatrix4x4 cumulativeTransform = parentTransform * localTransform;
        nodeCumulativeTransforms[node] = cumulativeTransform;

        for (auto child : node->children) {
            computeAnimTransforms(child, cumulativeTransform);
        }
    };

    computeAnimTransforms(nifParser->getRoot(), QMatrix4x4());

    // Transform rest-pose vertices by animated cumulative transforms
    QVector<bool> normalTransformed(normals.size(), false);
    for (int s = 0; s < shapeIndexRanges.size(); ++s) {
        if (s >= shapeOwnerNode.size()) continue;
        Nif::Node* ownerNode = shapeOwnerNode[s];
        if (!ownerNode || !nodeCumulativeTransforms.contains(ownerNode)) continue;
        const QMatrix4x4& animXform = nodeCumulativeTransforms[ownerNode];

        const int start = shapeIndexRanges[s].first;
        const int count = shapeIndexRanges[s].second - shapeIndexRanges[s].first;

        for (int i = 0; i < count; ++i) {
            unsigned int vi = indices[start + i];
            if (vi >= static_cast<unsigned int>(restVertices.size())) continue;
            vertices[vi] = animXform * restVertices[vi];
        }

        QMatrix3x3 normalMat = animXform.normalMatrix();
        const float* m = normalMat.constData();
        for (int vi = start; vi < start + count; ++vi) {
            if (vi >= indices.size()) break;
            unsigned int idx = indices[vi];
            if (idx >= static_cast<unsigned int>(restNormals.size())) continue;
            if (normalTransformed[idx]) continue;
            normalTransformed[idx] = true;
            QVector3D n = restNormals[idx];
            normals[idx] = QVector3D(
                m[0] * n.x() + m[1] * n.y() + m[2] * n.z(),
                m[3] * n.x() + m[4] * n.y() + m[5] * n.z(),
                m[6] * n.x() + m[7] * n.y() + m[8] * n.z()
            ).normalized();
        }
    }
    m_meshDirty = true;
}

void NifViewportWidget::setupParticleToolbar()
{
    m_particleToolbar = new QWidget(this);
    auto* particleLayout = new QHBoxLayout(m_particleToolbar);
    particleLayout->setContentsMargins(4, 2, 4, 2);
    particleLayout->setSpacing(4);

    auto* label = new QLabel(tr("Particles:"), m_particleToolbar);
    particleLayout->addWidget(label);

    m_particlePlayBtn = new QPushButton(tr("\u25B6"), m_particleToolbar);
    m_particlePlayBtn->setToolTip(tr("Play Particles"));
    m_particlePlayBtn->setCheckable(true);
    m_particlePlayBtn->setFixedWidth(30);
    particleLayout->addWidget(m_particlePlayBtn);

    m_particleStopBtn = new QPushButton(tr("\u23F9"), m_particleToolbar);
    m_particleStopBtn->setToolTip(tr("Stop Particles"));
    m_particleStopBtn->setFixedWidth(30);
    particleLayout->addWidget(m_particleStopBtn);

    m_particleToolbar->setVisible(false);

    auto* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        int toolbarIdx = mainLayout->indexOf(mainLayout->itemAt(0)->widget());
        mainLayout->insertWidget(toolbarIdx + 1, m_particleToolbar);
    }

    connect(m_particlePlayBtn, &QPushButton::clicked, this, [this](bool checked) {
        if (!m_particleSystem) return;
        if (checked) {
            m_particleSystem->start();
            m_particlePlayBtn->setText("\u23F8");
        } else {
            m_particleSystem->pause();
            m_particlePlayBtn->setText("\u25B6");
        }
        glWidget->update();
    });

    connect(m_particleStopBtn, &QPushButton::clicked, this, [this]() {
        if (!m_particleSystem) return;
        m_particleSystem->stop();
        m_particleSystem->reset();
        m_particlePlayBtn->setChecked(false);
        m_particlePlayBtn->setText("\u25B6");
        glWidget->update();
    });
}

void NifViewportWidget::initParticleSystems()
{
    if (m_particleSystem) {
        m_particleSystem->stop();
        m_particleSystem->reset();
    }
    delete m_particleSystem;
    m_particleSystem = nullptr;
    delete m_particleRenderer;
    m_particleRenderer = nullptr;

    if (!nifParser) {
        if (m_particleToolbar) m_particleToolbar->setVisible(false);
        return;
    }

    QList<ParticleSystemData*> systems = ParticleEffectsParser::parse(currentNifFile);
    if (systems.isEmpty()) {
        if (m_particleToolbar) m_particleToolbar->setVisible(false);
        return;
    }

    m_particleSystem = new ParticleSystem(this);
    m_particleSystem->setSettings(systems.first());
    m_particleRenderer = new ParticleRenderer();
    m_particleRenderer->setAdditive(systems.first()->additiveBlending);

    m_particlePlayBtn->setChecked(false);
    m_particlePlayBtn->setText("\u25B6");
    m_particleToolbar->setVisible(true);

    connect(m_particleSystem, &ParticleSystem::updated, this, [this]() {
        glWidget->update();
    });

    ParticleEffectsParser::cleanup(systems);
}
