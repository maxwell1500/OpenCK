#ifndef NIFVIEWPORT_HPP
#define NIFVIEWPORT_HPP

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <QVector4D>
#include <QVector2D>
#include <QColor>

class NifFile;

class NifViewport : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit NifViewport(QWidget* parent = nullptr);
    ~NifViewport() override;

    void loadNifFile(const NifFile& nifFile);
    void clearScene();

    void setViewMatrix(const QMatrix4x4& matrix);
    void setProjectionMatrix(const QMatrix4x4& matrix);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupShaders();
    void buildMeshFromNif(const NifFile& nifFile);
    void renderMesh();

    QOpenGLShaderProgram* mShaderProgram = nullptr;
    QOpenGLBuffer mVBO;
    QOpenGLVertexArrayObject mVAO;

    QMatrix4x4 mViewMatrix;
    QMatrix4x4 mProjectionMatrix;

    QVector<QVector3D> mVertices;
    QVector<QVector2D> mUVs;
    QVector<QVector3D> mNormals;
    QVector<quint32> mIndices;

    bool mInitialized = false;
    bool mMeshBuilt = false;

    // Camera controls
    float mRotX = 0.0f;
    float mRotY = 0.0f;
    float mZoom = 1.0f;
    QVector3D mPan = QVector3D(0.0f, 0.0f, 0.0f);

    bool mDragging = false;
    QPoint mLastMousePos;
};

#endif // NIFVIEWPORT_HPP
