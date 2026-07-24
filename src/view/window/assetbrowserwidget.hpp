#ifndef ASSETBROWSERWIDGET_HPP
#define ASSETBROWSERWIDGET_HPP

#include <QWidget>
#include <QTreeView>
#include <QListView>
#include <QFileSystemModel>
#include <QComboBox>
#include <QLineEdit>
#include <QSplitter>
#include <QLabel>
#include <QMap>
#include <QPoint>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class QOpenGLContext;
class QOffscreenSurface;
class QOpenGLShaderProgram;

class AssetBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AssetBrowserWidget(QWidget* parent = nullptr);
    ~AssetBrowserWidget();

    void setDataDirectories(const QStringList& dirs);

signals:
    void assetSelected(const QString& filePath, const QString& fileType);
    void assetDoubleClicked(const QString& filePath);
    void assetDragged(const QString& filePath);

private slots:
    void onDirectoryChanged(const QModelIndex& index);
    void onFileSelected(const QModelIndex& index);
    void onFileDoubleClicked(const QModelIndex& index);
    void onFilterChanged(int index);
    void onSearchTextChanged(const QString& text);
    void onFileContextMenu(const QPoint& pos);

private:
    void setupUI();
    void setupConnections();
    bool eventFilter(QObject* obj, QEvent* event) override;
    QString fileTypeForExtension(const QString& ext) const;
    void updatePreview(const QString& filePath);
    void clearPreview();
    QPixmap generateNifThumbnail(const QString& filePath);
    QStringList scanDependencies(const QString& filePath);
    void initPreviewGL();

    QSplitter* mMainSplitter;
    QTreeView* mDirTree;
    QListView* mFileList;
    QFileSystemModel* mDirModel;
    QFileSystemModel* mFileModel;
    QComboBox* mFilterCombo;
    QLineEdit* mSearchEdit;
    QLabel* mPreviewLabel;
    QLabel* mPreviewImage;
    QLabel* mFileInfoLabel;

    QStringList mDataDirs;
    enum class FilterMode { All, Models, Textures, Sounds };
    FilterMode mCurrentFilter;
    QStringList mModelExts;
    QStringList mTextureExts;
    QStringList mSoundExts;

    QMap<QString, QStringList> mDependencies;
    QMap<QString, QPixmap> mThumbnailCache;

    QOpenGLContext* mPreviewGLContext;
    QOffscreenSurface* mPreviewOffscreen;
    QOpenGLShaderProgram* mPreviewShader;
    QOpenGLBuffer* mPreviewVBO;
    QOpenGLBuffer* mPreviewIBO;
    QOpenGLVertexArrayObject* mPreviewVAO;
    bool mPreviewGLInitialized;
    QPoint mDragStartPos;
};

#endif // ASSETBROWSERWIDGET_HPP
