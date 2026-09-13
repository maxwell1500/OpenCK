#ifndef ESMWRITER_H
#define ESMWRITER_H

#include "common.hpp"
#include "tes4.hpp"
#include "records.hpp"

#include <QDataStream>
#include <QFile>

// Absolute file offset of the HEDR numRecords field: TES4 record header
// (24) + 'HEDR' name (4) + subrecord size (2) + version float (4).
const quint8 numRecordsPos = 34;
// Absolute file offset of the HEDR numRecords field for TES3: record header
// (16) + 'HEDR' name (4) + subrecord size (4) + version float (4) +
// file type (4) + 32-byte author + 256-byte description.
const quint16 tes3NumRecordsPos = 320;

class ESMWriter
{
public:
    ESMWriter();
    void setVersion(float version);
    void setAuthor(const QString& author);
    void setDescription(const QString& description);
    void setNumRecords(qint32 numRecords);
    void setNextObjectId(quint32 nextObjectId);
    void clearMasters();
    void addMaster(QString name, quint64 size = 0);

    // TES3 (Morrowind) output mode: 16-byte record headers, 4-byte subrecord
    // sizes, NUL-less strings, and a 320-byte HEDR record count offset.
    void setTes3(bool value) { m_tes3 = value; }
    bool tes3() const { return m_tes3; }
    void setTes3FileType(quint32 fileType) { header.tes3FileType = fileType; }
    void setFormatVersion(quint32 formatVersion) { header.formatVersion = formatVersion; }
    void setTes3Gmdt(const QByteArray& data) { header.tes3Gmdt = data; }
    void setTes3Scrd(const QByteArray& data) { header.tes3Scrd = data; }
    void setTes3Scrs(const QByteArray& data) { header.tes3Scrs = data; }

    void save(QFile& file);

    // Record flags for the TES4 header (FileFlag::Master, FileFlag::LightMaster).
    // Defaults to 0 (a plain .esp). Preserved on save so an ESL/ESM keeps its
    // master type when round-tripped.
    void setFileFlags(quint32 flags);
    quint32 fileFlags() const { return mFileFlags; }

    void startRecord(NAME name, RecHeader header = RecHeader());
    void endRecord();
    void startSubRecord(NAME name);
    void endSubRecord();
    // Top-level / children GRUP blocks (24-byte group header with a
    // size-patched at endGrup). label is the 4-byte record type for a
    // top-level group (type 0) or the owning cell's form id & 0xFFFFFF
    // for a cell-children group (type 6).
    void startGrup(quint32 label, quint32 groupType);
    void endGrup();

    template<typename T>
    void writeType(T data)
    {
        buf.resize(sizeof(T));
        memcpy(buf.data(), &data, sizeof(T));
        stream.writeRawData(buf.data(), sizeof(T));
    }

    template<typename T>
    void writeSubData(NAME name, T data)
    {
        startSubRecord(name);

        buf.resize(sizeof(T));
        memcpy(buf.data(), &data, sizeof(T));
        stream.writeRawData(buf.data(), sizeof(T));

        endSubRecord();
    }

    void writeZString(const QString& str);
    void writeSubZString(NAME name, const QString& str);

    void writeRawData(const char* data, qint32 size)
    {
        stream.writeRawData(data, size);
    }

    // Writes a raw-preserved subrecord, emitting the XXXX extended-size
    // prefix when the payload exceeds 64 KiB (the on-disk size field is a
    // u16 and cannot represent it).
    void writeRawSubRecord(const RawSubRecord& raw);

    void close();

private:
    Header header;
    qint32 recordsWritten;
    qint64 recSizePos;
    qint64 recPos;
    qint64 subSizePos;
    qint64 subPos;
    QVector<qint64> grupSizePosStack;

    QByteArray buf;
    QDataStream stream;
    quint32 mFileFlags = 0;
    bool m_tes3 = false;
};

#endif // ESMWRITER_H
