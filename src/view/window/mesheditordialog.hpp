#ifndef MESHEDITORDIALOG_HPP
#define MESHEDITORDIALOG_HPP

#include <QDialog>
#include <QTableWidget>

namespace Nif {
class NifParser;
}

class NifViewportWidget;

class MeshEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MeshEditorDialog(Nif::NifParser* parser,
                              NifViewportWidget* viewport,
                              QWidget* parent = nullptr);

private slots:
    void applyTransform();
    void loadVertices();
    void applyVertexEdits();
    void addVertex();
    void removeSelected();
    void onShapeSelected(int index);
    void pickColor();
    void browseTexture();
    void exportTexture();
    void applyMaterial();
    void loadFaces();
    void addFace();
    void removeSelectedFaces();
    void applyFaceEdits();
    void openTextureEditor();
    void extrudeSelectedFace();
    void bevelSelectedFace();

private:
    Nif::NifParser* mParser;
    NifViewportWidget* mViewport;
    QTableWidget* mTable;
    QTableWidget* mFaceTable;
    QMap<int, QColor> mShapeColors;
    QMap<int, QString> mShapeTextures;
    QColor mCurrentColor;
};

#endif // MESHEDITORDIALOG_HPP
