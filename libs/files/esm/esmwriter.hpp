#ifndef ESMWRITER_H
#define ESMWRITER_H

#include "common.hpp"
#include "tes4.hpp"

#include <QDataStream>
#include <QFile>

const quint8 numRecordsPos = 28;

class ESMWriter
{
public:
    ESMWriter();
    void setVersion(float version);
    void setAuthor(const QString& author);
    void setDescription(const QString& description);
    void setNumRecords(qint32 numRecords);
    void clearMasters();
    void addMaster(QString name, quint64 size = 0);

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

    void close();

private:
    Header header;
    qint32 recordsWritten;
    qint64 recSizePos;
    qint64 recPos;
    qint64 subSizePos;
    qint64 subPos;

    QByteArray buf;
    QDataStream stream;
    quint32 mFileFlags = 0;
};

#endif // ESMWRITER_H
