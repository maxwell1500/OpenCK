#include "esmwriter.hpp"

#include "common.hpp"

ESMWriter::ESMWriter()
{
    header.blank();
}

void ESMWriter::setVersion(float version)
{
    header.version = version;
}

void ESMWriter::setAuthor(const QString& author)
{
    header.author = author;
}

void ESMWriter::setDescription(const QString& description)
{
    header.description = description;
}

void ESMWriter::setNumRecords(qint32 numRecords)
{
    header.numRecords = numRecords;
}

void ESMWriter::setNextObjectId(quint32 nextObjectId)
{
    header.nextObjectID = nextObjectId;
}

void ESMWriter::clearMasters()
{
    header.masters.clear();
}

void ESMWriter::addMaster(QString name, quint64 size)
{
    MasterData master{ name, size };
    header.masters.push_back(master);
}

void ESMWriter::setFileFlags(quint32 flags)
{
    mFileFlags = flags;
}

void ESMWriter::save(QFile& file)
{
    stream.setDevice(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    recordsWritten = 0;

    RecHeader tes4Header;
    tes4Header.flags.val = mFileFlags;
    startRecord('TES4', tes4Header);
    header.save(*this);
    endRecord();
}

void ESMWriter::startRecord(NAME name, RecHeader header)
{
    recordsWritten++;
    recSizePos = stream.device()->pos() + static_cast<qint64>(sizeof(NAME));
    header.save(*this, swapName(name));
    recPos = stream.device()->pos();
}

void ESMWriter::endRecord()
{
    qint64 currentPos{ stream.device()->pos() };
    stream.device()->seek(recSizePos);
    writeType<quint32>(static_cast<quint32>(currentPos - recPos));
    stream.device()->seek(currentPos);
}

void ESMWriter::startSubRecord(NAME name)
{
    writeType<NAME>(swapName(name));
    subSizePos = stream.device()->pos();
    writeType<quint16>(0);
    subPos = stream.device()->pos();
}

void ESMWriter::endSubRecord()
{
    qint64 currentPos{ stream.device()->pos() };
    stream.device()->seek(subSizePos);
    writeType<quint16>(static_cast<quint16>(currentPos - subPos));
    stream.device()->seek(currentPos);
}

void ESMWriter::startGrup(quint32 label, quint32 groupType)
{
    writeType<NAME>(swapName(NAME('GRUP')));
    grupSizePos = stream.device()->pos();
    writeType<quint32>(0);      // size, patched in endGrup
    // Top-level group labels are record-type names stored as ASCII; cell
    // children group labels are the owning cell's form id, stored raw.
    writeType<quint32>(groupType == 0 ? swapName(label) : label);
    writeType<quint32>(groupType);
    writeType<quint8>(0);       // vc day
    writeType<quint8>(0);       // vc month
    writeType<quint8>(0);       // recent user
    writeType<quint8>(0);       // current user
    writeType<quint32>(0);      // unknown
}

void ESMWriter::endGrup()
{
    qint64 currentPos{ stream.device()->pos() };
    // The group size excludes the 'GRUP' name and the size field itself
    // (matches ESMReader::skipGrupHeader, which adds grupSize to the
    // position following the size field).
    stream.device()->seek(grupSizePos);
    writeType<quint32>(static_cast<quint32>(currentPos - grupSizePos - 4));
    stream.device()->seek(currentPos);
}

void ESMWriter::writeZString(const QString& str)
{
    qint32 size = static_cast<qint32>(str.size()) + 1;
    buf.resize(size);
    buf.fill('\0', size);
    QByteArray bytes{ str.toUtf8() };
    bytes.push_back('\0');
    stream.writeRawData(bytes.data(), bytes.size());
}

void ESMWriter::writeSubZString(NAME name, const QString &str)
{
    startSubRecord(name);
    writeZString(str);
    endSubRecord();
}

void ESMWriter::writeRawSubRecord(const RawSubRecord& raw)
{
    const qint32 size = static_cast<qint32>(raw.data.size());
    if (size > 0xFFFF)
    {
        // Extended-size subrecord: XXXX prefix carries the real size, then
        // the subrecord header with a 0 size field.
        writeType<NAME>(swapName(NAME('XXXX')));
        writeType<quint16>(4);
        writeType<quint32>(static_cast<quint32>(size));
        writeType<NAME>(swapName(raw.name));
        writeType<quint16>(0);
        writeRawData(raw.data.constData(), size);
    }
    else
    {
        startSubRecord(raw.name);
        writeRawData(raw.data.constData(), size);
        endSubRecord();
    }
}

void ESMWriter::close()
{
    // Do not include TES4 record in numRecords
    stream.device()->seek(numRecordsPos);
    writeType<quint32>(static_cast<quint32>(recordsWritten - 1));
}
