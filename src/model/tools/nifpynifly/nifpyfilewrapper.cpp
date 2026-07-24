#include "nifpyfilewrapper.hpp"

#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QString NifPyFileWrapper::mDefaultGame = "SKYRIM";
bool NifPyFileWrapper::mInitialized = false;

static QString runNifOperation(const QString& operation, const QStringList& extraArgs, int timeoutMs = 60000)
{
    QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts/blender/nif_operations.py";
    if (!QFile::exists(scriptPath)) {
        qWarning() << "NIF operations script not found:" << scriptPath;
        return {};
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    QStringList args;
    args << "--background" << "--python" << scriptPath << "--" << operation << extraArgs;

    process.start("blender", args);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        qWarning() << "NIF operation timed out:" << operation;
        return {};
    }

    if (process.exitCode() != 0) {
        QString err = QString::fromUtf8(process.readAllStandardError()).trimmed();
        qWarning() << "NIF operation failed:" << operation << "\n" << err;
        return {};
    }

    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

static QJsonObject parseJson(const QString& jsonStr)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << err.errorString();
        return {};
    }
    return doc.object();
}

static NifPyFileWrapper::Vertex parseVertex(const QJsonValue& val)
{
    QJsonObject o = val.toObject();
    NifPyFileWrapper::Vertex v;
    v.x = static_cast<float>(o["x"].toDouble());
    v.y = static_cast<float>(o["y"].toDouble());
    v.z = static_cast<float>(o["z"].toDouble());
    return v;
}

static NifPyFileWrapper::Triangle parseTriangle(const QJsonValue& val)
{
    QJsonObject o = val.toObject();
    NifPyFileWrapper::Triangle t;
    t.v0 = o["v0"].toInt();
    t.v1 = o["v1"].toInt();
    t.v2 = o["v2"].toInt();
    return t;
}

static NifPyFileWrapper::UV parseUV(const QJsonValue& val)
{
    QJsonObject o = val.toObject();
    NifPyFileWrapper::UV uv;
    uv.u = static_cast<float>(o["u"].toDouble());
    uv.v = static_cast<float>(o["v"].toDouble());
    return uv;
}

static void parseShapeData(const QJsonObject& sObj, NifPyFileWrapper::ShapeData& shape)
{
    shape.name = sObj["name"].toString();
    shape.hasSkinning = !sObj["bone_names"].toArray().isEmpty();

    QJsonArray verts = sObj["vertices"].toArray();
    for (int i = 0; i < verts.size(); ++i) {
        shape.vertices.append(parseVertex(verts[i]));
    }
    QJsonArray tris = sObj["triangles"].toArray();
    for (int i = 0; i < tris.size(); ++i) {
        shape.triangles.append(parseTriangle(tris[i]));
    }
    QJsonArray uvs = sObj["uvs"].toArray();
    for (int i = 0; i < uvs.size(); ++i) {
        shape.uvs.append(parseUV(uvs[i]));
    }
    QJsonArray norms = sObj["normals"].toArray();
    for (int i = 0; i < norms.size(); ++i) {
        shape.normals.append(parseVertex(norms[i]));
    }
    QJsonArray bones = sObj["bone_names"].toArray();
    for (int i = 0; i < bones.size(); ++i) {
        shape.boneNames.append(bones[i].toString());
    }
    QJsonObject texObj = sObj["textures"].toObject();
    for (auto it = texObj.begin(); it != texObj.end(); ++it) {
        shape.textures[it.key()] = it.value().toString();
    }
}

bool NifPyFileWrapper::initialize(const QString& pyniflyPath)
{
    if (mInitialized) {
        return true;
    }

    QDir pyniflyDir(pyniflyPath);
    if (!pyniflyDir.exists()) {
        qWarning() << "PyNifly path does not exist:" << pyniflyPath;
        return false;
    }

    if (!isPythonAvailable()) {
        qWarning() << "Python not found in PATH";
        return false;
    }

    QProcess verify;
    verify.setProcessChannelMode(QProcess::SeparateChannels);
    verify.start("python", QStringList() << "-c"
        << "import sys; sys.path.insert(0, r'" + pyniflyPath + "'); "
           "from pyn.pynifly import NifFile; print('OK')");
    if (!verify.waitForFinished(10000)) {
        verify.kill();
        qWarning() << "PyNifly verification timed out";
        return false;
    }

    QString output = QString::fromUtf8(verify.readAllStandardOutput()).trimmed();
    if (verify.exitCode() != 0 || output != "OK") {
        QString errStr = QString::fromUtf8(verify.readAllStandardError()).trimmed();
        qWarning() << "PyNifly not importable:" << errStr;
        return false;
    }

    mInitialized = true;
    qDebug() << "PyNifly initialized successfully from:" << pyniflyPath;
    return true;
}

void NifPyFileWrapper::shutdown()
{
    mInitialized = false;
}

bool NifPyFileWrapper::isInitialized()
{
    return mInitialized;
}

bool NifPyFileWrapper::isPythonAvailable()
{
    QProcess pythonCheck;
    pythonCheck.start("python", QStringList() << "--version");
    if (!pythonCheck.waitForFinished(5000)) {
        pythonCheck.kill();
        return false;
    }
    return pythonCheck.exitCode() == 0;
}

bool NifPyFileWrapper::loadNif(const QString& filePath, NifFileInfo& fileInfo)
{
    if (!mInitialized) {
        qWarning() << "PyNifly not initialized. Call initialize() first.";
        return false;
    }

    if (!QFile::exists(filePath)) {
        qWarning() << "NIF file does not exist:" << filePath;
        return false;
    }

    qDebug() << "Loading NIF:" << filePath;

    QString jsonStr = runNifOperation("extract_shapes", QStringList() << filePath);
    if (jsonStr.isEmpty()) return false;

    QJsonObject result = parseJson(jsonStr);
    if (result.isEmpty()) return false;

    if (result.contains("error")) {
        qWarning() << "loadNif error:" << result["error"].toString();
        return false;
    }

    fileInfo.filePath = filePath;
    fileInfo.game = result["game"].toString();
    fileInfo.version = 1000;
    fileInfo.hasCollision = false;

    QJsonArray boneArr = result["bone_names"].toArray();
    for (int i = 0; i < boneArr.size(); ++i) {
        fileInfo.boneNames.append(boneArr[i].toString());
    }

    QJsonArray shapesArr = result["shapes"].toArray();
    for (int i = 0; i < shapesArr.size(); ++i) {
        ShapeData shape;
        parseShapeData(shapesArr[i].toObject(), shape);
        fileInfo.shapes.append(shape);
    }

    for (int si = 0; si < fileInfo.shapes.size(); ++si) {
        const ShapeData& shape = fileInfo.shapes[si];
        for (auto it = shape.textures.constBegin(); it != shape.textures.constEnd(); ++it) {
            fileInfo.allTextures[it.key()] = it.value();
        }
    }

    return true;
}

bool NifPyFileWrapper::saveNif(const QString& outputPath, const QString& game,
                               const QVector<ShapeData>& shapes,
                               const QVector<QString>& boneNames)
{
    if (!mInitialized) {
        qWarning() << "PyNifly not initialized. Call initialize() first.";
        return false;
    }

    QJsonArray shapesArr;
    for (int si = 0; si < shapes.size(); ++si) {
        const ShapeData& shape = shapes[si];
        QJsonObject sObj;
        sObj["name"] = shape.name;

        QJsonArray verts;
        for (int i = 0; i < shape.vertices.size(); ++i) {
            QJsonObject vo;
            vo["x"] = shape.vertices[i].x;
            vo["y"] = shape.vertices[i].y;
            vo["z"] = shape.vertices[i].z;
            verts.append(vo);
        }
        sObj["vertices"] = verts;

        QJsonArray tris;
        for (int i = 0; i < shape.triangles.size(); ++i) {
            QJsonObject to;
            to["v0"] = shape.triangles[i].v0;
            to["v1"] = shape.triangles[i].v1;
            to["v2"] = shape.triangles[i].v2;
            tris.append(to);
        }
        sObj["triangles"] = tris;

        QJsonArray uvs;
        for (int i = 0; i < shape.uvs.size(); ++i) {
            QJsonObject uvo;
            uvo["u"] = shape.uvs[i].u;
            uvo["v"] = shape.uvs[i].v;
            uvs.append(uvo);
        }
        sObj["uvs"] = uvs;

        QJsonArray normals;
        for (int i = 0; i < shape.normals.size(); ++i) {
            QJsonObject no;
            no["x"] = shape.normals[i].x;
            no["y"] = shape.normals[i].y;
            no["z"] = shape.normals[i].z;
            normals.append(no);
        }
        sObj["normals"] = normals;

        QJsonArray boneArr;
        for (int i = 0; i < shape.boneNames.size(); ++i) {
            boneArr.append(shape.boneNames[i]);
        }
        sObj["bone_names"] = boneArr;

        QJsonObject texObj;
        for (auto it = shape.textures.constBegin(); it != shape.textures.constEnd(); ++it) {
            texObj[it.key()] = it.value();
        }
        sObj["textures"] = texObj;

        shapesArr.append(sObj);
    }

    QJsonArray bonesArr;
    for (int i = 0; i < boneNames.size(); ++i) {
        bonesArr.append(boneNames[i]);
    }

    QStringList opArgs;
    opArgs << outputPath << QJsonDocument(shapesArr).toJson(QJsonDocument::Compact)
           << QJsonDocument(bonesArr).toJson(QJsonDocument::Compact);

    QString jsonStr = runNifOperation("save_nif", opArgs, 120000);
    if (jsonStr.isEmpty()) return false;

    QJsonObject result = parseJson(jsonStr);
    if (result.contains("error")) {
        qWarning() << "saveNif error:" << result["error"].toString();
        return false;
    }

    return result["success"].toBool();
}

bool NifPyFileWrapper::extractShapes(const QString& filePath, QVector<ShapeData>& shapes)
{
    if (!mInitialized) {
        qWarning() << "PyNifly not initialized.";
        return false;
    }

    QString jsonStr = runNifOperation("extract_shapes", QStringList() << filePath);
    if (jsonStr.isEmpty()) return false;

    QJsonObject result = parseJson(jsonStr);
    if (result.isEmpty() || result.contains("error")) {
        qWarning() << "extractShapes error:" << result["error"].toString();
        return false;
    }

    QJsonArray shapesArr = result["shapes"].toArray();
    for (int i = 0; i < shapesArr.size(); ++i) {
        ShapeData shape;
        parseShapeData(shapesArr[i].toObject(), shape);
        shapes.append(shape);
    }

    return true;
}

bool NifPyFileWrapper::extractTextures(const QString& filePath, QMap<QString, QString>& textures)
{
    if (!mInitialized) {
        qWarning() << "PyNifly not initialized.";
        return false;
    }

    QString jsonStr = runNifOperation("extract_textures", QStringList() << filePath);
    if (jsonStr.isEmpty()) return false;

    QJsonObject result = parseJson(jsonStr);
    if (result.isEmpty() || result.contains("error")) {
        qWarning() << "extractTextures error:" << result["error"].toString();
        return false;
    }

    QJsonObject texObj = result["textures"].toObject();
    for (auto it = texObj.begin(); it != texObj.end(); ++it) {
        textures[it.key()] = it.value().toString();
    }

    return true;
}

bool NifPyFileWrapper::validateNif(const QString& filePath, QVector<ValidationError>& errors)
{
    if (!mInitialized) {
        qWarning() << "PyNifly not initialized.";
        return false;
    }

    QString jsonStr = runNifOperation("validate", QStringList() << filePath);
    if (jsonStr.isEmpty()) return false;

    QJsonObject result = parseJson(jsonStr);
    if (result.isEmpty() || result.contains("error")) {
        qWarning() << "validateNif error:" << result["error"].toString();
        return false;
    }

    QJsonArray errsArr = result["errors"].toArray();
    for (int i = 0; i < errsArr.size(); ++i) {
        QJsonObject eObj = errsArr[i].toObject();
        ValidationError err;
        QString sev = eObj["severity"].toString();
        if (sev == "error") err.severity = ValidationError::Error;
        else if (sev == "warning") err.severity = ValidationError::Warning;
        else err.severity = ValidationError::Info;
        err.field = eObj["field"].toString();
        err.message = eObj["message"].toString();
        err.blockNumber = eObj["block"].toInt();
        errors.append(err);
    }

    return true;
}

bool NifPyFileWrapper::compareNifs(const QString& nif1, const QString& nif2,
                                   QVector<QString>& vertexChanges,
                                   QVector<QString>& textureChanges)
{
    if (!mInitialized) {
        qWarning() << "PyNifly not initialized.";
        return false;
    }

    QString jsonStr = runNifOperation("compare", QStringList() << nif1 << nif2);
    if (jsonStr.isEmpty()) return false;

    QJsonObject result = parseJson(jsonStr);
    if (result.isEmpty() || result.contains("error")) {
        qWarning() << "compareNifs error:" << result["error"].toString();
        return false;
    }

    QJsonArray vertArr = result["vertex_changes"].toArray();
    for (int i = 0; i < vertArr.size(); ++i) {
        vertexChanges.append(vertArr[i].toString());
    }
    QJsonArray texArr = result["texture_changes"].toArray();
    for (int i = 0; i < texArr.size(); ++i) {
        textureChanges.append(texArr[i].toString());
    }

    return true;
}

QString NifPyFileWrapper::getPyniflyVersion()
{
    if (!mInitialized) {
        return "Not initialized";
    }

    QString jsonStr = runNifOperation("get_version", QStringList(), 10000);
    if (jsonStr.isEmpty()) return "unknown";

    QJsonObject result = parseJson(jsonStr);
    if (result.contains("error")) return "unknown";

    return result["version"].toString();
}

void NifPyFileWrapper::setDefaultGame(const QString& game)
{
    mDefaultGame = game;
}
