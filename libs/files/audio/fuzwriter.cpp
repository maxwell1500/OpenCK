#include "fuzwriter.hpp"

#include <QDataStream>
#include <QFile>

QByteArray FuzWriter::build(const QByteArray& lipData,
                            const QByteArray& audioData,
                            quint32 version)
{
    // A container with no audio stream cannot use the real form: the
    // reader's real-form sniff requires an audio tag after the lip. The
    // chunked form ([FourCC][size][data]) is what lip-only archives use,
    // and FuzParser accepts exactly that.
    if (audioData.isEmpty() && !lipData.isEmpty())
    {
        QByteArray out("FUZE", 4);
        out.append("LIPF", 4);
        const quint32 lipSize = static_cast<quint32>(lipData.size());
        for (int i = 0; i < 4; ++i)
        {
            out.append(static_cast<char>((lipSize >> (8 * i)) & 0xFF));
        }
        out.append(lipData);
        return out;
    }

    QByteArray out;
    out.reserve(12 + lipData.size() + audioData.size());

    QDataStream stream(&out, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.writeRawData("FUZE", 4);
    stream << version;
    stream << static_cast<quint32>(lipData.size());
    if (!lipData.isEmpty())
    {
        stream.writeRawData(lipData.constData(), lipData.size());
    }
    if (!audioData.isEmpty())
    {
        stream.writeRawData(audioData.constData(), audioData.size());
    }
    return out;
}

bool FuzWriter::writeFile(const QString& path, const QByteArray& lipData,
                          const QByteArray& audioData, quint32 version)
{
    const QByteArray bytes = build(lipData, audioData, version);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    const bool ok = file.write(bytes) == bytes.size();
    file.close();
    return ok;
}
