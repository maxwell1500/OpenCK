#include <QtTest>
#include <QFile>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QHash>
#include <QMap>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/Miscrecord.hpp"
#include "../../libs/files/esm/Questrecord.hpp"
#include "../../libs/files/esm/Armorrecord.hpp"
#include "../../libs/files/esm/effectshaderrecord.hpp"
#include "../../libs/files/esm/worldspacerecord.hpp"
#include "../../libs/files/esm/Inforecord.hpp"
#include "../../libs/files/log/logger.hpp"

// Structured-save fidelity gate: loads every record of the target types from
// the real Starfield.esm, re-saves through the STRUCTURED path (record
// save(), bypassing the verbatim fast path that covers untouched records in
// file round-trips), and byte-compares the emitted body with the source
// body. Mismatches are exactly the §1 structured-save polish list; MISC is
// the calibration type and must stay at zero.
class TestStructuredFidelity : public QObject
{
    Q_OBJECT

private:
    QString fixture() const
    {
        return qEnvironmentVariable(
                   "OPENCK_DATA_DIR",
                   QStringLiteral("C:/XboxGames/Starfield/Content/Data"))
            + QStringLiteral("/Starfield.esm");
    }

    struct Entry
    {
        QString type;
        QString id;
        QByteArray srcBody;
        qint64 outOff = 0;
        qint64 outSize = 0;
    };

    // True when bytes parse as a TES4 subrecord stream (name16... size16
    // chain with XXXX extension). Compressed spans fail this and are
    // counted as skipped, not mismatched.
    static bool parsesAsSubStream(const QByteArray& body)
    {
        const quint8* b = reinterpret_cast<const quint8*>(body.constData());
        qint64 pos = 0;
        const qint64 n = body.size();
        while (pos + 6 <= n)
        {
            const quint32 name = quint32(b[pos]) | (quint32(b[pos + 1]) << 8)
                | (quint32(b[pos + 2]) << 16) | (quint32(b[pos + 3]) << 24);
            const quint32 size = quint32(b[pos + 4]) | (quint32(b[pos + 5]) << 8);
            if (name == 0x58585858u && size == 4)
            {
                if (pos + 16 > n)
                    return false;
                const quint32 real = quint32(b[pos + 8]) | (quint32(b[pos + 9]) << 8)
                    | (quint32(b[pos + 10]) << 16) | (quint32(b[pos + 11]) << 24);
                pos += 16 + real;
            }
            else
            {
                pos += 6 + size;
            }
        }
        return pos == n;
    }

    static QString subStreamNames(const QByteArray& body)
    {
        const quint8* b = reinterpret_cast<const quint8*>(body.constData());
        qint64 pos = 0;
        const qint64 n = body.size();
        QStringList names;
        while (pos + 6 <= n)
        {
            char name[5] = { char(b[pos]), char(b[pos + 1]), char(b[pos + 2]), char(b[pos + 3]), 0 };
            const quint32 size = quint32(b[pos + 4]) | (quint32(b[pos + 5]) << 8);
            if (strncmp(name, "XXXX", 4) == 0 && size == 4)
            {
                if (pos + 16 > n)
                    break;
                const quint32 real = quint32(b[pos + 8]) | (quint32(b[pos + 9]) << 8)
                    | (quint32(b[pos + 10]) << 16) | (quint32(b[pos + 11]) << 24);
                names.append(QString("XXXX(%1)").arg(real));
                pos += 16 + real;
            }
            else
            {
                names.append(QString("%1(%2)").arg(name).arg(size));
                pos += 6 + size;
            }
            if (names.size() > 400)
            {
                names.append("...");
                break;
            }
        }
        return names.join(" ");
    }

    ESMReader* m_reader = nullptr;
    ESMWriter* m_out = nullptr;
    QFile* m_outFile = nullptr;
    QVector<Entry> m_entries;
    qint64 m_walked = 0;

    template <typename Record>
    void handleRecord(NAME code, const QString& type, Record& rec)
    {
        rec.load(*m_reader, false);
        Entry e;
        e.type = type;
        e.id = rec.editorId.isEmpty()
            ? QString("form_%1").arg(rec.formId, 8, 16, QChar('0'))
            : rec.editorId;
        e.srcBody = m_reader->lastRecordBody();
        e.outOff = m_outFile->pos();
        RecHeader rh;
        m_out->startRecord(code, rh);
        rec.save(*m_out);
        m_out->endRecord();
        e.outSize = m_outFile->pos() - e.outOff;
        m_entries.append(e);
        if ((m_entries.size() % 50000) == 0)
        {
            fprintf(stderr, "fidelity: %lld entries, src pos %lld\n",
                    static_cast<long long>(m_entries.size()), m_reader->filePos());
            fflush(stderr);
        }
    }

    // Index-driven visit: absolute seekTo per record, no GRUP tracking and
    // no sequential-skip surface (buildRecordIndex is the proven scanner).
    void handleByIndex(const RecordIndexEntry& entry)
    {
        m_reader->seekTo(entry.offset);
        NAME n = m_reader->readName();
        if (n == 0 || n == NAME('GRUP'))
            return;
        if (qEnvironmentVariableIsSet("FIDELITY_SKIPONLY"))
            return;
        switch (n)
        {
        case 'MISC': { MiscRecord r; handleRecord(n, "MISC", r); break; }
        case 'QUST': { QuestRecord r; handleRecord(n, "QUST", r); break; }
        case 'ARMO': { ArmorRecord r; handleRecord(n, "ARMO", r); break; }
        case 'EFSH': { EfshRecord r; handleRecord(n, "EFSH", r); break; }
        case 'WRLD': { WorldspaceRecord r; handleRecord(n, "WRLD", r); break; }
        case 'INFO': { InfoRecord r; handleRecord(n, "INFO", r); break; }
        default: break;
        }
        if (++m_walked % 1000000 == 0)
        {
            fprintf(stderr, "fidelity: walked %lld index entries\n",
                    static_cast<long long>(m_walked));
            fflush(stderr);
        }
    }

private slots:
    void initTestCase();
    void testStructuredFidelity();
    void testSyntheticStructuredSaves();
};

void TestStructuredFidelity::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Error);
}

void TestStructuredFidelity::testStructuredFidelity()
{
    const QString path = fixture();
    if (!QFile::exists(path))
        QSKIP("Starfield.esm fixture not available (set OPENCK_DATA_DIR)");

    ESMReader reader(path);
    reader.open();
    QVERIFY(!reader.tes3());
    fprintf(stderr, "fidelity: opened ok\n");
    fflush(stderr);
    // open() leaves the byte accounting at zero; resync it so isLeft()
    // works (same as Data::preload via seekTo/buildRecordIndex).
    reader.seekTo(reader.filePos());
    fprintf(stderr, "fidelity: walk start\n");
    fflush(stderr);

    QTemporaryFile outFile;
    QVERIFY(outFile.open());
    ESMWriter out;
    out.save(outFile);
    fprintf(stderr, "fidelity: out header ok\n");
    fflush(stderr);

    m_reader = &reader;
    m_out = &out;
    m_outFile = &outFile;
    m_entries.clear();
    m_walked = 0;
    QVector<RecordIndexEntry> index;
    reader.buildRecordIndex(index);
    fprintf(stderr, "fidelity: index built, %lld records\n", static_cast<long long>(index.size()));
    fflush(stderr);
    for (const RecordIndexEntry& entry : index)
        handleByIndex(entry);
    fprintf(stderr, "fidelity: walk done, %lld entries\n", static_cast<long long>(m_entries.size()));
    fflush(stderr);
    outFile.flush();
    outFile.close();

    // Read back the structured bodies: skip the TES4 file header record,
    // then each record is name(4) + size(u32 LE) + body.
    QFile back(outFile.fileName());
    QVERIFY(back.open(QIODevice::ReadOnly));
    const QByteArray all = back.readAll();
    back.close();
    fprintf(stderr, "fidelity: read back %lld bytes\n", static_cast<long long>(all.size()));
    fflush(stderr);
    const quint8* b = reinterpret_cast<const quint8*>(all.constData());
    auto u32At = [&](qint64 p) {
        return quint32(b[p]) | (quint32(b[p + 1]) << 8)
            | (quint32(b[p + 2]) << 16) | (quint32(b[p + 3]) << 24);
    };
    qint64 pos = 0;
    QVERIFY(all.size() >= 24);
    pos += 24 + u32At(4); // file header record
    int compared = 0;
    QMap<QString, int> matched;
    QMap<QString, int> mismatched;
    QMap<QString, int> skipped;
    QMap<QString, QStringList> mismatchIds;
    QSet<QString> mismatchDetailPrinted;
    for (const Entry& e : m_entries)
    {
        QVERIFY(pos + 24 <= all.size());
        const qint64 bodySize = u32At(pos + 4);
        const QByteArray saved = all.mid(pos + 24, bodySize);
        pos += 24 + bodySize;
        if (!parsesAsSubStream(e.srcBody))
        {
            skipped[e.type]++;
            continue;
        }
        compared++;
        if (saved == e.srcBody)
        {
            matched[e.type]++;
        }
        else
        {
            mismatched[e.type]++;
            qint64 diffAt = -1;
            const qint64 n = qMin(saved.size(), e.srcBody.size());
            for (qint64 i = 0; i < n; ++i)
            {
                if (saved.at(i) != e.srcBody.at(i))
                {
                    diffAt = i;
                    break;
                }
            }
            if (mismatchIds[e.type].size() < 3)
            {
                mismatchIds[e.type].append(QString("%1 src %2 saved %3 diff@%4")
                    .arg(e.id).arg(e.srcBody.size()).arg(saved.size()).arg(diffAt));
            }
            if (!mismatchDetailPrinted.contains(e.type))
            {
                mismatchDetailPrinted.insert(e.type);
                qInfo() << QString("  %1 first mismatch %2 src: %3")
                               .arg(e.type).arg(e.id).arg(subStreamNames(e.srcBody));
                qInfo() << QString("  %1 first mismatch %2 saved: %3")
                               .arg(e.type).arg(e.id).arg(subStreamNames(saved));
                if (diffAt >= 0)
                {
                    const qint64 lo = qMax<qint64>(0, diffAt - 16);
                    qInfo() << QString("  %1 bytes src@%2: %3")
                                   .arg(e.type).arg(diffAt)
                                   .arg(QString(e.srcBody.mid(lo, 48).toHex()));
                    qInfo() << QString("  %1 bytes saved@%2: %3")
                                   .arg(e.type).arg(diffAt)
                                   .arg(QString(saved.mid(lo, 48).toHex()));
                }
            }
        }
    }

    qInfo() << "structured fidelity over" << m_entries.size() << "records," << compared << "compared";
    QSet<QString> types;
    for (const Entry& e : m_entries)
        types.insert(e.type);
    for (const QString& t : types)
    {
        qInfo() << QString("  %1: matched %2 mismatched %3 skipped %4")
                       .arg(t).arg(matched.value(t)).arg(mismatched.value(t)).arg(skipped.value(t));
        for (const QString& id : mismatchIds.value(t))
            qInfo() << QString("    %1").arg(id);
    }

    // Calibration: MISC structured save is already order-exact.
    QCOMPARE(mismatched.value("MISC", 0), 0);
    QVERIFY(matched.value("MISC", 0) > 0);
}

namespace
{
// Hand-crafted TES4 subrecord: ASCII name + u16 LE size + payload.
QByteArray sr(const char* name, const QByteArray& payload)
{
    QByteArray out;
    out.append(name, 4);
    const quint16 sz = static_cast<quint16>(payload.size());
    out.append(char(sz & 0xFF));
    out.append(char((sz >> 8) & 0xFF));
    out.append(payload);
    return out;
}

QByteArray u32le(quint32 v)
{
    QByteArray out;
    out.append(char(v & 0xFF));
    out.append(char((v >> 8) & 0xFF));
    out.append(char((v >> 16) & 0xFF));
    out.append(char((v >> 24) & 0xFF));
    return out;
}

QByteArray f32le(float v)
{
    quint32 bits = 0;
    memcpy(&bits, &v, 4);
    return u32le(bits);
}

QByteArray zstr(const char* s)
{
    QByteArray out(s);
    out.append('\0');
    return out;
}

// Writes a minimal single-record plugin; returns its path (dir must outlive).
// Returns an empty path when the file cannot be written.
QString writePlugin(QTemporaryDir& dir, const QString& name, NAME code,
                    const QByteArray& body, quint32 formId)
{
    const QString path = dir.filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return QString();
    ESMWriter w;
    w.save(f);
    RecHeader rh;
    rh.id = formId;
    w.startRecord(code, rh);
    w.writeRawData(body.constData(), body.size());
    w.endRecord();
    w.close();
    f.close();
    return path;
}

// Body bytes of the single record in a plugin file (empty on parse failure).
QByteArray readPluginBody(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QByteArray();
    const QByteArray all = f.readAll();
    f.close();
    if (all.size() <= 24)
        return QByteArray();
    const quint8* b = reinterpret_cast<const quint8*>(all.constData());
    const quint32 tes4Size = quint32(b[4]) | (quint32(b[5]) << 8)
        | (quint32(b[6]) << 16) | (quint32(b[7]) << 24);
    const qint64 recStart = 24 + tes4Size;
    if (all.size() < recStart + 24)
        return QByteArray();
    const quint32 recSize = quint32(b[recStart + 4]) | (quint32(b[recStart + 5]) << 8)
        | (quint32(b[recStart + 6]) << 16) | (quint32(b[recStart + 7]) << 24);
    return all.mid(recStart + 24, recSize);
}

template <typename Record>
bool loadRecord(const QString& path, NAME code, Record& rec)
{
    if (path.isEmpty())
        return false;
    ESMReader reader(path);
    reader.open();
    if (reader.readName() != code)
        return false;
    rec.load(reader, false);
    return true;
}

template <typename Record>
QByteArray saveRecordToBody(QTemporaryDir& dir, const QString& name, NAME code,
                           const Record& rec, quint32 formId)
{
    const QString path = dir.filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return QByteArray();
    ESMWriter w;
    w.save(f);
    RecHeader rh;
    rh.id = formId;
    w.startRecord(code, rh);
    rec.save(w);
    w.endRecord();
    w.close();
    f.close();
    return readPluginBody(path);
}
}

// Synthetic structured saves: hand-crafted tricky bodies (duplicate QSDT,
// binary CNAM, differing MODL/INDX duplicates, empty FNAM/DATA, reordered
// EFSH, duplicate NAM2/NAM3) round-trip byte-exact, and edits land
// positionally. Always runs (no fixture).
void TestStructuredFidelity::testSyntheticStructuredSaves()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    // QUST: interleaved groups, duplicate QSDT, binary CNAM, questDesc,
    // view, missing QSDT, short QOBJ, short ALST, alias-group QSDT.
    QByteArray questBody;
    questBody += sr("EDID", zstr("SynthQuest"));
    questBody += sr("FULL", QByteArray("\x01\x02\x03\x04", 4));
    questBody += sr("DNAM", QByteArray(12, '\0'));
    questBody += sr("CNAM", zstr("Top-level description"));
    questBody += sr("NAM1", zstr("View"));
    questBody += sr("INDX", u32le(10));
    questBody += sr("QSDT", QByteArray("\x02", 1));
    questBody += sr("QSDT", QByteArray("\x05", 1));
    questBody += sr("CNAM", QByteArray("\x00\xe8\xf3\x02", 4));
    questBody += sr("INDX", u32le(20));
    questBody += sr("QOBJ", QByteArray("\x01\x02\x03\x04\x05\x06\x07\x08", 8));
    questBody += sr("FNAM", QByteArray("\xaa\xbb\xcc\xdd", 4));
    questBody += sr("ALST", QByteArray(8, '\0'));
    questBody += sr("ALID", zstr("alias-one"));
    questBody += sr("CTDA", QByteArray(8, '\0'));
    questBody += sr("QSDT", QByteArray("\x01", 1));
    {
        const QString src = writePlugin(dir, "synth_quest.esp", NAME('QUST'), questBody, 0x801);
        QVERIFY(!src.isEmpty());
        QuestRecord rec;
        QVERIFY(loadRecord(src, NAME('QUST'), rec));
        QCOMPARE(rec.stageIds.size(), 2);
        QCOMPARE(rec.stageFlags[0], quint8(0x05));
        const QByteArray saved = saveRecordToBody(dir, "synth_quest_out.esp", NAME('QUST'), rec, 0x801);
        QCOMPARE(saved, questBody);

        // Edit lands on the last duplicate only.
        rec.stageFlags[0] = 0x09;
        const QByteArray edited = saveRecordToBody(dir, "synth_quest_edit.esp", NAME('QUST'), rec, 0x801);
        QCOMPARE(edited.size(), questBody.size());
        QVERIFY(edited != questBody);
        // First QSDT intact, second carries the edit.
        const QByteArray qsdt("\x51\x53\x44\x54\x01\x00\x02", 7); // QSDT size 1 value 02
        const int firstQsdt = questBody.indexOf(qsdt);
        QVERIFY(firstQsdt >= 0);
        QCOMPARE(edited.mid(firstQsdt, 7), qsdt);
        QVERIFY(edited.contains(QByteArray("\x51\x53\x44\x54\x01\x00\x09", 7)));
    }

    // ARMO: differing MODL/INDX duplicates, 12-byte DATA, 8-zero FNAM.
    QByteArray armoBody;
    armoBody += sr("EDID", zstr("SynthArmor"));
    armoBody += sr("FLAG", u32le(0x12));
    armoBody += sr("DNAM", u32le(100));
    armoBody += sr("DATA", u32le(50) + f32le(2.5f) + QByteArray("\xde\xad\xbe\xef", 4));
    armoBody += sr("MODL", QByteArray("\x04\x22\x01\x00", 4));
    armoBody += sr("MODL", QByteArray("\x80\x05\x03\x00", 4));
    armoBody += sr("INDX", QByteArray("\x00\x00", 2));
    armoBody += sr("INDX", QByteArray("\x01\x00", 2));
    armoBody += sr("FNAM", QByteArray(8, '\0'));
    armoBody += sr("STOP", QByteArray());
    {
        const QString src = writePlugin(dir, "synth_armo.esp", NAME('ARMO'), armoBody, 0x802);
        ArmorRecord rec; QVERIFY(loadRecord(src, NAME('ARMO'), rec));
        const QByteArray saved = saveRecordToBody(dir, "synth_armo_out.esp", NAME('ARMO'), rec, 0x802);
        QCOMPARE(saved, armoBody);

        rec.value = 99;
        const QByteArray edited = saveRecordToBody(dir, "synth_armo_edit.esp", NAME('ARMO'), rec, 0x802);
        QCOMPARE(edited.size(), armoBody.size());
        QVERIFY(edited != armoBody);
    }

    // INFO without EDID, empty FNAM/HNAM: nothing invented, widths kept.
    QByteArray infoBody;
    infoBody += sr("ENAM", u32le(0x1234));
    infoBody += sr("FNAM", QByteArray());
    infoBody += sr("HNAM", QByteArray());
    infoBody += sr("CNAM", zstr("Response"));
    infoBody += sr("CTDA", QByteArray(32, '\0'));
    infoBody += sr("TLOI", u32le(0x5678));
    infoBody += sr("VMAP", zstr("voice"));
    infoBody += sr("INAM", u32le(0x9ABC));
    {
        const QString src = writePlugin(dir, "synth_info.esp", NAME('INFO'), infoBody, 0x803);
        InfoRecord rec; QVERIFY(loadRecord(src, NAME('INFO'), rec));
        const QByteArray saved = saveRecordToBody(dir, "synth_info_out.esp", NAME('INFO'), rec, 0x803);
        QCOMPARE(saved, infoBody);
    }

    // EFSH with ENAM before DATA and an empty DATA: order + width kept.
    QByteArray efshBody;
    efshBody += sr("EDID", zstr("SynthFX"));
    efshBody += sr("ENAM", u32le(0x1111));
    efshBody += sr("DATA", QByteArray());
    efshBody += sr("DNAM", QByteArray(8, '\0'));
    efshBody += sr("FLLD", QByteArray(4, '\0'));
    {
        const QString src = writePlugin(dir, "synth_efsh.esp", NAME('EFSH'), efshBody, 0x804);
        EfshRecord rec; QVERIFY(loadRecord(src, NAME('EFSH'), rec));
        const QByteArray saved = saveRecordToBody(dir, "synth_efsh_out.esp", NAME('EFSH'), rec, 0x804);
        QCOMPARE(saved, efshBody);
    }

    // WRLD with differing duplicate NAM2/NAM3: first stays typed, rest raw.
    QByteArray wrldBody;
    wrldBody += sr("EDID", zstr("SynthWorld"));
    wrldBody += sr("FULL", QByteArray("\x05\x00\x00\x00", 4));
    wrldBody += sr("CNAM", u32le(3));
    wrldBody += sr("NAM2", u32le(7));
    wrldBody += sr("NAM3", u32le(3));
    wrldBody += sr("MNAM", QByteArray(16, '\0'));
    wrldBody += sr("NAM2", u32le(9));
    wrldBody += sr("NAM3", u32le(5));
    wrldBody += sr("DNAM", QByteArray(8, '\0'));
    {
        const QString src = writePlugin(dir, "synth_wrld.esp", NAME('WRLD'), wrldBody, 0x805);
        WorldspaceRecord rec; QVERIFY(loadRecord(src, NAME('WRLD'), rec));
        QCOMPARE(rec.mapWidth, quint32(7));
        QCOMPARE(rec.mapHeight, quint32(3));
        const QByteArray saved = saveRecordToBody(dir, "synth_wrld_out.esp", NAME('WRLD'), rec, 0x805);
        QCOMPARE(saved, wrldBody);
    }

    qDebug() << "synthetic structured saves OK";
}

QTEST_MAIN(TestStructuredFidelity)
#include "test_structuredfidelity.moc"
