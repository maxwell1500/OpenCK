#include "assetbrowserwidget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QPixmap>
#include <QImage>
#include <QImageReader>
#include <QDesktopServices>
#include <QUrl>
#include <QMimeData>
#include <QApplication>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QDrag>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QMatrix4x4>
#include <QDataStream>
#include <QProcess>
#include <QMessageBox>
#include <functional>

#include "nifviewportwidget.hpp"
#include "../../libs/files/nif/nifparser.hpp"
#include "../../model/tools/blenderlauncher.hpp"
#include "../../model/tools/iconrenderer.hpp"
#include "logger.hpp"

static double parseWavDuration(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) return -1.0;

    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::LittleEndian);

    char riffId[4];
    if (ds.readRawData(riffId, 4) != 4 || memcmp(riffId, "RIFF", 4) != 0) return -1.0;

    quint32 fileSize;
    ds >> fileSize;

    char waveId[4];
    if (ds.readRawData(waveId, 4) != 4 || memcmp(waveId, "WAVE", 4) != 0) return -1.0;

    quint16 channels = 0;
    quint32 sampleRate = 0;
    quint32 byteRate = 0;
    quint16 bitsPerSample = 0;
    quint32 dataSize = 0;

    while (!ds.atEnd()) {
        char chunkId[4];
        if (ds.readRawData(chunkId, 4) != 4) break;
        quint32 chunkSize;
        ds >> chunkSize;

        if (memcmp(chunkId, "fmt ", 4) == 0) {
            quint16 audioFormat;
            ds >> audioFormat;
            if (audioFormat != 1) break;
            ds >> channels >> sampleRate >> byteRate;
            quint16 blockAlign;
            ds >> blockAlign >> bitsPerSample;
            if (chunkSize > 16) {
                f.seek(f.pos() + static_cast<qint64>(chunkSize - 16));
            }
        } else if (memcmp(chunkId, "data", 4) == 0) {
            dataSize = chunkSize;
            break;
        } else {
            f.seek(f.pos() + static_cast<qint64>(chunkSize));
        }
    }

    if (byteRate > 0 && dataSize > 0 && sampleRate > 0) {
        return static_cast<double>(dataSize) / static_cast<double>(byteRate);
    }
    if (sampleRate > 0 && channels > 0 && bitsPerSample > 0 && dataSize > 0) {
        double bytesPerSample = static_cast<double>(bitsPerSample) / 8.0;
        double totalSamples = static_cast<double>(dataSize)
                              / (static_cast<double>(channels) * bytesPerSample);
        return totalSamples / static_cast<double>(sampleRate);
    }
    return -1.0;
}

AssetBrowserWidget::AssetBrowserWidget(QWidget* parent)
    : QWidget(parent)
    , mMainSplitter(nullptr)
    , mDirTree(nullptr)
    , mFileList(nullptr)
    , mDirModel(nullptr)
    , mFileModel(nullptr)
    , mFilterCombo(nullptr)
    , mSearchEdit(nullptr)
    , mPreviewLabel(nullptr)
    , mPreviewImage(nullptr)
    , mFileInfoLabel(nullptr)
    , mCurrentFilter(FilterMode::All)
    , mPreviewGLContext(nullptr)
    , mPreviewOffscreen(nullptr)
    , mPreviewShader(nullptr)
    , mPreviewVBO(nullptr)
    , mPreviewIBO(nullptr)
    , mPreviewVAO(nullptr)
    , mPreviewGLInitialized(false)
{
    mModelExts << "nif" << "obj" << "fbx";
    mTextureExts << "dds" << "tga" << "png" << "jpg" << "jpeg" << "bmp" << "gif";
    mSoundExts << "wav" << "mp3" << "ogg" << "flac" << "xwm";

    setupUI();
    setupConnections();
}

AssetBrowserWidget::~AssetBrowserWidget()
{
    if (mPreviewGLInitialized && mPreviewGLContext) {
        mPreviewGLContext->makeCurrent(mPreviewOffscreen);
        delete mPreviewShader;
        mPreviewShader = nullptr;
        delete mPreviewVBO;
        mPreviewVBO = nullptr;
        delete mPreviewIBO;
        mPreviewIBO = nullptr;
        delete mPreviewVAO;
        mPreviewVAO = nullptr;
        mPreviewGLContext->doneCurrent();
    }
    delete mPreviewGLContext;
    delete mPreviewOffscreen;
}

void AssetBrowserWidget::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    auto* toolbar = new QHBoxLayout();

    mSearchEdit = new QLineEdit();
    mSearchEdit->setPlaceholderText(tr("Search assets..."));
    mSearchEdit->setClearButtonEnabled(true);
    toolbar->addWidget(mSearchEdit, 1);

    mFilterCombo = new QComboBox();
    mFilterCombo->addItems({tr("All Assets"), tr("Models"), tr("Textures"), tr("Sounds")});
    toolbar->addWidget(mFilterCombo);

    mainLayout->addLayout(toolbar);

    mMainSplitter = new QSplitter(Qt::Vertical, this);

    mDirModel = new QFileSystemModel(this);
    mDirModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    mDirModel->setRootPath("");

    mDirTree = new QTreeView();
    mDirTree->setModel(mDirModel);
    mDirTree->setHeaderHidden(true);
    for (int i = 1; i < mDirModel->columnCount(); ++i) {
        mDirTree->hideColumn(i);
    }

    mFileModel = new QFileSystemModel(this);
    mFileModel->setFilter(QDir::Files | QDir::NoDotAndDotDot);
    mFileModel->setRootPath("");
    mFileModel->setNameFilterDisables(false);

    mFileList = new QListView();
    mFileList->setModel(mFileModel);
    mFileList->setViewMode(QListView::IconMode);
    mFileList->setIconSize(QSize(64, 64));
    mFileList->setGridSize(QSize(96, 96));
    mFileList->setResizeMode(QListView::Adjust);
    mFileList->setWrapping(true);
    mFileList->setDragEnabled(true);
    mFileList->setSelectionMode(QAbstractItemView::SingleSelection);
    mFileList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(mFileList, &QListView::customContextMenuRequested,
            this, &AssetBrowserWidget::onFileContextMenu);

    mFileList->viewport()->installEventFilter(this);

    auto* browserSplitter = new QSplitter(Qt::Horizontal);
    browserSplitter->addWidget(mDirTree);
    browserSplitter->addWidget(mFileList);
    browserSplitter->setStretchFactor(0, 1);
    browserSplitter->setStretchFactor(1, 3);

    mMainSplitter->addWidget(browserSplitter);

    auto* previewPanel = new QWidget();
    auto* previewLayout = new QVBoxLayout(previewPanel);
    previewLayout->setContentsMargins(4, 4, 4, 4);

    mPreviewLabel = new QLabel(tr("Preview"));
    mPreviewLabel->setStyleSheet("font-weight: bold;");
    previewLayout->addWidget(mPreviewLabel);

    mPreviewImage = new QLabel();
    mPreviewImage->setAlignment(Qt::AlignCenter);
    mPreviewImage->setMinimumHeight(120);
    mPreviewImage->setStyleSheet("background-color: #333; border: 1px solid #555;");
    mPreviewImage->setText(tr("Select an asset to preview"));
    previewLayout->addWidget(mPreviewImage, 1);

    mFileInfoLabel = new QLabel();
    mFileInfoLabel->setWordWrap(true);
    previewLayout->addWidget(mFileInfoLabel);

    auto* previewButtons = new QHBoxLayout();
    auto* playBtn = new QPushButton(tr("Open in Default App"));
    playBtn->setEnabled(false);
    playBtn->setObjectName("playBtn");
    previewButtons->addWidget(playBtn);
    previewButtons->addStretch();
    previewLayout->addLayout(previewButtons);

    connect(playBtn, &QPushButton::clicked, this, [this]() {
        auto index = mFileList->currentIndex();
        if (index.isValid()) {
            QString path = mFileModel->filePath(index);
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        }
    });

    mMainSplitter->addWidget(previewPanel);
    mMainSplitter->setStretchFactor(0, 4);
    mMainSplitter->setStretchFactor(1, 1);

    mainLayout->addWidget(mMainSplitter, 1);
}

void AssetBrowserWidget::setupConnections()
{
    connect(mDirTree->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AssetBrowserWidget::onDirectoryChanged);
    connect(mFileList->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AssetBrowserWidget::onFileSelected);
    connect(mFileList, &QListView::doubleClicked,
            this, &AssetBrowserWidget::onFileDoubleClicked);
    connect(mFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AssetBrowserWidget::onFilterChanged);
    connect(mSearchEdit, &QLineEdit::textChanged,
            this, &AssetBrowserWidget::onSearchTextChanged);
}

bool AssetBrowserWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == mFileList->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                mDragStartPos = me->pos();
            }
        }
        if (event->type() == QEvent::MouseMove) {
            QMouseEvent* me = static_cast<QMouseEvent*>(event);
            if (me->buttons() & Qt::LeftButton) {
                if ((me->pos() - mDragStartPos).manhattanLength()
                    >= QApplication::startDragDistance()) {
                    QModelIndex idx = mFileList->indexAt(mDragStartPos);
                    if (idx.isValid()) {
                        QString path = mFileModel->filePath(idx);
                        emit assetDragged(path);
                    }
                    // Clear so we only emit once per gesture
                    mDragStartPos = QPoint(-1, -1);
                }
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void AssetBrowserWidget::setDataDirectories(const QStringList& dirs)
{
    mDataDirs = dirs;
    if (!dirs.isEmpty()) {
        mDirTree->setRootIndex(mDirModel->index(dirs.first()));
        mFileList->setRootIndex(mFileModel->index(dirs.first()));
        mFileModel->setRootPath(dirs.first());
    }
}

void AssetBrowserWidget::onDirectoryChanged(const QModelIndex& index)
{
    QString path = mDirModel->filePath(index);
    mFileList->setRootIndex(mFileModel->index(path));
    mFileModel->setRootPath(path);
    onFilterChanged(static_cast<int>(mCurrentFilter));
}

void AssetBrowserWidget::onFileSelected(const QModelIndex& index)
{
    QString filePath = mFileModel->filePath(index);
    updatePreview(filePath);

    QString ext = fileTypeForExtension(QFileInfo(filePath).suffix().toLower());
    emit assetSelected(filePath, ext);
}

void AssetBrowserWidget::onFileDoubleClicked(const QModelIndex& index)
{
    QString filePath = mFileModel->filePath(index);
    emit assetDoubleClicked(filePath);
}

void AssetBrowserWidget::onFilterChanged(int filterIndex)
{
    mCurrentFilter = static_cast<FilterMode>(filterIndex);
    QStringList nameFilters;

    switch (mCurrentFilter) {
    case FilterMode::Models:
        nameFilters = mModelExts;
        break;
    case FilterMode::Textures:
        nameFilters = mTextureExts;
        break;
    case FilterMode::Sounds:
        nameFilters = mSoundExts;
        break;
    case FilterMode::All:
    default:
        nameFilters = QStringList();
        break;
    }

    if (!nameFilters.isEmpty()) {
        QStringList wildcards;
        for (const auto& ext : nameFilters) {
            wildcards << QString("*.%1").arg(ext);
        }
        mFileModel->setNameFilters(wildcards);
    } else {
        mFileModel->setNameFilters(QStringList());
    }

    onSearchTextChanged(mSearchEdit->text());
}

void AssetBrowserWidget::onSearchTextChanged(const QString& text)
{
    if (text.isEmpty()) {
        onFilterChanged(static_cast<int>(mCurrentFilter));
        return;
    }

    QStringList baseExts;
    switch (mCurrentFilter) {
    case FilterMode::Models: baseExts = mModelExts; break;
    case FilterMode::Textures: baseExts = mTextureExts; break;
    case FilterMode::Sounds: baseExts = mSoundExts; break;
    default:
        baseExts = QStringList() << mModelExts << mTextureExts << mSoundExts;
        break;
    }

    QStringList matchingFilters;
    for (const auto& ext : baseExts) {
        matchingFilters << QString("*%1*.%2").arg(text, ext);
    }

    if (!matchingFilters.isEmpty()) {
        mFileModel->setNameFilters(matchingFilters);
    }
}

void AssetBrowserWidget::updatePreview(const QString& filePath)
{
    QFileInfo info(filePath);
    QString ext = info.suffix().toLower();
    QString type = fileTypeForExtension(ext);

    QStringList deps = scanDependencies(filePath);
    mDependencies[filePath] = deps;

    QString infoText = tr("Name: %1\nSize: %2 KB\nType: %3")
        .arg(info.fileName())
        .arg(info.size() / 1024)
        .arg(type);

    if (mSoundExts.contains(ext)) {
        double dur = parseWavDuration(filePath);
        infoText += tr("\nFormat: %1").arg(ext.toUpper());
        if (dur > 0.0) {
            int mins = static_cast<int>(dur) / 60;
            int secs = static_cast<int>(dur) % 60;
            infoText += tr("\nDuration: %1:%2")
                .arg(mins, 2, 10, QChar('0'))
                .arg(secs, 2, 10, QChar('0'));
        }
    }

    if (!deps.isEmpty()) {
        infoText += tr("\nDepends on:");
        int count = 0;
        for (const QString& dep : deps) {
            if (count >= 8) {
                infoText += tr("\n  ... and %1 more").arg(deps.size() - 8);
                break;
            }
            QFileInfo depInfo(dep);
            infoText += tr("\n  %1").arg(depInfo.fileName());
            ++count;
        }
    } else if (ext == "dds") {
        infoText += tr("\nDepends on: (standalone texture)");
    }

    mFileInfoLabel->setText(infoText);

    if (mTextureExts.contains(ext)) {
        QImage img = NifViewportWidget::loadTextureImage(filePath);
        if (!img.isNull()) {
            QPixmap pix = QPixmap::fromImage(
                img.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            mPreviewImage->setPixmap(pix);
            mPreviewImage->setStyleSheet("background-color: transparent; border: 1px solid #555;");
        } else {
            QImage qimg(filePath);
            if (!qimg.isNull()) {
                QPixmap pix = QPixmap::fromImage(
                    qimg.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                mPreviewImage->setPixmap(pix);
                mPreviewImage->setStyleSheet("background-color: transparent; border: 1px solid #555;");
            }
        }
        return;
    }

    if (mSoundExts.contains(ext)) {
        mPreviewImage->setPixmap(QPixmap());
        QString audioLabel = tr("Audio File\n%1").arg(info.fileName());
        double dur = parseWavDuration(filePath);
        if (dur > 0.0) {
            int mins = static_cast<int>(dur) / 60;
            int secs = static_cast<int>(dur) % 60;
            audioLabel += tr("\n%1:%2")
                .arg(mins, 2, 10, QChar('0'))
                .arg(secs, 2, 10, QChar('0'));
        }
        mPreviewImage->setText(audioLabel);
        mPreviewImage->setStyleSheet(
            "background-color: #333; border: 1px solid #555; color: #aaa;");
        auto* playBtn = findChild<QPushButton*>("playBtn");
        if (playBtn) playBtn->setEnabled(true);
        return;
    }

    if (mModelExts.contains(ext)) {
        auto* playBtn = findChild<QPushButton*>("playBtn");
        if (playBtn) playBtn->setEnabled(false);

        if (ext == "nif") {
            QPixmap thumb = generateNifThumbnail(filePath);
            if (!thumb.isNull()) {
                mPreviewImage->setPixmap(thumb);
                mPreviewImage->setStyleSheet(
                    "background-color: transparent; border: 1px solid #555;");
                return;
            }
        }

        mPreviewImage->setPixmap(QPixmap());
        mPreviewImage->setText(tr("3D Model\n%1").arg(info.fileName()));
        mPreviewImage->setStyleSheet(
            "background-color: #333; border: 1px solid #555; color: #aaa;");
        return;
    }

    clearPreview();
}

void AssetBrowserWidget::clearPreview()
{
    mPreviewImage->setPixmap(QPixmap());
    mPreviewImage->setText(tr("Select an asset to preview"));
    mPreviewImage->setStyleSheet("background-color: #333; border: 1px solid #555;");
    mFileInfoLabel->clear();

    auto* playBtn = findChild<QPushButton*>("playBtn");
    if (playBtn) playBtn->setEnabled(false);
}

void AssetBrowserWidget::initPreviewGL()
{
    if (mPreviewGLInitialized) return;

    mPreviewGLContext = new QOpenGLContext();
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    mPreviewGLContext->setFormat(fmt);
    if (!mPreviewGLContext->create()) {
        LOG_WARNING("AssetBrowser: failed to create offscreen GL context for NIF preview");
        mPreviewGLInitialized = false;
        return;
    }

    mPreviewOffscreen = new QOffscreenSurface();
    mPreviewOffscreen->setFormat(fmt);
    mPreviewOffscreen->create();

    if (!mPreviewGLContext->makeCurrent(mPreviewOffscreen)) {
        LOG_WARNING("AssetBrowser: failed to make offscreen GL context current");
        mPreviewGLContext->doneCurrent();
        mPreviewGLInitialized = false;
        return;
    }

    auto* f = mPreviewGLContext->functions();
    f->initializeOpenGLFunctions();

    mPreviewShader = new QOpenGLShaderProgram();

    static const char* vertSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aNormal;
        uniform mat4 mModel;
        uniform mat4 mView;
        uniform mat4 mProjection;
        out vec3 vNormal;
        void main() {
            gl_Position = mProjection * mView * mModel * vec4(aPosition, 1.0);
            vNormal = aNormal;
        }
    )";

    static const char* fragSrc = R"(
        #version 330 core
        in vec3 vNormal;
        out vec4 FragColor;
        uniform vec3 lightDir;
        uniform vec3 objectColor;
        uniform float ambientStrength;
        void main() {
            vec3 n = normalize(vNormal);
            float ambient = ambientStrength;
            float diffuse = max(dot(n, normalize(lightDir)), 0.0);
            vec3 result = (ambient + diffuse) * objectColor;
            FragColor = vec4(result, 1.0);
        }
    )";

    if (!mPreviewShader->addShaderFromSourceCode(QOpenGLShader::Vertex, vertSrc)) {
        LOG_WARNING("AssetBrowser: preview vertex shader compile failed");
    }
    if (!mPreviewShader->addShaderFromSourceCode(QOpenGLShader::Fragment, fragSrc)) {
        LOG_WARNING("AssetBrowser: preview fragment shader compile failed");
    }
    mPreviewShader->link();

    mPreviewVBO = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    mPreviewVBO->create();
    mPreviewIBO = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    mPreviewIBO->create();
    mPreviewVAO = new QOpenGLVertexArrayObject();
    mPreviewVAO->create();

    mPreviewGLContext->doneCurrent();
    mPreviewGLInitialized = true;
}

QPixmap AssetBrowserWidget::generateNifThumbnail(const QString& filePath)
{
    auto cacheIt = mThumbnailCache.find(filePath);
    if (cacheIt != mThumbnailCache.end()) {
        return cacheIt.value();
    }

    initPreviewGL();

    if (mPreviewGLInitialized) {
        Nif::NifParser parser;
        if (parser.load(filePath)) {
            QVector<QVector3D> allVerts;
            QVector<QVector3D> allNorms;
            QVector<unsigned int> allIndices;

            std::function<void(Nif::Node*)> collect = [&](Nif::Node* node) {
                if (!node) return;
                for (auto& shape : node->shapes) {
                    unsigned int base = static_cast<unsigned int>(allVerts.size());

                    if (shape.normals.isEmpty()) {
                        shape.recalculateNormals();
                    }

                    for (int vi = 0; vi < shape.vertices.size(); ++vi) {
                        const auto& v = shape.vertices[vi];
                        allVerts.append(QVector3D(v.x, v.y, v.z));
                        if (vi < shape.normals.size()) {
                            allNorms.append(QVector3D(
                                shape.normals[vi].x,
                                shape.normals[vi].y,
                                shape.normals[vi].z));
                        } else {
                            allNorms.append(QVector3D(0, 1, 0));
                        }
                    }

                    for (unsigned int idx : shape.indices) {
                        allIndices.append(base + idx);
                    }
                }
                for (auto* child : node->children) {
                    collect(child);
                }
            };

            collect(parser.getRoot());

            if (!allVerts.isEmpty() && !allIndices.isEmpty()) {
                if (!mPreviewGLContext->makeCurrent(mPreviewOffscreen)) {
                    mPreviewGLContext->doneCurrent();
                } else {
                    QOpenGLFramebufferObject fbo(256, 256);
                    fbo.bind();

                    auto* f = mPreviewGLContext->functions();
                    f->glViewport(0, 0, 256, 256);
                    f->glClearColor(0.22f, 0.22f, 0.22f, 1.0f);
                    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    f->glEnable(GL_DEPTH_TEST);

                    QVector<float> interleaved;
                    interleaved.reserve(allVerts.size() * 6);
                    for (int i = 0; i < allVerts.size(); ++i) {
                        interleaved << allVerts[i].x() << allVerts[i].y() << allVerts[i].z();
                        interleaved << allNorms[i].x() << allNorms[i].y() << allNorms[i].z();
                    }

                    mPreviewShader->bind();

                    mPreviewVAO->bind();
                    mPreviewVBO->bind();
                    mPreviewVBO->allocate(
                        interleaved.constData(),
                        interleaved.size() * static_cast<int>(sizeof(float)));

                    const int stride = 6 * static_cast<int>(sizeof(float));
                    mPreviewShader->setAttributeBuffer(0, GL_FLOAT, 0, 3, stride);
                    mPreviewShader->enableAttributeArray(0);
                    mPreviewShader->setAttributeBuffer(
                        1, GL_FLOAT, 3 * static_cast<int>(sizeof(float)), 3, stride);
                    mPreviewShader->enableAttributeArray(1);

                    mPreviewIBO->bind();
                    mPreviewIBO->allocate(
                        allIndices.constData(),
                        allIndices.size() * static_cast<int>(sizeof(unsigned int)));

                    QVector3D bbMin(1e6f, 1e6f, 1e6f);
                    QVector3D bbMax(-1e6f, -1e6f, -1e6f);
                    for (const auto& v : allVerts) {
                        bbMin.setX(qMin(bbMin.x(), v.x()));
                        bbMin.setY(qMin(bbMin.y(), v.y()));
                        bbMin.setZ(qMin(bbMin.z(), v.z()));
                        bbMax.setX(qMax(bbMax.x(), v.x()));
                        bbMax.setY(qMax(bbMax.y(), v.y()));
                        bbMax.setZ(qMax(bbMax.z(), v.z()));
                    }
                    QVector3D center = (bbMin + bbMax) * 0.5f;
                    float size = qMax(qMax(bbMax.x() - bbMin.x(),
                                           bbMax.y() - bbMin.y()),
                                      bbMax.z() - bbMin.z());
                    if (size < 0.001f) size = 1.0f;

                    QMatrix4x4 model;
                    model.translate(-center);
                    model.scale(1.8f / size);

                    QMatrix4x4 view;
                    view.rotate(25.0f, 1.0f, 0.0f, 0.0f);
                    view.rotate(-35.0f, 0.0f, 1.0f, 0.0f);
                    view.translate(0.0f, 0.0f, -1.0f);

                    QMatrix4x4 projection;
                    projection.perspective(45.0f, 1.0f, 0.01f, 100.0f);

                    mPreviewShader->setUniformValue("mModel", model);
                    mPreviewShader->setUniformValue("mView", view);
                    mPreviewShader->setUniformValue("mProjection", projection);
                    mPreviewShader->setUniformValue("lightDir",
                                                    QVector3D(0.5f, 1.0f, 0.3f));
                    mPreviewShader->setUniformValue("objectColor",
                                                    QVector3D(0.65f, 0.65f, 0.75f));
                    mPreviewShader->setUniformValue("ambientStrength", 0.35f);

                    f->glDrawElements(GL_TRIANGLES, allIndices.size(),
                                      GL_UNSIGNED_INT, nullptr);

                    mPreviewIBO->release();
                    mPreviewVBO->release();
                    mPreviewVAO->release();
                    mPreviewShader->release();

                    fbo.release();
                    QImage img = fbo.toImage();
                    mPreviewGLContext->doneCurrent();

                    if (!img.isNull()) {
                        QPixmap pix = QPixmap::fromImage(
                            img.mirrored(false, true).scaled(
                                256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                        mThumbnailCache[filePath] = pix;
                        return pix;
                    }
                }
            }
        }
    }

    // Fallback: display the first texture referenced by the NIF
    {
        Nif::NifParser parser;
        if (parser.load(filePath)) {
            std::function<QString(Nif::Node*)> findTex =
                [&](Nif::Node* node) -> QString {
                if (!node) return {};
                for (auto& shape : node->shapes) {
                    if (!shape.texture.isEmpty()) return shape.texture;
                }
                for (auto* child : node->children) {
                    QString t = findTex(child);
                    if (!t.isEmpty()) return t;
                }
                return {};
            };

            QString texRel = findTex(parser.getRoot());
            if (!texRel.isEmpty()) {
                QFileInfo nifInfo(filePath);
                QString texPath =
                    nifInfo.absolutePath() + "/" + QFileInfo(texRel).fileName();
                QImage img = NifViewportWidget::loadTextureImage(texPath);
                if (img.isNull()) {
                    img = NifViewportWidget::loadTextureImage(texRel);
                }
                if (!img.isNull()) {
                    QPixmap pix = QPixmap::fromImage(
                        img.scaled(256, 256, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation));
                    mThumbnailCache[filePath] = pix;
                    return pix;
                }
            }
        }
    }

    return QPixmap();
}

QStringList AssetBrowserWidget::scanDependencies(const QString& filePath)
{
    auto it = mDependencies.find(filePath);
    if (it != mDependencies.end()) {
        return it.value();
    }

    QStringList deps;
    QFileInfo info(filePath);
    QString ext = info.suffix().toLower();

    if (ext == "nif") {
        Nif::NifParser parser;
        if (parser.load(filePath)) {
            std::function<void(Nif::Node*)> gatherTex =
                [&](Nif::Node* node) {
                if (!node) return;
                for (auto& shape : node->shapes) {
                    if (!shape.texture.isEmpty()
                        && !deps.contains(shape.texture)) {
                        deps.append(shape.texture);
                    }
                }
                for (auto* child : node->children) {
                    gatherTex(child);
                }
            };
            gatherTex(parser.getRoot());
        }
    }

    mDependencies[filePath] = deps;
    return deps;
}

void AssetBrowserWidget::onFileContextMenu(const QPoint& pos)
{
    QModelIndex index = mFileList->indexAt(pos);
    if (!index.isValid()) return;

    QString filePath = mFileModel->filePath(index);

    QMenu menu(mFileList);
    QAction* copyPathAction = menu.addAction(tr("Copy Path"));
    QAction* openAction = menu.addAction(tr("Open in Editor"));

    const bool isNif = filePath.endsWith(QStringLiteral(".nif"), Qt::CaseInsensitive);
    QAction* iconAction = nullptr;
    if (isNif) {
        menu.addSeparator();
        iconAction = menu.addAction(tr("Generate Icon..."));
    }

    QAction* chosen = menu.exec(mFileList->viewport()->mapToGlobal(pos));
    if (chosen == copyPathAction) {
        QApplication::clipboard()->setText(filePath);
    } else if (chosen == openAction) {
        emit assetDoubleClicked(filePath);
    } else if (chosen == iconAction && isNif) {
        mPendingIconPath = filePath;
        generateIcon();
    }
}

QString AssetBrowserWidget::fileTypeForExtension(const QString& ext) const
{
    QString lower = ext.toLower();
    if (mModelExts.contains(lower)) return tr("Model");
    if (mTextureExts.contains(lower)) return tr("Texture");
    if (mSoundExts.contains(lower)) return tr("Sound");
    return tr("Unknown");
}

void AssetBrowserWidget::generateIcon()
{
    if (mPendingIconPath.isEmpty()) {
        return;
    }

    const QFileInfo nifInfo(mPendingIconPath);
    const QString outPath = nifInfo.absolutePath()
        + QStringLiteral("/%1_icon.png").arg(nifInfo.completeBaseName());

    QMessageBox::StandardButton choice = QMessageBox::question(this,
        tr("Generate Icon"),
        tr("Render %1 as a %2-size icon?\nOutput: %3")
            .arg(nifInfo.fileName())
            .arg(IconRenderer::contextSize(IconRenderer::Context::Inventory))
            .arg(outPath),
        QMessageBox::Ok | QMessageBox::Cancel);

    if (choice != QMessageBox::Ok) {
        return;
    }

    runIconRender(mPendingIconPath, IconRenderer::Context::Inventory);
}

void AssetBrowserWidget::runIconRender(const QString& nifPath, IconRenderer::Context ctx)
{
    if (!QFile::exists(IconRenderer::scriptPath())) {
        QMessageBox::warning(this, tr("Generate Icon"),
            tr("The icon generation script is missing:\n%1").arg(IconRenderer::scriptPath()));
        return;
    }

    const QString blender = BlenderLauncher::getRecommendedBlenderPath();
    if (blender.isEmpty()) {
        QMessageBox::warning(this, tr("Generate Icon"),
            tr("Blender was not found. Install Blender to generate icons."));
        return;
    }

    const QFileInfo nifInfo(nifPath);
    const QString outPath = nifInfo.absolutePath()
        + QStringLiteral("/%1_icon.png").arg(nifInfo.completeBaseName());

    auto* process = new QProcess(this);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, outPath, process](int exitCode, QProcess::ExitStatus) {
        process->deleteLater();
        if (exitCode == 0) {
            QMessageBox::information(this, tr("Generate Icon"),
                tr("Icon generated:\n%1").arg(outPath));
        } else {
            QMessageBox::warning(this, tr("Generate Icon"),
                tr("Icon generation failed."));
        }
    });

    const QStringList args = IconRenderer::blenderArguments(
        blender, nifPath, outPath, ctx);
    process->start(args.first(), args.mid(1));
    LOG_INFO(QString("AssetBrowser: generating icon for %1 (%2)")
        .arg(nifPath).arg(IconRenderer::contextName(ctx)));
}
