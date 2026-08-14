#include "worldviewwidget.hpp"
#include "nifviewportwidget.hpp"

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QTimer>
#include <QMatrix4x4>
#include <QVector3D>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <cmath>

#include "model/world/data.hpp"
#include "model/tools/editrecordcommand.hpp"
#include "model/tools/undostack.hpp"
#include "libs/files/esm/cellrecord.hpp"
#include "libs/files/esm/landrecord.hpp"
#include "libs/files/esm/refrecord.hpp"
#include "libs/files/esm/worldspacerecord.hpp"
#include "libs/files/esm/statrecord.hpp"
#include "libs/files/nif/nifparser.hpp"
#include "../../../libs/files/log/logger.hpp"

// --- WorldRefMesh ---

WorldRefMesh::~WorldRefMesh()
{
    delete vbo;
    delete ibo;
    delete vao;
}

// --- Shaders ---

static const char* terrainVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform float fogStart = 5000.0;
uniform float fogEnd = 30000.0;
uniform vec3 fogColor = vec3(0.4, 0.5, 0.6);
out vec3 fragNormal;
out vec3 fragColor;
out float fragFog;
void main() {
    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    gl_Position = mvpMatrix * worldPos;
    fragNormal = mat3(modelMatrix) * aNormal;
    fragColor = aColor;
    float dist = length(worldPos.xyz);
    fragFog = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
}
)";

static const char* terrainFragSrc = R"(
#version 330 core
in vec3 fragNormal;
in vec3 fragColor;
in float fragFog;
uniform vec3 lightDir = vec3(0.5, 0.8, 0.6);
uniform vec3 fogColor = vec3(0.4, 0.5, 0.6);
uniform bool fogEnabled = true;
out vec4 fragColorOut;
void main() {
    vec3 n = normalize(fragNormal);
    float diff = max(dot(n, normalize(lightDir)), 0.15);
    vec3 color = fragColor * diff;
    if (fogEnabled) color = mix(color, fogColor, fragFog);
    fragColorOut = vec4(color, 1.0);
}
)";

static const char* refVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform vec3 color = vec3(0.7, 0.7, 0.7);
uniform float fogStart = 5000.0;
uniform float fogEnd = 30000.0;
uniform vec3 fogColor = vec3(0.4, 0.5, 0.6);
out vec3 fragNormal;
out vec3 fragColor;
out float fragFog;
void main() {
    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    gl_Position = mvpMatrix * worldPos;
    fragNormal = mat3(modelMatrix) * aNormal;
    fragColor = color;
    float dist = length(worldPos.xyz);
    fragFog = clamp((dist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
}
)";

static const char* refFragSrc = R"(
#version 330 core
in vec3 fragNormal;
in vec3 fragColor;
in float fragFog;
uniform vec3 lightDir = vec3(0.5, 0.8, 0.6);
uniform vec3 fogColor = vec3(0.4, 0.5, 0.6);
uniform bool fogEnabled = true;
out vec4 fragColorOut;
void main() {
    vec3 n = normalize(fragNormal);
    float diff = max(dot(n, normalize(lightDir)), 0.15);
    vec3 color = fragColor * diff;
    if (fogEnabled) color = mix(color, fogColor, fragFog);
    fragColorOut = vec4(color, 1.0);
}
)";

static const char* waterVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform float time;
out vec2 uv;
out float worldY;
void main() {
    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    gl_Position = mvpMatrix * worldPos;
    uv = aPos.xz / 4096.0;
    worldY = worldPos.y;
}
)";

static const char* waterFragSrc = R"(
#version 330 core
in vec2 uv;
in float worldY;
uniform float time;
uniform vec3 waterColor = vec3(0.0, 0.2, 0.4);
uniform float opacity = 0.6;
out vec4 fragColor;
void main() {
    vec2 wave = uv * 0.5 + time * 0.05;
    float alpha = opacity + 0.1 * sin(wave.x * 3.0 + wave.y * 2.0);
    fragColor = vec4(waterColor, alpha);
}
)";

static const char* skyVertSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vUv;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUv = aPos * 0.5 + 0.5;
}
)";

static const char* skyFragSrc = R"(
#version 330 core
in vec2 vUv;
uniform vec3 topColor = vec3(0.2, 0.3, 0.6);
uniform vec3 bottomColor = vec3(0.6, 0.7, 0.9);
out vec4 fragColor;
void main() {
    vec3 color = mix(bottomColor, topColor, vUv.y);
    fragColor = vec4(color, 1.0);
}
)";

static const char* overlayVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 mvpMatrix;
out vec3 vColor;
void main() {
    gl_Position = mvpMatrix * vec4(aPos, 1.0);
    vColor = aColor;
}
)";

static const char* overlayFragSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 fragColor;
void main() {
    fragColor = vec4(vColor, 1.0);
}
)";

// --- WorldViewWidget ---

WorldViewWidget::WorldViewWidget(QWidget* parent)
    : QWidget(parent)
{
    sceneTimer.start();

    glWidget = new QOpenGLWidget(this);
    glWidget->setFocusPolicy(Qt::StrongFocus);
    glWidget->installEventFilter(this);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* toolbar = new QHBoxLayout();
    worldspaceCombo = new QComboBox();
    worldspaceCombo->setMinimumWidth(200);
    toolbar->addWidget(new QLabel("Worldspace:"));
    toolbar->addWidget(worldspaceCombo);

    waterCheck = new QCheckBox("Water");
    waterCheck->setChecked(true);
    toolbar->addWidget(waterCheck);

    waterHeightSpin = new QDoubleSpinBox();
    waterHeightSpin->setRange(-10000, 10000);
    waterHeightSpin->setValue(0);
    waterHeightSpin->setDecimals(0);
    waterHeightSpin->setFixedWidth(80);
    toolbar->addWidget(waterHeightSpin);

    fogCheck = new QCheckBox("Fog");
    fogCheck->setChecked(true);
    toolbar->addWidget(fogCheck);

    toolbar->addStretch();
    infoLabel = new QLabel("No worldspace loaded");
    toolbar->addWidget(infoLabel);

    auto* toolbarWidget = new QWidget();
    toolbarWidget->setLayout(toolbar);

    refList = new QListWidget();
    refList->setMaximumWidth(250);
    connect(refList, &QListWidget::currentRowChanged, this, [this](int row) {
        selectedRefIndex = row;
        buildReferences();
        glWidget->update();
    });
    connect(refList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) {
        showRefEditor(selectedRefIndex);
    });

    connect(waterCheck, &QCheckBox::toggled, this, [this]() {
        waterEnabled = waterCheck->isChecked();
        glWidget->update();
    });
    connect(waterHeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, [this]() {
            waterHeight = static_cast<float>(waterHeightSpin->value());
            glWidget->update();
        });
    connect(fogCheck, &QCheckBox::toggled, this, [this]() {
        fogEnabled = fogCheck->isChecked();
        glWidget->update();
    });

    rightLayout->addWidget(toolbarWidget);
    rightLayout->addWidget(glWidget);

    splitter->addWidget(rightPanel);
    splitter->addWidget(refList);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(splitter);

    keyTimer = new QTimer(this);
    keyTimer->setInterval(16);
    connect(keyTimer, &QTimer::timeout, this, [this]() {
        const float speed = 20.0f;
        if (keys[0]) targetPos += QVector3D(0, speed, 0);
        if (keys[1]) targetPos += QVector3D(0, -speed, 0);
        if (keys[2]) targetPos += QVector3D(-speed, 0, 0);
        if (keys[3]) targetPos += QVector3D(speed, 0, 0);
        updateCellStreaming();
        glWidget->update();
    });

    connect(worldspaceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int idx) {
            if (idx >= 0) {
                quint32 fid = worldspaceCombo->itemData(idx).toUInt();
                loadWorldspace(fid);
            }
        });
}

WorldViewWidget::~WorldViewWidget()
{
    clear();
    delete shaderProgram;
    delete overlayShader;
    delete refShader;
    delete waterShader;
    delete skyShader;
    delete terrainVbo;
    delete terrainIbo;
    delete terrainVao;
}

void WorldViewWidget::setData(Data* d)
{
    data = d;
    worldspaceCombo->clear();
    if (!data) return;
    const auto& wrs = data->getWorldspaceCollection();
    for (int i = 0; i < wrs.count(); ++i) {
        const auto& wr = wrs.getRecord(i).get();
        QString label = wr.editorId.isEmpty()
            ? QString("Worldspace 0x%1").arg(wr.formId, 8, 16, QChar('0'))
            : wr.editorId;
        worldspaceCombo->addItem(label, wr.formId);
    }
    if (worldspaceCombo->count() > 0)
        worldspaceCombo->setCurrentIndex(0);
}

void WorldViewWidget::loadWorldspace(quint32 worldspaceFormId)
{
    clear();
    currentWorldspace = worldspaceFormId;
    if (!data) return;

    infoLabel->setText(QString("Loading worldspace 0x%1...").arg(worldspaceFormId, 8, 16, QChar('0')));

    streamCenterX = 0;
    streamCenterY = 0;
    for (qint32 x = -streamRadius; x <= streamRadius; ++x)
        for (qint32 y = -streamRadius; y <= streamRadius; ++y) {
            loadCellTerrain(x, y);
            loadCellReferences(x, y);
            loadedCells.insert((static_cast<qint64>(x) << 32) | static_cast<quint32>(static_cast<qint32>(y)));
        }

    buildTerrain();
    buildCellGrid();
    buildReferences();

    refList->blockSignals(true);
    refList->clear();
    for (int i = 0; i < refInstances.size(); ++i) {
        const auto& ri = refInstances[i];
        QString label = ri.editorId.isEmpty()
            ? QString("Ref 0x%1").arg(ri.formId, 8, 16, QChar('0'))
            : ri.editorId;
        refList->addItem(label);
    }
    refList->blockSignals(false);

    infoLabel->setText(QString("Worldspace 0x%1 | %2 cells | %3 refs")
        .arg(worldspaceFormId, 8, 16, QChar('0'))
        .arg(loadedCells.size())
        .arg(refInstances.size()));

    glWidget->update();
}

void WorldViewWidget::clear()
{
    terrainVertices.clear();
    terrainNormals.clear();
    terrainColors.clear();
    terrainIndices.clear();
    terrainBuilt = false;
    delete gridVBO; gridVBO = nullptr;
    delete refMarkerVBO; refMarkerVBO = nullptr;
    delete axisVBO; axisVBO = nullptr;
    delete selectedRefVBO; selectedRefVBO = nullptr;
    refInstances.clear();
    for (auto& m : refMeshes) delete m;
    refMeshes.clear();
    loadedCells.clear();
    selectedRefIndex = -1;
    currentWorldspace = 0;
}

void WorldViewWidget::setupOpenGL()
{
    QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();
    gl->glEnable(GL_DEPTH_TEST);
    gl->glEnable(GL_CULL_FACE);
    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl->glClearColor(0.4f, 0.5f, 0.6f, 1.0f);
}

void WorldViewWidget::setupShaders()
{
    if (!shaderProgram) {
        shaderProgram = new QOpenGLShaderProgram();
        shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, terrainVertSrc);
        shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, terrainFragSrc);
        shaderProgram->link();
    }
    if (!refShader) {
        refShader = new QOpenGLShaderProgram();
        refShader->addShaderFromSourceCode(QOpenGLShader::Vertex, refVertSrc);
        refShader->addShaderFromSourceCode(QOpenGLShader::Fragment, refFragSrc);
        refShader->link();
    }
    if (!waterShader) {
        waterShader = new QOpenGLShaderProgram();
        waterShader->addShaderFromSourceCode(QOpenGLShader::Vertex, waterVertSrc);
        waterShader->addShaderFromSourceCode(QOpenGLShader::Fragment, waterFragSrc);
        waterShader->link();
    }
    if (!skyShader) {
        skyShader = new QOpenGLShaderProgram();
        skyShader->addShaderFromSourceCode(QOpenGLShader::Vertex, skyVertSrc);
        skyShader->addShaderFromSourceCode(QOpenGLShader::Fragment, skyFragSrc);
        skyShader->link();
    }
    if (!overlayShader) {
        overlayShader = new QOpenGLShaderProgram();
        overlayShader->addShaderFromSourceCode(QOpenGLShader::Vertex, overlayVertSrc);
        overlayShader->addShaderFromSourceCode(QOpenGLShader::Fragment, overlayFragSrc);
        overlayShader->link();
    }
}

void WorldViewWidget::updateCellStreaming()
{
    qint32 cx = static_cast<qint32>(floorf(targetPos.x() / cellSize));
    qint32 cy = static_cast<qint32>(floorf(targetPos.y() / cellSize));
    if (cx == streamCenterX && cy == streamCenterY) return;

    QSet<qint64> newCells;
    for (qint32 x = cx - streamRadius; x <= cx + streamRadius; ++x)
        for (qint32 y = cy - streamRadius; y <= cy + streamRadius; ++y)
            newCells.insert((static_cast<qint64>(x) << 32) | static_cast<quint32>(static_cast<qint32>(y)));

    // Unload cells no longer in range
    for (qint64 key : loadedCells) {
        if (!newCells.contains(key)) {
            qint32 x = static_cast<qint32>(key >> 32);
            qint32 y = static_cast<qint32>(key & 0xFFFFFFFF);
            unloadCellTerrain(x, y);
            unloadCellReferences(x, y);
        }
    }

    // Load new cells
    for (qint64 key : newCells) {
        if (!loadedCells.contains(key)) {
            qint32 x = static_cast<qint32>(key >> 32);
            qint32 y = static_cast<qint32>(key & 0xFFFFFFFF);
            loadCellTerrain(x, y);
            loadCellReferences(x, y);
        }
    }

    loadedCells = newCells;
    streamCenterX = cx;
    streamCenterY = cy;
    buildTerrain();
    buildReferences();
}

void WorldViewWidget::loadCellTerrain(qint32 cellX, qint32 cellY)
{
    if (!data) return;
    const auto& landCol = data->getLandCollection();
    for (int i = 0; i < landCol.count(); ++i) {
        const auto& land = landCol.getRecord(i).get();
        if (land.cellX == cellX && land.cellY == cellY && land.hasHeightData) {
            float cx = static_cast<float>(cellX) * cellSize;
            float cy = static_cast<float>(cellY) * cellSize;
            float base = land.baseHeight;
            int baseIdx = terrainVertices.size();
            for (int y = 0; y < 33; ++y) {
                for (int x = 0; x < 33; ++x) {
                    float hx = cx + static_cast<float>(x) * (cellSize / 32.0f);
                    float hy = cy + static_cast<float>(y) * (cellSize / 32.0f);
                    float hz = base + static_cast<float>(land.heightData[x][y]) * 0.5f;
                    terrainVertices.append(QVector3D(hx, hy, hz));
                    float t = (hz - base) / 200.0f;
                    t = qBound(0.0f, t, 1.0f);
                    terrainColors.append(QVector3D(0.2f + t * 0.3f, 0.4f + t * 0.3f, 0.1f + t * 0.2f));
                }
            }
            for (int y = 0; y < 32; ++y) {
                for (int x = 0; x < 32; ++x) {
                    int idx = baseIdx + y * 33 + x;
                    terrainIndices.append(idx);
                    terrainIndices.append(idx + 33);
                    terrainIndices.append(idx + 1);
                    terrainIndices.append(idx + 1);
                    terrainIndices.append(idx + 33);
                    terrainIndices.append(idx + 34);
                }
            }
            int newSize = terrainVertices.size();
            terrainNormals.resize(newSize);
            for (int i = baseIdx; i < newSize; ++i)
                terrainNormals[i] = QVector3D(0, 0, 0);
            int triStart = (baseIdx / 33) * 32 * 6;
            for (int i = triStart; i < terrainIndices.size(); i += 3) {
                int i0 = terrainIndices[i];
                int i1 = terrainIndices[i + 1];
                int i2 = terrainIndices[i + 2];
                QVector3D e1 = terrainVertices[i1] - terrainVertices[i0];
                QVector3D e2 = terrainVertices[i2] - terrainVertices[i0];
                QVector3D n = QVector3D::crossProduct(e1, e2).normalized();
                terrainNormals[i0] += n;
                terrainNormals[i1] += n;
                terrainNormals[i2] += n;
            }
            for (int i = baseIdx; i < newSize; ++i)
                terrainNormals[i].normalize();
            return;
        }
    }
}

void WorldViewWidget::unloadCellTerrain(qint32 cellX, qint32 cellY)
{
    float cx = static_cast<float>(cellX) * cellSize;
    float cy = static_cast<float>(cellY) * cellSize;
    // Remove vertices in this cell's bounds
    QVector<int> toRemove;
    for (int i = 0; i < terrainVertices.size(); ++i) {
        if (terrainVertices[i].x() >= cx && terrainVertices[i].x() < cx + cellSize &&
            terrainVertices[i].y() >= cy && terrainVertices[i].y() < cy + cellSize) {
            toRemove.prepend(i);
        }
    }
    for (int idx : toRemove) {
        terrainVertices.removeAt(idx);
        terrainNormals.removeAt(idx);
        terrainColors.removeAt(idx);
    }
    // Rebuild indices (simplified: just clear and rebuild)
    terrainIndices.clear();
    terrainBuilt = false;
}

void WorldViewWidget::loadCellReferences(qint32 cellX, qint32 cellY)
{
    if (!data) return;
    const auto& refCol = data->getRefrCollection();
    float cx = static_cast<float>(cellX) * cellSize;
    float cy = static_cast<float>(cellY) * cellSize;
    for (int i = 0; i < refCol.count(); ++i) {
        const auto& ref = refCol.getRecord(i).get();
        if (ref.posX >= cx && ref.posX < cx + cellSize &&
            ref.posY >= cy && ref.posY < cy + cellSize) {
            WorldRefInstance inst;
            inst.formId = ref.formId;
            inst.baseId = ref.baseId;
            inst.position = QVector3D(ref.posX, ref.posY, ref.posZ);
            inst.rotation = QVector3D(ref.rotX, ref.rotY, ref.rotZ);
            inst.scale = ref.scale;
            inst.editorId = ref.editorId;
            inst.mesh = nullptr;
            inst.dataIndex = i;
            refInstances.append(inst);
        }
    }
}

void WorldViewWidget::unloadCellReferences(qint32 cellX, qint32 cellY)
{
    float cx = static_cast<float>(cellX) * cellSize;
    float cy = static_cast<float>(cellY) * cellSize;
    for (int i = refInstances.size() - 1; i >= 0; --i) {
        if (refInstances[i].position.x() >= cx && refInstances[i].position.x() < cx + cellSize &&
            refInstances[i].position.y() >= cy && refInstances[i].position.y() < cy + cellSize) {
            refInstances.removeAt(i);
        }
    }
}

QString WorldViewWidget::findModelPath(quint32 baseFormId)
{
    if (!data) return {};
    const auto& statCol = data->getStatCollection();
    for (int i = 0; i < statCol.count(); ++i) {
        const auto& stat = statCol.getRecord(i).get();
        if (stat.formId == baseFormId && !stat.modelPath.isEmpty())
            return stat.modelPath;
    }
    const auto& actiCol = data->getActiCollection();
    for (int i = 0; i < actiCol.count(); ++i) {
        const auto& acti = actiCol.getRecord(i).get();
        if (acti.formId == baseFormId && !acti.modelPath.isEmpty())
            return acti.modelPath;
    }
    const auto& miscCol = data->getMiscCollection();
    for (int i = 0; i < miscCol.count(); ++i) {
        const auto& misc = miscCol.getRecord(i).get();
        if (misc.formId == baseFormId && !misc.modelPath.isEmpty())
            return misc.modelPath;
    }
    const auto& furnCol = data->getFurnCollection();
    for (int i = 0; i < furnCol.count(); ++i) {
        const auto& furn = furnCol.getRecord(i).get();
        if (furn.formId == baseFormId && !furn.modelPath.isEmpty())
            return furn.modelPath;
    }
    const auto& doorCol = data->getDoorCollection();
    for (int i = 0; i < doorCol.count(); ++i) {
        const auto& door = doorCol.getRecord(i).get();
        if (door.formId == baseFormId && !door.modelPath.isEmpty())
            return door.modelPath;
    }
    const auto& lighCol = data->getLighCollection();
    for (int i = 0; i < lighCol.count(); ++i) {
        const auto& ligh = lighCol.getRecord(i).get();
        if (ligh.formId == baseFormId && !ligh.modelPath.isEmpty())
            return ligh.modelPath;
    }
    const auto& treeCol = data->getTreeCollection();
    for (int i = 0; i < treeCol.count(); ++i) {
        const auto& tree = treeCol.getRecord(i).get();
        if (tree.formId == baseFormId && !tree.modelPath.isEmpty())
            return tree.modelPath;
    }
    const auto& msttCol = data->getMsttCollection();
    for (int i = 0; i < msttCol.count(); ++i) {
        const auto& mstt = msttCol.getRecord(i).get();
        if (mstt.formId == baseFormId && !mstt.modelPath.isEmpty())
            return mstt.modelPath;
    }
    const auto& floraCol = data->getFlorCollection();
    for (int i = 0; i < floraCol.count(); ++i) {
        const auto& flora = floraCol.getRecord(i).get();
        if (flora.formId == baseFormId && !flora.modelPath.isEmpty())
            return flora.modelPath;
    }
    return {};
}

WorldRefMesh* WorldViewWidget::loadNifMesh(const QString& nifPath)
{
    auto it = refMeshes.find(nifPath);
    if (it != refMeshes.end()) return it.value();

    Nif::NifParser parser;
    if (!parser.load(nifPath)) return nullptr;

    auto* mesh = new WorldRefMesh();
    auto* root = parser.getRoot();
    if (!root) { delete mesh; return nullptr; }

    std::function<void(Nif::Node*)> collect = [&](Nif::Node* node) {
        for (const auto& shape : node->shapes) {
            int baseIdx = mesh->vertices.size();
            for (const auto& v : shape.vertices)
                mesh->vertices.append(QVector3D(v.x, v.y, v.z));
            for (const auto& n : shape.normals)
                mesh->normals.append(QVector3D(n.x, n.y, n.z));
            for (unsigned int idx : shape.indices)
                mesh->indices.append(baseIdx + idx);
        }
        for (auto* child : node->children)
            collect(child);
    };
    collect(root);

    if (mesh->vertices.isEmpty() || mesh->indices.isEmpty()) {
        delete mesh;
        return nullptr;
    }

    if (mesh->normals.size() != mesh->vertices.size()) {
        mesh->normals.resize(mesh->vertices.size());
        for (int i = 0; i < mesh->normals.size(); ++i)
            mesh->normals[i] = QVector3D(0, 0, 1);
    }

    mesh->vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    mesh->ibo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    mesh->vao = new QOpenGLVertexArrayObject();
    mesh->vao->create();
    mesh->vao->bind();

    QVector<float> interleaved;
    interleaved.reserve(mesh->vertices.size() * 6);
    for (int i = 0; i < mesh->vertices.size(); ++i) {
        interleaved.append(mesh->vertices[i].x());
        interleaved.append(mesh->vertices[i].y());
        interleaved.append(mesh->vertices[i].z());
        interleaved.append(mesh->normals[i].x());
        interleaved.append(mesh->normals[i].y());
        interleaved.append(mesh->normals[i].z());
    }

    mesh->vbo->create();
    mesh->vbo->bind();
    mesh->vbo->allocate(interleaved.constData(), interleaved.size() * sizeof(float));

    mesh->ibo->create();
    mesh->ibo->bind();
    mesh->ibo->allocate(mesh->indices.constData(), mesh->indices.size() * sizeof(unsigned int));

    refShader->bind();
    refShader->enableAttributeArray(0);
    refShader->setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));
    refShader->enableAttributeArray(1);
    refShader->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));
    refShader->release();

    mesh->vao->release();
    mesh->loaded = true;

    refMeshes.insert(nifPath, mesh);
    return mesh;
}

void WorldViewWidget::buildTerrain()
{
    if (terrainVertices.isEmpty()) return;

    if (!terrainVao) terrainVao = new QOpenGLVertexArrayObject();
    if (!terrainVbo) terrainVbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    if (!terrainIbo) terrainIbo = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);

    terrainVao->create();
    terrainVao->bind();

    QVector<float> interleaved;
    interleaved.reserve(terrainVertices.size() * 9);
    for (int i = 0; i < terrainVertices.size(); ++i) {
        interleaved.append(terrainVertices[i].x());
        interleaved.append(terrainVertices[i].y());
        interleaved.append(terrainVertices[i].z());
        interleaved.append(terrainNormals[i].x());
        interleaved.append(terrainNormals[i].y());
        interleaved.append(terrainNormals[i].z());
        interleaved.append(terrainColors[i].x());
        interleaved.append(terrainColors[i].y());
        interleaved.append(terrainColors[i].z());
    }

    terrainVbo->create();
    terrainVbo->bind();
    terrainVbo->allocate(interleaved.constData(), interleaved.size() * sizeof(float));

    terrainIbo->create();
    terrainIbo->bind();
    terrainIbo->allocate(terrainIndices.constData(), terrainIndices.size() * sizeof(unsigned int));

    shaderProgram->bind();
    shaderProgram->enableAttributeArray(0);
    shaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, 9 * sizeof(float));
    shaderProgram->enableAttributeArray(1);
    shaderProgram->setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 9 * sizeof(float));
    shaderProgram->enableAttributeArray(2);
    shaderProgram->setAttributeBuffer(2, GL_FLOAT, 6 * sizeof(float), 3, 9 * sizeof(float));
    shaderProgram->release();

    terrainVao->release();
    terrainBuilt = true;
}

void WorldViewWidget::buildCellGrid()
{
    if (!gridVBO) gridVBO = new OverlayVBO();
    QVector<OverlayVertex> gridVerts;
    for (int x = minCellX; x <= maxCellX; ++x) {
        float px = static_cast<float>(x) * cellSize;
        float y0 = static_cast<float>(minCellY) * cellSize;
        float y1 = static_cast<float>(maxCellY) * cellSize;
        gridVerts.append({QVector3D(px, y0, 0), QVector3D(0.5f, 0.5f, 0.5f)});
        gridVerts.append({QVector3D(px, y1, 0), QVector3D(0.5f, 0.5f, 0.5f)});
    }
    for (int y = minCellY; y <= maxCellY; ++y) {
        float py = static_cast<float>(y) * cellSize;
        float x0 = static_cast<float>(minCellX) * cellSize;
        float x1 = static_cast<float>(maxCellX) * cellSize;
        gridVerts.append({QVector3D(x0, py, 0), QVector3D(0.5f, 0.5f, 0.5f)});
        gridVerts.append({QVector3D(x1, py, 0), QVector3D(0.5f, 0.5f, 0.5f)});
    }
    gridVBO->build(gridVerts, 0x0001);
}

void WorldViewWidget::buildReferences()
{
    if (!refMarkerVBO) refMarkerVBO = new OverlayVBO();
    if (!axisVBO) axisVBO = new OverlayVBO();
    if (!selectedRefVBO) selectedRefVBO = new OverlayVBO();

    for (auto& inst : refInstances) {
        if (!inst.mesh) {
            QString modelPath = findModelPath(inst.baseId);
            if (!modelPath.isEmpty())
                inst.mesh = loadNifMesh(modelPath);
        }
    }

    QVector<OverlayVertex> markerVerts;
    for (const auto& inst : refInstances) {
        if (inst.mesh && inst.mesh->loaded) continue;
        QVector3D pos = inst.position;
        float s = 20.0f;
        markerVerts.append({pos + QVector3D(-s, 0, 0), QVector3D(1, 0, 0)});
        markerVerts.append({pos + QVector3D(s, 0, 0), QVector3D(1, 0, 0)});
        markerVerts.append({pos + QVector3D(0, -s, 0), QVector3D(0, 1, 0)});
        markerVerts.append({pos + QVector3D(0, s, 0), QVector3D(0, 1, 0)});
        markerVerts.append({pos + QVector3D(0, 0, -s), QVector3D(0, 0, 1)});
        markerVerts.append({pos + QVector3D(0, 0, s), QVector3D(0, 0, 1)});
    }
    refMarkerVBO->build(markerVerts, 0x0001);

    QVector<OverlayVertex> selVerts;
    if (selectedRefIndex >= 0 && selectedRefIndex < refInstances.size()) {
        const auto& inst = refInstances[selectedRefIndex];
        QVector3D pos = inst.position;
        float s = 40.0f;
        auto addEdge = [&](QVector3D a, QVector3D b) {
            selVerts.append({pos + a, QVector3D(1, 1, 0)});
            selVerts.append({pos + b, QVector3D(1, 1, 0)});
        };
        addEdge(QVector3D(-s, -s, -s), QVector3D(s, -s, -s));
        addEdge(QVector3D(s, -s, -s), QVector3D(s, s, -s));
        addEdge(QVector3D(s, s, -s), QVector3D(-s, s, -s));
        addEdge(QVector3D(-s, s, -s), QVector3D(-s, -s, -s));
        addEdge(QVector3D(-s, -s, s), QVector3D(s, -s, s));
        addEdge(QVector3D(s, -s, s), QVector3D(s, s, s));
        addEdge(QVector3D(s, s, s), QVector3D(-s, s, s));
        addEdge(QVector3D(-s, s, s), QVector3D(-s, -s, s));
        addEdge(QVector3D(-s, -s, -s), QVector3D(-s, -s, s));
        addEdge(QVector3D(s, -s, -s), QVector3D(s, -s, s));
        addEdge(QVector3D(s, s, -s), QVector3D(s, s, s));
        addEdge(QVector3D(-s, s, -s), QVector3D(-s, s, s));
    }
    selectedRefVBO->build(selVerts, 0x0001);

    QVector<OverlayVertex> axisVerts;
    float axLen = 100.0f;
    axisVerts.append({QVector3D(0, 0, 0), QVector3D(1, 0, 0)});
    axisVerts.append({QVector3D(axLen, 0, 0), QVector3D(1, 0, 0)});
    axisVerts.append({QVector3D(0, 0, 0), QVector3D(0, 1, 0)});
    axisVerts.append({QVector3D(0, axLen, 0), QVector3D(0, 1, 0)});
    axisVerts.append({QVector3D(0, 0, 0), QVector3D(0, 0, 1)});
    axisVerts.append({QVector3D(0, 0, axLen), QVector3D(0, 0, 1)});
    axisVBO->build(axisVerts, 0x0001);
}

int WorldViewWidget::pickRef(const QPoint& screenPos)
{
    int w = glWidget->width();
    int h = glWidget->height();
    float aspect = static_cast<float>(w) / static_cast<float>(h);

    QMatrix4x4 proj;
    proj.perspective(60.0f, aspect, 1.0f, 100000.0f);

    QMatrix4x4 view;
    float radX = qDegreesToRadians(rotationX);
    float radY = qDegreesToRadians(rotationY);
    float cx = zoom * cosf(radX) * sinf(radY);
    float cy = zoom * sinf(radX);
    float cz = zoom * cosf(radX) * cosf(radY);
    view.lookAt(targetPos + QVector3D(cx, cy, cz), targetPos, QVector3D(0, 1, 0));

    QMatrix4x4 mvp = proj * view;

    float bestDist = 20.0f;
    int bestIdx = -1;
    for (int i = 0; i < refInstances.size(); ++i) {
        QVector4D clip = mvp * QVector4D(refInstances[i].position, 1.0f);
        if (clip.w() <= 0) continue;
        float sx = (clip.x() / clip.w()) * 0.5f + 0.5f;
        float sy = (clip.y() / clip.w()) * 0.5f + 0.5f;
        float px = sx * w;
        float py = (1.0f - sy) * h;
        float dist = QVector2D(px - screenPos.x(), py - screenPos.y()).length();
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

void WorldViewWidget::showRefEditor(int index)
{
    if (index < 0 || index >= refInstances.size()) return;
    auto& inst = refInstances[index];

    QDialog dlg(this);
    dlg.setWindowTitle("Reference Properties");
    auto* form = new QFormLayout(&dlg);

    auto* editorIdEdit = new QLineEdit(inst.editorId);
    editorIdEdit->setReadOnly(true);
    form->addRow("Editor ID:", editorIdEdit);

    auto* formIdLabel = new QLabel(QString("0x%1").arg(inst.formId, 8, 16, QChar('0')));
    form->addRow("Form ID:", formIdLabel);

    auto* baseIdLabel = new QLabel(QString("0x%1").arg(inst.baseId, 8, 16, QChar('0')));
    form->addRow("Base ID:", baseIdLabel);

    auto* posX = new QDoubleSpinBox(); posX->setRange(-100000, 100000); posX->setValue(inst.position.x());
    auto* posY = new QDoubleSpinBox(); posY->setRange(-100000, 100000); posY->setValue(inst.position.y());
    auto* posZ = new QDoubleSpinBox(); posZ->setRange(-100000, 100000); posZ->setValue(inst.position.z());
    form->addRow("Position X:", posX);
    form->addRow("Position Y:", posY);
    form->addRow("Position Z:", posZ);

    auto* rotX = new QDoubleSpinBox(); rotX->setRange(-360, 360); rotX->setValue(inst.rotation.x());
    auto* rotY = new QDoubleSpinBox(); rotY->setRange(-360, 360); rotY->setValue(inst.rotation.y());
    auto* rotZ = new QDoubleSpinBox(); rotZ->setRange(-360, 360); rotZ->setValue(inst.rotation.z());
    form->addRow("Rotation X:", rotX);
    form->addRow("Rotation Y:", rotY);
    form->addRow("Rotation Z:", rotZ);

    auto* scaleSpin = new QDoubleSpinBox(); scaleSpin->setRange(0.1, 10); scaleSpin->setDecimals(2); scaleSpin->setValue(inst.scale);
    form->addRow("Scale:", scaleSpin);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        inst.position = QVector3D(
            static_cast<float>(posX->value()),
            static_cast<float>(posY->value()),
            static_cast<float>(posZ->value()));
        inst.rotation = QVector3D(
            static_cast<float>(rotX->value()),
            static_cast<float>(rotY->value()),
            static_cast<float>(rotZ->value()));
        inst.scale = static_cast<float>(scaleSpin->value());

        if (data && inst.dataIndex >= 0) {
            auto& coll = data->getRefrCollection();
            if (inst.dataIndex < coll.size()) {
                RefrRecord original = coll.getRecord(inst.dataIndex).get();
                RefrRecord edited = original;
                edited.posX = static_cast<float>(posX->value());
                edited.posY = static_cast<float>(posY->value());
                edited.posZ = static_cast<float>(posZ->value());
                edited.rotX = static_cast<float>(rotX->value());
                edited.rotY = static_cast<float>(rotY->value());
                edited.rotZ = static_cast<float>(rotZ->value());
                edited.scale = static_cast<float>(scaleSpin->value());
                auto* cmd = new EditRecordCommand<RefrRecord>(
                    &coll, inst.dataIndex, original, edited,
                    QStringLiteral("Transform Reference 0x%1").arg(edited.formId, 8, 16, QChar('0')));
                if (UndoStack* undo = data->getUndoStack(); undo && cmd->hasChanged())
                    undo->push(cmd);
                else
                    delete cmd;
            }
        }

        buildReferences();
        glWidget->update();
    }
}

void WorldViewWidget::renderScene()
{
    QOpenGLFunctions* gl = QOpenGLContext::currentContext()->functions();
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int w = glWidget->width();
    int h = glWidget->height();
    float aspect = static_cast<float>(w) / static_cast<float>(h);

    QMatrix4x4 proj;
    proj.perspective(60.0f, aspect, 1.0f, 100000.0f);

    QMatrix4x4 view;
    float radX = qDegreesToRadians(rotationX);
    float radY = qDegreesToRadians(rotationY);
    float cx = zoom * cosf(radX) * sinf(radY);
    float cy = zoom * sinf(radX);
    float cz = zoom * cosf(radX) * cosf(radY);
    view.lookAt(targetPos + QVector3D(cx, cy, cz), targetPos, QVector3D(0, 1, 0));

    QMatrix4x4 mvp = proj * view;

    // Sky
    if (skyShader) {
        gl->glDisable(GL_DEPTH_TEST);
        skyShader->bind();
        static const float skyVerts[] = {-1, -1, 1, -1, -1, 1, 1, 1};
        skyShader->enableAttributeArray(0);
        skyShader->setAttributeArray(0, GL_FLOAT, skyVerts, 2);
        gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        skyShader->disableAttributeArray(0);
        skyShader->release();
        gl->glEnable(GL_DEPTH_TEST);
    }

    // Fog uniforms
    float fogStart = zoom * 0.5f;
    float fogEnd = zoom * 2.0f;
    QVector3D fogColor(0.4f, 0.5f, 0.6f);

    // Terrain
    if (terrainBuilt && terrainVao) {
        shaderProgram->bind();
        shaderProgram->setUniformValue("mvpMatrix", mvp);
        QMatrix4x4 identity;
        shaderProgram->setUniformValue("modelMatrix", identity);
        shaderProgram->setUniformValue("lightDir", QVector3D(0.5f, 0.8f, 0.6f));
        shaderProgram->setUniformValue("fogStart", fogStart);
        shaderProgram->setUniformValue("fogEnd", fogEnd);
        shaderProgram->setUniformValue("fogColor", fogColor);
        shaderProgram->setUniformValue("fogEnabled", fogEnabled);
        terrainVao->bind();
        gl->glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, nullptr);
        terrainVao->release();
        shaderProgram->release();
    }

    // Water
    if (waterEnabled && waterShader) {
        gl->glEnable(GL_BLEND);
        gl->glDepthMask(GL_FALSE);
        waterShader->bind();
        QMatrix4x4 waterModel;
        float halfSize = static_cast<float>(streamRadius + 1) * cellSize;
        waterModel.translate(0, waterHeight, 0);
        waterModel.scale(halfSize, 1, halfSize);
        waterShader->setUniformValue("mvpMatrix", mvp);
        waterShader->setUniformValue("modelMatrix", waterModel);
        waterShader->setUniformValue("time", sceneTimer.elapsed() / 1000.0f);
        static const float waterVerts[] = {
            -1, 0, -1,   1, 0, -1,   -1, 0, 1,
            1, 0, -1,    1, 0, 1,    -1, 0, 1
        };
        waterShader->enableAttributeArray(0);
        waterShader->setAttributeArray(0, GL_FLOAT, waterVerts, 3);
        gl->glDrawArrays(GL_TRIANGLES, 0, 6);
        waterShader->disableAttributeArray(0);
        waterShader->release();
        gl->glDepthMask(GL_TRUE);
        gl->glDisable(GL_BLEND);
    }

    // Reference meshes
    refShader->bind();
    refShader->setUniformValue("mvpMatrix", mvp);
    refShader->setUniformValue("lightDir", QVector3D(0.5f, 0.8f, 0.6f));
    refShader->setUniformValue("fogStart", fogStart);
    refShader->setUniformValue("fogEnd", fogEnd);
    refShader->setUniformValue("fogColor", fogColor);
    refShader->setUniformValue("fogEnabled", fogEnabled);
    for (int i = 0; i < refInstances.size(); ++i) {
        const auto& inst = refInstances[i];
        if (!inst.mesh || !inst.mesh->loaded) continue;

        QMatrix4x4 model;
        model.translate(inst.position);
        model.rotate(inst.rotation.z(), 0, 0, 1);
        model.rotate(inst.rotation.y(), 0, 1, 0);
        model.rotate(inst.rotation.x(), 1, 0, 0);
        model.scale(inst.scale);
        refShader->setUniformValue("modelMatrix", model);

        QVector3D color = (i == selectedRefIndex)
            ? QVector3D(1.0f, 1.0f, 0.2f)
            : QVector3D(0.7f, 0.7f, 0.7f);
        refShader->setUniformValue("color", color);

        inst.mesh->vao->bind();
        gl->glDrawElements(GL_TRIANGLES, inst.mesh->indices.size(), GL_UNSIGNED_INT, nullptr);
        inst.mesh->vao->release();
    }
    refShader->release();

    // Overlays
    if (gridVBO) gridVBO->draw(overlayShader, mvp);
    if (refMarkerVBO) refMarkerVBO->draw(overlayShader, mvp);
    if (selectedRefVBO) selectedRefVBO->draw(overlayShader, mvp);
    if (axisVBO) axisVBO->draw(overlayShader, mvp);
}

void WorldViewWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(60, 60, 60));
}

void WorldViewWidget::resizeEvent(QResizeEvent*) {}

void WorldViewWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int idx = pickRef(event->pos());
        if (idx >= 0) {
            selectedRefIndex = idx;
            refList->blockSignals(true);
            refList->setCurrentRow(idx);
            refList->blockSignals(false);
            buildReferences();
            glWidget->update();
        } else {
            dragging = true;
            lastMousePos = event->pos();
        }
    }
}

void WorldViewWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int idx = pickRef(event->pos());
        if (idx >= 0) showRefEditor(idx);
    }
}

void WorldViewWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (dragging) {
        float dx = static_cast<float>(event->pos().x() - lastMousePos.x());
        float dy = static_cast<float>(event->pos().y() - lastMousePos.y());
        rotationY += dx * 0.3f;
        rotationX += dy * 0.3f;
        rotationX = qBound(-89.0f, rotationX, 89.0f);
        lastMousePos = event->pos();
        glWidget->update();
    }
}

void WorldViewWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        dragging = false;
}

void WorldViewWidget::wheelEvent(QWheelEvent* event)
{
    zoom *= (1.0f - event->angleDelta().y() / 1200.0f);
    zoom = qBound(10.0f, zoom, 50000.0f);
    glWidget->update();
}

void WorldViewWidget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_W: keys[0] = true; keyTimer->start(); break;
    case Qt::Key_S: keys[1] = true; keyTimer->start(); break;
    case Qt::Key_A: keys[2] = true; keyTimer->start(); break;
    case Qt::Key_D: keys[3] = true; keyTimer->start(); break;
    }
}

void WorldViewWidget::keyReleaseEvent(QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_W: keys[0] = false; break;
    case Qt::Key_S: keys[1] = false; break;
    case Qt::Key_A: keys[2] = false; break;
    case Qt::Key_D: keys[3] = false; break;
    }
    if (!keys[0] && !keys[1] && !keys[2] && !keys[3])
        keyTimer->stop();
}

bool WorldViewWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == glWidget) {
        switch (event->type()) {
        case QEvent::Paint: {
            setupShaders();
            setupOpenGL();
            renderScene();
            return true;
        }
        case QEvent::Resize:
            glWidget->update();
            break;
        default: break;
        }
    }
    return QWidget::eventFilter(obj, event);
}
