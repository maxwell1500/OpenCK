#ifndef PLUGINSIGNATURE_HPP
#define PLUGINSIGNATURE_HPP

#include <QString>
#include <QByteArray>
#include <QCryptographicHash>

class PluginSignature {
public:
    static QByteArray computeHash(const QString& filePath);
    static QByteArray computeHash(const QByteArray& data);
    static bool verifyHash(const QString& filePath, const QByteArray& expectedHash);
    static QString hashToString(const QByteArray& hash);
    static QByteArray stringToHash(const QString& hashString);
    static bool saveSignature(const QString& pluginPath, const QString& sigPath);
    static bool loadSignature(const QString& sigPath, QByteArray& outHash);
    static bool validatePlugin(const QString& pluginPath);
    
private:
    static const int HASH_LENGTH = 32; // SHA-256
};

#endif
