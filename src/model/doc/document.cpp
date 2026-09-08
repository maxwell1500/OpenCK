#include "document.hpp"

#include "../doc/messages.hpp"
#include "../world/metadata.hpp"
#include "../world/irecordcollection.hpp"
#include "../world/basecollection.hpp"
#include "../../view/messageboxhelper.hpp"
#include "logger.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QSet>

Document::Document(const QStringList& contentFiles, const QString& savePath, bool isNew) :
    paths(FilePaths(QCoreApplication::applicationName())),
    contentFiles(contentFiles),
    savePath(savePath),
    newFile(isNew),
    base(false)
{
    data = std::make_unique<Data>(contentFiles, paths);   
    LOG_DEBUG(QString("Document created: %1, %2 files, isNew=%3")
        .arg(savePath)
        .arg(contentFiles.size())
        .arg(isNew ? "true" : "false"));
    
    if (newFile)
    {
        LOG_DEBUG("Creating new document");
        if (contentFiles.size() == 1)
        {
            createNew();
        }
    }
    else
    {
        // Preserve the TES4 header flags (Master / LightMaster) from the
        // loaded plugin so a round-trip save keeps its file type.
        const auto& header = data->getReaderHeader();
        if (header.recHeader.id != 0 || header.recHeader.size != 0)
            mFileFlags = header.recHeader.flags.val;
    }

    reports.reset(new ReportModel());
}

Document::~Document()
{
    LOG_DEBUG(QString("Document destroyed: %1").arg(savePath));
}

// Writes one record (modified or deleted) as a standalone record.
template<typename ESXRecord>
static void writeRecordState(ESMWriter& writer, NAME tag, const Record<ESXRecord>& rec)
{
    RecHeader recHeader;
    recHeader.id = rec.get().formId;
    if (rec.state == State_Deleted)
    {
        recHeader.flags.val = 0x00002000;   // Deleted
        writer.startRecord(tag, recHeader);
        writer.startSubRecord(static_cast<NAME>('DELE'));
        writer.writeType<quint32>(0);
        writer.endSubRecord();
        writer.endRecord();
    }
    else
    {
        writer.startRecord(tag, recHeader);
        rec.get().save(writer);
        writer.endRecord();
    }
}

// Key identifying one record across the ordered replay and the grouped
// fallback so neither pass emits it twice.
static quint64 saveKey(uint32_t tag, quint32 formId)
{
    return (static_cast<quint64>(tag) << 32) | formId;
}

void Document::save(const QString& savePath)
{
    LOG_INFO(QString("Saving document to: %1").arg(savePath));
    ESMWriter writer;

    QFile saveFile{ savePath };
    if (!saveFile.open(QIODevice::WriteOnly))
        return;

    // Set the full TES4 header BEFORE the TES4 record is written, or the
    // MAST/version/author fields never reach the file.
    const auto& readerHeader = data->getReaderHeader();
    writer.setFileFlags(mFileFlags);
    writer.setVersion(readerHeader.version > 0.0f ? readerHeader.version : 1.0f);
    if (!readerHeader.author.isEmpty())
        writer.setAuthor(readerHeader.author);
    if (!readerHeader.description.isEmpty())
        writer.setDescription(readerHeader.description);
    writer.setNextObjectId(readerHeader.nextObjectID);
    for (const auto& master : readerHeader.masters)
    {
        writer.addMaster(master.name, master.size);
    }
    LOG_INFO(QString("Writing header with %1 master files").arg(readerHeader.masters.size()));

    writer.save(saveFile);

    struct RecordTypeTag {
        const IRecordCollection* collection;
        uint32_t tag;
    };

    // REFR and ACHR are written inside their owning cell's children group,
    // not as top-level records.
    QVector<RecordTypeTag> saveableCollections = {
            {&data->getGameSettings(),    'GMST'},
            {&data->getNpcCollection(),   'NPC_'},
            {&data->getWeaponCollection(), 'WEAP'},
            {&data->getArmorCollection(), 'ARMO'},
            {&data->getSpellCollection(), 'SPEL'},
            {&data->getMagicCollection(), 'MGEF'},
            {&data->getQuestCollection(), 'QUST'},
            {&data->getDialCollection(),  'DIAL'},
            {&data->getInfoCollection(),  'INFO'},
            {&data->getGlobCollection(), 'GLOB'},
            {&data->getLcrtCollection(), 'LCRT'},
            {&data->getPackCollection(), 'PACK'},
            {&data->getTreeCollection(), 'TREE'},
            {&data->getAlchCollection(), 'ALCH'},
            {&data->getIngrCollection(), 'INGR'},
            {&data->getContCollection(), 'CONT'},
            {&data->getEnchCollection(), 'ENCH'},
            {&data->getBookCollection(), 'BOOK'},
            {&data->getMiscCollection(), 'MISC'},
            {&data->getActiCollection(), 'ACTI'},
            {&data->getStatCollection(), 'STAT'},
            {&data->getRaceCollection(), 'RACE'},
            {&data->getClassCollection(), 'CLAS'},
            {&data->getFactCollection(), 'FACT'},
            {&data->getPerkCollection(), 'PERK'},
            {&data->getCellCollection(), 'CELL'},
            {&data->getWorldspaceCollection(), 'WRLD'},
            {&data->getLocationCollection(), 'LCTN'},
            {&data->getMaterialCollection(), 'MATL'},
            {&data->getLandCollection(), 'LAND'},
            {&data->getSounCollection(), 'SOUN'},
            {&data->getWthrCollection(), 'WTHR'},
            {&data->getLtexCollection(), 'LTEX'},
            {&data->getScenCollection(), 'SCEN'},
            {&data->getAmmoCollection(), 'AMMO'},
            {&data->getAppaCollection(), 'APPA'},
            {&data->getAvifCollection(), 'AVIF'},
            {&data->getBsgnCollection(), 'BSGN'},
            {&data->getClmtCollection(), 'CLMT'},
            {&data->getClotCollection(), 'CLOT'},
            {&data->getCobjCollection(), 'COBJ'},
            {&data->getCreatureCollection(), 'CREA'},
            {&data->getCstyCollection(), 'CSTY'},
            {&data->getDoorCollection(), 'DOOR'},
            {&data->getEfshCollection(), 'EFSH'},
            {&data->getExplCollection(), 'EXPL'},
            {&data->getEyesCollection(), 'EYES'},
            {&data->getFlorCollection(), 'FLOR'},
            {&data->getFlstCollection(), 'FLST'},
            {&data->getFurnCollection(), 'FURN'},
            {&data->getGrassCollection(), 'GRAS'},
            {&data->getHairCollection(), 'HAIR'},
            {&data->getIdleCollection(), 'IDLE'},
            {&data->getIdlmCollection(), 'IDLM'},
            {&data->getImgsCollection(), 'IMGS'},
            {&data->getKeymCollection(), 'KEYM'},
            {&data->getKywdCollection(), 'KYWD'},
            {&data->getLighCollection(), 'LIGH'},
            {&data->getLscrCollection(), 'LSCR'},
            {&data->getLvlcCollection(), 'LVLC'},
            {&data->getLvliCollection(), 'LVLI'},
            {&data->getLvspCollection(), 'LVSP'},
            {&data->getMesgCollection(), 'MESG'},
            {&data->getMsttCollection(), 'MSTT'},
            {&data->getNavmCollection(), 'NAVM'},
            {&data->getNoteCollection(), 'NOTE'},
            {&data->getOtftCollection(), 'OTFT'},
            {&data->getProjCollection(), 'PROJ'},
            {&data->getRegnCollection(), 'REGN'},
            {&data->getRoadCollection(), 'ROAD'},
            {&data->getScptCollection(), 'SCPT'},
            {&data->getScrlCollection(), 'SCRL'},
            {&data->getSlgmCollection(), 'SLGM'},
            {&data->getSmqnCollection(), 'SMQN'},
            {&data->getSpgdCollection(), 'SPGD'},
            {&data->getScolCollection(), 'SCOL'},
            {&data->getTxstCollection(), 'TXST'},
            {&data->getWateCollection(), 'WATR'},
            {&data->getPlanetCollection(), 'PNDT'},
            {&data->getAnioCollection(),  'ANIO'},
            {&data->getArtvCollection(),  'ARTV'},
            {&data->getClfmCollection(),  'CLFM'},
            {&data->getDebrCollection(),  'DEBR'},
            {&data->getEcznCollection(),  'ECZN'},
            {&data->getHazdCollection(),  'HAZD'},
            {&data->getIpctCollection(),  'IPCT'},
            {&data->getIpdsCollection(),  'IPDS'},
            {&data->getMustCollection(),  'MUST'},
            {&data->getRelaCollection(),  'RELA'},
            {&data->getRevbCollection(),  'REVB'},
            {&data->getShouCollection(),  'SHOU'},
            {&data->getHdptCollection(),  'HDPT'},
            {&data->getTermCollection(),  'TERM'},
            {&data->getMattCollection(),  'MATT'},
            {&data->getMovtCollection(),  'MOVT'},
            {&data->getMuscCollection(),  'MUSC'},
            {&data->getAactCollection(),  'AACT'},
            {&data->getAamdCollection(),  'AAMD'},
            {&data->getAapdCollection(),  'AAPD'},
            {&data->getAddnCollection(),  'ADDN'},
            {&data->getAffeCollection(),  'AFFE'},
            {&data->getAmbsCollection(),  'AMBS'},
            {&data->getAmdlCollection(),  'AMDL'},
            {&data->getAopfCollection(),  'AOPF'},
            {&data->getAopsCollection(),  'AOPS'},
            {&data->getAoruCollection(),  'AORU'},
            {&data->getArmaCollection(),  'ARMA'},
            {&data->getArtoCollection(),  'ARTO'},
            {&data->getAspcCollection(),  'ASPC'},
            {&data->getAtmoCollection(),  'ATMO'},
            {&data->getAvmdCollection(),  'AVMD'},
            {&data->getBiomCollection(),  'BIOM'},
            {&data->getBmmoCollection(),  'BMMO'},
            {&data->getBmodCollection(),  'BMOD'},
            {&data->getBndsCollection(),  'BNDS'},
            {&data->getBptdCollection(),  'BPTD'},
            {&data->getCamsCollection(),  'CAMS'},
            {&data->getChalCollection(),  'CHAL'},
            {&data->getCldfCollection(),  'CLDF'},
            {&data->getCndfCollection(),  'CNDF'},
            {&data->getCollCollection(),  'COLL'},
            {&data->getCpthCollection(),  'CPTH'},
            {&data->getDlbrCollection(),  'DLBR'},
            {&data->getCur3Collection(),  'CUR3'},
            {&data->getCurvCollection(),  'CURV'},
            {&data->getDfobCollection(),  'DFOB'},
            {&data->getDmgtCollection(),  'DMGT'},
            {&data->getDobjCollection(),  'DOBJ'},
            {&data->getEfsqCollection(),  'EFSQ'},
            {&data->getEqupCollection(),  'EQUP'},
            {&data->getFfkwCollection(),  'FFKW'},
            {&data->getFogvCollection(),  'FOGV'},
            {&data->getForcCollection(),  'FORC'},
            {&data->getFstpCollection(),  'FSTP'},
            {&data->getFstsCollection(),  'FSTS'},
            {&data->getFxpdCollection(),  'FXPD'},
            {&data->getGbfmCollection(),  'GBFM'},
            {&data->getGbftCollection(),  'GBFT'},
            {&data->getGcvrCollection(),  'GCVR'},
            {&data->getImadCollection(),  'IMAD'},
            {&data->getInnrCollection(),  'INNR'},
            {&data->getIresCollection(),  'IRES'},
            {&data->getKssmCollection(),  'KSSM'},
            {&data->getLayrCollection(),  'LAYR'},
            {&data->getLensCollection(),  'LENS'},
            {&data->getLgdiCollection(),  'LGDI'},
            {&data->getLgtmCollection(),  'LGTM'},
            {&data->getLmswCollection(),  'LMSW'},
            {&data->getLvlbCollection(),  'LVLB'},
            {&data->getLvlnCollection(),  'LVLN'},
            {&data->getLvlpCollection(),  'LVLP'},
            {&data->getLvscCollection(),  'LVSC'},
            {&data->getMaamCollection(),  'MAAM'},
            {&data->getMrhpCollection(),  'MRPH'},
            {&data->getMtptCollection(),  'MTPT'},
            {&data->getNaviCollection(),  'NAVI'},
            {&data->getNocmCollection(),  'NOCM'},
            {&data->getOmodCollection(),  'OMOD'},
            {&data->getOswpCollection(),  'OSWP'},
            {&data->getOvisCollection(),  'OVIS'},
            {&data->getPcbnCollection(),  'PCBN'},
            {&data->getPccnCollection(),  'PCCN'},
            {&data->getPcmtCollection(),  'PCMT'},
            {&data->getPdclCollection(),  'PDCL'},
            {&data->getPgreCollection(),  'PGRE'},
            {&data->getPhzdCollection(),  'PHZD'},
            {&data->getPkinCollection(),  'PKIN'},
            {&data->getPmftCollection(),  'PMFT'},
            {&data->getPsdcCollection(),  'PSDC'},
            {&data->getPtstCollection(),  'PTST'},
            {&data->getRfgpCollection(),  'RFGP'},
            {&data->getRsgdCollection(),  'RSGD'},
            {&data->getRspjCollection(),  'RSPJ'},
            {&data->getSdltCollection(),  'SDLT'},
            {&data->getSechCollection(),  'SECH'},
            {&data->getSfbkCollection(),  'SFBK'},
            {&data->getSfpcCollection(),  'SFPC'},
            {&data->getSfptCollection(),  'SFPT'},
            {&data->getSftrCollection(),  'SFTR'},
            {&data->getSmbnCollection(),  'SMBN'},
            {&data->getSmenCollection(),  'SMEN'},
            {&data->getSpchCollection(),  'SPCH'},
            {&data->getStagCollection(),  'STAG'},
            {&data->getStbhCollection(),  'STBH'},
            {&data->getStdtCollection(),  'STDT'},
            {&data->getStmpCollection(),  'STMP'},
            {&data->getStndCollection(),  'STND'},
            {&data->getSunpCollection(),  'SUNP'},
            {&data->getTmlmCollection(),  'TMLM'},
            {&data->getToddCollection(),  'TODD'},
            {&data->getTravCollection(),  'TRAV'},
            {&data->getTrnsCollection(),  'TRNS'},
            {&data->getVoliCollection(),  'VOLI'},
            {&data->getVtypCollection(),  'VTYP'},
            {&data->getWbarCollection(),  'WBAR'},
            {&data->getWkmfCollection(),  'WKMF'},
            {&data->getWthsCollection(),  'WTHS'},
            {&data->getWwedCollection(),  'WWED'},
            {&data->getZoomCollection(),  'ZOOM'},
        };

    // Ordered replay of the edited plugin's own load order. The source
    // file is usually interleaved (records appended as the mod was
    // edited), so regrouping by type would scramble an untouched
    // round-trip. Replaying the recorded sequence keeps it
    // payload-identical; anything not in it (new records, master
    // overrides) falls through to the type-grouped pass below.
    // REFR/ACHR are not replayed here: they ride inside their CELL's
    // children group, or the orphan fallback when their CELL is absent.
    QSet<quint64> written;
    {
        QHash<uint32_t, const IRecordCollection*> collByTag;
        QHash<uint32_t, QHash<quint32, int>> indexByTag;
        for (const auto& entry : saveableCollections)
        {
            collByTag.insert(entry.tag, entry.collection);
            QHash<quint32, int>& idx = indexByTag[entry.tag];
            for (int i = 0; i < entry.collection->count(); ++i)
                idx.insert(entry.collection->getFormId(i), i);
        }

        uint32_t openTag = 0;
        bool grupOpen = false;
        const auto ensureGrup = [&](uint32_t tag) {
            if (grupOpen && openTag == tag)
                return;
            if (grupOpen)
                writer.endGrup();
            writer.startGrup(tag, 0);
            openTag = tag;
            grupOpen = true;
        };

        const uint32_t refrTag = static_cast<uint32_t>('REFR');
        const uint32_t achrTag = static_cast<uint32_t>('ACHR');
        const uint32_t cellTag = static_cast<uint32_t>('CELL');
        const auto& cells = data->getCellCollection();
        for (const auto& ref : data->pluginOrder())
        {
            const uint32_t tag = ref.type;
            if (tag == refrTag || tag == achrTag)
                continue;
            const auto itc = collByTag.find(tag);
            if (itc == collByTag.end())
                continue;
            const int idx = indexByTag[tag].value(ref.formId, -1);
            if (idx < 0)
                continue;
            if (written.contains(saveKey(tag, ref.formId)))
                continue;
            if (tag == cellTag)
            {
                if (idx >= cells.size())
                    continue;
                const auto& rec = cells.getRecord(idx);
                if (rec.state != State_Modified && rec.state != State_ModifiedOnly
                    && rec.state != State_Deleted)
                    continue;
                ensureGrup(tag);
                writeRecordState(writer, static_cast<NAME>('CELL'), rec);
                written.insert(saveKey(tag, ref.formId));
                written.insert(saveKey(tag, cells.getFormId(idx)));
                writeCellChildrenGroups(writer, cells.getFormId(idx));
                markCellChildrenWritten(cells.getFormId(idx), written);
                continue;
            }
            if ((*itc)->isRecordSaveable(idx))
            {
                ensureGrup(tag);
                if ((*itc)->saveRecordAt(writer, tag, idx))
                {
                    written.insert(saveKey(tag, ref.formId));
                    written.insert(saveKey(tag, (*itc)->getFormId(idx)));
                }
            }
        }
        if (grupOpen)
            writer.endGrup();
    }

        for (const auto& entry : saveableCollections)
        {
            // Skip types with nothing left to emit: every saveable record
            // was already emitted by the ordered replay below, and no
            // erased-formId stubs are pending (countModifiedRecords
            // over-counts those by design, so compare against the visible
            // saveable records instead).
            int visibleSaveable = 0;
            bool anyUnwritten = false;
            for (int i = 0; i < entry.collection->count(); ++i)
            {
                if (!entry.collection->isRecordSaveable(i))
                    continue;
                ++visibleSaveable;
                if (!written.contains(saveKey(entry.tag, entry.collection->getFormId(i))))
                    anyUnwritten = true;
            }
            const bool maybeStubs =
                entry.collection->countModifiedRecords() > visibleSaveable;
            if (!anyUnwritten && !maybeStubs)
                continue;

            // Wrap each record type in a top-level group (Bethesda layout).
            writer.startGrup(static_cast<quint32>(entry.tag), 0);

            if (static_cast<quint32>(entry.tag) == static_cast<quint32>('CELL'))
            {
                // CELL records must each be followed by their cell-children
                // group, so they cannot use the bulk save path.
                const auto& cells = data->getCellCollection();
                for (int i = 0; i < cells.size(); ++i)
                {
                    const auto& rec = cells.getRecord(i);
                    if (rec.state != State_Modified && rec.state != State_ModifiedOnly
                        && rec.state != State_Deleted)
                        continue;
                    if (written.contains(saveKey(static_cast<quint32>('CELL'), cells.getFormId(i))))
                        continue;
                    writeRecordState(writer, static_cast<NAME>('CELL'), rec);
                    written.insert(saveKey(static_cast<quint32>('CELL'), cells.getFormId(i)));
                    writeCellChildrenGroups(writer, cells.getFormId(i));
                    markCellChildrenWritten(cells.getFormId(i), written);
                }
            }
            else
            {
                entry.collection->saveModifiedRecordsExcept(writer, entry.tag, written);
                for (int i = 0; i < entry.collection->count(); ++i)
                    written.insert(saveKey(entry.tag, entry.collection->getFormId(i)));
            }

            writer.endGrup();
        }

        // References whose parent cell is unknown or not part of this plugin
        // are emitted in a top-level group as a fallback so no reference is
        // silently dropped.
        const auto& refrColl = data->getRefrCollection();
        const auto& achrColl = data->getAchrCollection();
        const auto& cellColl = data->getCellCollection();
        QSet<quint32> savedCellIds;
        for (int i = 0; i < cellColl.size(); ++i)
        {
            const auto& rec = cellColl.getRecord(i);
            if (rec.state == State_Modified || rec.state == State_ModifiedOnly
                || rec.state == State_Deleted)
                savedCellIds.insert(cellColl.getFormId(i));
        }
        const auto orphanWriter = [&](const auto& coll, NAME tag) {
            bool any = false;
            for (int i = 0; i < coll.size(); ++i)
            {
                const auto& rec = coll.getRecord(i);
                if (rec.state != State_Modified && rec.state != State_ModifiedOnly
                    && rec.state != State_Deleted)
                    continue;
                if (savedCellIds.contains(data->parentCellOfRefr(rec.get().formId)))
                    continue;
                any = true;
                break;
            }
            if (!any)
                return;
            writer.startGrup(static_cast<quint32>(tag), 0);
            for (int i = 0; i < coll.size(); ++i)
            {
                const auto& rec = coll.getRecord(i);
                if (rec.state != State_Modified && rec.state != State_ModifiedOnly
                    && rec.state != State_Deleted)
                    continue;
                if (savedCellIds.contains(data->parentCellOfRefr(rec.get().formId)))
                    continue;
                writeRecordState(writer, tag, rec);
            }
            writer.endGrup();
        };
        orphanWriter(refrColl, static_cast<NAME>('REFR'));
        orphanWriter(achrColl, static_cast<NAME>('ACHR'));

    writer.close();
}

void Document::markCellChildrenWritten(quint32 cellId, QSet<quint64>& written) const
{
    // Mirrors writeCellChildrenGroups' selection so the grouped fallback
    // never re-emits references already saved inside a cell-children group.
    const auto& refrColl = data->getRefrCollection();
    for (int r = 0; r < refrColl.size(); ++r)
    {
        const auto& rec = refrColl.getRecord(r);
        if (rec.state != State_Modified && rec.state != State_ModifiedOnly
            && rec.state != State_Deleted)
            continue;
        if (data->parentCellOfRefr(rec.get().formId) == cellId)
            written.insert(saveKey(static_cast<uint32_t>('REFR'), rec.get().formId));
    }
    const auto& achrColl = data->getAchrCollection();
    for (int a = 0; a < achrColl.size(); ++a)
    {
        const auto& rec = achrColl.getRecord(a);
        if (rec.state != State_Modified && rec.state != State_ModifiedOnly
            && rec.state != State_Deleted)
            continue;
        if (data->parentCellOfRefr(rec.get().formId) == cellId)
            written.insert(saveKey(static_cast<uint32_t>('ACHR'), rec.get().formId));
    }
}

void Document::writeCellChildrenGroups(ESMWriter& writer, quint32 cellId)
{
    const auto& refrColl = data->getRefrCollection();
    const auto& achrColl = data->getAchrCollection();

    QVector<const Record<RefrRecord>*> refrs;
    for (int r = 0; r < refrColl.size(); ++r)
    {
        const auto& rec = refrColl.getRecord(r);
        if (rec.state != State_Modified && rec.state != State_ModifiedOnly
            && rec.state != State_Deleted)
            continue;
        if (data->parentCellOfRefr(rec.get().formId) == cellId)
            refrs.append(&rec);
    }
    QVector<const Record<AchrRecord>*> achrs;
    for (int a = 0; a < achrColl.size(); ++a)
    {
        const auto& rec = achrColl.getRecord(a);
        if (rec.state != State_Modified && rec.state != State_ModifiedOnly
            && rec.state != State_Deleted)
            continue;
        if (data->parentCellOfRefr(rec.get().formId) == cellId)
            achrs.append(&rec);
    }

    if (refrs.isEmpty() && achrs.isEmpty())
        return;

    // Cell-children group (type 6) labeled with the owning cell's id.
    writer.startGrup(cellId & 0xFFFFFF, 6);
    for (const auto* rec : refrs)
        writeRecordState(writer, static_cast<NAME>('REFR'), *rec);
    for (const auto* rec : achrs)
        writeRecordState(writer, static_cast<NAME>('ACHR'), *rec);
    writer.endGrup();
}

void Document::createNew()
{
    newFile = true;
    base = false;
    contentFiles.clear();
    data = std::make_unique<Data>(contentFiles, paths);
    LOG_INFO("New empty plugin created");
}

bool Document::isNewFile() const
{
    return newFile;
}

bool Document::isBase() const
{
    return base;
}

const QString Document::getSavePath() const
{
    return savePath;
}

QStringList Document::getContentFiles() const
{
    return contentFiles;
}

std::shared_ptr<ReportModel> Document::getReport()
{
    return reports;
}

const Data& Document::getData() const
{
    return *data;
}

Data& Document::getData()
{
    return *data;
}
