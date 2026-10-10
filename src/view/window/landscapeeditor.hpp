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
class QCheckBox;

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

#include "../../../model/tools/brushdefinition.hpp"
#include "../../../model/tools/brushalphamask.hpp"
#include "../../../model/tools/terrainblock.hpp"

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
    double maxMaterialOpacity = 1.0;
    bool applySlopeInfluence = false;
    double slopeThreshold = 0.0;   // degrees; painting fades above this
    double slopeFalloff = 1.0;     // smoothness of the fade
    bool slopeInvert = false;      // paint below threshold instead

    // Returns a 0..1 multiplier for painting at the given slope angle
    // (degrees). With slope influence disabled the result is 1.0. With it
    // enabled, the layer fades out across [threshold, threshold+falloff]
    // (inverted: fades in).
    double slopeModifier(double slopeDegrees) const
    {
        if (!applySlopeInfluence)
            return 1.0;
        const double start = slopeThreshold;
        const double end = slopeThreshold + slopeFalloff;
        double t = 0.0;
        if (slopeDegrees <= start)
            t = 1.0;
        else if (slopeDegrees >= end)
            t = 0.0;
        else
            t = 1.0 - (slopeDegrees - start) / (end - start);
        return slopeInvert ? (1.0 - t) : t;
    }
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

    // Brush settings accessors
    int getBrushSize() const { return brushSize; }
    void setBrushSize(int size);
    int getBrushStrength() const { return brushStrength; }
    void setBrushStrength(int strength);
    int getActiveBrushIndex() const { return activeBrushIndex; }
    void setActiveBrushIndex(int index);
    const QVector<BrushDefinition>& getBrushes() const { return brushes; }

    // Terrain raycasting and brush projection
    // Intersects ray with 3D heightmap and sets hover/brush ring position
    bool raycastTerrain(const QVector3D& rayOrigin, const QVector3D& rayDir, int& outGridX, int& outGridY, float* outElevation = nullptr) const;
    QPoint screenToTerrain(const QPoint& screenPos) const;
    bool isBrushRingVisible() const { return mBrushRingVisible; }
    QPoint brushRingPosition() const { return mBrushRingPos; }

    // Landscape texture painting
    enum class PaintMode { Sculpt, PaintTexture };
    PaintMode getPaintMode() const { return mPaintMode; }
    void setPaintMode(PaintMode mode) { mPaintMode = mode; }
    int getActiveTextureLayerIndex() const { return mActiveTextureLayerIndex; }
    void setActiveTextureLayerIndex(int idx) { mActiveTextureLayerIndex = idx; }
    void paintTexture(int x, int y);

    // Heightmap read access
    const QVector<float>& getHeightmap() const { return heightmap; }
    int getTerrainSize() const { return terrainSize; }
    float getMinHeight() const { return minHeight; }
    float getMaxHeight() const { return maxHeight; }
    void applyBrush(int x, int y);
    float getHeightAt(int x, int y) const;
    void setHeightAt(int x, int y, float height);
    void saveHeightmap(LandRecord& rec);
    void applyHeightmap();

    // Saves the current edited heightmap into the loaded LandRecord
    // (creating one for the current cell if none exists). Returns the
    // LandRecord that was written, or nullptr if no cell is loaded.
    LandRecord* saveLandscapeToRecord();

    // Writes the water-plane height (XCLW) from the Water tab back into
    // the loaded CellRecord and marks it modified in the collection.
    void saveWaterToCell();
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void leaveEvent(QEvent* event) override;
private slots:
    void onBrushSizeChanged(int size);
    void onBrushStrengthChanged(int strength);
    void onBrushTypeChanged(int type);
    void onBrushSelected(int index);
    void onHeightLimitChanged(int height);
    void onSaveClicked();
    void onLoadClicked();
    void onCopyHeightmapClicked();
    void onPasteHeightmapClicked();
    void onCutRegionClicked();
    void onPasteRegionClicked();
    void onImportR32Clicked();
    void onExportR32Clicked();
    void onLoadBrushesClicked();
    void onLoadMaskClicked();
    void onClearMaskClicked();

    void onAddLayer();
    void onRemoveLayer();
    void onMoveLayerUp();
    void onMoveLayerDown();
    void onAutoPaint();

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
    int brushType; // 0=Raise, 1=Lower, 2=Smooth, 3=Flat (legacy)
    int heightLimit;
    QVector<BrushDefinition> brushes;
    int activeBrushIndex;
    BrushAlphaMask brushMask;
    QComboBox* brushCombo;
    QPushButton* loadBrushesButton;
    QPushButton* loadMaskButton;
    QPushButton* clearMaskButton;

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
    QPushButton* cutRegionButton;
    QPushButton* pasteRegionButton;
    QLabel* statusLabel;

    bool dragging;
    QPoint lastMousePos;
    QVector<float> originalHeightmap;
    bool hasOriginalState;
    QRect strokeDirtyRect;

    // Brush ring visual projection state
    bool mBrushRingVisible = false;
    QPoint mBrushRingPos{ -1, -1 };
    QOpenGLBuffer ringVbo;
    QOpenGLShaderProgram* ringShaderProgram = nullptr;
    void renderBrushRing();

    // Paint mode & texture quad painting state
    PaintMode mPaintMode = PaintMode::Sculpt;
    int mActiveTextureLayerIndex = 0;
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

    // Terrain-block cutting clipboard
    TerrainBlock::Block mCutRegion;
    int cutBlockGrid;              // grid alignment used when cutting

    // Property tabs
    QTabWidget* propertyTabWidget;

    // Texture Layers tab widgets
    QTableWidget* textureLayerTable;
    QPushButton* addLayerButton;
    QPushButton* removeLayerButton;
    QPushButton* moveLayerUpButton;
    QPushButton* moveLayerDownButton;
    QPushButton* autoPaintButton;

    // Vegetation tab widgets
    QTableWidget* vegetationTable;
    QPushButton* addPlantButton;
    QPushButton* removePlantButton;

    // Water tab widgets
    QCheckBox* waterEnabledCheckBox;
    QDoubleSpinBox* waterHeightSpinBox;
    QComboBox* waterTypeCombo;
    QDoubleSpinBox* depthAttenuationSpinBox;
    QDoubleSpinBox* reflectionAmountSpinBox;
    QPushButton* applyWaterButton;
};

#endif // LANDSCAPEEDITOR_HPP
