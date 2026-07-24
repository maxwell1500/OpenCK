#include "nifviewport.hpp"
#include "../files/esm/nifrecord.hpp"
#include "../files/esm/nifparser.hpp"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QDebug>

NifViewport::NifViewport(QWidget* parent)
    : QOpenGLWidget(parent)
    , mVBO(QOpenGLBuffer::VertexBuffer)
{
    setFocusPolicy(Qt::StrongFocus);
}

NifViewport::~NifViewport()
{
    makeCurrent();

    delete mShaderProgram;

    mVBO.destroy();
    mVAO.destroy();

    doneCurrent();
}

void NifViewport::loadNifFile(const NifFile& nifFile)
{
    makeCurrent();

    buildMeshFromNif(nifFile);
    mMeshBuilt = true;

    doneCurrent();

    update();
}

void NifViewport::clearScene()
{
    makeCurrent();

    mVertices.clear();
    mUVs.clear();
    mNormals.clear();
    mIndices.clear();
    mMeshBuilt = false;

    doneCurrent();

    update();
}

void NifViewport::setViewMatrix(const QMatrix4x4& matrix)
{
    mViewMatrix = matrix;
    update();
}

void NifViewport::setProjectionMatrix(const QMatrix4x4& matrix)
{
    mProjectionMatrix = matrix;
    update();
}

void NifViewport::initializeGL()
{
    initializeOpenGLFunctions();

    setupShaders();

    mVBO.create();
    mVAO.create();

    mInitialized = true;

    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
}

void NifViewport::paintGL()
{
    if (!mInitialized)
    {
        return;
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!mMeshBuilt)
    {
        return;
    }

    renderMesh();
}

void NifViewport::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

void NifViewport::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        mDragging = true;
        mLastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    else if (event->button() == Qt::RightButton)
    {
        mDragging = true;
        mLastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
}

void NifViewport::mouseMoveEvent(QMouseEvent* event)
{
    if (mDragging)
    {
        QPoint delta = event->pos() - mLastMousePos;

        if (event->buttons() & Qt::LeftButton)
        {
            mRotX += delta.y() * 0.5f;
            mRotY += delta.x() * 0.5f;
        }
        else if (event->buttons() & Qt::RightButton)
        {
            mPan.setX(mPan.x() + delta.x() * 0.01f);
            mPan.setY(mPan.y() - delta.y() * 0.01f);
        }

        mLastMousePos = event->pos();
        update();
    }
}

void NifViewport::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton)
    {
        mDragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

void NifViewport::wheelEvent(QWheelEvent* event)
{
    mZoom *= 1.1f;
    if (mZoom > 5.0f)
    {
        mZoom = 5.0f;
    }
    else if (mZoom < 0.1f)
    {
        mZoom = 0.1f;
    }

    update();
}

void NifViewport::setupShaders()
{
    const QString vertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec2 aTexCoord;

        uniform mat4 mModel;
        uniform mat4 mView;
        uniform mat4 mProjection;

        out vec3 vNormal;
        out vec2 vTexCoord;

        void main()
        {
            gl_Position = mProjection * mView * mModel * vec4(aPosition, 1.0);
            vNormal = aNormal;
            vTexCoord = aTexCoord;
        }
    )";

    const QString fragmentShaderSource = R"(
        #version 330 core
        in vec3 vNormal;
        in vec2 vTexCoord;

        out vec4 FragColor;

        uniform vec3 lightDir;
        uniform vec3 viewPos;
        uniform vec3 objectColor;
        uniform float shininess;

        void main()
        {
            vec3 normal = normalize(vNormal);
            vec3 lightDirection = normalize(lightDir);

            float ambient = 0.3f;
            float diffuse = max(dot(normal, lightDirection), 0.0f);

            vec3 viewDirection = normalize(viewPos - vec3(0.0));
            vec3 reflectDirection = reflect(-lightDirection, normal);
            float specular = pow(max(dot(viewDirection, reflectDirection), 0.0f), shininess);

            vec3 result = (ambient + diffuse + specular) * objectColor;

            FragColor = vec4(result, 1.0);
        }
    )";

    mShaderProgram = new QOpenGLShaderProgram();
    mShaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    mShaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    mShaderProgram->link();

    mShaderProgram->bind();

    mShaderProgram->setUniformValue("lightDir", QVector3D(0.5f, 1.0f, 0.3f));
    mShaderProgram->setUniformValue("viewPos", QVector3D(0.0f, 0.0f, 5.0f));
    mShaderProgram->setUniformValue("shininess", 32.0f);

    mShaderProgram->release();
}

void NifViewport::buildMeshFromNif(const NifFile& nifFile)
{
    Q_UNUSED(nifFile)

    mVertices.clear();
    mUVs.clear();
    mNormals.clear();
    mIndices.clear();

    for (const auto& record : nifFile.getRecords())
    {
        if (!record)
        {
            continue;
        }

        NifTriShape* triShape = record->castTo<NifTriShape>();
        if (!triShape)
        {
            continue;
        }

        NifTriShapeData* triData = record->castTo<NifTriShapeData>();
        if (!triData || triData->vertices.isEmpty())
        {
            continue;
        }

        for (const auto& vertex : triData->vertices)
        {
            mVertices.append(QVector3D(vertex.x, vertex.y, vertex.z));
        }

        if (!triData->uvs.isEmpty())
        {
            for (const auto& uv : triData->uvs)
            {
                mUVs.append(QVector2D(uv.u, uv.v));
            }
        }
        else
        {
            for (int i = 0; i < triData->vertices.size(); i++)
            {
                mUVs.append(QVector2D(0.0f, 0.0f));
            }
        }

        if (!triData->normals.isEmpty())
        {
            for (const auto& normal : triData->normals)
            {
                mNormals.append(QVector3D(normal.x, normal.y, normal.z));
            }
        }
        else
        {
            for (int i = 0; i < triData->vertices.size(); i++)
            {
                mNormals.append(QVector3D(0.0f, 1.0f, 0.0f));
            }
        }

        for (quint32 index : triData->indices)
        {
            mIndices.append(index);
        }
    }
}

void NifViewport::renderMesh()
{
    if (mVertices.isEmpty() || mIndices.isEmpty())
    {
        return;
    }

    mShaderProgram->bind();

    mVAO.bind();
    mVBO.bind();

    quintptr offset = 0;

    mVBO.allocate(mVertices.constData(), mVertices.size() * sizeof(QVector3D));

    mShaderProgram->setAttributeBuffer(0, GL_FLOAT, 0, 3, sizeof(QVector3D));
    mShaderProgram->enableAttributeArray(0);

    if (!mNormals.isEmpty())
    {
        mVBO.allocate(mNormals.constData(), mNormals.size() * sizeof(QVector3D));

        mShaderProgram->setAttributeBuffer(1, GL_FLOAT, mVertices.size() * sizeof(QVector3D), 3, sizeof(QVector3D));
        mShaderProgram->enableAttributeArray(1);
    }

    if (!mUVs.isEmpty())
    {
        mVBO.allocate(mUVs.constData(), mUVs.size() * sizeof(QVector2D));

        mShaderProgram->setAttributeBuffer(2, GL_FLOAT, (mVertices.size() + mNormals.size()) * sizeof(QVector3D), 2, sizeof(QVector2D));
        mShaderProgram->enableAttributeArray(2);
    }

    QMatrix4x4 model;

    float scale = 0.01f;
    model.scale(scale);

    float rotXRad = mRotX * 3.14159265f / 180.0f;
    float rotYRad = mRotY * 3.14159265f / 180.0f;

    QMatrix4x4 rotXMatrix;
    rotXMatrix.rotate(rotXRad, 1.0f, 0.0f, 0.0f);

    QMatrix4x4 rotYMatrix;
    rotYMatrix.rotate(rotYRad, 0.0f, 1.0f, 0.0f);

    QMatrix4x4 view = mViewMatrix * rotYMatrix * rotXMatrix;
    view.translate(mPan);
    view.scale(mZoom);

    mShaderProgram->setUniformValue("mModel", model);
    mShaderProgram->setUniformValue("mView", view);
    mShaderProgram->setUniformValue("mProjection", mProjectionMatrix);
    mShaderProgram->setUniformValue("objectColor", QVector3D(0.8f, 0.8f, 0.8f));

    glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, nullptr);

    mVBO.release();
    mVAO.release();

    mShaderProgram->release();
}
