#include "skyrimgameparser.hpp"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

SkyrimGameParser::SkyrimGameParser() {
}

void SkyrimGameParser::parse(QIODevice* device) {
    QFile* file = qobject_cast<QFile*>(device);
    if (!file) return;
    
    QString content = QString::fromUtf8(file->readAll());
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    QJsonObject root = doc.object();
    
    QString type = root.value("type").toString();
    if (type == "ESM") {
        // Handle ESM
    } else if (type == "FLI4") {
        // Handle FLI4
    }
}
