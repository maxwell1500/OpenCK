#ifndef LANDSCAPEEDITOR_HPP
#define LANDSCAPEEDITOR_HPP

#include <QWidget>

class QOpenGLWidget;
class QSlider;
class QLabel;
class QPushButton;
class QSpinBox;
class QComboBox;
class QTabWidget;
class QTableWidget;
class QDoubleSpinBox;
class QLineEdit;
class QGroupBox;
class QOpenGLShaderProgram;

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class CellRecord;
class LandRecord;
class Data;
class UndoStack;
class BrushTool;

struct TextureLayer
{
    int index;
    QString texturePath;
    double opacity;
};

struct VegetationEntry
{
    QString formID;
    int density;
    double minHeight;
    double maxHeight;
};

class LandscapeEditor : public QWidget
{
    Q_OBJECT

public:
    explicit LandscapeEditor(QWidget* parent = nullptr);
    ~LandscapeEditor();

    void loadCell(CellRecord* cell);
    void clear();
    void setData(Data* data);
    void setUndoStack(UndoStack* stack);

    void saveHeightmap(LandRecord& rec);
    void applyHeightmap();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onBrushSizeChanged(int size);
    void onBrushStrengthChanged(int strength);
    void onBrushTypeChanged(int type);
    void onHeightLimitChanged(int height);
    void onSaveClicked();
    void onLoadClicked();
    void onCopyHeightmapClicked();
    void onPasteHeightmapClicked();

    void onAddLayer();
    void onRemoveLayer();
    void onMoveLayerUp();
    void onMoveLayerDown();

    void onAddPlant();
    void onRemovePlant();

    void onUndo();
    void onRedo();

private:
    void setupUI();
    void setupOpenGL();
    void loadHeightmap();
    void saveHeightmap();
    void renderTerrain();
    void applyBrush(int x, int y);
    float getHeightAt(int x, int y) const;
    void setHeightAt(int x, int y, float height);

    void setupTextureLayersTab(QWidget* tab);
    void setupVegetationTab(QWidget* tab);
    void setupWaterTab(QWidget* tab);

    void refreshTextureLayerTable();
    void refreshVegetationTable();

    void loadLand(LandRecord* land);
    void saveToLand(LandRecord* land);

    // Data model
    Data* mData;
    UndoStack* mUndoStack;

    // Brush tool (emits stroke signals for viewport repaint)
    BrushTool* mBrushTool;

    // Terrain data
    QVector<float> heightmap;
    int terrainSize;
    float minHeight;
    float maxHeight;

    // Brush settings
    int brushSize;
    int brushStrength;
    int brushType; // 0=Raise, 1=Lower, 2=Smooth, 3=Flat
    int heightLimit;

    // Cell data
    CellRecord* currentCell;

    // Land data
    LandRecord* currentLand;

    // OpenGL
    QOpenGLWidget* glWidget;
    QOpenGLShaderProgram* shaderProgram;
    QOpenGLBuffer vertexVbo;
    QOpenGLBuffer normalVbo;
    QOpenGLVertexArrayObject vao;
    float viewRotX, viewRotY;
    float viewZoom;

    // UI elements
    QSlider* brushSizeSlider;
    QSlider* brushStrengthSlider;
    QSlider* mHeightSlider;
    QComboBox* brushTypeCombo;
    QSpinBox* heightLimitSpin;
    QPushButton* saveButton;
    QPushButton* loadButton;
    QPushButton* mApplyButton;
    QPushButton* copyHeightmapButton;
    QPushButton* pasteHeightmapButton;
    QLabel* statusLabel;

    bool dragging;
    QPoint lastMousePos;
    QVector<float> originalHeightmap;
    bool hasOriginalState;

    // Texture layer data
    QVector<TextureLayer> textureLayers;

    // Vegetation data
    QVector<VegetationEntry> vegetationEntries;

    // Water data
    double waterHeight;
    int waterTypeIndex;
    double depthAttenuation;
    double reflectionAmount;

    // Clipboard
    QVector<float> copiedHeightmap;
    bool hasCopiedHeightmap;

    // Property tabs
    QTabWidget* propertyTabWidget;

    // Texture Layers tab widgets
    QTableWidget* textureLayerTable;
    QPushButton* addLayerButton;
    QPushButton* removeLayerButton;
    QPushButton* moveLayerUpButton;
    QPushButton* moveLayerDownButton;

    // Vegetation tab widgets
    QTableWidget* vegetationTable;
    QPushButton* addPlantButton;
    QPushButton* removePlantButton;

    // Water tab widgets
    QDoubleSpinBox* waterHeightSpinBox;
    QComboBox* waterTypeCombo;
    QDoubleSpinBox* depthAttenuationSpinBox;
    QDoubleSpinBox* reflectionAmountSpinBox;
};

#endif // LANDSCAPEEDITOR_HPP
