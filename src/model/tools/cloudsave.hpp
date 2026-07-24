#ifndef CLOUDSAVE_HPP
#define CLOUDSAVE_HPP

#include <QString>
#include <QStringList>
#include <QObject>
#include <QSettings>

class CloudSave : public QObject {
    Q_OBJECT
public:
    enum class CloudProvider { None, OneDrive, GoogleDrive, Dropbox };
    
    struct CloudInfo {
        CloudProvider provider = CloudProvider::None;
        QString path;
        bool available = false;
    };
    
    explicit CloudSave(QObject* parent = nullptr);
    
    CloudInfo detect() const;
    bool syncToCloud(const QString& localPath, const QString& cloudPath);
    bool syncFromCloud(const QString& cloudPath, const QString& localPath);
    QStringList listCloudFiles(const QString& cloudPath);
    bool isAvailable() const;
    
signals:
    void syncProgress(int percent);
    void syncComplete(bool success);
    void syncError(const QString& error);
    
private:
    CloudInfo detectOneDrive() const;
    CloudInfo detectGoogleDrive() const;
    CloudInfo detectDropbox() const;
    QString settingOrDefault(const QString& key, const QString& defaultValue) const;
    bool copyFileWithProgress(const QString& src, const QString& dst);
};

#endif
