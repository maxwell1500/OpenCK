#ifndef PREVIEWWINDOWDOCK_HPP
#define PREVIEWWINDOWDOCK_HPP

#include <QWidget>
#include <QString>
#include "../../model/tools/primitivemeshgenerator.hpp"
#include "../../model/world/ckid.hpp"

class QLabel;
class QPushButton;
class QCheckBox;
class QDoubleSpinBox;
class NifViewportWidget;
class Data;

/// Dedicated 3D Preview Window (Creation Kit Dialog 315).
/// Hosts an independent NifViewportWidget to render selected object meshes
/// or category primitives, completely independent from the main Render Window.
class PreviewWindowDock : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewWindowDock(QWidget* parent = nullptr);
    ~PreviewWindowDock() override;

    NifViewportWidget* viewport() const { return mViewport; }

    void setDataDir(const QString& dataDir);
    void configureData(Data* data);

    void previewRecord(const QString& editorId, quint32 formId,
                       const QString& modelPath, const QString& resolvedNifPath,
                       CkId::Type type = CkId::Type_None);

    void loadNif(const QString& filePath, const QString& title = QString());
    void showPreviewPrimitive(PrimitiveMeshGenerator::Type type, float size = 50.0f, const QString& title = QString());
    void clear();
    void zoomToFit();

    QString currentEditorId() const { return mCurrentEditorId; }
    quint32 currentFormId() const { return mCurrentFormId; }
    QString currentModelPath() const { return mCurrentModelPath; }
    QString infoText() const;

    bool isGroundPlaneEnabled() const;
    void setGroundPlaneEnabled(bool enabled);
    float groundPlaneHeight() const;
    void setGroundPlaneHeight(float height);
    int triangleCount() const;

public slots:
    void onResetCamera();
    void onLoadFileClicked();

signals:
    void previewUpdated(const QString& editorId, quint32 formId);

private:
    void setupUI();
    void updateInfoLabel();

    NifViewportWidget* mViewport = nullptr;
    QLabel* mInfoLabel = nullptr;
    QPushButton* mResetCameraBtn = nullptr;
    QPushButton* mLoadFileBtn = nullptr;
    QPushButton* mClearBtn = nullptr;
    QCheckBox* mGroundPlaneCheck = nullptr;
    QDoubleSpinBox* mGroundHeightSpin = nullptr;

    QString mCurrentEditorId;
    quint32 mCurrentFormId = 0;
    QString mCurrentModelPath;
    QString mCurrentResolvedPath;
    CkId::Type mCurrentType = CkId::Type_None;
    QString mDataDir;
};

#endif // PREVIEWWINDOWDOCK_HPP
