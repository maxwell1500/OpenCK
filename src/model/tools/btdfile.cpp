#include "btdfile.hpp"

#include <QJsonArray>

QJsonObject BtdFile::toJson() const
{
    QJsonObject obj;
    obj.insert(QStringLiteral("fileName"), fileName);
    obj.insert(QStringLiteral("gridSize"), gridSize);
    obj.insert(QStringLiteral("textureCount"), textureCount);

    QJsonArray names;
    for (const QString& n : textureNames)
        names.append(n);
    obj.insert(QStringLiteral("textureNames"), names);

    QJsonArray quads;
    for (quint16 q : quadIndices)
        quads.append(static_cast<int>(q));
    obj.insert(QStringLiteral("quadIndices"), quads);
    return obj;
}

BtdFile BtdFile::fromJson(const QJsonObject& obj)
{
    BtdFile file;
    file.fileName = obj.value(QStringLiteral("fileName")).toString();
    file.gridSize = obj.value(QStringLiteral("gridSize")).toInt(0);
    file.textureCount = obj.value(QStringLiteral("textureCount")).toInt(0);

    const QJsonArray names = obj.value(QStringLiteral("textureNames")).toArray();
    for (const QJsonValue& v : names)
        file.textureNames.append(v.toString());

    const QJsonArray quads = obj.value(QStringLiteral("quadIndices")).toArray();
    for (const QJsonValue& v : quads)
        file.quadIndices.append(static_cast<quint16>(v.toInt(0)));
    return file;
}

bool BtdFile::build(int gridSize, const QStringList& textureNames,
                    const QVector<quint16>& quadIndices, BtdFile& out)
{
    if (gridSize <= 0 || quadIndices.size() != gridSize * gridSize)
        return false;
    if (textureNames.isEmpty())
        return false;

    out.gridSize = gridSize;
    out.textureNames = textureNames;
    out.quadIndices = quadIndices;
    out.textureCount = 0;
    for (quint16 q : quadIndices)
        out.textureCount = qMax(out.textureCount, static_cast<int>(q) + 1);
    return true;
}

QString BtdFile::textureForQuad(int x, int y) const
{
    if (x < 0 || y < 0 || x >= gridSize || y >= gridSize)
        return QString();
    const int index = quadIndices.value(y * gridSize + x, 0);
    if (index >= 0 && index < textureNames.size())
        return textureNames[index];
    return QString::number(index);
}
