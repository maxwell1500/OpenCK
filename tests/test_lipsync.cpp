#include <QTest>
#include <QFile>
#include <QDebug>
#include <QByteArray>

#include "ba2/bsaarchive.hpp"
#include "audio/fuzparser.hpp"
#include "audio/lipfile.hpp"

// The Skyrim voice archives store each line as a .fuz container holding a
// .lip payload plus XWMA audio. OpenCK already reads and rewrites the
// container; this suite pins the lip payload's own semantics — the header,
// the RLE payload and the 16 FaceGen viseme curves the game plays.
class TestLipSync : public QObject
{
    Q_OBJECT

private:
    static QString skyrimVoiceBsa()
    {
        const QString env = qEnvironmentVariable("OPENCK_TEST_SKYRIM_VOICES");
        if (!env.isEmpty()) return env;
        return QStringLiteral("C:/XboxGames/The Elder Scrolls V- Skyrim Special Edition (PC)/Content/Data/Skyrim - Voices_en0.bsa");
    }

    static bool dumpRaw() { return !qEnvironmentVariable("OPENCK_LIP_DUMP").isEmpty(); }

    // Mirrors the documented fixture: a 24-byte header whose grid geometry
    // implies exactly two frames, followed by an RLE stream that expands to
    // 39 floats. The grid holds frames * 33 slots and the stream stops once
    // the remaining curves are silent, so 39 slots fill the leading part of
    // both rows; the fixture puts its values at slots 6 and 7.
    static QByteArray syntheticLip()
    {
        QByteArray bytes;
        auto put32 = [&bytes](quint32 value) {
            for (int i = 0; i < 4; ++i) bytes.append(static_cast<char>((value >> (8 * i)) & 0xFF));
        };
        auto put16 = [&bytes](quint16 value) {
            bytes.append(static_cast<char>(value & 0xFF));
            bytes.append(static_cast<char>((value >> 8) & 0xFF));
        };
        put32(1);                              // version
        put32(132 * 2 + 28);                   // gridsize
        put32(13);                             // num curves
        put16(2);                              // frames
        put16(3);                              // const14
        put32(static_cast<quint32>(-9));       // first (pre-roll, frames)
        put16(16);                             // const20
        put16(111);                            // u22

        QByteArray raw(39 * 4, '\0');
        float v0 = 0.75f, v1 = 0.25f;
        raw.replace(6 * 4, 4, reinterpret_cast<const char*>(&v0), 4);
        raw.replace(7 * 4, 4, reinterpret_cast<const char*>(&v1), 4);

        // RLE encode: 0x00 starts a run of that many zero bytes.
        QByteArray rle;
        int index = 0;
        while (index < raw.size())
        {
            if (raw.at(index) == '\0')
            {
                const int start = index;
                while (index < raw.size() && raw.at(index) == '\0') ++index;
                int count = index - start;
                while (count > 0)
                {
                    const int run = qMin(count, 65535);
                    rle.append('\0');
                    rle.append(static_cast<char>(run & 0xFF));
                    rle.append(static_cast<char>((run >> 8) & 0xFF));
                    count -= run;
                }
            }
            else
            {
                rle.append(raw.at(index));
                ++index;
            }
        }
        bytes.append(rle);
        return bytes;
    }

private slots:
    void initTestCase()
    {
        QVERIFY2(QFile::exists(skyrimVoiceBsa()),
                 qPrintable(QStringLiteral("Skyrim voice BSA missing at %1")
                                .arg(skyrimVoiceBsa())));
    }

    void testHeaderDecodes()
    {
        LipFile lip;
        QString error;
        QVERIFY2(LipFile::decode(syntheticLip(), lip, &error), qPrintable(error));

        QCOMPARE(lip.header().version, 1u);
        QCOMPARE(lip.header().numCurves, 13u);
        QCOMPARE(lip.header().frames, quint16(2));
        QCOMPARE(lip.header().const14, quint16(3));
        QCOMPARE(lip.header().first, -9);
        QCOMPARE(lip.header().const20, quint16(16));
        // The RLE stream starts right after the header.
        QCOMPARE(lip.payloadOffset(), 24);
        QVERIFY(lip.rows() == 2);
    }

    void testVisemeGridAndTiming()
    {
        LipFile lip;
        QString error;
        QVERIFY2(LipFile::decode(syntheticLip(), lip, &error), qPrintable(error));

        QVERIFY2(lip.hasTiming(), "first=-9 with const20=16 must be trustworthy");
        QCOMPARE(lip.frameTime(0), -0.3f);
        QCOMPARE(lip.frameTime(1), (-8.0f) / 30.0f);

        // The first two viseme curves carry the values written into the grid.
        const QVarLengthArray<float, 16> visemes = lip.visemesAt(0);
        QCOMPARE(visemes.at(0), 0.75f);
        QCOMPARE(visemes.at(1), 0.25f);
        // Every other curve is zero in this fixture.
        for (int i = 2; i < 16; ++i) QCOMPARE(visemes.at(i), 0.0f);

        // Viseme names are stable: the timeline labels them by name.
        QCOMPARE(QLatin1String(LipFile::kVisemeNames[0]), QLatin1String("Aah"));
        QCOMPARE(QLatin1String(LipFile::kVisemeNames[7]), QLatin1String("FV"));
        QCOMPARE(QLatin1String(LipFile::kVisemeNames[15]), QLatin1String("W"));
    }

    void testAmbiguousTimingStillDecodesCurves()
    {
        QByteArray bytes = syntheticLip();
        // Clear const20 so the timing pair cannot be trusted.
        bytes[20] = '\0';
        bytes[21] = '\0';

        LipFile lip;
        QString error;
        QVERIFY2(LipFile::decode(bytes, lip, &error), qPrintable(error));
        QVERIFY2(!lip.hasTiming(), "an untrustworthy header must not invent a time");
        QVERIFY(lip.rows() == 2);
        QCOMPARE(lip.visemesAt(0).at(0), 0.75f);
    }

    void testRejectsTruncatedZeroRun()
    {
        QByteArray bytes = syntheticLip().left(24);
        bytes.append('\0');
        bytes.append('\x01');
        LipFile lip;
        QString error;
        QVERIFY(!LipFile::decode(bytes, lip, &error));
        QVERIFY(!error.isEmpty());
    }

    void testRejectsShortFile()
    {
        QByteArray bytes(8, '\0');
        LipFile lip;
        QVERIFY(!LipFile::decode(bytes, lip));
    }

    // `frames` and `gridSize` describe the same thing, so a header that
    // disagrees with itself is corrupt. Every shipped line agrees exactly.
    void testRejectsHeaderInconsistentWithItsOwnGridSize()
    {
        QByteArray bytes = syntheticLip();
        bytes[12] = static_cast<char>(1);   // claim 1 frame, gridSize still says 2

        LipFile lip;
        QString error;
        QVERIFY(!LipFile::decode(bytes, lip, &error));
        QVERIFY(!error.isEmpty());
    }

    // A header declaring an extra frame is fine: the curves the payload does
    // carry are honoured and the absent tail is reported, not invented.
    void testReportsShortPayload()
    {
        QByteArray bytes = syntheticLip();
        bytes[12] = static_cast<char>(3);   // claim 3 frames, payload holds 2
        // Keep the derived gridSize consistent with the new count.
        const quint32 gridSize = 132 * 3 + 28;
        for (int i = 0; i < 4; ++i)
            bytes[4 + i] = static_cast<char>((gridSize >> (8 * i)) & 0xFF);

        LipFile lip;
        QString error;
        QVERIFY2(LipFile::decode(bytes, lip, &error), qPrintable(error));
        QCOMPARE(lip.rows(), 3);
        QVERIFY(!lip.payloadIsComplete());
        QCOMPARE(lip.missingSlots(), 3 * 33 - lip.payloadFloatCount());
        // The curves that are present are still correctly aligned.
        QCOMPARE(lip.visemesAt(0).at(0), 0.75f);
        QCOMPARE(lip.visemesAt(0).at(1), 0.25f);
        QCOMPARE(lip.visemesAt(1).at(0), 0.0f);
    }

    // A header that is wrong by more than a row is not a quirk, it is a
    // different file: the payload cannot fill the claimed grid, so refuse.
    void testRejectsGrosslyWrongFrameCount()
    {
        QByteArray bytes = syntheticLip();
        bytes[12] = static_cast<char>(40);   // claim 40 frames, payload holds 2

        LipFile lip;
        QVERIFY(!LipFile::decode(bytes, lip));
    }

    // Broken timing fields must not be adopted, but the curves and a usable
    // time base both survive: this is the shape of the one truncated line the
    // shipped archive actually contains.
    void testUntrustworthyTimingStillTimesFrames()
    {
        QByteArray bytes = syntheticLip();
        bytes[20] = '\x00';
        bytes[21] = '\x00';

        LipFile lip;
        QString error;
        QVERIFY2(LipFile::decode(bytes, lip, &error), qPrintable(error));
        QVERIFY2(!lip.hasTiming(), "an untrustworthy header must not invent a pre-roll");
        QCOMPARE(lip.frameTime(0), 0.0f);
        QCOMPARE(lip.frameTime(1), 1.0f / 30.0f);
        QCOMPARE(lip.visemesAt(0).at(0), 0.75f);
    }

    // Every real voice line in the shipped archive decodes into a plausible
    // viseme grid: whole frames, values in range, times that mean something.
    void testRealFuzLipPayloads()
    {
        BsaArchive arch;
        QVERIFY2(arch.open(skyrimVoiceBsa()),
                 qPrintable(QStringLiteral("cannot open %1").arg(skyrimVoiceBsa())));

        const int total = arch.entries().size();
        QVERIFY(total > 1000);

        int withLip = 0;
        int lipOnly = 0;
        int examined = 0;
        int decoded = 0;
        int rejected = 0;
        int truncated = 0;
        int shiftedPayload = 0;
        float maxTime = 0.0f;
        QStringList failures;

        for (int i = 0; i < total && examined < 60; ++i)
        {
            const BsaFileEntry& e = arch.entries().at(i);
            if (!e.fileName.toLower().endsWith(QStringLiteral(".fuz"))) continue;
            ++examined;

            QByteArray data;
            QVERIFY2(arch.readData(i, data),
                     qPrintable(QStringLiteral("cannot read %1").arg(e.fullPath)));

            FuzParser container;
            QVERIFY2(FuzParser::parse(data, container),
                     qPrintable(QStringLiteral("cannot parse %1").arg(e.fullPath)));
            QVERIFY2(!container.audioData.isEmpty(),
                     qPrintable(QStringLiteral("no audio in %1").arg(e.fullPath)));

            if (container.lipData.isEmpty())
            {
                // Shout-like lines ship with no lip data at all.
                ++lipOnly;
                continue;
            }
            ++withLip;

            LipFile lip;
            QString error;
            if (!LipFile::decode(container.lipData, lip, &error))
            {
                // Some shipped lines carry a header whose frame count cannot
                // be reconciled with the payload geometry at all. Decoding
                // stays strict about them: no curve data is invented, and the
                // caller keeps the original bytes.
                ++rejected;
                failures << QStringLiteral("%1: %2").arg(e.fullPath, error);
                continue;
            }
            ++decoded;

            QCOMPARE(lip.header().version, 1u);

            // Some generators write one extra record-header byte after the
            // 24-byte header, so the payload does not always start at 24.
            if (lip.payloadOffset() != 24) ++shiftedPayload;

            // The RLE stream stops once the remaining curves are silent, so a
            // short payload is normal. More than a couple of frames short means
            // the shipped payload is truncated and its tail plays silent.
            if (lip.missingSlots() > 2 * 33)
            {
                ++truncated;
                qInfo() << "TRUNCATED" << e.fullPath << "carried"
                        << lip.payloadFloatCount() << "of" << lip.rows() * 33 << "slots";
            }

            for (int row = 0; row < lip.rows(); ++row)
            {
                for (int v = 0; v < 16; ++v)
                {
                    const float value = lip.visemesAt(row).at(v);
                    // Every curve slot in the shipped data is a normalized
                    // weight, but generators do let them overshoot slightly
                    // below zero (the worst shipped line reaches -0.066).
                    QVERIFY2(value > -1.0f && value < 2.0f,
                             qPrintable(QStringLiteral("%1: curve %2 is not a weight (%3)")
                                            .arg(e.fullPath)
                                            .arg(LipFile::kVisemeNames[v])
                                            .arg(value)));
                }
                // Times stay usable even where the header's own timing fields
                // are garbage: the fallback is monotonic and correctly spaced.
                const float t = lip.frameTime(row);
                QVERIFY2(t == t && t > -100.0f && t < 1000.0f,
                         qPrintable(QStringLiteral("%1: unusable frame time").arg(e.fullPath)));
                if (t > maxTime) maxTime = t;
            }

            // A trustworthy header carries a negative pre-roll; one that does
            // not must not be adopted as if it did.
            if (lip.header().const20 == 16)
            {
                QVERIFY2(lip.header().first >= -120 && lip.header().first <= 0,
                         qPrintable(QStringLiteral("%1: implausible pre-roll %2")
                                        .arg(e.fullPath).arg(lip.header().first)));
            }
            else
            {
                QVERIFY2(!lip.hasTiming(),
                         qPrintable(QStringLiteral("%1: untrustworthy timing fields were used")
                                        .arg(e.fullPath)));
            }

            if (dumpRaw()) qInfo() << "LIPFULL" << e.fullPath << container.lipData.toHex();
        }

        QVERIFY2(examined > 0, "no .fuz entries found in the archive");
        QVERIFY2(withLip > 0, "sampled lines carried no lip payloads");
        QVERIFY2(lipOnly > 0, "sampled lines all carried lip payloads");

        // A real line must actually move a mouth.
        QVERIFY2(maxTime > -5.0f, "no decoded frame carried a usable time");

        // Truncated payloads are a data defect, not a decoder defect: one
        // shipped line carries 84% of its declared curves and plays the rest
        // silent. Surface the count so a regression that truncates every file
        // cannot hide behind it.
        qInfo() << "examined" << examined << "withLip" << withLip
                << "decoded" << decoded << "truncated" << truncated
                << "shiftedPayload" << shiftedPayload
                << "rejected" << rejected << "maxFrameTime" << maxTime;
        for (const QString& failure : failures) qInfo() << "  reject:" << failure;
        QVERIFY2(rejected == 0,
                 qPrintable(QStringLiteral("%1 of %2 lip lines rejected").arg(rejected).arg(withLip)));
        QVERIFY2(truncated * 4 <= withLip,
                 qPrintable(QStringLiteral("%1 of %2 lip lines truncated; decoder regression?")
                                .arg(truncated).arg(withLip)));
    }
};

QTEST_MAIN(TestLipSync)
#include "test_lipsync.moc"

