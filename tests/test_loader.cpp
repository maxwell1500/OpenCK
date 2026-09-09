#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QFile>
#include <cstdio>
#include <windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>
#pragma comment(lib, "dbghelp.lib")

#include <algorithm>

// Page-heap (Application Verifier) turns a heap-buffer overrun into an
// ACCESS_VIOLATION at the exact write site. This unhandled-exception filter
// walks the stack and prints symbol names so we can see which loader wrote
// past its buffer without needing an interactive debugger.
static void printStackTrace(EXCEPTION_POINTERS* ep)
{
    HANDLE proc = GetCurrentProcess();
    SymInitialize(proc, nullptr, TRUE);
    const int maxFrames = 64;
    void* frames[maxFrames] = {};
    const USHORT n = CaptureStackBackTrace(0, maxFrames, frames, nullptr);
    fprintf(stderr, "\n=== STACK TRACE (page-heap overrun) ===\n");
    for (USHORT i = 0; i < n; ++i)
    {
        DWORD64 addr = reinterpret_cast<DWORD64>(frames[i]);
        char buf[sizeof(SYMBOL_INFO) + 256] = {};
        SYMBOL_INFO* si = reinterpret_cast<SYMBOL_INFO*>(buf);
        si->SizeOfStruct = sizeof(SYMBOL_INFO);
        si->MaxNameLen = 256;
        DWORD64 disp = 0;
        if (SymFromAddr(proc, addr, &disp, si))
            fprintf(stderr, "#%02u %s+0x%llx\n", i,
                si->Name, static_cast<unsigned long long>(disp));
        else
            fprintf(stderr, "#%02u 0x%p\n", i, frames[i]);
    }
    fflush(stderr);
}

// 0xC0000421 = STATUS_VERIFIER_STOP (Application Verifier / page heap caught
// a corruption, e.g. a small overrun into a canary on free).
// 0x80000003 = STATUS_BREAKPOINT (light page heap raises int3 on free-time
// corruption detection when no debugger is attached; catchable here).
static bool isFatalHeapCode(DWORD code)
{
    return code == EXCEPTION_ACCESS_VIOLATION
        || code == STATUS_HEAP_CORRUPTION
        || code == 0xC0000421
        || code == 0x80000003;
}

static LONG WINAPI stackTraceFilter(EXCEPTION_POINTERS* ep)
{
    if (ep && ep->ExceptionRecord && isFatalHeapCode(ep->ExceptionRecord->ExceptionCode))
    {
        if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
            && ep->ExceptionRecord->NumberParameters >= 2)
        {
            fprintf(stderr, "ACCESS_VIOLATION at address 0x%p (write=%lld)\n",
                reinterpret_cast<void*>(ep->ExceptionRecord->ExceptionInformation[1]),
                static_cast<long long>(ep->ExceptionRecord->ExceptionInformation[0]));
        }
        printStackTrace(ep);
    }
    // Terminate: an overrun cannot safely resume, and STATUS_HEAP_CORRUPTION
    // is a fast-fail that the OS would raise on the next heap op anyway.
    fflush(stderr);
    ExitProcess(1);
}

// Vectored handler registered FIRST so it fires on the raw SEH exception
// before _set_se_translator (in Data::continueLoading) converts SEH to a
// C++ exception. This is what lets us catch the page-heap verifier stop.
static LONG CALLBACK vehHandler(EXCEPTION_POINTERS* ep)
{
    if (ep && ep->ExceptionRecord && isFatalHeapCode(ep->ExceptionRecord->ExceptionCode))
    {
        if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
            && ep->ExceptionRecord->NumberParameters >= 2)
        {
            fprintf(stderr, "ACCESS_VIOLATION at address 0x%p (write=%lld)\n",
                reinterpret_cast<void*>(ep->ExceptionRecord->ExceptionInformation[1]),
                static_cast<long long>(ep->ExceptionRecord->ExceptionInformation[0]));
        }
        else
        {
            fprintf(stderr, "FATAL HEAP EXCEPTION 0x%08X at address 0x%p\n",
                static_cast<unsigned>(ep->ExceptionRecord->ExceptionCode),
                ep->ExceptionRecord->ExceptionAddress);
            for (ULONG i = 0; i < ep->ExceptionRecord->NumberParameters; ++i)
                fprintf(stderr, "  param[%lu] = 0x%p\n", i,
                    reinterpret_cast<void*>(ep->ExceptionRecord->ExceptionInformation[i]));
        }
        printStackTrace(ep);
        fflush(stderr);
        ExitProcess(1);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

// Full page heap (DPH) cannot be enabled on a heap once its low-fragmentation
// heap has auto-engaged, and by initTestCase() time thousands of allocations
// have already run. Static initializers execute during CRT startup, before
// main(), while heaps are still in their default mode - so arm DPH here.
namespace
{
struct EarlyPageHeap
{
    EarlyPageHeap()
    {
        if (!getenv("OPENCK_FULL_PAGEHEAP"))
            return;
        ULONG compat = 2;
        ULONG n = GetProcessHeaps(0, nullptr);
        if (n == 0 || n > 128)
            n = (n > 128) ? 128 : 0;
        int ok = 0, fail = 0;
        if (n > 0)
        {
            HANDLE heaps[128] = {};
            n = GetProcessHeaps(n, heaps);
            for (ULONG i = 0; i < n; ++i)
            {
                if (HeapSetInformation(heaps[i], HeapCompatibilityInformation,
                        &compat, sizeof(compat)))
                    ++ok;
                else
                    ++fail;
            }
        }
        fprintf(stderr, "[early-ph] compat=2 ok=%d fail=%d of %u heaps\n",
            ok, fail, n);
        fflush(stderr);
    }
};
EarlyPageHeap g_earlyPageHeap;
}

#include "../../src/model/doc/documentmediator.hpp"
#include "../../src/model/doc/document.hpp"
#include "../../src/model/world/ckid.hpp"
#include "../../src/model/world/record.hpp"
#include "../../src/model/window/objectwindow.hpp"
#include "../../libs/files/esm/esmwriter.hpp"
#include "../../libs/files/esm/npcrecord.hpp"
#include "../../libs/files/esm/worldspacerecord.hpp"
#include "../../libs/files/esm/glob.hpp"
#include "../../libs/files/esm/Statrecord.hpp"
#include "../../libs/files/esm/Dialrecord.hpp"
#include "../../libs/files/esm/subrecordsnapshot.hpp"
#include <cstring>
#include <QTextStream>
#include "../../libs/files/log/logger.hpp"

// Loader protocol test against the live Document/Loader/Data pipeline.
// The Loader does the heavy parsing work, so it runs on a dedicated thread
// driven by the DocumentMediator's tick timer plus its own self-resume.
class TestLoaderSinglePass : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDocumentLoadedEmittedExactlyOnce();
    void testLoaderDoesNotRespawnPreload();
    void testDocumentInsertedAfterIdleTicks();
    void testDeferredMasterMaterialization();
    void testObjectWindowDeferredCategoryFetch();
    void testRealSeydaNeenDocument();
    void testSaveRoundTripGRUP();
    void testFormIdAllocation();
    void testSaveRoundTripSubrecordIdentical();
    void testSyntheticMultiTypeRoundTrip();
    void testMasterRecordSaveStateMachine();
    void testMaterializationMatrixZeroWarnings();
    void testDialInfoParentWalking();
    void testGrupSizeConsistent();
    void testDiscoverFormIdSubrecordLayouts();

private:
    static bool writeTestPlugin(const QString& path)
    {
        return writeTestPluginEx(path, "LoaderTestNPC", 0x1234, "Loader Test Character", 5);
    }

    static bool writeTestPluginEx(const QString& path, const QString& editorId,
        quint32 formId, const QString& fullName, int level)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return false;
        ESMWriter writer;
        writer.setAuthor("Loader Test");
        writer.save(file);

        NpcRecord npc;
        npc.editorId = editorId;
        npc.formId = formId;
        npc.fullName = fullName;
        npc.level = level;

        RecHeader recHeader;
        recHeader.id = formId;
        writer.startRecord('NPC_', recHeader);
        npc.save(writer);
        writer.endRecord();

        writer.close();
        file.close();
        return true;
    }

    static Document* makeDocument(DocumentMediator& mediator,
        const QString& fileName, const QString& dir)
    {
        Document* doc = mediator.makeDocument(
            QStringList{ fileName }, dir + "/" + fileName, false);
        const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
        return doc;
    }

    // The edited plugin is selectable so a corrupt file on disk (SeydaNeen.esp
    // turned out to be damaged) does not poison the real-data gates.
    static QStringList realMasters() {
        return { QStringLiteral("Starfield.esm"),
                 QStringLiteral("The Elder Star System - Magnus.esm") };
    }
    static QString realPlugin()
    {
        return qEnvironmentVariable("OPENCK_TEST_PLUGIN",
            QStringLiteral("Vvardenfell.esp"));
    }
};

void TestLoaderSinglePass::initTestCase()
{
    AddVectoredExceptionHandler(1, vehHandler);
    SetUnhandledExceptionFilter(stackTraceFilter);

    // Enable FULL page heap (guard pages) on the process heap so a *large*
    // buffer overrun raises a catchable ACCESS_VIOLATION at the write site.
    // Gated behind an env var (it is slow and changes heap layout).
    if (getenv("OPENCK_FULL_PAGEHEAP"))
    {
        ULONG compat = 2;
        ULONG n = GetProcessHeaps(0, nullptr);
        if (n > 128)
            n = 128;
        int ok = 0, fail = 0;
        if (n > 0)
        {
            HANDLE heaps[128] = {};
            n = GetProcessHeaps(n, heaps);
            for (ULONG i = 0; i < n; ++i)
            {
                if (HeapSetInformation(heaps[i], HeapCompatibilityInformation,
                        &compat, sizeof(compat)))
                    ++ok;
                else
                    ++fail;
            }
        }
        fprintf(stderr, "[init-ph] compat=2 ok=%d fail=%d of %u heaps\n",
            ok, fail, n);
        fflush(stderr);
    }

    // Route Debug-CRT heap corruption reports to stderr (no dialog) so a
    // corruption in a Debug-CRT allocation (FormComponents std::vector,
    // Component objects) is caught by _CrtCheckMemory with an allocation stack.
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    OpenCK::Logging::Logger::instance().setMinLevel(OpenCK::Logging::LogLevel::Debug);
    OpenCK::Logging::Logger::instance().init(QStringLiteral(
        "C:/Users/max/AppData/Local/Temp/opencode/test_loader_log.txt"));
}

void TestLoaderSinglePass::testDocumentLoadedEmittedExactlyOnce()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPlugin(tmp.filePath("loader_regression.esm")));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = makeDocument(mediator, "loader_regression.esm", tmp.path());
    mediator.insertDocument(doc);

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);
}

void TestLoaderSinglePass::testLoaderDoesNotRespawnPreload()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPlugin(tmp.filePath("loader_regression_a.esm")));
    QVERIFY(writeTestPlugin(tmp.filePath("loader_regression_b.esm")));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    mediator.insertDocument(makeDocument(mediator, "loader_regression_a.esm", tmp.path()));
    mediator.insertDocument(makeDocument(mediator, "loader_regression_b.esm", tmp.path()));

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 2, 30000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);
    QCOMPARE(stopped.at(1).at(1).toBool(), true);

    // A loader that re-preloads finished documents would emit further
    // loadingStopped signals during this grace period.
    QTest::qWait(750);
    QCOMPARE(stopped.count(), 2);
}

void TestLoaderSinglePass::testDocumentInsertedAfterIdleTicks()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPlugin(tmp.filePath("loader_idle.esm")));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    // Reproduces the GUI scenario that used to deadlock: the tick timer
    // fires many times with an empty queue, and only then does a document
    // arrive. The loader must pick it up instead of blocking its thread's
    // event queue (the old QWaitCondition design never processed the
    // queued loadDocument in this case).
    QTest::qWait(500);

    mediator.insertDocument(makeDocument(mediator, "loader_idle.esm", tmp.path()));

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);
}

void TestLoaderSinglePass::testDeferredMasterMaterialization()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPluginEx(tmp.filePath("master_regression.esm"), "masternpc", 0x1111, "Master Character", 1));
    QVERIFY(writeTestPluginEx(tmp.filePath("plugin_regression.esm"), "pluginnpc", 0x2222, "Plugin Character", 2));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ "master_regression.esm", "plugin_regression.esm" },
        tmp.path() + "/plugin_regression.esm", false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(tmp.path());
    mediator.insertDocument(doc);

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    Data& data = doc->getData();

    // The master is deferred: only the edited plugin's NPC was parsed.
    QCOMPARE(data.getNpcCollection().size(), 1);
    QCOMPARE(data.getNpcCollection().getId(0), QStringLiteral("pluginnpc"));
    QCOMPARE(data.masterIndexCount(static_cast<int>(CkId::Type_Npc_)), 1);

    // Materializing the type parses the master's NPC too.
    QCOMPARE(data.ensureTypeLoaded(static_cast<int>(CkId::Type_Npc_)), 1);
    QCOMPARE(data.getNpcCollection().size(), 2);
    QVERIFY(data.getNpcCollection().searchId(QStringLiteral("masternpc")) >= 0);

    // A second materialization is a no-op (index entries consumed).
    QCOMPARE(data.ensureTypeLoaded(static_cast<int>(CkId::Type_Npc_)), 0);
    QCOMPARE(data.getNpcCollection().size(), 2);
    QCOMPARE(data.masterIndexCount(static_cast<int>(CkId::Type_Npc_)), 0);
}

// Regression for the Object Window crash on deferred categories: expanding
// a category whose type has both parsed (plugin) and deferred (master)
// records must materialize the master records through fetchMore() without
// resetting the model mid-fetch.
void TestLoaderSinglePass::testObjectWindowDeferredCategoryFetch()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPluginEx(tmp.filePath("master_regression.esm"), "masternpc", 0x1111, "Master Character", 1));
    QVERIFY(writeTestPluginEx(tmp.filePath("plugin_regression.esm"), "pluginnpc", 0x2222, "Plugin Character", 2));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ "master_regression.esm", "plugin_regression.esm" },
        tmp.path() + "/plugin_regression.esm", false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(tmp.path());
    mediator.insertDocument(doc);

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    ObjectWindowModel model;
    model.setData(&doc->getData());

    // Locate the NPC category across groups.
    QModelIndex npcCategory;
    bool found = false;
    for (int g = 0; g < model.rowCount() && !found; ++g)
    {
        const QModelIndex groupIdx = model.index(g, 0);
        for (int c = 0; c < model.rowCount(groupIdx); ++c)
        {
            const QModelIndex catIdx = model.index(c, 0, groupIdx);
            const int flatId = model.getCategoryIndex(catIdx);
            if (model.getCategoryType(flatId) == static_cast<int>(CkId::Type_Npc_))
            {
                npcCategory = catIdx;
                found = true;
                break;
            }
        }
    }
    QVERIFY(found);

    // Before the fetch: the plugin NPC is visible, the master NPC is
    // deferred, and the category reports the combined count.
    QCOMPARE(model.rowCount(npcCategory), 1);
    QVERIFY(model.canFetchMore(npcCategory));

    model.fetchMore(npcCategory);

    // Materialization + rebuild are synchronous; the master NPC row is
    // present immediately after fetchMore returns.
    QCOMPARE(model.rowCount(npcCategory), 2);
    QVERIFY(!model.canFetchMore(npcCategory));
    QCOMPARE(doc->getData().getNpcCollection().searchId(QStringLiteral("masternpc")) >= 0, true);
}

// Real-data gate: open the real plugin together with its masters (Starfield
// + Magnus) in the intended order — masters first, edited plugin last. The
// masters must be indexed (deferred), the plugin's records parsed eagerly,
// and a small master record type materializable on demand. Skips when the
// Starfield data dir is absent.
void TestLoaderSinglePass::testRealSeydaNeenDocument()
{
    const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    const QString plugin = realPlugin();
    if (!QFileInfo::exists(dir + '/' + plugin))
        QSKIP(qPrintable(QStringLiteral("Starfield data dir or %1 not found").arg(plugin)));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        realMasters() + QStringList{ plugin },
        dir + '/' + plugin, false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
    mediator.insertDocument(doc);

    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 120000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    Data& data = doc->getData();

    // The edited plugin's own records parsed eagerly.
    QVERIFY(data.getStatCollection().size() > 0);
    QVERIFY(data.getRefrCollection().size() > 0);
    QVERIFY(data.getCellCollection().size() > 0);
    qDebug() << plugin << "eager records: stat" << data.getStatCollection().size()
             << "refr" << data.getRefrCollection().size()
             << "cell" << data.getCellCollection().size();

    // Masters are deferred: their NPCs/weather are in the index, not parsed.
    QVERIFY(data.masterIndexCount(static_cast<int>(CkId::Type_Npc_)) > 0);
    QVERIFY(data.masterIndexCount(static_cast<int>(CkId::Type_Wthr_)) > 0);

    // Materialize a small master type end-to-end (weather records).
    const int before = data.getWthrCollection().size();
    const int loaded = data.ensureTypeLoaded(static_cast<int>(CkId::Type_Wthr_));
    QVERIFY(loaded > 0);
    QCOMPARE(data.getWthrCollection().size(), before + loaded);
    qDebug() << "materialized master weather records:" << loaded;

    // Deferred master records of other types remain untouched.
    QVERIFY(data.masterIndexCount(static_cast<int>(CkId::Type_Npc_)) > 0);

    // Materialize the record types whose subrecord parsers were recently
    // fixed (CNTO/SPIT/CTDA/PTDT) from the real master, end-to-end. A
    // misaligned parser here is what made expanding Object Window
    // categories crash or hang.
    const struct { CkId::Type type; const char* label; } materialize[] = {
        { CkId::Type_Cont_, "cont" },
        { CkId::Type_Npc_, "npc" },
        { CkId::Type_Spel_, "spel" },
        { CkId::Type_Info_, "info" },
        { CkId::Type_Pack_, "pack" },
    };
    for (const auto& m : materialize)
    {
        const int n = data.ensureTypeLoaded(static_cast<int>(m.type));
        qDebug() << "materialized master" << m.label << "records:" << n;
        QVERIFY(n > 0);
    }

    // ==== User-reported Object Window click path on real data ====
    // Expanding the Worldspace category must materialize Starfield's
    // worldspaces (not just the plugin's single one), rebuild the model,
    // and a record click (currentChanged -> getFormComponentsForIndex)
    // must resolve components without crashing.
    ObjectWindowModel model;
    model.setData(&data);

    QModelIndex wrldCategory;
    bool found = false;
    for (int g = 0; g < model.rowCount() && !found; ++g)
    {
        const QModelIndex groupIdx = model.index(g, 0);
        for (int c = 0; c < model.rowCount(groupIdx); ++c)
        {
            const QModelIndex catIdx = model.index(c, 0, groupIdx);
            if (model.getCategoryType(model.getCategoryIndex(catIdx))
                == static_cast<int>(CkId::Type_WRLD_))
            {
                wrldCategory = catIdx;
                found = true;
                break;
            }
        }
    }
    QVERIFY(found);
    QCOMPARE(model.rowCount(wrldCategory), 1);
    QVERIFY(model.canFetchMore(wrldCategory));

    // Materialization of a large type is time-sliced (20 ms timer batches)
    // so the UI never freezes; the test snakes the event loop until the
    // category is fully drained.
    model.fetchMore(wrldCategory);

    QTRY_VERIFY_WITH_TIMEOUT(!model.canFetchMore(wrldCategory), 120000);
    QVERIFY(model.rowCount(wrldCategory) > 1);
    qDebug() << "worldspaces after expand:" << model.rowCount(wrldCategory);

// Worldspace-to-cell mapping is derived while loading: Data exposes each
// worldspace's non-interior cells; the fix for "Stored Cells: 0" is that a
// worldspace with grid cells actually reports them.
{
    int totalMapped = 0;
    const auto& wsc = data.getWorldspaceCollection();
    for (int i = 0; i < wsc.size(); ++i)
    {
        const WorldspaceRecord& ws = wsc.getRecord(i).get();
        const QVector<quint32> mapped = data.cellsInWorldspace(ws.formId);
        totalMapped += mapped.size();
        for (quint32 c : mapped)
        {
            // Every reported cell is a real (non-deleted) cell in the
            // collection.
            bool found = false;
            const auto& cells = data.getCellCollection();
            for (int ci = 0; ci < cells.size(); ++ci)
            {
                const auto& cellRec = cells.getRecord(ci);
                if (cellRec.isDeleted()) continue;
                if (cellRec.get().formId == c) { found = true; break; }
            }
            QVERIFY(found);
        }
    }
    QVERIFY(totalMapped > 0);
}

    // Click every worldspace row the way the Inspector wiring does.
    for (int r = 0; r < model.rowCount(wrldCategory); ++r)
    {
        const QModelIndex recIdx = model.index(r, 0, wrldCategory);
        QVERIFY(model.isRecord(recIdx));
        const int cat = model.getCategoryIndex(recIdx);
        const int rec = model.getRecordIndex(recIdx);
        QVERIFY(!model.getRecordEditorId(cat, rec).isEmpty());
        QVERIFY(!model.getRecordFormId(cat, rec).isEmpty());
    }
    qDebug() << "worldspace click-path simulation OK";
}

// Real-data save gate: loading SeydaNeen with its masters and saving must
// emit a structurally valid plugin (TES4 header with real masters, top-level
// GRUPs, cell-children GRUPs) that reloads to the same record counts.
void TestLoaderSinglePass::testSaveRoundTripGRUP()
{
    const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    const QString plugin = realPlugin();
    if (!QFileInfo::exists(dir + '/' + plugin))
        QSKIP(qPrintable(QStringLiteral("Starfield data dir or %1 not found").arg(plugin)));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        realMasters() + QStringList{ plugin },
        dir + '/' + plugin, false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 120000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    const int statCount = doc->getData().getStatCollection().size();
    const int refrCount = doc->getData().getRefrCollection().size();
    const int cellCount = doc->getData().getCellCollection().size();
    QVERIFY(statCount > 0 && refrCount > 0 && cellCount > 0);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString savedPath = tmp.path() + QStringLiteral("/plugin_saved.esp");
    doc->save(savedPath);
    QVERIFY(QFileInfo::exists(savedPath));

    QFile f(savedPath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray bytes = f.readAll();
    f.close();
    QVERIFY(bytes.size() > 24);
    QCOMPARE(QByteArray(bytes.constData(), 4), QByteArray("TES4"));

    // Scan for GRUP blocks: 'GRUP' + size(u32) + label(4) + groupType(u32).
    bool foundStatGrup = false;
    bool foundCellChildrenGrup = false;
    for (int i = 0; i + 24 <= bytes.size(); ++i)
    {
        if (QByteArray(bytes.constData() + i, 4) != QByteArray("GRUP"))
            continue;
        const quint32 label = static_cast<quint8>(bytes.at(i + 8));
        const quint32 type = static_cast<quint8>(bytes.at(i + 12));
        if (label == static_cast<quint8>('S') && type == 0)
            foundStatGrup = true;
        if (type == 6)
            foundCellChildrenGrup = true;
        i += 23; // next potential header start
    }
    QVERIFY(foundStatGrup);
    QVERIFY(foundCellChildrenGrup);

    // Walk the saved file and confirm every record still parses, and the
    // TES4 record count matches what was written.
    ESMReader reader(savedPath);
    reader.open();
    int totalSeen = 0, seenStat = 0, seenCell = 0, seenRefr = 0;
    while (reader.isLeft())
    {
        const NAME name = reader.readName();
        if (name == static_cast<NAME>('GRUP'))
        {
            reader.skipGrupHeader();
            continue;
        }
        reader.readHeader();
        ++totalSeen;
        if (name == static_cast<NAME>('STAT')) ++seenStat;
        else if (name == static_cast<NAME>('CELL')) ++seenCell;
        else if (name == static_cast<NAME>('REFR')) ++seenRefr;
        reader.skip(static_cast<int>(reader.recLeft()));
    }
    QCOMPARE(reader.getHeader().numRecords, totalSeen);
    QCOMPARE(seenStat, statCount);
    QCOMPARE(seenCell, cellCount);
    QCOMPARE(seenRefr, refrCount);
    qDebug() << "saved plugin round-trips: stat" << seenStat
             << "refr" << seenRefr << "cell" << seenCell;
}

// New records must be allocated in the edited plugin's form-id space
// (load-order index in the high byte), not a fabricated range.
void TestLoaderSinglePass::testFormIdAllocation()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPluginEx(tmp.filePath("master_formid.esm"), "masternpc", 0x1111, "Master", 1));
    QVERIFY(writeTestPluginEx(tmp.filePath("plugin_formid.esp"), "pluginnpc", 0x2222, "Plugin", 2));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    Document* doc = mediator.makeDocument(
        QStringList{ "master_formid.esm", "plugin_formid.esp" },
        tmp.path() + "/plugin_formid.esp", false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(tmp.path());
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);
    QCOMPARE(stopped.at(0).at(1).toBool(), true);

    // plugin_formid.esp is index 1 in the load order -> high byte 0x01.
    const quint32 a = doc->getData().createNewRecord(CkId::Type_Npc_, QStringLiteral("NewA"));
    QCOMPARE(a >> 24, static_cast<quint32>(0x01));
    QVERIFY((a & 0xFFFFFF) >= 0x800);
    qDebug() << "allocated form id" << QString::number(a, 16);
}

// Synthetic multi-type round-trip: write a plugin with one record of each
// major type, load it, save untouched, and verify the subrecord payloads
// are identical. Always runs (no real-data dependency).
void TestLoaderSinglePass::testSyntheticMultiTypeRoundTrip()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString pluginPath = tmp.filePath("synth_roundtrip.esp");

    {
        QFile file(pluginPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("Synthetic Round-Trip Test");
        writer.save(file);

        {
            NpcRecord npc;
            npc.initComponents();
            npc.editorId = QStringLiteral("SynthNPC");
            npc.formId = 0x801;
            npc.fullName = QStringLiteral("Synthetic NPC");
            npc.level = 10;
            RecHeader h; h.id = 0x801;
            writer.startRecord('NPC_', h);
            npc.save(writer);
            writer.endRecord();
        }
        {
            GlobalVariable glob;
            glob.editorId = QStringLiteral("SynthGlob");
            glob.value = Variant(quint32(42));
            glob.constant = false;
            RecHeader h; h.id = 0x802;
            writer.startRecord('GLOB', h);
            glob.save(writer);
            writer.endRecord();
        }
        {
            StatRecord stat;
            stat.editorId = QStringLiteral("SynthStat");
            stat.formId = 0x803;
            stat.iconPath = QStringLiteral("icons\\test.dds");
            stat.modelPath = QStringLiteral("meshes\\test.nif");
            stat.flags = 0;
            RecHeader h; h.id = 0x803;
            writer.startRecord('STAT', h);
            stat.save(writer);
            writer.endRecord();
        }
        {
            WorldspaceRecord wrl;
            wrl.editorId = QStringLiteral("SynthWRLD");
            wrl.formId = 0x804;
            wrl.name = QStringLiteral("Synthetic World");
            RecHeader h; h.id = 0x804;
            writer.startRecord('WRLD', h);
            wrl.save(writer);
            writer.endRecord();
        }
        {
            DialRecord dial;
            dial.editorId = QStringLiteral("SynthDIAL");
            dial.formId = 0x805;
            dial.topicName = QStringLiteral("Synthetic Topic");
            dial.hasInam = true;
            dial.responseIds = { 0x806, 0x807 };
            RecHeader h; h.id = 0x805;
            writer.startRecord('DIAL', h);
            dial.save(writer);
            writer.endRecord();
        }

        writer.close();
        file.close();
    }

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ QStringLiteral("synth_roundtrip.esp") },
        tmp.path() + QStringLiteral("/synth_roundtrip.esp"), false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(tmp.path());
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);

    QTemporaryDir out;
    const QString savedPath = out.path() + QStringLiteral("/synth_saved.esp");
    doc->save(savedPath);
    QVERIFY(QFileInfo::exists(savedPath));

    const auto src = openck::collectRecordSnapshots(pluginPath);
    const auto dst = openck::collectRecordSnapshots(savedPath);

    QCOMPARE(src.size(), dst.size());
    QStringList diffs;
    const int n = qMin(src.size(), dst.size());
    for (int i = 0; i < n; ++i)
    {
        if (src.at(i) != dst.at(i))
            diffs.append(QStringLiteral("%1: %2 0x%3 (subs %4 -> %5)")
                .arg(i)
                .arg(openck::snapshotName(src.at(i).type))
                .arg(src.at(i).formId, 8, 16, QChar('0'))
                .arg(src.at(i).subs.size())
                .arg(dst.at(i).subs.size()));
    }
    if (!diffs.isEmpty())
        qWarning().noquote() << "synthetic round-trip diffs:\n" << diffs.join(QStringLiteral("\n"));
    QVERIFY2(diffs.isEmpty(),
        qPrintable(QStringLiteral("synthetic round-trip is not payload-identical (%1 differ)")
            .arg(diffs.size())));
    qDebug() << "synthetic multi-type round-trip OK" << src.size() << "records";
}

// Untouched round-trip must be payload-identical (Phase 1.2): load SeydaNeen
// with its masters, save without any edit, and diff every record's list of
// subrecords (name + payload) against the source file.
void TestLoaderSinglePass::testSaveRoundTripSubrecordIdentical()
{
    const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    const QString plugin = realPlugin();
    if (!QFileInfo::exists(dir + '/' + plugin))
        QSKIP(qPrintable(QStringLiteral("Starfield data dir or %1 not found").arg(plugin)));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        realMasters() + QStringList{ plugin },
        dir + '/' + plugin, false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 120000);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString savedPath = tmp.path() + QStringLiteral("/plugin_saved.esp");
    doc->save(savedPath);
    QVERIFY(QFileInfo::exists(savedPath));

    const auto src = openck::collectRecordSnapshots(dir + '/' + plugin);
    const auto dst = openck::collectRecordSnapshots(savedPath);

    QStringList diffs;
    if (src.size() != dst.size())
        diffs.append(QStringLiteral("record count %1 -> %2").arg(src.size()).arg(dst.size()));
    const int n = qMin(src.size(), dst.size());
    for (int i = 0; i < n; ++i)
    {
        if (src.at(i) != dst.at(i))
            diffs.append(QStringLiteral("%1: %2 0x%3 (subs %4 -> %5)")
                .arg(i)
                .arg(openck::snapshotName(src.at(i).type))
                .arg(src.at(i).formId, 8, 16, QChar('0'))
                .arg(src.at(i).subs.size())
                .arg(dst.at(i).subs.size()));
    }
    if (!diffs.isEmpty())
        qWarning().noquote() << "untouched round-trip subrecord diffs (first 25 of"
                             << diffs.size() << "):\n"
                             << diffs.mid(0, 25).join(QStringLiteral("\n"));
    QVERIFY2(diffs.isEmpty(),
        qPrintable(QStringLiteral("untouched round-trip is not payload-identical (%1 records differ)")
            .arg(diffs.size())));
}

// Master-record state machine on save (Phase 1.3): a materialized (deferred)
// master record saved without edits must NOT be emitted as an override; an
// edit must promote Base -> Modified and the record must then be saved.
void TestLoaderSinglePass::testMasterRecordSaveStateMachine()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(writeTestPluginEx(tmp.filePath("msm_master.esm"), "masternpc", 0x1111, "Master Character", 1));
    QVERIFY(writeTestPluginEx(tmp.filePath("msm_plugin.esp"), "pluginnpc", 0x2222, "Plugin Character", 2));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ "msm_master.esm", "msm_plugin.esp" },
        tmp.path() + "/msm_plugin.esp", false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(tmp.path());
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 15000);

    Data& data = doc->getData();

    // The master's NPC is deferred until materialized, and materializes as State_Base.
    QCOMPARE(data.ensureTypeLoaded(static_cast<int>(CkId::Type_Npc_)), 1);
    QCOMPARE(data.getNpcCollection().size(), 2);
    const int masterIdx = data.getNpcCollection().searchId(QStringLiteral("masternpc"));
    QVERIFY(masterIdx >= 0);
    QCOMPARE(data.getNpcCollection().getRecord(masterIdx).state, State_Base);

    // Untouched save: the materialized master must not be emitted at all.
    QTemporaryDir out;
    const QString saved1 = out.path() + QStringLiteral("/untouched.esp");
    doc->save(saved1);
    {
        const auto snaps = openck::collectRecordSnapshots(saved1);
        bool foundMaster = false;
        bool foundPlugin = false;
        for (const auto& s : snaps)
        {
            if (s.type != NAME('NPC_')) continue;
            if (s.formId == 0x1111) foundMaster = true;
            if (s.formId == 0x2222) foundPlugin = true;
        }
        QVERIFY(!foundMaster);
        QVERIFY(foundPlugin);
    }

    // Edit the master record: promotion Base -> Modified -> emitted as an override.
    Record<NpcRecord>& masterRec = data.getNpcCollection().getRecord(masterIdx);
    masterRec.setModified(masterRec.get());
    QCOMPARE(masterRec.state, State_Modified);
    masterRec.get().level = 99;

    const QString saved2 = out.path() + QStringLiteral("/edited.esp");
    doc->save(saved2);
    {
        const auto snaps = openck::collectRecordSnapshots(saved2);
        const auto it = std::find_if(snaps.cbegin(), snaps.cend(), [](const openck::RecordSnapshot& s) {
            return s.type == NAME('NPC_') && s.formId == 0x1111;
        });
        QVERIFY2(it != snaps.cend(), "edited master record was not emitted as an override");
        QVERIFY(snaps.size() >= 2);
    }
    qDebug() << "master-record state machine save OK";
}

// Full-materialization matrix over the real masters (Phase 1.1 + 4.2): every
// record type that has deferred master records is materialized end-to-end,
// the loader warnings are collected, and the gate asserts zero remaining
// warnings (the ~8 residual INFO/PACK/NPC misalignments must go to zero).
void TestLoaderSinglePass::testMaterializationMatrixZeroWarnings()
{
    const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    // The edited plugin is selectable so a corrupt file on disk (SeydaNeen.esp
    // turned out to be damaged) does not poison the zero-warnings gate.
    const QString plugin = qEnvironmentVariable("OPENCK_TEST_PLUGIN",
        QStringLiteral("Vvardenfell.esp"));
    if (!QFileInfo::exists(dir + '/' + plugin))
        QSKIP(qPrintable(QStringLiteral("Starfield data dir or %1 not found").arg(plugin)));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ QStringLiteral("Starfield.esm"),
                     QStringLiteral("The Elder Star System - Magnus.esm"),
                     plugin },
        dir + '/' + plugin, false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 120000);

    auto& logger = OpenCK::Logging::Logger::instance();
    // CAPTURE eager-load warnings (the item 1.1 target) before clearing.
    const QStringList eagerWarnings = logger.warnings();
    if (!eagerWarnings.isEmpty())
        qWarning().noquote() << "EAGER warnings (" << eagerWarnings.size() << "):\n"
                             << eagerWarnings.join(QStringLiteral("\n"));
    logger.clearWarnings();

    // Diagnose the materialization heap-corruption cluster. The OS heap
    // (not the CRT heap) fast-fails with 0xC0000374 at the next alloc/free
    // after a buffer overrun. Walk the whole process heap with HeapValidate
    // after each type so we learn which type FIRST corrupts it.
    const auto heapOk = []() -> bool {
        return HeapValidate(GetProcessHeap(), 0, nullptr) != FALSE;
    };

    Data& data = doc->getData();

    if (!heapOk())
        qWarning() << "[matrix] HEAP ALREADY CORRUPT after eager load (culprit is an eager-load type)";

    int loadedTotal = 0;
    int typesWithDeferredRecords = 0;
    int firstCorruptType = -1;
    for (int t = CkId::Type_Gmst; t < CkId::NumTypes; ++t)
    {
        if (data.masterIndexCount(t) <= 0)
            continue;
        const QString tname = CkId(static_cast<CkId::Type>(t)).getTypeName();
        const int n = data.ensureTypeLoaded(t);
        loadedTotal += n;
        ++typesWithDeferredRecords;
        fprintf(stderr, "[matrix] type %s (%d) -> +%d records (total %d)\n",
            qPrintable(tname), t, n, loadedTotal);
        fflush(stderr);
        if (firstCorruptType < 0 && !heapOk())
        {
            firstCorruptType = t;
            qWarning() << "[matrix] HEAP CORRUPT after materializing type"
                       << tname << "(" << t << ")";
        }
        if (!_CrtCheckMemory())
            qWarning() << "[matrix] DBGCRT HEAP CORRUPT after materializing type"
                       << tname << "(" << t << ")";
    }
    if (firstCorruptType >= 0)
        qWarning() << "[matrix] first corrupting type index =" << firstCorruptType;
    QVERIFY(typesWithDeferredRecords > 0);
    QVERIFY(loadedTotal > 0);
    qDebug() << "materialization matrix loaded" << loadedTotal << "master records across"
             << typesWithDeferredRecords << "types";

    const QStringList warnings = logger.warnings();
    if (!warnings.isEmpty())
        qWarning().noquote() << "reader warnings after full materialization ("
                             << warnings.size() << "):\n"
                             << warnings.join(QStringLiteral("\n"));
    QVERIFY2(warnings.isEmpty(), "reader warnings must reach zero after full materialization");
}

// DIAL/INFO relationship walking (Phase 1.5): after materializing the
// dialogue types, every INFO must be reachable from its parent DIAL via
// infosUnderDial(), and at least one topic must have responses.
void TestLoaderSinglePass::testDialInfoParentWalking()
{
    const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    const QString plugin = realPlugin();
    if (!QFileInfo::exists(dir + '/' + plugin))
        QSKIP(qPrintable(QStringLiteral("Starfield data dir or %1 not found").arg(plugin)));

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        realMasters() + QStringList{ plugin },
        dir + '/' + plugin, false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 120000);

    Data& data = doc->getData();
    QVERIFY(data.ensureTypeLoaded(static_cast<int>(CkId::Type_Dial_)) > 0);
    const int infosLoaded = data.ensureTypeLoaded(static_cast<int>(CkId::Type_Info_));
    QVERIFY(infosLoaded > 0);

    int topicsWithResponses = 0;
    const auto& dials = data.getDialCollection();
    for (int i = 0; i < dials.size(); ++i)
    {
        const quint32 dialId = dials.getRecord(i).get().formId;
        if (data.infosUnderDial(dialId).size() > 0)
            ++topicsWithResponses;
    }
    QVERIFY(topicsWithResponses > 0);
    qDebug() << "dial topics with responses:" << topicsWithResponses;
}

// ESMWriter group-size stack: nested groups (a CELL group containing a
// cell-children group) must each have their own size patched correctly.
// A single grupSizePos field would leave the outer group's size at zero.
void TestLoaderSinglePass::testGrupSizeConsistent()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const QString path = tmp.path() + QStringLiteral("/nested_groups.esp");

    // Build a minimal file with one top-level group containing a nested group.
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        ESMWriter writer;
        writer.setAuthor("grup-test");
        writer.save(f);

        // Top-level group: type 0, label 'CELL' (swapped on disk).
        writer.startGrup(NAME('CELL'), 0);

        // Fake record inside the top-level group (24-byte header + 4 bytes).
        RecHeader rh;
        rh.size = 4;
        writer.startRecord('TEST', rh);
        writer.writeType<quint32>(0xDEADBEEF);
        writer.endRecord();

        // Nested cell-children group (type 6) inside the top-level group.
        writer.startGrup(0x1234, 6);
        RecHeader rh2;
        rh2.size = 4;
        writer.startRecord('REFT', rh2);
        writer.writeType<quint32>(0xCAFEBABE);
        writer.endRecord();
        writer.endGrup();  // closes children group

        writer.endGrup();  // closes top-level group
        writer.close();
        f.close();
    }

    // Read back the raw bytes.
    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray bytes = f.readAll();
    f.close();
    QVERIFY(bytes.size() > 24);
    QCOMPARE(QByteArray(bytes.constData(), 4), QByteArray("TES4"));

    // Skip the entire TES4 record: 24-byte header + payload.
    const quint8* data = reinterpret_cast<const quint8*>(bytes.constData());
    const quint32 tes4Size = *reinterpret_cast<const quint32*>(data + 4);
    qint64 pos = 24 + tes4Size;

    // Scan for every GRUP header in the file (top-level and nested).
    struct GrupInfo {
        qint64  offset;
        quint32 declaredSize;
        quint32 groupType;
    };
    QList<GrupInfo> groups;
    while (pos + 24 <= bytes.size())
    {
        if (QByteArray(bytes.constData() + pos, 4) == QByteArray("GRUP"))
        {
            const quint32 size    = *reinterpret_cast<const quint32*>(data + pos + 4);
            const quint32 grpType = *reinterpret_cast<const quint32*>(data + pos + 12);
            groups.append({ pos, size, grpType });
        }
        ++pos;
    }

    // We expect exactly 2 GRUPs: top-level CELL (type 0) and nested
    // children (type 6).
    QCOMPARE(groups.size(), 2);
    QCOMPARE(groups.at(0).groupType, static_cast<quint32>(0));
    QCOMPARE(groups.at(1).groupType, static_cast<quint32>(6));

    // The top-level group must not have size 0 (the old single-slot bug).
    QVERIFY2(groups.at(0).declaredSize > 0,
        "outer GRUP size is 0 — likely the single-slot grupSizePos bug");

    // The GRUP size field counts everything after the size field itself:
    // the 16-byte header tail (label+type+vc+unknown) plus content.
    // Inner GRUP: 16 (header tail) + 28 (record: 24-byte hdr + 4 payload) = 44.
    QCOMPARE(groups.at(1).declaredSize, static_cast<quint32>(44));

    // Outer GRUP: 16 (header tail) + 28 (TEST record) + 52 (inner GRUP) = 96.
    QCOMPARE(groups.at(0).declaredSize, static_cast<quint32>(96));

    qDebug() << "nested grup sizes:" << groups.at(0).declaredSize
             << "(outer)" << groups.at(1).declaredSize << "(inner)";
}

void TestLoaderSinglePass::testDiscoverFormIdSubrecordLayouts()
{
    if (!qEnvironmentVariableIsSet("OPENCK_RUN_LONG_TESTS"))
        QSKIP("Diagnostic: requires OPENCK_RUN_LONG_TESTS=1");

    const QString dir = QStringLiteral("C:/XboxGames/Starfield/Content/Data");
    const QString esmPath = dir + "/Starfield.esm";
    if (!QFileInfo::exists(esmPath))
        QSKIP("Starfield.esm not found");

    DocumentMediator mediator;
    QSignalSpy stopped(&mediator, &DocumentMediator::loadingStopped);
    QVERIFY(stopped.isValid());

    Document* doc = mediator.makeDocument(
        QStringList{ QStringLiteral("Starfield.esm") },
        dir + "/Starfield.esm", false);
    const_cast<FilePaths&>(doc->getData().getPaths()).dataDir.setPath(dir);
    mediator.insertDocument(doc);
    QTRY_COMPARE_WITH_TIMEOUT(stopped.count(), 1, 600000);

    Data& data = doc->getData();

    // Collect all known FormIDs.
    QSet<quint32> formIds;
    for (const auto& tc : data.allCollectionsWithTypes())
    {
        IRecordCollection* col = tc.collection;
        if (!col) continue;
        for (int i = 0; i < col->count(); ++i)
        {
            quint32 fid = col->getFormId(i);
            if (fid != 0)
                formIds.insert(fid);
        }
    }

    // Scan raw subrecords for FormID references.
    QHash<QString, int> hits;
    int totalRecords = 0;
    for (const auto& tc : data.allCollectionsWithTypes())
    {
        IRecordCollection* col = tc.collection;
        if (!col) continue;
        QString typeName = CkId(tc.type).getTypeName();
        for (int i = 0; i < col->count(); ++i)
        {
            ++totalRecords;
            auto raws = col->rawSubRecordsAt(i);
            for (const auto& raw : raws)
            {
                QString subStr = QString::fromLatin1(reinterpret_cast<const char*>(&raw.name), 4);
                for (int off = 0; off + 4 <= raw.data.size(); off += 4)
                {
                    quint32 v = 0;
                    std::memcpy(&v, raw.data.constData() + off, 4);
                    if (formIds.contains(v))
                    {
                        hits[typeName + '|' + subStr + '|' + QString::number(off)]++;
                    }
                }
            }
        }
    }

    QFile diagFile(QStringLiteral("C:/Users/max/AppData/Local/Temp/opencode/formid_layouts.txt"));
    diagFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream diag(&diagFile);
    diag << "Records: " << totalRecords << "\n";
    diag << "Known FormIDs: " << formIds.size() << "\n";
    diag << "Distinct (type, sub, offset) hits: " << hits.size() << "\n";
    diag << "=== FILTERED (offset < 128, count >= 5) ===\n";
    int filteredCount = 0;
    for (auto it = hits.constBegin(); it != hits.constEnd(); ++it)
    {
        const QStringList parts = it.key().split('|');
        if (parts.size() < 3) continue;
        int off = parts[2].toInt();
        if (off < 128 && it.value() >= 5)
        {
            diag << it.key() << " x" << it.value() << "\n";
            ++filteredCount;
        }
    }
    diag << "Filtered count: " << filteredCount << "\n";
    diagFile.close();
}

QTEST_GUILESS_MAIN(TestLoaderSinglePass)
#include "test_loader.moc"
