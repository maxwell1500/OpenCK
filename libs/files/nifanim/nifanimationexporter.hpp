#pragma once

#include <QString>

class NifAnimation;

class NifAnimationExporter
{
public:
    static bool exportToJson(const NifAnimation* animation, const QString& filePath);
    static bool exportToXml(const NifAnimation* animation, const QString& filePath);
};
