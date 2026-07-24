#pragma once

#include <QString>

class NifAnimation;

class NifAnimationImporter
{
public:
    static NifAnimation* importFromJson(const QString& filePath);
    static NifAnimation* importFromXml(const QString& filePath);
};
