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
