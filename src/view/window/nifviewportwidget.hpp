#ifndef NIFVIEWPORTWIDGET_HPP
#define NIFVIEWPORTWIDGET_HPP

#include <QWidget>
#include <memory>
#include <QMap>
#include <QMatrix4x4>
#include <QVector3D>

#include "gizmomath.hpp"

class QOpenGLWidget;
class QOpenGLFunctions;
class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QSplitter;
class QSlider;
class QComboBox;
class QPushButton;
class QTimer;
class QKeyEvent;
class QSpinBox;
class QDoubleSpinBox;
class QActionGroup;
class QAction;

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLTexture>

class NifAnimationState;
struct ParticleSystemData;
class NifAnimation;
class ParticleSystem;
class ParticleRenderer;

struct ViewportCellRef {
    QVector3D position;
    bool enabled = true;
    int dataIndex = -1;                              // index into Data refrCollection, -1 if not backed
    float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;     // radians (matches RefrRecord on-disk)
    float scale = 1.0f;
};

namespace Nif {
class NifParser;
struct Node;
struct TriShape;
struct CollisionShape;
}

struct OverlayVertex {
    QVector3D position;
    QVector3D color;
};

struct OverlayVBO {
    QOpenGLBuffer buffer{QOpenGLBuffer::VertexBuffer};
    int vertexCount = 0;
    bool dirty = true;
    unsigned int primitiveMode = 0x0004; // GL_TRIANGLES

    ~OverlayVBO() { clear(); }
    void build(const QVector<OverlayVertex>& vertices, unsigned int mode = 0x0004);
    void draw(QOpenGLShaderProgram* shader, const QMatrix4x4& mvp);
    void clear();
};

class NifViewportWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NifViewportWidget(QWidget* parent = nullptr);
    ~NifViewportWidget();

    void loadNif(const QString& fileName);
    void clear();

    bool saveNif(const QString& fileName);
    void exportNif();
    static QImage loadTextureImage(const QString& path);
    void translateMesh(float dx, float dy, float dz);
    void scaleMesh(float factor);
    void refreshMesh();
    int vertexCount() const;
    QString currentFile() const { return currentNifFile; }

signals:
    void refSelected(int dataIndex);
    void refHovered(int dataIndex);
    void refTransformPreview(int dataIndex, const QVector3D& position,
                             const QVector3D& rotation, float scale);
    void refTransformCommitted(int dataIndex, const QVector3D& position,
                               const QVector3D& rotation, float scale);

public:
    void setSelectedRefIndex(int index);
    void setSelectedRefByDataIndex(int dataIndex);
    int selectedRefIndex() const { return mSelectedRefIndex; }
    void focusOnReference(const QVector3D& gameUnitsPos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

public:
    void updateCamera();
    void onFilterChanged(int index);
    void toggleHierarchy();

    const Nif::NifParser* getNifParser() const;

    void setNavmeshData(const QVector<QVector3D>& triangles);
    void setPathData(const QVector<QVector3D>& waypoints);
    void highlightNavmeshTriangle(int index);

    void setCellReferences(const QVector<ViewportCellRef>& refs);

    void updateParticleSystem(const ParticleSystemData* data);

private:
    void setupOpenGL();
    void buildMesh();
    void buildMeshFromNode(Nif::Node* node, const QMatrix4x4& parentTransform);
    void renderMesh();
    void setupShaders();
    void clearTextures();
    void ensureTexture(int index, const QString& path);
    void ensureDefaultTexture();
    void applyFilterToTexture(QOpenGLTexture& tex);

    void buildHierarchyTree();
    void populateTreeItem(QTreeWidgetItem* parentItem, Nif::Node* node, const QMatrix4x4& parentTransform);
    void updateNodeMetadata();
    void drawNodeAxis(const QMatrix4x4& modelView);

    QOpenGLWidget* glWidget;
    std::unique_ptr<Nif::NifParser> nifParser;

    QString currentNifFile;
    QString nifFileDir;

    QOpenGLShaderProgram* shaderProgram;
    QOpenGLShaderProgram* m_overlayShader = nullptr;
    QOpenGLBuffer vbo;
    QOpenGLBuffer ibo;
    QOpenGLVertexArrayObject vao;

    QVector<QVector3D> vertices;
    QVector<QVector3D> normals;
    QVector<QVector2D> uvs;
    QVector<unsigned int> indices;
    bool meshBuilt;
    bool m_meshDirty = true;

    QVector<QPair<int, int>> shapeIndexRanges;
    QVector<QColor> shapeBaseColors;
    QVector<QString> shapeTexturePaths;
    QVector<QOpenGLTexture*> shapeTextures;
    QVector<int> shapeAlphaModes;
    QVector<float> shapeOpacity;
    QVector<QVector3D> shapeSpecularColors;
    QVector<float> shapeSpecularExponents;
    QVector<QVector3D> shapeEmissionColors;
    QVector<QPair<QVector3D, QVector3D>> shapeBounds;
    QOpenGLTexture* defaultTexture = nullptr;
    bool texturesBuilt;

    float rotationX, rotationY;
    float zoom;
    QVector3D cameraPos;
    bool dragging;
    QPoint lastMousePos;

    int selectedShape;
    bool highlightEnabled;

    enum class TexFilter { Linear, Nearest, Mipmap };
    TexFilter currentFilter = TexFilter::Linear;
    bool wireframeMode = false;
    bool gridEnabled = false;
    bool axisEnabled = false;
    bool boundsEnabled = false;

    bool collisionEnabled = false;

    bool navmeshEnabled = false;
    QVector<QVector3D> navmeshTriangles;
    int mHighlightedTriangle = -1;

    bool pathEnabled = false;
    QVector<QVector3D> pathWaypoints;

    bool cellGridEnabled = false;
    QVector3D cellOrigin;
    QVector<ViewportCellRef> cellReferences;

    OverlayVBO m_navmeshVBO;
    OverlayVBO m_gridVBO;
    OverlayVBO m_bboxVBO;
    OverlayVBO m_pathVBO;
    OverlayVBO m_highlightTriVBO;
    OverlayVBO m_collisionVBO;
    OverlayVBO m_axisVBO_R, m_axisVBO_G, m_axisVBO_B;
    OverlayVBO m_cellGridVBO;
    OverlayVBO m_cellRefVBO;
    OverlayVBO m_nodeAxisVBO_R, m_nodeAxisVBO_G, m_nodeAxisVBO_B;

    QSplitter* mainSplitter;
    QTreeWidget* hierarchyTree;
    QLabel* nodeInfoLabel;
    bool hierarchyVisible;

    Nif::Node* selectedNode;
    QMap<Nif::Node*, QMatrix4x4> nodeCumulativeTransforms;
    QVector<Nif::Node*> shapeOwnerNode;

    NifAnimationState* animState;
    NifAnimation* nifAnimData;
    QSlider* animTimeline;
    QLabel* animTimeLabel;
    QLabel* animClipLabel;
    QComboBox* animSpeedCombo;
    QComboBox* animClipCombo;
    QPushButton* animPlayBtn;
    QPushButton* animStopBtn;
    QPushButton* animLoopBtn;
    QWidget* animToolbar;
    bool animDraggingSlider;

    QComboBox* animBlendClipCombo = nullptr;
    QSlider* animBlendWeightSlider = nullptr;
    QLabel* animBlendWeightLabel = nullptr;
    QWidget* animBlendToolbar = nullptr;

    void setupAnimToolbar();
    void setupParticleToolbar();
    QString formatAnimTime(float seconds) const;
    void initAnimationState();
    void initParticleSystems();
    void applyAnimationFrame();

    QVector<QVector3D> restVertices;
    QVector<QVector3D> restNormals;

    ParticleSystem* m_particleSystem = nullptr;
    ParticleRenderer* m_particleRenderer = nullptr;
    QPushButton* m_particlePlayBtn = nullptr;
    QPushButton* m_particleStopBtn = nullptr;
    QWidget* m_particleToolbar = nullptr;

    enum class EditMode { Select, Move, Rotate, Scale };
    EditMode mEditMode = EditMode::Select;
    QActionGroup* mEditModeGroup = nullptr;
    QAction* mActionSelect = nullptr;
    QAction* mActionMove = nullptr;
    QAction* mActionRotate = nullptr;
    QAction* mActionScale = nullptr;
    QAction* mActionSnapGrid = nullptr;
    QAction* mActionSnapAngle = nullptr;
    QSpinBox* mSnapAngleSpin = nullptr;
    QDoubleSpinBox* mSnapGridSpin = nullptr;
    bool mSnapToGrid = false;
    bool mSnapToAngle = false;
    int mSnapAngleIncrement = 15;
    double mSnapGridSize = 1.0;

    int mSelectedRefIndex = -1;
    int mHoverRefIndex = -1;
    bool mGizmoDragging = false;
    int mGizmoAxis = -1;                 // 0=X, 1=Y, 2=Z, -1=none
    int mHoverAxis = -1;
    QPoint mGizmoStartScreen;
    QPointF mGizmoStartValue;            // per-axis float at drag start
    QVector3D mGizmoStartPos;            // world-space ref position at drag start
    float mGizmoStartRotRad[3] = {0,0,0};
    float mGizmoStartScale = 1.0f;
    OverlayVBO m_translateGizmoVBO;
    OverlayVBO m_rotateGizmoVBO;
    OverlayVBO m_scaleGizmoVBO;

    void setEditMode(EditMode mode);
    void buildTranslateGizmo(float len, const QVector3D& origin, int highlightAxis);
    void buildRotateGizmo(float len, const QVector3D& origin, int highlightAxis);
    void buildScaleGizmo(float len, const QVector3D& origin, int highlightAxis);
    int pickGizmoAxis(const QPoint& pos, const gizmo::ViewTransform& t);
    int pickRefMarker(const QPoint& pos, const gizmo::ViewTransform& t);
    void applyGizmoDrag(const QPoint& currentPos);
    void commitGizmoDrag();
    gizmo::ViewTransform currentViewTransform() const;
};

#endif // NIFVIEWPORTWIDGET_HPP
