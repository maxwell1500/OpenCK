#include "fuzparser.hpp"

#include <QFile>

#include "../log/logger.hpp"

namespace {
quint32 readU32(const QByteArray& bytes, int pos)
{
    if (pos + 4 > bytes.size()) return 0;
    return static_cast<quint32>(
        static_cast<quint8>(bytes.at(pos))
        | (static_cast<quint8>(bytes.at(pos + 1)) << 8)
        | (static_cast<quint8>(bytes.at(pos + 2)) << 16)
        | (static_cast<quint8>(bytes.at(pos + 3)) << 24));
}
}

bool FuzParser::parse(const QByteArray& bytes, FuzParser& out)
{
    out = FuzParser();

    if (bytes.size() < 8 || bytes.mid(0, 4) != QByteArray("FUZE", 4))
        return false;

    // Real format: "FUZE" + version(4) + lipSize(4) + lip data + audio RIFF.
    // Fall back to the chunked form ([FourCC][size][data]) for files produced
    // by tools that emit that layout.
    bool realForm = true;
    const quint32 version = readU32(bytes, 4);
    const quint32 lipSize = readU32(bytes, 8);
    if (version == 0 || version > 0xFFFF ||
        static_cast<qint64>(12) + lipSize + 8 > bytes.size())
    {
        realForm = false;
    }
    else
    {
        // The lip data region must look plausible: the audio that follows is
        // a RIFF ("RIFF"/"RIFX") or WAVE container.
        const int audioStart = 12 + static_cast<int>(lipSize);
        const QByteArray audioTag = bytes.mid(audioStart, 4);
        if (audioTag != "RIFF" && audioTag != "RIFX" && audioTag != "WAVE")
            realForm = false;
    }

    if (realForm)
    {
        out.lipData = bytes.mid(12, static_cast<int>(lipSize));

        int audioStart = 12 + static_cast<int>(lipSize);
        const QByteArray audioTag = bytes.mid(audioStart, 4);
        if (audioTag == "RIFF" || audioTag == "RIFX")
        {
            // The audio is the whole remaining RIFF container.
            out.audioData = bytes.mid(audioStart);
            out.audioFourCC = QString::fromLatin1(bytes.mid(audioStart + 8, 4));
        }
        else
        {
            out.audioData = bytes.mid(audioStart);
            out.audioFourCC = QString::fromLatin1(audioTag);
        }
        LOG_DEBUG(QString("FuzParser: real form - lip=%1 audio=%2 (%3)")
            .arg(out.lipData.size()).arg(out.audioData.size())
            .arg(out.audioFourCC.isEmpty() ? QStringLiteral("none") : out.audioFourCC));
        return true;
    }

    // Chunked form (legacy tools): [4-byte FourCC][4-byte size][data] after
    // the "FUZE" magic.
    int pos = 4;
    while (pos + 8 <= bytes.size())
    {
        const QByteArray fourCC = bytes.mid(pos, 4);
        const quint32 size = readU32(bytes, pos + 4);
        pos += 8;

        if (pos + static_cast<int>(size) > bytes.size())
        {
            LOG_WARNING("FuzParser: chunk extends past end of file");
            break;
        }

        const QByteArray data = bytes.mid(pos, static_cast<int>(size));
        pos += static_cast<int>(size);

        if (fourCC == "LIPF")
        {
            out.lipData = data;
        }
        else if (fourCC == "XWAV" || fourCC == "XWM " || fourCC == "WAVE")
        {
            out.audioData = data;
            out.audioFourCC = QString::fromLatin1(fourCC);
        }
    }

    LOG_DEBUG(QString("FuzParser: chunked form - lip=%1 audio=%2 (%3)")
        .arg(out.lipData.size()).arg(out.audioData.size())
        .arg(out.audioFourCC.isEmpty() ? QStringLiteral("none") : out.audioFourCC));
    return true;
}

bool FuzParser::loadFile(const QString& path, FuzParser& out)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        LOG_WARNING(QString("FuzParser::loadFile: cannot open %1").arg(path));
        return false;
    }
    const QByteArray data = file.readAll();
    file.close();
    return parse(data, out);
}
