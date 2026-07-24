#include "pluginsignature.hpp"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

QByteArray PluginSignature::computeHash(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(&file)) {
        return QByteArray();
    }
    
    return hash.result();
}

QByteArray PluginSignature::computeHash(const QByteArray& data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

bool PluginSignature::verifyHash(const QString& filePath, const QByteArray& expectedHash) {
    QByteArray actualHash = computeHash(filePath);
    return !actualHash.isEmpty() && (actualHash == expectedHash);
}

QString PluginSignature::hashToString(const QByteArray& hash) {
    return hash.toHex();
}

QByteArray PluginSignature::stringToHash(const QString& hashString) {
    return QByteArray::fromHex(hashString.toUtf8());
}

bool PluginSignature::saveSignature(const QString& pluginPath, const QString& sigPath) {
    QByteArray hash = computeHash(pluginPath);
    if (hash.isEmpty()) {
        return false;
    }
    
    QFile file(sigPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    stream << hashToString(hash);
    
    return true;
}

bool PluginSignature::loadSignature(const QString& sigPath, QByteArray& outHash) {
    QFile file(sigPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }
    
    QTextStream stream(&file);
    QString hashStr;
    stream >> hashStr;
    
    outHash = stringToHash(hashStr);
    return outHash.length() == HASH_LENGTH;
}

bool PluginSignature::validatePlugin(const QString& pluginPath) {
    QFileInfo info(pluginPath);
    QString sigPath = info.absolutePath() + "/" + info.completeBaseName() + ".sig";
    
    if (!QFile::exists(sigPath)) {
        return true; // Unsigned plugins are considered valid
    }
    
    QByteArray savedHash;
    if (!loadSignature(sigPath, savedHash)) {
        return false;
    }
    
    return verifyHash(pluginPath, savedHash);
}
