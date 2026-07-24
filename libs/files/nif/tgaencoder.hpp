#pragma once

#include <QImage>
#include <QString>

class TgaEncoder {
public:
    static bool encode(const QImage& src, const QString& outPath, bool useRle = true);
};
