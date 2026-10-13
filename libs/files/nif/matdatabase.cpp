#include "matdatabase.hpp"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QByteArray>

namespace {

quint32 le32(const char* p)
{
    return quint32(quint8(p[0])) | (quint32(quint8(p[1])) << 8)
         | (quint32(quint8(p[2])) << 16) | (quint32(quint8(p[3])) << 24);
}

// Reads a DIFF record's field payload, which is "u16 0 + u16 length + text +
// 00 FF FF". The length counts one byte past the end of the text, so the
// terminator rather than the count delimits it.
// Reads a DIFF record payload: "u16 0 + u16 length + text + 00 FF FF".
QString fieldText(const QByteArray& payload)
{
    if (payload.size() < 7)
        return QString();
    return QString::fromUtf8(payload.constData() + 4, payload.size() - 7);
}

// A texture file's channel comes from its name suffix, the same vocabulary the
// loose .mat files use ("..._color.DDS"). This is how the slot map in
// MatChannel was measured.
int channelOf(const QString& path)
{
    static const struct { const char* suffix; int channel; } table[] = {
        { "_normal",     1 },
        { "_opacity",    2 },
        { "_rough",      3 },
        { "_roughness",  3 },
        { "_metal",      4 },
        { "_metalness",  4 },
        { "_ao",         5 },
        { "_height",     6 },
        { "_emissive",   7 },
        { "_transmissive", 8 },
        { "_mask",      10 },
        { "_curvature",  9 },
        { "_color",      0 },
        { "_albedo",     0 },
    };

    const QString lower = path.toLower();
    const int dot = lower.lastIndexOf(QLatin1Char('.'));
    const QString stem = (dot > 0) ? lower.left(dot) : lower;
    const int cut = stem.lastIndexOf(QLatin1Char('_'));

    // Trailing digits ("_01") are part of the name, not of the channel.
    QString tail = (cut > 0) ? stem.mid(cut) : QString();
    while (!tail.isEmpty() && tail.back().isDigit())
        tail.chop(1);

    for (const auto& e : table)
        if (tail == QLatin1String(e.suffix))
            return e.channel;
    return -1;
}

// Field type of a DIFF record, or -1 when the bytes are not one.
int fieldType(const QByteArray& ba, qint64 at)
{
    if (at + 12 > ba.size())
        return -1;
    const char* p = ba.constData() + at;
    if (p[0] != 'D' || p[1] != 'I' || p[2] != 'F' || p[3] != 'F')
        return -1;
    const quint32 len = le32(p + 4);
    if (len < 4 || at + 8 + len > ba.size())
        return -1;
    return static_cast<int>(le32(p + 8));
}

} // namespace

MatDatabase::MatDatabase(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return;

    // The material objects live far past the header tables, so start where the
    // name records begin.
    static const qint64 kNameRegion = 24 << 20;
    if (!f.seek(qMin(f.size(), kNameRegion))) {
        f.close();
        return;
    }
    const qint64 base = f.pos();
    const QByteArray blob = f.readAll();
    f.close();

    // A texture set is named "<Family>_TextureSet<n>" and the texture-file
    // records that follow it, until the next name record, belong to it.
    static const QRegularExpression nameRe(
        QStringLiteral("[A-Za-z0-9_]+_TextureSet[0-9]+"),
        QRegularExpression::CaseInsensitiveOption);
    int pos = 0;
    const int n = blob.size();
    while (pos < n) {
        const int nameAt = blob.indexOf("_TextureSet", pos);
        if (nameAt < 0)
            break;

        // Walk back to the start of the identifier this name fragment sits in.
        int begin = nameAt;
        while (begin > 0) {
            const char c = blob.at(begin - 1);
            const bool ident = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                             || (c >= '0' && c <= '9') || c == '_';
            if (!ident) break;
            --begin;
        }

        // ...and forward past the role and its digits.
        int end = nameAt + 11;
        while (end < n && blob.at(end) >= '0' && blob.at(end) <= '9')
            ++end;
        const QString name = QString::fromUtf8(blob.constData() + begin, end - begin);

        if (nameRe.match(name).hasMatch() && !m_sets.contains(name)) {
            QList<Slot>& slotList = m_sets[name];
            int at = end;
            int guard = 0;
            while (at + 12 <= n && guard++ < 4096) {
                const int type = fieldType(blob, at);
                if (type < 0) {
                    const int nxt = blob.indexOf("DIFF", at);
                    if (nxt < 0 || nxt - at > 128)
                        break;
                    at = nxt;
                    continue;
                }
                const quint32 len = le32(blob.constData() + at + 4);
                if (type == 0x1F4)
                    break; // the next object's name ends this run
                if (type == 0x38A) {
                    const QString path = fieldText(blob.mid(at + 12, len - 4));
                    const int ch = channelOf(path);
                    bool dup = false;
                    for (const Slot& s : slotList)
                        if (s.index == ch) { dup = true; break; }
                    if (ch >= 0 && !dup)
                        slotList.push_back({ ch, path });
                }
                at += 8 + len;
            }
            if (slotList.isEmpty())
                m_sets.remove(name);
        }

        pos = nameAt + 1;
    }

    m_valid = !m_sets.isEmpty();
}

QString MatDatabase::matDocument(const QString& textureSetName) const
{
    const auto it = m_sets.constFind(textureSetName);
    if (it == m_sets.constEnd() || it->isEmpty())
        return QString();

    QString comps;
    for (const Slot& s : *it) {
        QString escaped = s.path;
        escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
        comps += QStringLiteral(",{\"Index\":%1,\"Type\":\"BSMaterial::MRTextureFile\","
                                "\"Data\":{\"FileName\":\"%2\"}}")
                    .arg(s.index).arg(escaped);
    }

    return QStringLiteral("{\"Objects\":[{\"Components\":["
                          "{\"Index\":0,\"Type\":\"BSComponentDB::CTName\","
                          "\"Data\":{\"Name\":\"%1\"}}%2]}]}")
        .arg(textureSetName, comps);
}
