#include "cloudsave.hpp"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QFileInfoList>
#include <QProcessEnvironment>

CloudSave::CloudSave(QObject* parent) : QObject(parent) {
}

QString CloudSave::settingOrDefault(const QString& key, const QString& defaultValue) const {
    QSettings settings("OpenCK", "OpenCK");
    QString val = settings.value(key).toString();
    return val.isEmpty() ? defaultValue : val;
}

CloudSave::CloudInfo CloudSave::detect() const {
    CloudInfo info = detectOneDrive();
    if (info.available) {
        return info;
    }
    
    info = detectGoogleDrive();
    if (info.available) {
        return info;
    }
    
    info = detectDropbox();
    return info;
}

CloudSave::CloudInfo CloudSave::detectOneDrive() const {
    CloudInfo info;
    QSettings settings("OpenCK", "OpenCK");
    QString overridePath = settings.value("CloudSave/OneDrivePath").toString();
    
    if (!overridePath.isEmpty() && QDir(overridePath).exists()) {
        info.provider = CloudProvider::OneDrive;
        info.path = overridePath;
        info.available = true;
        return info;
    }
    
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    QString oneDrivePath = env.value("OneDrive");
    if (oneDrivePath.isEmpty()) {
        oneDrivePath = env.value("OneDriveConsumer");
    }
    
    if (!oneDrivePath.isEmpty() && QDir(oneDrivePath).exists()) {
        info.provider = CloudProvider::OneDrive;
        info.path = oneDrivePath;
        info.available = true;
    }
    
    return info;
}

CloudSave::CloudInfo CloudSave::detectGoogleDrive() const {
    CloudInfo info;
    QSettings settings("OpenCK", "OpenCK");
    QString overridePath = settings.value("CloudSave/GoogleDrivePath").toString();
    
    if (!overridePath.isEmpty() && QDir(overridePath).exists()) {
        info.provider = CloudProvider::GoogleDrive;
        info.path = overridePath;
        info.available = true;
        return info;
    }
    
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    QString gdrivePath = env.value("Google Drive");
    
    QStringList candidates;
    candidates << gdrivePath;
    
    QString home = QDir::homePath();
#ifdef Q_OS_WIN
    candidates << home + "/Google Drive"
               << home + "/Google Drive (old)"
               << home + "/Google Drive/My Drive";
    QFileInfo userProfile(env.value("USERPROFILE"));
    if (userProfile.exists()) {
        candidates << userProfile.absoluteFilePath() + "/Google Drive"
                   << userProfile.absoluteFilePath() + "/Google Drive (old)";
    }
    QString localAppData = env.value("LOCALAPPDATA");
    if (!localAppData.isEmpty()) {
        candidates << localAppData + "/Google/DriveFileStreamer"
                   << localAppData + "/Google/Chrome/User Data/Default/WebData";
    }
#elif defined(Q_OS_MAC)
    candidates << home + "/Library/CloudStorage/GoogleDrive"
               << home + "/Google Drive"
               << home + "/Google Drive (old)";
#elif defined(Q_OS_LINUX)
    candidates << home + "/Google Drive"
               << home + "/Google Drive (old)"
               << home + "/.local/share/gvfs/google-drive:host=gmail.com"
               << home + "/.local/share/gvfs/google-drive~host=gmail.com";
#endif
    
    for (const QString& candidate : candidates) {
        if (candidate.isEmpty()) continue;
        if (QDir(candidate).exists()) {
            info.provider = CloudProvider::GoogleDrive;
            info.path = candidate;
            info.available = true;
            return info;
        }
    }
    
    return info;
}

CloudSave::CloudInfo CloudSave::detectDropbox() const {
    CloudInfo info;
    QSettings settings("OpenCK", "OpenCK");
    QString overridePath = settings.value("CloudSave/DropboxPath").toString();
    
    if (!overridePath.isEmpty() && QDir(overridePath).exists()) {
        info.provider = CloudProvider::Dropbox;
        info.path = overridePath;
        info.available = true;
        return info;
    }
    
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    
    QString dropboxPath = env.value("Dropbox");
    
    QStringList candidates;
    candidates << dropboxPath;
    
    QString home = QDir::homePath();
#ifdef Q_OS_WIN
    candidates << home + "/Dropbox"
               << home + "/Dropbox (Personal)"
               << home + "/Dropbox (Business)"
               << home + "/Dropbox (Work)";
    QString localAppData = env.value("LOCALAPPDATA");
    if (!localAppData.isEmpty()) {
        candidates << localAppData + "/Dropbox"
                   << localAppData + "/Dropbox/update";
    }
#elif defined(Q_OS_MAC)
    candidates << home + "/Library/CloudStorage/Dropbox"
               << home + "/Dropbox"
               << home + "/Dropbox (Personal)"
               << home + "/Dropbox (Business)";
#elif defined(Q_OS_LINUX)
    candidates << home + "/Dropbox"
               << home + "/.dropbox"
               << home + "/.local/share/dropbox";
#endif
    
    for (const QString& candidate : candidates) {
        if (candidate.isEmpty()) continue;
        if (QDir(candidate).exists()) {
            info.provider = CloudProvider::Dropbox;
            info.path = candidate;
            info.available = true;
            return info;
        }
    }
    
    return info;
}

bool CloudSave::syncToCloud(const QString& localPath, const QString& cloudPath) {
    if (!isAvailable()) {
        emit syncError("No cloud storage available");
        return false;
    }
    
    return copyFileWithProgress(localPath, cloudPath);
}

bool CloudSave::syncFromCloud(const QString& cloudPath, const QString& localPath) {
    if (!isAvailable()) {
        emit syncError("No cloud storage available");
        return false;
    }
    
    return copyFileWithProgress(cloudPath, localPath);
}

QStringList CloudSave::listCloudFiles(const QString& cloudPath) {
    QStringList files;
    QDir dir(cloudPath);
    
    if (!dir.exists()) {
        return files;
    }
    
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files);
    for (const QFileInfo& fileInfo : fileInfoList) {
        files.append(fileInfo.fileName());
    }
    
    return files;
}

bool CloudSave::isAvailable() const {
    return detect().available;
}

bool CloudSave::copyFileWithProgress(const QString& src, const QString& dst) {
    QFile sourceFile(src);
    if (!sourceFile.open(QIODevice::ReadOnly)) {
        emit syncError("Cannot open source file: " + sourceFile.errorString());
        return false;
    }
    
    QFile destFile(dst);
    if (!destFile.open(QIODevice::WriteOnly)) {
        emit syncError("Cannot open destination file: " + destFile.errorString());
        return false;
    }
    
    qint64 totalSize = sourceFile.size();
    qint64 bytesCopied = 0;
    const qint64 chunkSize = 1024 * 1024; // 1MB chunks
    
    while (!sourceFile.atEnd()) {
        QByteArray chunk = sourceFile.read(chunkSize);
        if (chunk.isEmpty()) {
            break;
        }
        
        qint64 written = destFile.write(chunk);
        if (written != chunk.size()) {
            emit syncError("Write error: " + destFile.errorString());
            return false;
        }
        
        bytesCopied += written;
        if (totalSize > 0) {
            int percent = static_cast<int>((bytesCopied * 100) / totalSize);
            emit syncProgress(percent);
        }
    }
    
    sourceFile.close();
    destFile.close();
    
    emit syncProgress(100);
    emit syncComplete(true);
    return true;
}
