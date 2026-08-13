#ifndef ESMESMREADER_H
#define ESMESMREADER_H

#include "esmfile.hpp"
#include "records.hpp"
#include "tes4.hpp"

#include <QFile>
#include <QString>

// ESMReader reads plugin files via a memory-mapped view of the file
// (QFile::map), so parsing is zero-copy: the OS page cache pages the file
// in on demand and the reader memcpys field-sized slices out of the mapped
// region. Compressed records (flag 0x00040000) are inflated once into a
// QByteArray and reads switch to that buffer until the record is drained.
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
        // Always peek from the mapped file, regardless of whether reads are
        // currently drawing from a decompressed record buffer. The file
        // position is the source of truth for "where the next record starts".
        T data{};
        peekRaw(reinterpret_cast<char*>(&data), sizeof(T));
        return data;
    }

    qint64 grupEnd() const { return mGrupEnd; }
    void skipToGrupEnd();

    template<typename T>
    inline T readType(bool recHeader = false)
    {
        T data{};
        readRaw(reinterpret_cast<char*>(&data), sizeof(T));
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

        T data{};
        readRaw(reinterpret_cast<char*>(&data), sizeof(T));
        esm.forward(sizeof(T));
        return data;
    }

    void readRawSubData(QByteArray& data)
    {
        qint64 sz = esm.subLeft;
        if (sz > 0)
        {
            data.resize(static_cast<int>(sz));
            readRaw(data.data(), sz);
            esm.forward(sz);
        }
        else
        {
            data.clear();
        }
    }

private:
    // Copies `len` bytes from the current read position into `dest` and
    // advances the position. Short reads are zero-filled (mirrors the old
    // QDataStream::readRawData behavior the callers rely on).
    void readRaw(char* dest, qint64 len);
    // Like readRaw but does not advance (always from the mapped file).
    void peekRaw(char* dest, qint64 len) const;

    [[noreturn]] void notifyFailure(const QString& msg);
    void decompressCurrentRecord(int compressedSize);
    void restoreStreamFromCompression();

    ESMFile esm;
    quint32 mCurrentFormId = 0;
    qint64 mGrupEnd = 0;

    uchar* m_mapped = nullptr;
    qint64 m_mappedSize = 0;
    qint64 m_pos = 0;

    QByteArray m_decompData;
    qint64 m_decompPos = 0;
    bool m_inDecomp = false;

    Header header;
};

#endif // ESMESMREADER_H
