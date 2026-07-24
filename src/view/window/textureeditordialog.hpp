#ifndef TEXTUREEDITORDIALOG_HPP
#define TEXTUREEDITORDIALOG_HPP

#include <QDialog>

class TexturePaintWidget;
class NifViewportWidget;

namespace Nif { class NifParser; }

class TextureEditorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TextureEditorDialog(Nif::NifParser* parser,
                                 NifViewportWidget* viewport,
                                 int shapeIndex,
                                 QWidget* parent = nullptr);

private slots:
    void saveTexture();
    void exportAsTga();
    void exportAsDds();

private:
    Nif::NifParser* mParser;
    NifViewportWidget* mViewport;
    int mShapeIndex;
    TexturePaintWidget* m_paintWidget;
};

#endif // TEXTUREEDITORDIALOG_HPP
