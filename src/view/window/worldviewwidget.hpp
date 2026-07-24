#ifndef WORLDVIEWWIDGET_HPP
#define WORLDVIEWWIDGET_HPP

#include <QWidget>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector>
#include <QMap>
#include <QString>
#include <QSet>
#include <QElapsedTimer>

class QOpenGLWidget;
class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class QTimer;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QComboBox;
class QLabel;
class QListWidget;
class QCheckBox;
class QDoubleSpinBox;

class Data;
struct OverlayVertex;
struct OverlayVBO;

struct WorldRefMesh {
    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<unsigned int> indices;
    QOpenGLBuffer* vbo = nullptr;
    QOpenGLBuffer* ibo = nullptr;
    QOpenGLVertexArrayObject* vao = nullptr;
    bool loaded = false;
    ~WorldRefMesh();
};

struct WorldRefInstance {
    quint32 formId = 0;
    quint32 baseId = 0;
    QVector3D position;
    QVector3D rotation;
    float scale = 1.0f;
    QString editorId;
    WorldRefMesh* mesh = nullptr;
};

class WorldViewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WorldViewWidget(QWidget* parent = nullptr);
    ~WorldViewWidget();

    void setData(Data* data);
    void loadWorldspace(quint32 worldspaceFormId);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupOpenGL();
    void setupShaders();
    void buildTerrain();
    void buildCellGrid();
    void buildReferences();
    void renderScene();
    void loadCellTerrain(qint32 cellX, qint32 cellY);
    void unloadCellTerrain(qint32 cellX, qint32 cellY);
    void loadCellReferences(qint32 cellX, qint32 cellY);
    void unloadCellReferences(qint32 cellX, qint32 cellY);
    void updateCellStreaming();
    WorldRefMesh* loadNifMesh(const QString& nifPath);
    QString findModelPath(quint32 baseFormId);
    int pickRef(const QPoint& screenPos);
    void showRefEditor(int index);

    QOpenGLWidget* glWidget;
    Data* data = nullptr;
    quint32 currentWorldspace = 0;

    QOpenGLShaderProgram* shaderProgram = nullptr;
    QOpenGLShaderProgram* overlayShader = nullptr;
    QOpenGLShaderProgram* refShader = nullptr;
    QOpenGLShaderProgram* waterShader = nullptr;
    QOpenGLShaderProgram* skyShader = nullptr;

    QOpenGLBuffer* terrainVbo = nullptr;
    QOpenGLBuffer* terrainIbo = nullptr;
    QOpenGLVertexArrayObject* terrainVao = nullptr;
    QVector<QVector3D> terrainVertices;
    QVector<QVector3D> terrainNormals;
    QVector<QVector3D> terrainColors;
    QVector<unsigned int> terrainIndices;
    bool terrainBuilt = false;

    OverlayVBO* gridVBO = nullptr;
    OverlayVBO* refMarkerVBO = nullptr;
    OverlayVBO* axisVBO = nullptr;
    OverlayVBO* selectedRefVBO = nullptr;

    QVector<WorldRefInstance> refInstances;
    QMap<QString, WorldRefMesh*> refMeshes;
    int selectedRefIndex = -1;

    QSet<qint64> loadedCells;
    qint32 streamCenterX = 0, streamCenterY = 0;
    int streamRadius = 3;

    float rotationX = 30.0f, rotationY = -45.0f;
    float zoom = 500.0f;
    QVector3D targetPos{0, 0, 0};
    bool dragging = false;
    QPoint lastMousePos;
    bool keys[4] = {};

    int minCellX = -5, maxCellX = 5;
    int minCellY = -5, maxCellY = 5;
    float cellSize = 4096.0f;

    float waterHeight = 0.0f;
    bool waterEnabled = true;
    bool fogEnabled = true;
    QElapsedTimer sceneTimer;

    QComboBox* worldspaceCombo = nullptr;
    QLabel* infoLabel = nullptr;
    QListWidget* refList = nullptr;
    QCheckBox* waterCheck = nullptr;
    QCheckBox* fogCheck = nullptr;
    QDoubleSpinBox* waterHeightSpin = nullptr;
    QTimer* keyTimer = nullptr;
};

#endif
