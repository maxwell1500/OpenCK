#include <QtTest>
#include <QFile>
#include <QHash>
#include <QMap>
#include <QSet>
#include <cstring>
#include <functional>

#include "../../libs/files/esm/esmreader.hpp"
#include "../../libs/files/esm/tes4.hpp"
#include "../../libs/files/esm/tes3datalayout.hpp"
#include "../../libs/components/tes3_components.hpp"
#include "../../libs/files/log/logger.hpp"

// TES3 DATA-subrecord layout survey + typed-layout conformance, gated on the
// real Morrowind.esm. The survey prints the ground-truth (record type ->
// DATA payload sizes -> count, per-lane value ranges, samples) that
// libs/files/esm/tes3datalayout.* encodes.
class TestTes3Data : public QObject
{
    Q_OBJECT

private:
    QString fixture() const
    {
        return qEnvironmentVariable(
            "OPENCK_TEST_MORROWIND_ESM",
            QStringLiteral("C:/XboxGames/The Elder Scrolls III- Morrowind (PC)/Content/"
                           "Morrowind GOTY English/Data Files/Morrowind.esm"));
    }

    static QString spellName(NAME name)
    {
        char buf[5] = { char((name >> 24) & 0xFF), char((name >> 16) & 0xFF),
                        char((name >> 8) & 0xFF), char(name & 0xFF), 0 };
        return QString::fromLatin1(buf);
    }

    static NAME unspellName(const QString& spelled)
    {
        const QByteArray latin = spelled.toLatin1();
        if (latin.size() != 4)
            return 0;
        return (NAME(static_cast<quint8>(latin[0])) << 24)
            | (NAME(static_cast<quint8>(latin[1])) << 16)
            | (NAME(static_cast<quint8>(latin[2])) << 8)
            | NAME(static_cast<quint8>(latin[3]));
    }

    // Walks every record; calls visit(type, dataPayload) for each DATA
    // subrecord. Returns the total record count, or -1 when the fixture is
    // missing.
    static qint64 walkData(const QString& path,
                           const std::function<void(const QString&, const QByteArray&)>& visit,
                           quint32* outNumRecords = nullptr)
    {
        if (!QFile::exists(path))
            return -1;
        ESMReader reader(path);
        reader.open();
        if (!reader.tes3())
            return -1;
        if (outNumRecords)
            *outNumRecords = reader.getHeader().numRecords;
        qint64 count = 0;
        while (reader.isLeft())
        {
            NAME name = reader.readName();
            if (name == 0)
                break;
            reader.readHeader();
            const QString type = spellName(name);
            count++;
            while (reader.isRecLeft())
            {
                if (reader.recLeft() < 0)
                    break;
                NAME sub = reader.readNSubHeader();
                if (sub == 0)
                    break;
                QByteArray payload;
                reader.readRawSubData(payload);
                if (sub == NAME('DATA'))
                    visit(type, payload);
            }
        }
        return count;
    }

private slots:
    void initTestCase();
    void testDataSizeSurvey();
    void testDataLaneStats();
    void testInfoParentCorrelation();
    void testInfoValueSets();
    void testNestedRefCorrelation();
    void testLayoutConformance();
};

void TestTes3Data::initTestCase()
{
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Error);
}

void TestTes3Data::testDataSizeSurvey()
{
    // (record type, DATA size) -> count, plus per-type record/DATA stats.
    QMap<QPair<QString, int>, int> histogram;
    QHash<QString, int> recordCount;
    QHash<QString, int> dataCount;
    QHash<QString, int> multiData;
    QHash<QString, int> datasInRecord;
    // Up to 3 sample payloads per (type, size) group for layout analysis.
    QMap<QPair<QString, int>, QVector<QByteArray>> samples;
    quint32 numRecords = 0;

    // The walker visits per DATA; record denominators are counted below.
    const qint64 total = walkData(fixture(),
        [&](const QString& type, const QByteArray& payload) {
            const QPair<QString, int> key(type, payload.size());
            histogram[key]++;
            dataCount[type]++;
            datasInRecord[type]++;
            if (samples[key].size() < 3)
                samples[key].append(payload);
        },
        &numRecords);
    if (total < 0)
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");
    QCOMPARE(total, qint64(numRecords));

    // Per-type record denominators need a second pass counting records.
    {
        ESMReader reader(fixture());
        reader.open();
        while (reader.isLeft())
        {
            NAME name = reader.readName();
            if (name == 0)
                break;
            reader.readHeader();
            recordCount[spellName(name)]++;
            reader.skipRemainingRecord();
        }
    }

    int totalData = 0;
    for (auto it = dataCount.constBegin(); it != dataCount.constEnd(); ++it)
        totalData += it.value();
    qInfo() << "TES3 DATA survey:" << totalData << "DATA subrecords in" << total << "records";
    for (auto it = histogram.constBegin(); it != histogram.constEnd(); ++it)
    {
        qInfo() << QString("  %1 size %2: %3 (of %4 records)")
                       .arg(it.key().first)
                       .arg(it.key().second)
                       .arg(it.value())
                       .arg(recordCount.value(it.key().first));
        for (const QByteArray& sample : samples.value(it.key()))
            qInfo() << QString("    sample: %1").arg(QString(sample.toHex()));
    }
    QVERIFY(totalData > 0);
    Q_UNUSED(datasInRecord);
    Q_UNUSED(multiData);
}

void TestTes3Data::testDataLaneStats()
{
    // Per-lane value ranges for the fixed-size DATA groups. u8 lanes print
    // min/max/distinct; each 4-byte lane prints u32 min/max plus the float
    // reading, so widths (u8 vs u16 vs u32 vs float) fall out of the data.
    struct Lane8 { quint8 lo = 255; quint8 hi = 0; QSet<quint8> values; };
    struct Lane32 { quint32 ulo = 0xFFFFFFFFu; quint32 uhi = 0; float flo = 0; float fhi = 0; bool first = true; };
    QMap<QPair<QString, int>, QVector<Lane8>> lanes8;
    QMap<QPair<QString, int>, QVector<Lane32>> lanes32;

    const qint64 total = walkData(fixture(),
        [&](const QString& type, const QByteArray& payload) {
            const int n = payload.size();
            if (n <= 0 || n > 32 || (type == QLatin1String("LTEX")))
                return; // variable-length strings are decoded as text, not lanes
            const QPair<QString, int> key(type, n);
            if (!lanes8.contains(key))
            {
                lanes8[key] = QVector<Lane8>(n);
                lanes32[key] = QVector<Lane32>(n / 4);
            }
            const quint8* b = reinterpret_cast<const quint8*>(payload.constData());
            for (int i = 0; i < n; ++i)
            {
                Lane8& l = lanes8[key][i];
                l.lo = qMin(l.lo, b[i]);
                l.hi = qMax(l.hi, b[i]);
                if (l.values.size() <= 300)
                    l.values.insert(b[i]);
            }
            for (int i = 0; i < n / 4; ++i)
            {
                quint32 v;
                memcpy(&v, b + i * 4, 4);
                float f;
                memcpy(&f, &v, 4);
                Lane32& l = lanes32[key][i];
                l.ulo = qMin(l.ulo, v);
                l.uhi = qMax(l.uhi, v);
                if (l.first)
                {
                    l.flo = f;
                    l.fhi = f;
                    l.first = false;
                }
                else
                {
                    l.flo = qMin(l.flo, f);
                    l.fhi = qMax(l.fhi, f);
                }
            }
        });
    if (total < 0)
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");

    for (auto it = lanes8.constBegin(); it != lanes8.constEnd(); ++it)
    {
        const QString type = it.key().first;
        const int n = it.key().second;
        QString line;
        for (int i = 0; i < n; ++i)
        {
            const Lane8& l = it.value()[i];
            line += QString("%1:[%2-%3]#%4 ").arg(i).arg(l.lo).arg(l.hi)
                .arg(l.values.size() > 300 ? QStringLiteral("many") : QString::number(l.values.size()));
        }
        qInfo() << QString("lanes %1[%2] u8: %3").arg(type).arg(n).arg(line);
        QString line32;
        const QVector<Lane32>& l32 = lanes32[it.key()];
        for (int i = 0; i < l32.size(); ++i)
        {
            const Lane32& l = l32[i];
            line32 += QString("%1:u[%2-%3] f[%4-%5] ")
                .arg(i * 4).arg(l.ulo).arg(l.uhi).arg(l.flo).arg(l.fhi);
        }
        qInfo() << QString("lanes %1[%2] u32/float: %3").arg(type).arg(n).arg(line32);
    }
}

// INFO DATA byte 0 spans the same 0-4 range as the DIAL type byte. INFOs
// physically follow their DIAL, so correlate the two: (dialType, infoByte0)
// pair histogram plus the byte-1..3 constancy check.
void TestTes3Data::testInfoParentCorrelation()
{
    const QString path = fixture();
    if (!QFile::exists(path))
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");

    ESMReader reader(path);
    reader.open();
    QVERIFY(reader.tes3());

    QMap<QPair<int, int>, int> pairs;
    int infos = 0;
    int mismatchedTail = 0;
    int lastDialType = -1;
    while (reader.isLeft())
    {
        NAME name = reader.readName();
        if (name == 0)
            break;
        reader.readHeader();
        if (name != NAME('DIAL') && name != NAME('INFO'))
        {
            reader.skipRemainingRecord();
            continue;
        }
        const bool isDial = (name == NAME('DIAL'));
        while (reader.isRecLeft())
        {
            if (reader.recLeft() < 0)
                break;
            NAME sub = reader.readNSubHeader();
            if (sub == 0)
                break;
            QByteArray payload;
            reader.readRawSubData(payload);
            if (sub != NAME('DATA') || payload.isEmpty())
                continue;
            if (isDial)
                lastDialType = static_cast<quint8>(payload[0]);
            else
            {
                infos++;
                pairs[qMakePair(lastDialType, static_cast<int>(static_cast<quint8>(payload[0])))]++;
                if (payload.size() != 12 || payload[1] != 0 || payload[2] != 0
                    || payload[3] != 0 || payload[11] != 0)
                    mismatchedTail++;
            }
        }
    }
    qInfo() << "INFO/DIAL correlation over" << infos << "infos, tail mismatches:" << mismatchedTail;
    for (auto it = pairs.constBegin(); it != pairs.constEnd(); ++it)
        qInfo() << QString("  dial %1 -> info[0] %2: %3").arg(it.key().first).arg(it.key().second).arg(it.value());
    QVERIFY(infos > 0);
}

// Distinct value sets for the varying INFO DATA bytes, to name the typed
// fields correctly (disposition vs rank vs gender vs pc-rank).
void TestTes3Data::testInfoValueSets()
{
    const QString path = fixture();
    if (!QFile::exists(path))
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");

    ESMReader reader(path);
    reader.open();
    QVERIFY(reader.tes3());

    QSet<int> b4, b5, b8, b9, b10;
    QMap<int, int> b4hist;
    while (reader.isLeft())
    {
        NAME name = reader.readName();
        if (name == 0)
            break;
        reader.readHeader();
        if (name != NAME('INFO'))
        {
            reader.skipRemainingRecord();
            continue;
        }
        while (reader.isRecLeft())
        {
            if (reader.recLeft() < 0)
                break;
            NAME sub = reader.readNSubHeader();
            if (sub == 0)
                break;
            QByteArray payload;
            reader.readRawSubData(payload);
            if (sub != NAME('DATA') || payload.size() != 12)
                continue;
            const quint8* b = reinterpret_cast<const quint8*>(payload.constData());
            b4.insert(b[4]);
            b5.insert(b[5]);
            b8.insert(b[8]);
            b9.insert(b[9]);
            b10.insert(b[10]);
            b4hist[b[4]]++;
        }
    }
    const auto dump = [](const QSet<int>& s) {
        QList<int> l(s.begin(), s.end());
        std::sort(l.begin(), l.end());
        QString out;
        for (int v : l)
            out += QString("%1 ").arg(v);
        return out;
    };
    qInfo() << "INFO b4 distinct:" << b4.size() << dump(b4);
    qInfo() << "INFO b5:" << dump(b5);
    qInfo() << "INFO b8:" << dump(b8);
    qInfo() << "INFO b9:" << dump(b9);
    qInfo() << "INFO b10:" << dump(b10);
    int over100 = 0;
    for (auto it = b4hist.constBegin(); it != b4hist.constEnd(); ++it)
        if (it.key() > 100)
            over100 += it.value();
    qInfo() << "INFO b4 values > 100:" << over100 << "of 23693";
    QVERIFY(!b4.isEmpty());
}

// CELL nests placed references as subrecords: each FRMR should pair with one
// 24-byte transform DATA. PGRD DATA lane 3 should equal its PGRI count.
void TestTes3Data::testNestedRefCorrelation()
{
    const QString path = fixture();
    if (!QFile::exists(path))
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");

    ESMReader reader(path);
    reader.open();
    QVERIFY(reader.tes3());

    qint64 cellMismatch = 0;
    qint64 cells = 0;
    qint64 pgrdMismatch = 0;
    qint64 pgrds = 0;
    while (reader.isLeft())
    {
        NAME name = reader.readName();
        if (name == 0)
            break;
        reader.readHeader();
        if (name != NAME('CELL') && name != NAME('PGRD'))
        {
            reader.skipRemainingRecord();
            continue;
        }
        const bool isCell = (name == NAME('CELL'));
        int frmr = 0;
        int data24 = 0;
        int pgri = 0;
        quint32 lane3 = 0;
        while (reader.isRecLeft())
        {
            if (reader.recLeft() < 0)
                break;
            NAME sub = reader.readNSubHeader();
            if (sub == 0)
                break;
            QByteArray payload;
            reader.readRawSubData(payload);
            if (isCell)
            {
                if (sub == NAME('FRMR'))
                    frmr++;
                if (sub == NAME('DATA') && payload.size() == 24)
                    data24++;
            }
            else
            {
                if (sub == NAME('PGRI'))
                    pgri++;
                if (sub == NAME('DATA') && payload.size() == 12)
                    memcpy(&lane3, payload.constData() + 8, 4);
            }
        }
        if (isCell)
        {
            cells++;
            if (frmr != data24)
            {
                cellMismatch++;
                if (cellMismatch < 5)
                    qInfo() << "CELL frmr/data24 mismatch:" << frmr << data24;
            }
        }
        else
        {
            pgrds++;
            if (quint32(pgri) != lane3)
            {
                pgrdMismatch++;
                if (pgrdMismatch < 5)
                    qInfo() << "PGRD pgri/lane3 mismatch:" << pgri << lane3;
            }
        }
    }
    qInfo() << "cells:" << cells << "mismatched:" << cellMismatch
            << "pgrds:" << pgrds << "mismatched:" << pgrdMismatch;
    QVERIFY(cells > 0 && pgrds > 0);
}

// Every DATA with a proven layout must decode field-by-field and re-encode
// to the exact original bytes through the real Tes3Data_Component path.
// Groups without a layout (PGRD counts) are reported, not decoded.
void TestTes3Data::testLayoutConformance()
{
    QVERIFY(unspellName(spellName(NAME('DIAL'))) == NAME('DIAL'));

    QMap<QPair<QString, int>, int> uncovered;
    qint64 covered = 0;
    qint64 failed = 0;
    QStringList failureNotes;

    const qint64 total = walkData(fixture(),
        [&](const QString& type, const QByteArray& payload) {
            const NAME code = unspellName(type);
            const Tes3DataLayout* layout = tes3DataLayoutFor(code, payload.size());
            if (!layout)
            {
                uncovered[qMakePair(type, payload.size())]++;
                return;
            }
            tescomponents::Tes3Data_Component comp;
            comp.data = payload;
            comp.decode(code, payload);
            if (!comp.typedValid)
            {
                failed++;
                if (failureNotes.size() < 5)
                    failureNotes.append(QString("decode %1[%2]").arg(type).arg(payload.size()));
                return;
            }
            for (int i = 0; i < comp.typedFields.size(); ++i)
            {
                if (!comp.setTypedValue(i, comp.typedFields[i].value))
                {
                    failed++;
                    if (failureNotes.size() < 5)
                        failureNotes.append(QString("re-encode %1[%2] field %3")
                            .arg(type).arg(payload.size()).arg(i));
                    return;
                }
            }
            if (comp.data != payload)
            {
                failed++;
                if (failureNotes.size() < 5)
                    failureNotes.append(QString("drift %1[%2]").arg(type).arg(payload.size()));
                return;
            }
            covered++;
        });
    if (total < 0)
        QSKIP("Morrowind.esm fixture not available (set OPENCK_TEST_MORROWIND_ESM)");

    qInfo() << "TES3 layout conformance: covered" << covered << "failed" << failed;
    for (const QString& note : failureNotes)
        qInfo() << "  " << note;
    for (auto it = uncovered.constBegin(); it != uncovered.constEnd(); ++it)
        qInfo() << QString("  uncovered %1 size %2: %3 (hex fallback)")
                       .arg(it.key().first).arg(it.key().second).arg(it.value());
    // PGRD counts must stay undecoded (they shadow sibling subrecords).
    QVERIFY(tes3DataLayoutFor(NAME('PGRD'), 12) == nullptr);
    QCOMPARE(failed, 0);
    QVERIFY(covered > 300000);
}

QTEST_MAIN(TestTes3Data)
#include "test_tes3data.moc"
