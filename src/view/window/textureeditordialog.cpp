#include "textureeditordialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QToolButton>
#include <QSpinBox>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QComboBox>
#include <QColorDialog>

#include "texturepaintwidget.hpp"
#include "nifviewportwidget.hpp"
#include "../../libs/files/nif/nifparser.hpp"
#include "../../libs/files/nif/tgaencoder.hpp"
#include "../../libs/files/nif/ddsencoder.hpp"
#include "logger.hpp"

TextureEditorDialog::TextureEditorDialog(Nif::NifParser* parser,
                                         NifViewportWidget* viewport,
                                         int shapeIndex,
                                         QWidget* parent)
    : QDialog(parent)
    , mParser(parser)
    , mViewport(viewport)
    , mShapeIndex(shapeIndex)
{
    setWindowTitle(tr("Texture Editor"));
    setMinimumSize(700, 600);

    auto* mainLayout = new QVBoxLayout(this);

    // --- Toolbar ---
    auto* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));

    // Brush tool
    auto* brushBtn = toolbar->addAction(tr("Brush"));
    brushBtn->setCheckable(true);
    brushBtn->setChecked(true);

    // Eraser tool
    auto* eraserBtn = toolbar->addAction(tr("Eraser"));
    eraserBtn->setCheckable(true);

    // Color picker button
    auto* colorAction = toolbar->addAction(tr("Color"));
    colorAction->setToolTip(tr("Pick color"));

    toolbar->addSeparator();

    // Brush size
    auto* sizeLabel = new QLabel(tr("Size:"));
    toolbar->addWidget(sizeLabel);
    auto* sizeSpin = new QSpinBox();
    sizeSpin->setRange(1, 100);
    sizeSpin->setValue(8);
    sizeSpin->setObjectName("sizeSpin");
    toolbar->addWidget(sizeSpin);

    toolbar->addSeparator();

    // Filter buttons
    auto* blurBtn = toolbar->addAction(tr("Blur"));
    auto* sharpenBtn = toolbar->addAction(tr("Sharpen"));
    auto* brightnessBtn = toolbar->addAction(tr("Brightness+"));
    auto* brightnessMinusBtn = toolbar->addAction(tr("Brightness-"));
    auto* contrastBtn = toolbar->addAction(tr("Contrast+"));
    auto* contrastMinusBtn = toolbar->addAction(tr("Contrast-"));

    toolbar->addSeparator();

    // Mipmap info
    auto* mipLabel = toolbar->addAction(tr("Mipmaps"));
    mipLabel->setToolTip(tr("Generate mipmaps and export as DDS"));

    toolbar->addSeparator();

    // Save/Export
    auto* saveBtn = toolbar->addAction(tr("Save"));
    auto* exportTgaBtn = toolbar->addAction(tr("Export TGA"));
    auto* exportDdsBtn = toolbar->addAction(tr("Export DDS"));

    mainLayout->addWidget(toolbar);

    // --- Paint widget ---
    m_paintWidget = new TexturePaintWidget(this);

    // Load current shape texture
    if (mParser && mParser->getRoot() && mShapeIndex >= 0
        && mShapeIndex < mParser->getRoot()->shapes.size()) {
        const auto& shape = mParser->getRoot()->shapes[mShapeIndex];
        if (!shape.texture.isEmpty()) {
            QImage tex = NifViewportWidget::loadTextureImage(shape.texture);
            if (!tex.isNull()) {
                m_paintWidget->setImage(tex);
            }
        }
    }

    mainLayout->addWidget(m_paintWidget, 1);

    // --- Connections ---
    connect(brushBtn, &QAction::triggered, this, [this]() {
        m_paintWidget->m_tool = TexturePaintWidget::Brush;
        m_paintWidget->m_brushBtn->setChecked(true);
    });
    connect(eraserBtn, &QAction::triggered, this, [this]() {
        m_paintWidget->m_tool = TexturePaintWidget::Eraser;
        m_paintWidget->m_eraserBtn->setChecked(true);
    });
    connect(colorAction, &QAction::triggered, this, [this]() {
        QColor c = QColorDialog::getColor(m_paintWidget->m_brushColor, this, tr("Pick Color"));
        if (c.isValid()) {
            m_paintWidget->m_brushColor = c;
            if (m_paintWidget->m_colorBtn) {
                m_paintWidget->m_colorBtn->setStyleSheet(
                    QString("background-color: %1").arg(c.name()));
            }
        }
    });
    connect(sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int v) {
        m_paintWidget->m_brushSize = v;
    });

    connect(blurBtn, &QAction::triggered, this, [this]() { m_paintWidget->blur(3); });
    connect(sharpenBtn, &QAction::triggered, this, [this]() { m_paintWidget->sharpen(50); });
    connect(brightnessBtn, &QAction::triggered, this, [this]() { m_paintWidget->brightness(10); });
    connect(brightnessMinusBtn, &QAction::triggered, this, [this]() { m_paintWidget->brightness(-10); });
    connect(contrastBtn, &QAction::triggered, this, [this]() { m_paintWidget->contrast(10); });
    connect(contrastMinusBtn, &QAction::triggered, this, [this]() { m_paintWidget->contrast(-10); });

    connect(mipLabel, &QAction::triggered, this, [this]() {
        QVector<QImage> mips = TexturePaintWidget::generateMipmaps(m_paintWidget->getImage());
        QMessageBox::information(this, tr("Mipmaps"),
            tr("Generated %1 mipmap levels.\nUse Export DDS to save with mipmaps.").arg(mips.size()));
    });

    connect(saveBtn, &QAction::triggered, this, &TextureEditorDialog::saveTexture);
    connect(exportTgaBtn, &QAction::triggered, this, &TextureEditorDialog::exportAsTga);
    connect(exportDdsBtn, &QAction::triggered, this, &TextureEditorDialog::exportAsDds);

    // --- Dialog buttons ---
    auto* btnLayout = new QHBoxLayout();
    auto* closeBtn = new QPushButton(tr("Close"));
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void TextureEditorDialog::saveTexture()
{
    if (!mParser || !mParser->getRoot() || mShapeIndex < 0
        || mShapeIndex >= mParser->getRoot()->shapes.size()) return;

    const auto& shape = mParser->getRoot()->shapes[mShapeIndex];
    QString path = shape.texture;
    if (path.isEmpty()) {
        path = QFileDialog::getSaveFileName(this, tr("Save Texture"),
            QString(), tr("PNG Images (*.png);;TGA Images (*.tga)"));
        if (path.isEmpty()) return;
    }

    if (!m_paintWidget->saveImage(path)) {
        QMessageBox::critical(this, tr("Save Failed"),
            tr("Failed to save texture to: %1").arg(path));
        return;
    }
    LOG_INFO(QString("Texture saved: %1").arg(path));
}

void TextureEditorDialog::exportAsTga()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Export TGA"),
        QString(), tr("TGA Images (*.tga)"));
    if (path.isEmpty()) return;

    if (!TgaEncoder::encode(m_paintWidget->getImage(), path)) {
        QMessageBox::critical(this, tr("Export Failed"),
            tr("Failed to export TGA to: %1").arg(path));
        return;
    }
    LOG_INFO(QString("TGA exported: %1").arg(path));
}

void TextureEditorDialog::exportAsDds()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Export DDS"),
        QString(), tr("DDS Images (*.dds)"));
    if (path.isEmpty()) return;

    if (!DdsEncoder::encode(m_paintWidget->getImage(), path, 5)) {
        QMessageBox::critical(this, tr("Export Failed"),
            tr("Failed to export DDS to: %1").arg(path));
        return;
    }
    LOG_INFO(QString("DDS exported: %1").arg(path));
}
