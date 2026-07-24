#ifndef ESMESMREADER_H
#define ESMESMREADER_H

#include "esmfile.hpp"
#include "records.hpp"
#include "tes4.hpp"

#include <QBuffer>
#include <QDataStream>
#include <QFile>
#include <QString>

class ESMReader
{
public:
    ESMReader(const QString& path);
    ESMReader(const QString& path, const FilePaths& filePaths);
    ~ESMReader();

    void open();
    void startStream();

    NAME readName();
    bool isNextName(NAME name);
    void skipGrupHeader();
    RecHeader readHeader();
    NAME readNSubHeader();
    quint16 readSubHeader();
    QString readZString();
    QString readSubZString(NAME name);

    quint32 currentFormId() const { return mCurrentFormId; }

    bool isLeft();
    bool isRecLeft();
    qint64 recLeft() const { return esm.recLeft; }
    bool isSubLeft();
    qint64 subLeft() const { return esm.subLeft; }
    int recordCount();

    void skipRecord();
    void skipRemainingRecord();
    void skipSub();
    void skip(int bytes);

    const Header& getHeader() const;
    Header getHeader();

    const Strings& getStrings() const;

    bool localised() const;

    qint64 filePos() const;
    void seekTo(qint64 pos);

    template<typename T>
    T peekType()
    {
        // Always peek from the file, regardless of whether the stream is
        // currently reading from a decompressed buffer. The file position
        // is the source of truth for "where the next record starts".
        T data;
        qint64 savedPos = esm.file.pos();
        char buf[sizeof(T)];
        qint64 bytesRead = esm.file.peek(buf, sizeof(T));
        (void)bytesRead;
        memcpy(&data, buf, sizeof(T));
        esm.file.seek(savedPos);
        return data;
    }

    qint64 grupEnd() const { return mGrupEnd; }
    void skipToGrupEnd();

    template<typename T>
    inline T readType(bool recHeader = false)
    {
        T data;
        buf.resize(sizeof(T));
        stream.readRawData(buf.data(), sizeof(T));
        memcpy(&data, buf.data(), sizeof(T));
        esm.forward(sizeof(T), recHeader);
        return data;
    }

    template<typename T>
    T readSubData(NAME expectedName)
    {
        NAME actualName = readNSubHeader();

        if (actualName != expectedName)
        {
            throw std::runtime_error("Error process subrecord - unexpected name.");
        }

        T data;
        buf.resize(sizeof(T));
        stream.readRawData(buf.data(), sizeof(T));
        memcpy(&data, buf.data(), sizeof(T));
        esm.forward(sizeof(T));
        return data;
    }

    void readRawSubData(QByteArray& data)
    {
        qint64 sz = esm.subLeft;
        if (sz > 0)
        {
            data.resize(static_cast<int>(sz));
            stream.readRawData(data.data(), static_cast<int>(sz));
            esm.forward(sz);
        }
        else
        {
            data.clear();
        }
    }

private:
    [[noreturn]] void notifyFailure(const QString& msg);
    void decompressCurrentRecord(int compressedSize);
    void restoreStreamFromCompression();

    ESMFile esm;
    QDataStream stream;
    QByteArray buf;
    quint32 mCurrentFormId = 0;
    qint64 mGrupEnd = 0;
    QScopedPointer<QBuffer> compressedBuffer;
    QByteArray compressedData;
    qint64 mCompressedFileStart = 0;

    Header header;
};

#endif // ESMESMREADER_H
