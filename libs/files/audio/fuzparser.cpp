#include "fuzparser.hpp"

#include <QFile>

#include "../log/logger.hpp"

bool FuzParser::parse(const QByteArray& bytes, FuzParser& out)
{
    out = FuzParser();

    if (bytes.size() < 8 || bytes.mid(0, 4) != QByteArray("FUZE", 4))
        return false;

    int pos = 4;
    while (pos + 8 <= bytes.size())
    {
        const QByteArray fourCC = bytes.mid(pos, 4);
        const quint32 size = static_cast<quint32>(
            static_cast<quint8>(bytes.at(pos + 4))
            | (static_cast<quint8>(bytes.at(pos + 5)) << 8)
            | (static_cast<quint8>(bytes.at(pos + 6)) << 16)
            | (static_cast<quint8>(bytes.at(pos + 7)) << 24));
        pos += 8;

        if (pos + static_cast<int>(size) > bytes.size())
        {
            LOG_WARNING("FuzParser: chunk extends past end of file");
            break;
        }

        const QByteArray data = bytes.mid(pos, size);
        pos += size;

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

    LOG_DEBUG(QString("FuzParser: lip=%1 bytes audio=%2 bytes (%3)")
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
