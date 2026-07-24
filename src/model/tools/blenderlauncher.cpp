#include "blenderlauncher.hpp"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QProcess>
#include <QTemporaryDir>
#include <QFile>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include "../../libs/files/log/logger.hpp"

QStringList BlenderLauncher::getCommonBlenderPaths()
{
    QStringList paths;

#if defined(OPENCK_BUNDLED_BLENDER_VERSION) && defined(OPENCK_BUNDLED_BLENDER_PLATFORM)
    QString bundledPath = QCoreApplication::applicationDirPath()
        + QStringLiteral("/blender/blender-") OPENCK_BUNDLED_BLENDER_VERSION
          QStringLiteral("-") OPENCK_BUNDLED_BLENDER_PLATFORM
        + QStringLiteral("/blender.exe");
    if (QFile::exists(bundledPath)) {
        paths << bundledPath;
    }
#endif

#ifdef Q_OS_WIN
    paths << "C:\\Program Files\\Blender Foundation\\Blender 2.93\\blender.exe"
          << "C:\\Program Files\\Blender Foundation\\Blender 3.6\\blender.exe"
          << "C:\\Program Files\\Blender Foundation\\Blender 4.0\\blender.exe"
          << "C:\\Program Files\\Blender Foundation\\Blender 4.4\\blender.exe"
          << "C:\\Program Files\\Blender Foundation\\Blender\\blender.exe"
          << QDir::homePath() + "\\AppData\\Local\\Programs\\Blender\\blender.exe";

    QSettings blenderReg("HKEY_CURRENT_USER\\Software\\Blender Foundation\\Blender", QSettings::IniFormat);
    for (const QString& key : blenderReg.childGroups()) {
        QString installPath = blenderReg.value(key + "/Install_Location").toString();
        if (!installPath.isEmpty()) {
            paths.prepend(installPath + "\\blender.exe");
        }
    }

#elif defined(Q_OS_MAC)
    paths << "/Applications/Blender.app/Contents/MacOS/Blender"
          << "/Applications/Blender 2.93.app/Contents/MacOS/Blender"
          << "/Applications/Blender 3.6.app/Contents/MacOS/Blender"
          << "/Applications/Blender 4.0.app/Contents/MacOS/Blender";

#else
    paths << "/usr/bin/blender"
          << "/usr/local/bin/blender"
          << QDir::homePath() + "/snap/blender/common/.local/share/Steam/steamapps/common/Blender/blender";
#endif

    return paths;
}

bool BlenderLauncher::validateBlenderInstallation(const QString& path)
{
    if (!QFile::exists(path)) {
        return false;
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(path, QStringList() << "--version");
    process.waitForFinished(5000);

    return process.exitCode() == 0 || process.exitCode() == 129;
}

BlenderLauncher::BlenderInfo BlenderLauncher::findBlender()
{
    BlenderInfo info;
    info.isNifCompatible = false;
    info.hasPython = true;

    QStringList candidatePaths = getCommonBlenderPaths();

    QString blenderInPath = QStandardPaths::findExecutable("blender");
    if (!blenderInPath.isEmpty()) {
        candidatePaths.prepend(blenderInPath);
    }

    for (const QString& path : candidatePaths) {
        if (validateBlenderInstallation(path)) {
            info.path = path;
            info.version = getBlenderVersion(path);
            info.isNifCompatible = isNifCompatibleVersion(info.version);

            QString pyniflyPath = detectPyNiflyInstallation(path);
            if (!pyniflyPath.isEmpty()) {
                info.nifAddonName = "PyNifly";
                info.nifAddonPath = pyniflyPath;
                info.isNifCompatible = true;
            } else {
                QString niftoolsPath = detectNifToolsInstallation(path);
                if (!niftoolsPath.isEmpty()) {
                    info.nifAddonName = "NifTools";
                    info.nifAddonPath = niftoolsPath;
                    info.isNifCompatible = true;
                }
            }

            LOG_INFO(QString("Found Blender: %1 (version %2, NIF compatible: %3, addon: %4)")
                        .arg(path).arg(info.version)
                        .arg(info.isNifCompatible ? "Yes" : "No")
                        .arg(info.nifAddonName.isEmpty() ? "none" : info.nifAddonName));
            return info;
        }
    }

    LOG_WARNING("Blender not found. Install Blender 2.93+ for advanced 3D editing features.");
    return info;
}

bool BlenderLauncher::isBlenderAvailable()
{
    return !findBlender().path.isEmpty();
}

QString BlenderLauncher::getBlenderVersion(const QString& blenderPath)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(blenderPath, QStringList() << "--version");
    process.waitForFinished(5000);

    QString output = QString::fromUtf8(process.readAllStandardOutput());

    int versionIndex = output.indexOf("Blender ");
    if (versionIndex != -1) {
        QString versionStr = output.mid(versionIndex + 8);
        int dotPos = versionStr.indexOf('.');
        if (dotPos != -1) {
            return versionStr.left(dotPos);
        }
    }

    return QString();
}

int BlenderLauncher::parseVersionNumber(const QString& version)
{
    QStringList parts = version.split('.');
    if (parts.size() >= 2) {
        int major = parts[0].toInt();
        int minor = parts[1].toInt();
        return major * 100 + minor;
    }
    return 0;
}

bool BlenderLauncher::isNifCompatibleVersion(const QString& version)
{
    int versionNum = parseVersionNumber(version);
    return versionNum >= 404;
}

bool BlenderLauncher::openInBlender(const QString& filePath, const QString& blenderPath)
{
    QString actualBlenderPath = blenderPath;
    if (actualBlenderPath.isEmpty()) {
        BlenderInfo info = findBlender();
        if (info.path.isEmpty()) {
            LOG_ERROR("Blender not found. Please install Blender 2.93+ to use this feature.");
            return false;
        }
        actualBlenderPath = info.path;
    }

    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        LOG_ERROR(QString("File not found: %1").arg(filePath));
        return false;
    }

    QProcess::startDetached(actualBlenderPath, QStringList() << filePath);
    LOG_INFO(QString("Opened %1 in Blender").arg(filePath));
    return true;
}

bool BlenderLauncher::exportToBlender(const QString& modelPath, const QString& editorId, CkId::Type recordType)
{
    QFileInfo fileInfo(modelPath);
    if (!fileInfo.exists()) {
        LOG_ERROR(QString("Model file not found: %1").arg(modelPath));
        return false;
    }

    if (modelPath.endsWith(".nif", Qt::CaseInsensitive)) {
        return openInBlender(modelPath);
    }

    LOG_INFO(QString("Exporting %1 (%2) to Blender").arg(editorId).arg(CkId(recordType).getTypeName()));
    return openInBlender(modelPath);
}

bool BlenderLauncher::importFromBlender(const QString& blenderOutputPath, QString& outputPath)
{
    QFileInfo fileInfo(blenderOutputPath);
    if (!fileInfo.exists()) {
        LOG_ERROR(QString("Blender output not found: %1").arg(blenderOutputPath));
        return false;
    }

    outputPath = blenderOutputPath;
    LOG_INFO(QString("Imported from Blender: %1").arg(blenderOutputPath));
    return true;
}

QString BlenderLauncher::getRecommendedBlenderPath()
{
    BlenderInfo info = findBlender();
    if (!info.path.isEmpty()) {
        return info.path;
    }

    QStringList paths = getCommonBlenderPaths();
    if (!paths.isEmpty()) {
        return paths.first();
    }

    return QString();
}

QProcess* BlenderLauncher::runHeadless(const QString& scriptPath, const QStringList& args)
{
    BlenderInfo info = findBlender();
    if (info.path.isEmpty()) {
        LOG_ERROR("Blender not found. Cannot run headless mode.");
        return nullptr;
    }

    QProcess* process = new QProcess();
    process->setProcessChannelMode(QProcess::SeparateChannels);
    QStringList blenderArgs;
    blenderArgs << "--background"
                << "--python"
                << scriptPath;
    blenderArgs.append(args);

    process->start(info.path, blenderArgs);

    if (!process->waitForStarted(5000)) {
        LOG_ERROR("Failed to start Blender headless mode");
        delete process;
        return nullptr;
    }

    LOG_INFO("Blender headless mode started");
    return process;
}

static QString runBlenderOp(const QString& operation, const QStringList& extraArgs, int timeoutMs = 60000)
{
    QString scriptPath = QCoreApplication::applicationDirPath() + "/scripts/blender/blender_operations.py";
    if (!QFile::exists(scriptPath)) {
        QString err = QString("Blender operations script not found: %1").arg(scriptPath);
        qWarning() << err;
        return QString("{\"success\": false, \"error\": \"%1\"}").arg(err);
    }

    BlenderLauncher::BlenderInfo info = BlenderLauncher::findBlender();
    if (info.path.isEmpty()) {
        QString err = "Blender not found for operation. Install Blender 3.0+ from https://www.blender.org/download/";
        qWarning() << err;
        return QString("{\"success\": false, \"error\": \"%1\"}").arg(err);
    }

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    QStringList args;
    args << "--background" << "--python" << scriptPath << "--" << operation << extraArgs;

    process.start(info.path, args);
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        QString err = QString("Blender operation timed out after %1 ms: %2").arg(timeoutMs).arg(operation);
        qWarning() << err;
        return QString("{\"success\": false, \"error\": \"%1\"}").arg(err);
    }

    if (process.exitCode() != 0) {
        QString errOut = QString::fromUtf8(process.readAllStandardError()).trimmed();
        QString err = QString("Blender operation failed: %1\nError: %2").arg(operation).arg(errOut);
        qWarning() << err;
        return QString("{\"success\": false, \"error\": %1}").arg(QString("\"%1\"").arg(errOut.replace("\"", "\\\"")));
    }

    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}

QString BlenderLauncher::findNifAddonInBlender(const QString& blenderPath, const QString& addonName)
{
    Q_UNUSED(addonName);

    QStringList opArgs;
    QString jsonStr = runBlenderOp("detect_addons", QStringList(), 30000);
    if (jsonStr.isEmpty()) return {};

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return {};

    QJsonObject result = doc.object();
    if (result.contains("error")) return {};

    if (addonName.contains("pynifly", Qt::CaseInsensitive)) {
        QJsonObject py = result["pynifly"].toObject();
        if (py["installed"].toBool()) return py["path"].toString();
    } else if (addonName.contains("niftools", Qt::CaseInsensitive)) {
        QJsonObject nt = result["niftools"].toObject();
        if (nt["installed"].toBool()) return nt["path"].toString();
    }

    return {};
}

QString BlenderLauncher::detectPyNiflyInstallation(const QString& blenderPath)
{
    return findNifAddonInBlender(blenderPath, "pynifly");
}

QString BlenderLauncher::detectNifToolsInstallation(const QString& blenderPath)
{
    return findNifAddonInBlender(blenderPath, "niftools");
}

BlenderLauncher::NifExportResult BlenderLauncher::exportNifViaBlender(const QString& nifPath, const QString& outputPath, const QString& blenderPath)
{
    Q_UNUSED(blenderPath);
    NifExportResult result;
    result.success = false;

    QStringList opArgs;
    opArgs << "" << outputPath << "SKYRIM";

    QString jsonStr = runBlenderOp("export_nif", opArgs, 120000);
    if (jsonStr.isEmpty()) {
        result.errorMessage = "Blender export process failed";
        return result;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        result.errorMessage = "Failed to parse Blender output";
        return result;
    }

    QJsonObject obj = doc.object();
    result.success = obj["success"].toBool();
    result.outputPath = obj["output"].toString();
    if (obj.contains("error")) {
        result.errorMessage = obj["error"].toString();
    }

    return result;
}

bool BlenderLauncher::importNifViaBlender(const QString& nifPath, const QString& blendPath, const QString& blenderPath)
{
    Q_UNUSED(blenderPath);

    QStringList opArgs;
    opArgs << nifPath;
    if (!blendPath.isEmpty()) opArgs << blendPath;

    QString jsonStr = runBlenderOp("import_nif", opArgs, 120000);
    if (jsonStr.isEmpty()) return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return false;

    return doc.object()["success"].toBool();
}

bool BlenderLauncher::generateNifPreview(const QString& nifPath, const QString& previewDir, const PreviewSettings& settings)
{
    QStringList opArgs;
    opArgs << nifPath
           << QString::number(settings.width)
           << QString::number(settings.height)
           << QString::number(settings.angles);

    QString jsonStr = runBlenderOp("generate_preview", opArgs, 300000);
    if (jsonStr.isEmpty()) return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return false;

    return doc.object()["success"].toBool();
}

bool BlenderLauncher::batchExportModels(const QString& pluginPath, const QString& outputDir, const QString& blenderPath)
{
    Q_UNUSED(blenderPath);

    QStringList opArgs;
    opArgs << pluginPath << outputDir;

    QString jsonStr = runBlenderOp("batch_export", opArgs, 600000);
    if (jsonStr.isEmpty()) return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return false;

    return doc.object()["success"].toBool();
}

bool BlenderLauncher::validateNifFile(const QString& nifPath, QStringList& errors)
{
    errors.clear();

    QStringList opArgs;
    opArgs << nifPath;

    QString jsonStr = runBlenderOp("validate_nif", opArgs, 120000);
    if (jsonStr.isEmpty()) return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return false;

    QJsonObject result = doc.object();
    QJsonArray errsArr = result["errors"].toArray();
    for (int i = 0; i < errsArr.size(); ++i) {
        errors.append(errsArr[i].toString());
    }

    return result["valid"].toBool();
}

bool BlenderLauncher::installNifToolsAddon(const QString& blenderPath)
{
    if (!blenderPath.isEmpty()) {
        // Try installing with a specific Blender path
        QProcess process;
        process.setProcessChannelMode(QProcess::SeparateChannels);
        QStringList args;
        args << "--background" << "--python-expr"
             << "import addon_utils; addon_utils.enable('niftools', default=True, persistent=True)";
        process.start(blenderPath, args);
        if (!process.waitForFinished(60000)) {
            process.kill();
            LOG_ERROR("Blender addon installation timed out");
            return false;
        }
        return process.exitCode() == 0;
    }

    QStringList opArgs;
    opArgs << "install";
    QString jsonStr = runBlenderOp("install_niftools_addon", opArgs, 120000);
    if (jsonStr.isEmpty()) return false;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) return false;

    return doc.object()["success"].toBool();
}

BlenderLauncher::NifToolsInstallStatus BlenderLauncher::checkNifToolsInstallation(const QString& blenderPath)
{
    Q_UNUSED(blenderPath);
    NifToolsInstallStatus status;

    QString jsonStr = runBlenderOp("check_niftools", {}, 30000);
    if (jsonStr.isEmpty()) {
        status.errorMessage = "Blender check operation failed";
        return status;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        status.errorMessage = "Failed to parse Blender output";
        return status;
    }

    QJsonObject result = doc.object();
    status.installed = result["installed"].toBool();
    status.enabled = result["enabled"].toBool();
    status.version = result["version"].toString();
    status.path = result["path"].toString();
    if (result.contains("error")) {
        status.errorMessage = result["error"].toString();
    }

    return status;
}
