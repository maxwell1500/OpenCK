#include "document.hpp"

#include "../doc/messages.hpp"
#include "../world/metadata.hpp"
#include "../world/irecordcollection.hpp"
#include "../world/basecollection.hpp"
#include "../../view/messageboxhelper.hpp"
#include "logger.hpp"

#include <QCoreApplication>
#include <QFile>

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

void Document::save(const QString& savePath)
{
    LOG_INFO(QString("Saving document to: %1").arg(savePath));
    ESMWriter writer;

    QFile saveFile{ savePath };
    if (saveFile.open(QIODevice::WriteOnly))
    {
        writer.setFileFlags(mFileFlags);
        writer.save(saveFile);

        LOG_DEBUG("Writing TES4 header");
        
        const auto& metaData = data->getMetaData().getRecords();
        for (const auto& record : metaData)
        {
            writer.addMaster(record.get().editorId);
        }
        LOG_INFO(QString("Added %1 master files").arg(metaData.size()));
        
        writer.setVersion(1.0f);

        struct RecordTypeTag {
            const IRecordCollection* collection;
            uint32_t tag;
        };

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
            {&data->getRefrCollection(), 'REFR'},
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
            {&data->getAchrCollection(),  'ACHR'},
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
        };

        for (const auto& entry : saveableCollections)
        {
            entry.collection->saveModifiedRecords(writer, entry.tag);
        }
        
        writer.close();
    }
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
