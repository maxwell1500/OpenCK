#include "previewwindowdock.hpp"
#include "nifviewportwidget.hpp"
#include "../../model/world/data.hpp"
#include "logger.hpp"

#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFileInfo>

PreviewWindowDock::PreviewWindowDock(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

PreviewWindowDock::~PreviewWindowDock() = default;

void PreviewWindowDock::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(2);

    auto* headerLayout = new QHBoxLayout();
    headerLayout->setContentsMargins(4, 2, 4, 2);

    mInfoLabel = new QLabel(tr("No object selected"), this);
    mInfoLabel->setStyleSheet(QStringLiteral("font-weight: bold;"));
    headerLayout->addWidget(mInfoLabel, 1);

    mResetCameraBtn = new QPushButton(tr("Reset Camera"), this);
    mResetCameraBtn->setToolTip(tr("Center and frame object in viewport (Zoom to Fit)"));
    headerLayout->addWidget(mResetCameraBtn);

    mLoadFileBtn = new QPushButton(tr("Load NIF..."), this);
    mLoadFileBtn->setToolTip(tr("Preview an arbitrary NIF file from disk"));
    headerLayout->addWidget(mLoadFileBtn);

    mClearBtn = new QPushButton(tr("Clear"), this);
    mClearBtn->setToolTip(tr("Clear the preview"));
    headerLayout->addWidget(mClearBtn);

    mainLayout->addLayout(headerLayout);

    mViewport = new NifViewportWidget(this);
    mViewport->setObjectName(QStringLiteral("PreviewWindowViewport"));
    mainLayout->addWidget(mViewport, 1);

    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->setContentsMargins(4, 2, 4, 2);

    mGroundPlaneCheck = new QCheckBox(tr("Ground plane"), this);
    mGroundPlaneCheck->setToolTip(tr("Show or hide the ground grid plane"));
    controlsLayout->addWidget(mGroundPlaneCheck);

    auto* heightLabel = new QLabel(tr("Height:"), this);
    controlsLayout->addWidget(heightLabel);

    mGroundHeightSpin = new QDoubleSpinBox(this);
    mGroundHeightSpin->setRange(-10000.0, 10000.0);
    mGroundHeightSpin->setSingleStep(10.0);
    mGroundHeightSpin->setValue(0.0);
    mGroundHeightSpin->setSuffix(tr(" u"));
    mGroundHeightSpin->setToolTip(tr("Adjust elevation of the ground plane"));
    controlsLayout->addWidget(mGroundHeightSpin);

    controlsLayout->addStretch();
    mainLayout->addLayout(controlsLayout);

    connect(mGroundPlaneCheck, &QCheckBox::toggled, this, [this](bool checked) {
        if (mViewport) mViewport->setGridEnabled(checked);
    });
    connect(mGroundHeightSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
        if (mViewport) mViewport->setGridHeight(static_cast<float>(val));
    });

    connect(mResetCameraBtn, &QPushButton::clicked, this, &PreviewWindowDock::onResetCamera);
    connect(mLoadFileBtn, &QPushButton::clicked, this, &PreviewWindowDock::onLoadFileClicked);
    connect(mClearBtn, &QPushButton::clicked, this, &PreviewWindowDock::clear);
}

void PreviewWindowDock::setDataDir(const QString& dataDir)
{
    mDataDir = dataDir;
    if (mViewport)
        mViewport->setDataDir(dataDir);
}

void PreviewWindowDock::configureData(Data* data)
{
    if (!data) return;
    setDataDir(data->getPaths().dataDir.absolutePath());
}

void PreviewWindowDock::previewRecord(const QString& editorId, quint32 formId,
                                      const QString& modelPath, const QString& resolvedNifPath,
                                      CkId::Type type)
{
    mCurrentEditorId = editorId;
    mCurrentFormId = formId;
    mCurrentModelPath = modelPath;
    mCurrentResolvedPath = resolvedNifPath;
    mCurrentType = type;

    updateInfoLabel();

    if (!mViewport)
        return;

    mViewport->setCellReferences({});

    bool loaded = false;
    if (!resolvedNifPath.isEmpty() && QFileInfo::exists(resolvedNifPath))
    {
        mViewport->loadNif(resolvedNifPath);
        loaded = true;
    }
    else if (!modelPath.isEmpty() && !mDataDir.isEmpty())
    {
        QString candidate = mDataDir + QLatin1Char('/') + modelPath;
        if (QFileInfo::exists(candidate))
        {
            mViewport->loadNif(candidate);
            loaded = true;
        }
    }

    if (!loaded)
    {
        PrimitiveMeshGenerator::Type prim = PrimitiveMeshGenerator::Type::Cube;
        if (type == CkId::Type_Weap_) prim = PrimitiveMeshGenerator::Type::Cylinder;
        else if (type == CkId::Type_Spel_ || type == CkId::Type_Ligh_) prim = PrimitiveMeshGenerator::Type::Sphere;
        else if (type == CkId::Type_Acti_ || type == CkId::Type_Door_) prim = PrimitiveMeshGenerator::Type::Cylinder;
        else if (type == CkId::Type_Npc_ || type == CkId::Type_Crea_) prim = PrimitiveMeshGenerator::Type::Cylinder;
        mViewport->showPreviewPrimitive(prim, 50.0f);
    }

    mViewport->zoomToFit();
    emit previewUpdated(editorId, formId);
}

void PreviewWindowDock::loadNif(const QString& filePath, const QString& title)
{
    mCurrentEditorId = title.isEmpty() ? QFileInfo(filePath).fileName() : title;
    mCurrentFormId = 0;
    mCurrentModelPath = filePath;
    mCurrentResolvedPath = filePath;
    mCurrentType = CkId::Type_None;

    updateInfoLabel();

    if (mViewport && QFileInfo::exists(filePath))
    {
        mViewport->setCellReferences({});
        mViewport->loadNif(filePath);
        mViewport->zoomToFit();
        emit previewUpdated(mCurrentEditorId, 0);
    }
}

void PreviewWindowDock::showPreviewPrimitive(PrimitiveMeshGenerator::Type type, float size, const QString& title)
{
    mCurrentEditorId = title;
    mCurrentFormId = 0;
    mCurrentModelPath.clear();
    mCurrentResolvedPath.clear();
    mCurrentType = CkId::Type_None;

    updateInfoLabel();

    if (mViewport)
    {
        mViewport->setCellReferences({});
        mViewport->showPreviewPrimitive(type, size);
        mViewport->zoomToFit();
        emit previewUpdated(title, 0);
    }
}

void PreviewWindowDock::clear()
{
    mCurrentEditorId.clear();
    mCurrentFormId = 0;
    mCurrentModelPath.clear();
    mCurrentResolvedPath.clear();
    mCurrentType = CkId::Type_None;

    if (mViewport)
    {
        mViewport->clear();
        mViewport->clearPreviewPrimitive();
    }

    updateInfoLabel();
}

void PreviewWindowDock::zoomToFit()
{
    if (mViewport)
        mViewport->zoomToFit();
}

void PreviewWindowDock::onResetCamera()
{
    zoomToFit();
}

bool PreviewWindowDock::isGroundPlaneEnabled() const
{
    return mViewport ? mViewport->isGridEnabled() : false;
}

void PreviewWindowDock::setGroundPlaneEnabled(bool enabled)
{
    if (mGroundPlaneCheck && mGroundPlaneCheck->isChecked() != enabled)
        mGroundPlaneCheck->setChecked(enabled);
    if (mViewport)
        mViewport->setGridEnabled(enabled);
}

float PreviewWindowDock::groundPlaneHeight() const
{
    return mViewport ? mViewport->gridHeight() : 0.0f;
}

void PreviewWindowDock::setGroundPlaneHeight(float height)
{
    if (mGroundHeightSpin && !qFuzzyCompare(static_cast<float>(mGroundHeightSpin->value()), height))
        mGroundHeightSpin->setValue(static_cast<double>(height));
    if (mViewport)
        mViewport->setGridHeight(height);
}

int PreviewWindowDock::triangleCount() const
{
    return mViewport ? mViewport->triangleCount() : 0;
}

void PreviewWindowDock::onLoadFileClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Preview NIF File"),
        mDataDir, tr("NIF Files (*.nif);;All Files (*)"));
    if (!filePath.isEmpty())
    {
        loadNif(filePath);
        LOG_INFO(QStringLiteral("Preview NIF loaded: %1").arg(filePath));
    }
}

QString PreviewWindowDock::infoText() const
{
    return mInfoLabel ? mInfoLabel->text() : QString();
}

void PreviewWindowDock::updateInfoLabel()
{
    if (!mInfoLabel)
        return;

    if (mCurrentEditorId.isEmpty() && mCurrentFormId == 0)
    {
        mInfoLabel->setText(tr("No object selected"));
        return;
    }

    int tris = mViewport ? mViewport->triangleCount() : 0;
    QString text;
    if (mCurrentFormId != 0)
    {
        QString formIdHex = QStringLiteral("%1").arg(mCurrentFormId, 8, 16, QLatin1Char('0')).toUpper();
        text = QStringLiteral("Preview Object: '%1' (%2) - %3 triangles, M #: %4")
            .arg(mCurrentEditorId, formIdHex)
            .arg(tris)
            .arg(!mCurrentModelPath.isEmpty() ? mCurrentModelPath : QStringLiteral("none"));
    }
    else
    {
        text = QStringLiteral("Preview File: \"%1\" - %2 triangles, M #: %3")
            .arg(mCurrentEditorId)
            .arg(tris)
            .arg(!mCurrentModelPath.isEmpty() ? mCurrentModelPath : QStringLiteral("none"));
    }

    mInfoLabel->setText(text);
}
