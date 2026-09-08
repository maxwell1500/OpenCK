#ifndef SUBRECORDSNAPSHOT_HPP
#define SUBRECORDSNAPSHOT_HPP

// Per-record list of subrecords (name + payload) taken directly from the
// on-disk file via ESMReader, without going through any record loader. Used
// to diff an untouched round-trip saved plugin against its source so the
// save path can be proven payload-identical per record type.
//
// Header-only so both the subrecord_diff tool and the test suite can include
// it without new library targets.

#include "esmreader.hpp"
#include "records.hpp"

#include <QFileInfo>
#include <QString>
#include <QVector>

namespace openck {

struct SubSnapshot
{
    NAME name = 0;
    QByteArray payload;
};

struct RecordSnapshot
{
    NAME type = 0;
    quint32 formId = 0;
    quint32 flags = 0;
    quint32 size = 0;
    quint8 vcDay = 0;
    quint8 vcMonth = 0;
    quint8 vcLastUser = 0;
    quint8 vcCurrUser = 0;
    quint16 version = 0;
    quint16 unknown = 0;
    QVector<SubSnapshot> subs;

    bool operator==(const RecordSnapshot& o) const
    {
        // Header metadata (size/flags/versions) is deliberately not compared:
        // the save path re-encodes headers (e.g. drops the compressed flag),
        // so the round-trip contract is subrecord name + payload identity.
        if (type != o.type || formId != o.formId
            || subs.size() != o.subs.size())
            return false;
        for (int i = 0; i < subs.size(); ++i)
            if (subs[i].name != o.subs[i].name || subs[i].payload != o.subs[i].payload)
                return false;
        return true;
    }
    bool operator!=(const RecordSnapshot& o) const { return !(*this == o); }
};

inline QString snapshotName(NAME n)
{
    if (n == 0)
        return QStringLiteral("NULL");
    return QString(QChar((n >> 24) & 0xFF)) + QChar((n >> 16) & 0xFF)
        + QChar((n >> 8) & 0xFF) + QChar(n & 0xFF);
}

// Walks the file at top level (TES4 excluded), recording each record and its
// subrecords. Iteration is bounded by the file position rather than
// ESMReader::isLeft() because compressed records decrement the byte
// accounting twice (compressed payload charged at parse, decompressed bytes
// charged on read) which would end the walk prematurely.
inline QVector<RecordSnapshot> collectRecordSnapshots(const QString& path)
{
    QVector<RecordSnapshot> out;
    ESMReader reader(path);
    reader.open();

    const qint64 fileSize = QFileInfo(path).size();
    while (reader.filePos() < fileSize)
    {
        NAME name = 0;
        try
        {
            name = reader.readName();
        }
        catch (...)
        {
            break;
        }
        if (name == 0)
            break;

        if (name == NAME('GRUP'))
        {
            reader.skipGrupHeader();
            continue;
        }

        RecHeader header = reader.readHeader();
        RecordSnapshot snap;
        snap.type = name;
        snap.formId = header.id;
        snap.flags = header.flags.val;
        snap.size = header.size;
        snap.vcDay = header.vcDay;
        snap.vcMonth = header.vcMonth;
        snap.vcLastUser = header.vcLastUser;
        snap.vcCurrUser = header.vcCurrUser;
        snap.version = header.version;
        snap.unknown = header.unknown;

        while (reader.isRecLeft())
        {
            NAME sub = reader.readNSubHeader();
            if (sub == 0)
                break;
            SubSnapshot ss;
            ss.name = sub;
            reader.readRawSubData(ss.payload);
            snap.subs.append(ss);
        }

        out.append(snap);
    }
    return out;
}

} // namespace openck

#endif // SUBRECORDSNAPSHOT_HPP