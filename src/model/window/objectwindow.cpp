#include "objectwindow.hpp"

#include "../world/data.hpp"
#include "../world/collection.hpp"
#include "../world/idcollection.hpp"
#include "../tools/objectwindowfilter.hpp"

#include <QJsonObject>

ObjectWindowModel::ObjectWindowModel(QObject* parent)
    : QAbstractItemModel(parent),
      mData(nullptr)
{
    mMaterializeTimer.setInterval(20);
    mMaterializeTimer.setSingleShot(false);
    connect(&mMaterializeTimer, &QTimer::timeout, this, &ObjectWindowModel::materializeTick);
}

void ObjectWindowModel::setData(Data* data)
{
    beginResetModel();

    mMaterializeTimer.stop();
    mJob.active = false;
    mData = data;
    mCategories.clear();
    mGroups.clear();
    mFilter.clear();

    if (mData)
    {
        initCategories(mData);
    }

    endResetModel();
}

void ObjectWindowModel::initCategories(Data* data)
{
    auto addCategory = [this, data](const QString& name, CkId::Type typeId) {
        Category cat;
        cat.name = name;
        cat.typeId = static_cast<int>(typeId);
        cat.totalRecords = 0;

        switch (typeId)
        {
        case CkId::Type_Gmst:
            cat.totalRecords = data->getGameSettings().size();
            break;
        case CkId::Type_Npc_:
            cat.totalRecords = data->getNpcCollection().size();
            break;
        case CkId::Type_Weap_:
            cat.totalRecords = data->getWeaponCollection().size();
            break;
        case CkId::Type_Armor_:
            cat.totalRecords = data->getArmorCollection().size();
            break;
        case CkId::Type_Spel_:
            cat.totalRecords = data->getSpellCollection().size();
            break;
        case CkId::Type_Magic_:
            cat.totalRecords = data->getMagicCollection().size();
            break;
        case CkId::Type_Quest_:
            cat.totalRecords = data->getQuestCollection().size();
            break;
        case CkId::Type_Dial_:
            cat.totalRecords = data->getDialCollection().size();
            break;
        case CkId::Type_Info_:
            cat.totalRecords = data->getInfoCollection().size();
            break;
        case CkId::Type_Glob_:
            cat.totalRecords = data->getGlobCollection().size();
            break;
        case CkId::Type_Lcrt_:
            cat.totalRecords = data->getLcrtCollection().size();
            break;
        case CkId::Type_Pack_:
            cat.totalRecords = data->getPackCollection().size();
            break;
        case CkId::Type_Tree_:
            cat.totalRecords = data->getTreeCollection().size();
            break;
        case CkId::Type_Alch_:
            cat.totalRecords = data->getAlchCollection().size();
            break;
        case CkId::Type_Ingr_:
            cat.totalRecords = data->getIngrCollection().size();
            break;
        case CkId::Type_Cont_:
            cat.totalRecords = data->getContCollection().size();
            break;
        case CkId::Type_Ench_:
            cat.totalRecords = data->getEnchCollection().size();
            break;
        case CkId::Type_Book_:
            cat.totalRecords = data->getBookCollection().size();
            break;
        case CkId::Type_Misc_:
            cat.totalRecords = data->getMiscCollection().size();
            break;
        case CkId::Type_Acti_:
            cat.totalRecords = data->getActiCollection().size();
            break;
        case CkId::Type_Stat_:
            cat.totalRecords = data->getStatCollection().size();
            break;
        case CkId::Type_Race_:
            cat.totalRecords = data->getRaceCollection().size();
            break;
        case CkId::Type_Class_:
            cat.totalRecords = data->getClassCollection().size();
            break;
        case CkId::Type_Fact_:
            cat.totalRecords = data->getFactCollection().size();
            break;
        case CkId::Type_PerK_:
            cat.totalRecords = data->getPerkCollection().size();
            break;
        case CkId::Type_Soun_:
            cat.totalRecords = data->getSounCollection().size();
            break;
        case CkId::Type_Wthr_:
            cat.totalRecords = data->getWthrCollection().size();
            break;
        case CkId::Type_Ltex_:
            cat.totalRecords = data->getLtexCollection().size();
            break;
        case CkId::Type_Cel_:
            cat.totalRecords = data->getCellCollection().size();
            break;
        case CkId::Type_WRLD_:
            cat.totalRecords = data->getWorldspaceCollection().size();
            break;
        case CkId::Type_LOCT_:
            cat.totalRecords = data->getLocationCollection().size();
            break;
        case CkId::Type_Plnt_:
            cat.totalRecords = data->getPlanetCollection().size();
            break;
        case CkId::Type_Refr_:
            cat.totalRecords = data->getRefrCollection().size();
            break;
        case CkId::Type_Material_:
            cat.totalRecords = data->getMaterialCollection().size();
            break;
        case CkId::Type_Land_:
            cat.totalRecords = data->getLandCollection().size();
            break;
        case CkId::Type_Ammo_:
            cat.totalRecords = data->getAmmoCollection().size();
            break;
        case CkId::Type_Appa_:
            cat.totalRecords = data->getAppaCollection().size();
            break;
        case CkId::Type_Avif_:
            cat.totalRecords = data->getAvifCollection().size();
            break;
        case CkId::Type_Bsgn_:
            cat.totalRecords = data->getBsgnCollection().size();
            break;
        case CkId::Type_Clmt_:
            cat.totalRecords = data->getClmtCollection().size();
            break;
        case CkId::Type_Clot_:
            cat.totalRecords = data->getClotCollection().size();
            break;
        case CkId::Type_Cobj_:
            cat.totalRecords = data->getCobjCollection().size();
            break;
        case CkId::Type_Crea_:
            cat.totalRecords = data->getCreatureCollection().size();
            break;
        case CkId::Type_Csty_:
            cat.totalRecords = data->getCstyCollection().size();
            break;
        case CkId::Type_Door_:
            cat.totalRecords = data->getDoorCollection().size();
            break;
        case CkId::Type_Efsh_:
            cat.totalRecords = data->getEfshCollection().size();
            break;
        case CkId::Type_Expl_:
            cat.totalRecords = data->getExplCollection().size();
            break;
        case CkId::Type_Eyes_:
            cat.totalRecords = data->getEyesCollection().size();
            break;
        case CkId::Type_Flor_:
            cat.totalRecords = data->getFlorCollection().size();
            break;
        case CkId::Type_Flst_:
            cat.totalRecords = data->getFlstCollection().size();
            break;
        case CkId::Type_Furn_:
            cat.totalRecords = data->getFurnCollection().size();
            break;
        case CkId::Type_Grass_:
            cat.totalRecords = data->getGrassCollection().size();
            break;
        case CkId::Type_Hair_:
            cat.totalRecords = data->getHairCollection().size();
            break;
        case CkId::Type_Idle_:
            cat.totalRecords = data->getIdleCollection().size();
            break;
        case CkId::Type_Idlm_:
            cat.totalRecords = data->getIdlmCollection().size();
            break;
        case CkId::Type_Imgs_:
            cat.totalRecords = data->getImgsCollection().size();
            break;
        case CkId::Type_Keym_:
            cat.totalRecords = data->getKeymCollection().size();
            break;
        case CkId::Type_Kywd_:
            cat.totalRecords = data->getKywdCollection().size();
            break;
        case CkId::Type_Ligh_:
            cat.totalRecords = data->getLighCollection().size();
            break;
        case CkId::Type_Lscr_:
            cat.totalRecords = data->getLscrCollection().size();
            break;
        case CkId::Type_Lvlc_:
            cat.totalRecords = data->getLvlcCollection().size();
            break;
        case CkId::Type_Lvli_:
            cat.totalRecords = data->getLvliCollection().size();
            break;
        case CkId::Type_Lvsp_:
            cat.totalRecords = data->getLvspCollection().size();
            break;
        case CkId::Type_Mesg_:
            cat.totalRecords = data->getMesgCollection().size();
            break;
        case CkId::Type_Mstt_:
            cat.totalRecords = data->getMsttCollection().size();
            break;
        case CkId::Type_Navm_:
            cat.totalRecords = data->getNavmCollection().size();
            break;
        case CkId::Type_Note_:
            cat.totalRecords = data->getNoteCollection().size();
            break;
        case CkId::Type_Otft_:
            cat.totalRecords = data->getOtftCollection().size();
            break;
        case CkId::Type_Proj_:
            cat.totalRecords = data->getProjCollection().size();
            break;
        case CkId::Type_Regn_:
            cat.totalRecords = data->getRegnCollection().size();
            break;
        case CkId::Type_Road_:
            cat.totalRecords = data->getRoadCollection().size();
            break;
        case CkId::Type_Scpt_:
            cat.totalRecords = data->getScptCollection().size();
            break;
        case CkId::Type_Scrl_:
            cat.totalRecords = data->getScrlCollection().size();
            break;
        case CkId::Type_Slgm_:
            cat.totalRecords = data->getSlgmCollection().size();
            break;
        case CkId::Type_Smqn_:
            cat.totalRecords = data->getSmqnCollection().size();
            break;
        case CkId::Type_Spgd_:
            cat.totalRecords = data->getSpgdCollection().size();
            break;
        case CkId::Type_Scol_:
            cat.totalRecords = data->getScolCollection().size();
            break;
        case CkId::Type_Txst_:
            cat.totalRecords = data->getTxstCollection().size();
            break;
        case CkId::Type_Wate_:
            cat.totalRecords = data->getWateCollection().size();
            break;
        case CkId::Type_Scen_:
            cat.totalRecords = data->getScenCollection().size();
            break;
        case CkId::Type_Anio_:
            cat.totalRecords = data->getAnioCollection().size();
            break;
        case CkId::Type_Artv_:
            cat.totalRecords = data->getArtvCollection().size();
            break;
        case CkId::Type_Clfm_:
            cat.totalRecords = data->getClfmCollection().size();
            break;
        case CkId::Type_Debr_:
            cat.totalRecords = data->getDebrCollection().size();
            break;
        case CkId::Type_Eczn_:
            cat.totalRecords = data->getEcznCollection().size();
            break;
        case CkId::Type_Hazd_:
            cat.totalRecords = data->getHazdCollection().size();
            break;
        case CkId::Type_Ipct_:
            cat.totalRecords = data->getIpctCollection().size();
            break;
        case CkId::Type_Must_:
            cat.totalRecords = data->getMustCollection().size();
            break;
        case CkId::Type_Rela_:
            cat.totalRecords = data->getRelaCollection().size();
            break;
        case CkId::Type_Revb_:
            cat.totalRecords = data->getRevbCollection().size();
            break;
        case CkId::Type_Shou_:
            cat.totalRecords = data->getShouCollection().size();
            break;
        case CkId::Type_Hdpt_:
            cat.totalRecords = data->getHdptCollection().size();
            break;
        case CkId::Type_Term_:
            cat.totalRecords = data->getTermCollection().size();
            break;
        case CkId::Type_Matt_:
            cat.totalRecords = data->getMattCollection().size();
            break;
        case CkId::Type_Movt_:
            cat.totalRecords = data->getMovtCollection().size();
            break;
        case CkId::Type_Musc_:
            cat.totalRecords = data->getMuscCollection().size();
            break;
        case CkId::Type_Aact_:
            cat.totalRecords = data->getAactCollection().size();
            break;
        case CkId::Type_Aamd_:
            cat.totalRecords = data->getAamdCollection().size();
            break;
        case CkId::Type_Aapd_:
            cat.totalRecords = data->getAapdCollection().size();
            break;
        case CkId::Type_Achr_:
            cat.totalRecords = data->getAchrCollection().size();
            break;
        case CkId::Type_Addn_:
            cat.totalRecords = data->getAddnCollection().size();
            break;
        case CkId::Type_Affe_:
            cat.totalRecords = data->getAffeCollection().size();
            break;
        case CkId::Type_Ambs_:
            cat.totalRecords = data->getAmbsCollection().size();
            break;
        case CkId::Type_Amdl_:
            cat.totalRecords = data->getAmdlCollection().size();
            break;
        case CkId::Type_Aopf_:
            cat.totalRecords = data->getAopfCollection().size();
            break;
        case CkId::Type_Aops_:
            cat.totalRecords = data->getAopsCollection().size();
            break;
        case CkId::Type_Aoru_:
            cat.totalRecords = data->getAoruCollection().size();
            break;
        case CkId::Type_Arma_:
            cat.totalRecords = data->getArmaCollection().size();
            break;
        case CkId::Type_Arto_:
            cat.totalRecords = data->getArtoCollection().size();
            break;
        case CkId::Type_Aspc_:
            cat.totalRecords = data->getAspcCollection().size();
            break;
        case CkId::Type_Atmo_:
            cat.totalRecords = data->getAtmoCollection().size();
            break;
        case CkId::Type_Avmd_:
            cat.totalRecords = data->getAvmdCollection().size();
            break;
        case CkId::Type_Biom_:
            cat.totalRecords = data->getBiomCollection().size();
            break;
        case CkId::Type_Bmmo_:
            cat.totalRecords = data->getBmmoCollection().size();
            break;
        case CkId::Type_Bmod_:
            cat.totalRecords = data->getBmodCollection().size();
            break;
        case CkId::Type_Bnds_:
            cat.totalRecords = data->getBndsCollection().size();
            break;
        case CkId::Type_Bptd_:
            cat.totalRecords = data->getBptdCollection().size();
            break;
        case CkId::Type_Cams_:
            cat.totalRecords = data->getCamsCollection().size();
            break;
        case CkId::Type_Chal_:
            cat.totalRecords = data->getChalCollection().size();
            break;
        case CkId::Type_Cldf_:
            cat.totalRecords = data->getCldfCollection().size();
            break;
        case CkId::Type_Cndf_:
            cat.totalRecords = data->getCndfCollection().size();
            break;
        case CkId::Type_Coll_:
            cat.totalRecords = data->getCollCollection().size();
            break;
        case CkId::Type_Cpth_:
            cat.totalRecords = data->getCpthCollection().size();
            break;
        case CkId::Type_Dlbr_:
            cat.totalRecords = data->getDlbrCollection().size();
            break;
        case CkId::Type_Cur3_:
            cat.totalRecords = data->getCur3Collection().size();
            break;
        case CkId::Type_Curv_:
            cat.totalRecords = data->getCurvCollection().size();
            break;
        case CkId::Type_Dfob_:
            cat.totalRecords = data->getDfobCollection().size();
            break;
        case CkId::Type_Dmgt_:
            cat.totalRecords = data->getDmgtCollection().size();
            break;
        case CkId::Type_Dobj_:
            cat.totalRecords = data->getDobjCollection().size();
            break;
        case CkId::Type_Efsq_:
            cat.totalRecords = data->getEfsqCollection().size();
            break;
        case CkId::Type_Equp_:
            cat.totalRecords = data->getEqupCollection().size();
        case CkId::Type_Ffkw_:
            cat.totalRecords = data->getFfkwCollection().size();
            break;
        case CkId::Type_Fogv_:
            cat.totalRecords = data->getFogvCollection().size();
            break;
        case CkId::Type_Forc_:
            cat.totalRecords = data->getForcCollection().size();
            break;
        case CkId::Type_Fstp_:
            cat.totalRecords = data->getFstpCollection().size();
            break;
        case CkId::Type_Fsts_:
            cat.totalRecords = data->getFstsCollection().size();
            break;
        case CkId::Type_Fxpd_:
            cat.totalRecords = data->getFxpdCollection().size();
            break;
        case CkId::Type_Gbfm_:
            cat.totalRecords = data->getGbfmCollection().size();
            break;
        case CkId::Type_Gbft_:
            cat.totalRecords = data->getGbftCollection().size();
            break;
        case CkId::Type_Gcvr_:
            cat.totalRecords = data->getGcvrCollection().size();
            break;
        case CkId::Type_Imad_:
            cat.totalRecords = data->getImadCollection().size();
            break;
        case CkId::Type_Innr_:
            cat.totalRecords = data->getInnrCollection().size();
            break;
        case CkId::Type_Ires_:
            cat.totalRecords = data->getIresCollection().size();
            break;
        case CkId::Type_Kssm_:
            cat.totalRecords = data->getKssmCollection().size();
            break;
        case CkId::Type_Layr_:
            cat.totalRecords = data->getLayrCollection().size();
            break;
        case CkId::Type_Lens_:
            cat.totalRecords = data->getLensCollection().size();
            break;
        case CkId::Type_Lgdi_:
            cat.totalRecords = data->getLgdiCollection().size();
            break;
        case CkId::Type_Lgtm_:
            cat.totalRecords = data->getLgtmCollection().size();
            break;
        case CkId::Type_Lmsw_:
            cat.totalRecords = data->getLmswCollection().size();
            break;
        case CkId::Type_Lvlb_:
            cat.totalRecords = data->getLvlbCollection().size();
            break;
        case CkId::Type_Lvln_:
            cat.totalRecords = data->getLvlnCollection().size();
            break;
        case CkId::Type_Lvlp_:
            cat.totalRecords = data->getLvlpCollection().size();
            break;
        case CkId::Type_Lvsc_:
            cat.totalRecords = data->getLvscCollection().size();
            break;
        case CkId::Type_Maam_:
            cat.totalRecords = data->getMaamCollection().size();
            break;
        case CkId::Type_Mrhp_:
            cat.totalRecords = data->getMrhpCollection().size();
            break;
        case CkId::Type_Mtpt_:
            cat.totalRecords = data->getMtptCollection().size();
            break;
        case CkId::Type_Navi_:
            cat.totalRecords = data->getNaviCollection().size();
            break;
        case CkId::Type_Nocm_:
            cat.totalRecords = data->getNocmCollection().size();
            break;
        case CkId::Type_Omod_:
            cat.totalRecords = data->getOmodCollection().size();
            break;
        case CkId::Type_Oswp_:
            cat.totalRecords = data->getOswpCollection().size();
            break;
        case CkId::Type_Ovis_:
            cat.totalRecords = data->getOvisCollection().size();
            break;
        case CkId::Type_Pcbn_:
            cat.totalRecords = data->getPcbnCollection().size();
            break;
        case CkId::Type_Pccn_:
            cat.totalRecords = data->getPccnCollection().size();
            break;
        case CkId::Type_Pcmt_:
            cat.totalRecords = data->getPcmtCollection().size();
            break;
        case CkId::Type_Pdcl_:
            cat.totalRecords = data->getPdclCollection().size();
            break;
        case CkId::Type_Pgre_:
            cat.totalRecords = data->getPgreCollection().size();
        case CkId::Type_Phzd_:
            cat.totalRecords = data->getPhzdCollection().size();
            break;
        case CkId::Type_Pkin_:
            cat.totalRecords = data->getPkinCollection().size();
            break;
        case CkId::Type_Pmft_:
            cat.totalRecords = data->getPmftCollection().size();
            break;
        case CkId::Type_Psdc_:
            cat.totalRecords = data->getPsdcCollection().size();
            break;
        case CkId::Type_Ptst_:
            cat.totalRecords = data->getPtstCollection().size();
            break;
        case CkId::Type_Rfgp_:
            cat.totalRecords = data->getRfgpCollection().size();
            break;
        case CkId::Type_Rsgd_:
            cat.totalRecords = data->getRsgdCollection().size();
            break;
        case CkId::Type_Rspj_:
            cat.totalRecords = data->getRspjCollection().size();
            break;
        case CkId::Type_Sdlt_:
            cat.totalRecords = data->getSdltCollection().size();
            break;
        case CkId::Type_Sech_:
            cat.totalRecords = data->getSechCollection().size();
            break;
        case CkId::Type_Sfbk_:
            cat.totalRecords = data->getSfbkCollection().size();
            break;
        case CkId::Type_Sfpc_:
            cat.totalRecords = data->getSfpcCollection().size();
            break;
        case CkId::Type_Sfpt_:
            cat.totalRecords = data->getSfptCollection().size();
            break;
        case CkId::Type_Sftr_:
            cat.totalRecords = data->getSftrCollection().size();
            break;
        case CkId::Type_Smbn_:
            cat.totalRecords = data->getSmbnCollection().size();
            break;
        case CkId::Type_Smen_:
            cat.totalRecords = data->getSmenCollection().size();
            break;
        case CkId::Type_Spch_:
            cat.totalRecords = data->getSpchCollection().size();
            break;
        case CkId::Type_Stag_:
            cat.totalRecords = data->getStagCollection().size();
            break;
        case CkId::Type_Stbh_:
            cat.totalRecords = data->getStbhCollection().size();
            break;
        case CkId::Type_Stdt_:
            cat.totalRecords = data->getStdtCollection().size();
            break;
        case CkId::Type_Stmp_:
            cat.totalRecords = data->getStmpCollection().size();
            break;
        case CkId::Type_Stnd_:
            cat.totalRecords = data->getStndCollection().size();
            break;
        case CkId::Type_Sunp_:
            cat.totalRecords = data->getSunpCollection().size();
            break;
        case CkId::Type_Tmlm_:
            cat.totalRecords = data->getTmlmCollection().size();
            break;
        case CkId::Type_Todd_:
            cat.totalRecords = data->getToddCollection().size();
            break;
        case CkId::Type_Trav_:
            cat.totalRecords = data->getTravCollection().size();
            break;
        case CkId::Type_Trns_:
            cat.totalRecords = data->getTrnsCollection().size();
            break;
        case CkId::Type_Voli_:
            cat.totalRecords = data->getVoliCollection().size();
            break;
        case CkId::Type_Vtyp_:
            cat.totalRecords = data->getVtypCollection().size();
            break;
        case CkId::Type_Wbar_:
            cat.totalRecords = data->getWbarCollection().size();
            break;
        case CkId::Type_Wkmf_:
            cat.totalRecords = data->getWkmfCollection().size();
            break;
        case CkId::Type_Wths_:
            cat.totalRecords = data->getWthsCollection().size();
            break;
        case CkId::Type_Wwed_:
            cat.totalRecords = data->getWwedCollection().size();
            break;
        case CkId::Type_Zoom_:
            cat.totalRecords = data->getZoomCollection().size();
            break;
        default:
            break;
        }

        // Deferred masters: some records of this type were indexed, not
        // parsed. parsedCount is what the eager pass produced; the rest
        // comes from the master index and is materialized via fetchMore()
        // when the user expands the category.
        cat.parsedCount = cat.totalRecords;
        const int pending = data->masterIndexCount(static_cast<CkId::Type>(typeId));
        if (pending > 0)
        {
            cat.totalRecords = cat.parsedCount + pending;
            cat.pendingMaterialize = true;
        }

        for (int i = 0; i < cat.parsedCount; i++)
        {
            VisibleRecord rec;
            rec.actualIndex = i;

            QString editorId;
            QString formId;

            switch (typeId)
            {
            case CkId::Type_Gmst:
                editorId = data->getGameSettings().getId(i);
                formId = QString();
                break;
            case CkId::Type_Npc_:
                editorId = data->getNpcCollection().getId(i);
                formId = formatFormId(data->getNpcCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Weap_:
                editorId = data->getWeaponCollection().getId(i);
                formId = formatFormId(data->getWeaponCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Armor_:
                editorId = data->getArmorCollection().getId(i);
                formId = formatFormId(data->getArmorCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Spel_:
                editorId = data->getSpellCollection().getId(i);
                formId = formatFormId(data->getSpellCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Magic_:
                editorId = data->getMagicCollection().getId(i);
                formId = formatFormId(data->getMagicCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Quest_:
                editorId = data->getQuestCollection().getId(i);
                formId = formatFormId(data->getQuestCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Dial_:
                editorId = data->getDialCollection().getId(i);
                formId = formatFormId(data->getDialCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Info_:
                editorId = data->getInfoCollection().getId(i);
                formId = formatFormId(data->getInfoCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Glob_:
                editorId = data->getGlobCollection().getId(i);
                formId = QString();
                break;
            case CkId::Type_Lcrt_:
                editorId = data->getLcrtCollection().getId(i);
                formId = QString();
                break;
            case CkId::Type_Pack_:
                editorId = data->getPackCollection().getId(i);
                formId = formatFormId(data->getPackCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Tree_:
                editorId = data->getTreeCollection().getId(i);
                formId = formatFormId(data->getTreeCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Alch_:
                editorId = data->getAlchCollection().getId(i);
                formId = formatFormId(data->getAlchCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ingr_:
                editorId = data->getIngrCollection().getId(i);
                formId = formatFormId(data->getIngrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Cont_:
                editorId = data->getContCollection().getId(i);
                formId = formatFormId(data->getContCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ench_:
                editorId = data->getEnchCollection().getId(i);
                formId = formatFormId(data->getEnchCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Book_:
                editorId = data->getBookCollection().getId(i);
                formId = formatFormId(data->getBookCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Misc_:
                editorId = data->getMiscCollection().getId(i);
                formId = formatFormId(data->getMiscCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Acti_:
                editorId = data->getActiCollection().getId(i);
                formId = formatFormId(data->getActiCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Stat_:
                editorId = data->getStatCollection().getId(i);
                formId = formatFormId(data->getStatCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Race_:
                editorId = data->getRaceCollection().getId(i);
                formId = formatFormId(data->getRaceCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Class_:
                editorId = data->getClassCollection().getId(i);
                formId = formatFormId(data->getClassCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Fact_:
                editorId = data->getFactCollection().getId(i);
                formId = formatFormId(data->getFactCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_PerK_:
                editorId = data->getPerkCollection().getId(i);
                formId = formatFormId(data->getPerkCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Soun_:
                editorId = data->getSounCollection().getId(i);
                formId = formatFormId(data->getSounCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Wthr_:
                editorId = data->getWthrCollection().getId(i);
                formId = formatFormId(data->getWthrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ltex_:
                editorId = data->getLtexCollection().getId(i);
                formId = formatFormId(data->getLtexCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Cel_:
                editorId = data->getCellCollection().getId(i);
                formId = formatFormId(data->getCellCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_WRLD_:
                editorId = data->getWorldspaceCollection().getId(i);
                formId = formatFormId(data->getWorldspaceCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_LOCT_:
                editorId = data->getLocationCollection().getId(i);
                formId = formatFormId(data->getLocationCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Plnt_:
                editorId = data->getPlanetCollection().getId(i);
                formId = formatFormId(data->getPlanetCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Refr_:
                editorId = data->getRefrCollection().getId(i);
                formId = formatFormId(data->getRefrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Material_:
                editorId = data->getMaterialCollection().getId(i);
                formId = formatFormId(data->getMaterialCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Land_:
                editorId = data->getLandCollection().getId(i);
                formId = formatFormId(data->getLandCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ammo_:
                editorId = data->getAmmoCollection().getId(i);
                formId = formatFormId(data->getAmmoCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Appa_:
                editorId = data->getAppaCollection().getId(i);
                formId = formatFormId(data->getAppaCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Avif_:
                editorId = data->getAvifCollection().getId(i);
                formId = formatFormId(data->getAvifCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Bsgn_:
                editorId = data->getBsgnCollection().getId(i);
                formId = formatFormId(data->getBsgnCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Clmt_:
                editorId = data->getClmtCollection().getId(i);
                formId = formatFormId(data->getClmtCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Clot_:
                editorId = data->getClotCollection().getId(i);
                formId = formatFormId(data->getClotCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Cobj_:
                editorId = data->getCobjCollection().getId(i);
                formId = formatFormId(data->getCobjCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Crea_:
                editorId = data->getCreatureCollection().getId(i);
                formId = formatFormId(data->getCreatureCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Csty_:
                editorId = data->getCstyCollection().getId(i);
                formId = formatFormId(data->getCstyCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Door_:
                editorId = data->getDoorCollection().getId(i);
                formId = formatFormId(data->getDoorCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Efsh_:
                editorId = data->getEfshCollection().getId(i);
                formId = formatFormId(data->getEfshCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Expl_:
                editorId = data->getExplCollection().getId(i);
                formId = formatFormId(data->getExplCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Eyes_:
                editorId = data->getEyesCollection().getId(i);
                formId = formatFormId(data->getEyesCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Flor_:
                editorId = data->getFlorCollection().getId(i);
                formId = formatFormId(data->getFlorCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Flst_:
                editorId = data->getFlstCollection().getId(i);
                formId = formatFormId(data->getFlstCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Furn_:
                editorId = data->getFurnCollection().getId(i);
                formId = formatFormId(data->getFurnCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Grass_:
                editorId = data->getGrassCollection().getId(i);
                formId = formatFormId(data->getGrassCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Hair_:
                editorId = data->getHairCollection().getId(i);
                formId = formatFormId(data->getHairCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Idle_:
                editorId = data->getIdleCollection().getId(i);
                formId = formatFormId(data->getIdleCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Idlm_:
                editorId = data->getIdlmCollection().getId(i);
                formId = formatFormId(data->getIdlmCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Imgs_:
                editorId = data->getImgsCollection().getId(i);
                formId = formatFormId(data->getImgsCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Keym_:
                editorId = data->getKeymCollection().getId(i);
                formId = formatFormId(data->getKeymCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Kywd_:
                editorId = data->getKywdCollection().getId(i);
                formId = formatFormId(data->getKywdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ligh_:
                editorId = data->getLighCollection().getId(i);
                formId = formatFormId(data->getLighCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lscr_:
                editorId = data->getLscrCollection().getId(i);
                formId = formatFormId(data->getLscrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lvlc_:
                editorId = data->getLvlcCollection().getId(i);
                formId = formatFormId(data->getLvlcCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lvli_:
                editorId = data->getLvliCollection().getId(i);
                formId = formatFormId(data->getLvliCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lvsp_:
                editorId = data->getLvspCollection().getId(i);
                formId = formatFormId(data->getLvspCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Mesg_:
                editorId = data->getMesgCollection().getId(i);
                formId = formatFormId(data->getMesgCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Mstt_:
                editorId = data->getMsttCollection().getId(i);
                formId = formatFormId(data->getMsttCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Navm_:
                editorId = data->getNavmCollection().getId(i);
                formId = formatFormId(data->getNavmCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Note_:
                editorId = data->getNoteCollection().getId(i);
                formId = formatFormId(data->getNoteCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Otft_:
                editorId = data->getOtftCollection().getId(i);
                formId = formatFormId(data->getOtftCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Proj_:
                editorId = data->getProjCollection().getId(i);
                formId = formatFormId(data->getProjCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Regn_:
                editorId = data->getRegnCollection().getId(i);
                formId = formatFormId(data->getRegnCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Road_:
                editorId = data->getRoadCollection().getId(i);
                formId = formatFormId(data->getRoadCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Scpt_:
                editorId = data->getScptCollection().getId(i);
                formId = formatFormId(data->getScptCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Scrl_:
                editorId = data->getScrlCollection().getId(i);
                formId = formatFormId(data->getScrlCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Slgm_:
                editorId = data->getSlgmCollection().getId(i);
                formId = formatFormId(data->getSlgmCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Smqn_:
                editorId = data->getSmqnCollection().getId(i);
                formId = formatFormId(data->getSmqnCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Spgd_:
                editorId = data->getSpgdCollection().getId(i);
                formId = formatFormId(data->getSpgdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Scol_:
                editorId = data->getScolCollection().getId(i);
                formId = formatFormId(data->getScolCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Txst_:
                editorId = data->getTxstCollection().getId(i);
                formId = formatFormId(data->getTxstCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Wate_:
                editorId = data->getWateCollection().getId(i);
                formId = formatFormId(data->getWateCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Scen_:
                editorId = data->getScenCollection().getId(i);
                formId = formatFormId(data->getScenCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Anio_:
                editorId = data->getAnioCollection().getId(i);
                formId = formatFormId(data->getAnioCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Artv_:
                editorId = data->getArtvCollection().getId(i);
                formId = formatFormId(data->getArtvCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Clfm_:
                editorId = data->getClfmCollection().getId(i);
                formId = formatFormId(data->getClfmCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Debr_:
                editorId = data->getDebrCollection().getId(i);
                formId = formatFormId(data->getDebrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Eczn_:
                editorId = data->getEcznCollection().getId(i);
                formId = formatFormId(data->getEcznCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Hazd_:
                editorId = data->getHazdCollection().getId(i);
                formId = formatFormId(data->getHazdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ipct_:
                editorId = data->getIpctCollection().getId(i);
                formId = formatFormId(data->getIpctCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Must_:
                editorId = data->getMustCollection().getId(i);
                formId = formatFormId(data->getMustCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Rela_:
                editorId = data->getRelaCollection().getId(i);
                formId = formatFormId(data->getRelaCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Revb_:
                editorId = data->getRevbCollection().getId(i);
                formId = formatFormId(data->getRevbCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Shou_:
                editorId = data->getShouCollection().getId(i);
                formId = formatFormId(data->getShouCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Hdpt_:
                editorId = data->getHdptCollection().getId(i);
                formId = formatFormId(data->getHdptCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Term_:
                editorId = data->getTermCollection().getId(i);
                formId = formatFormId(data->getTermCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Matt_:
                editorId = data->getMattCollection().getId(i);
                formId = formatFormId(data->getMattCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Movt_:
                editorId = data->getMovtCollection().getId(i);
                formId = formatFormId(data->getMovtCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Musc_:
                editorId = data->getMuscCollection().getId(i);
                formId = formatFormId(data->getMuscCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Aact_:
                editorId = data->getAactCollection().getId(i);
                formId = formatFormId(data->getAactCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Aamd_:
                editorId = data->getAamdCollection().getId(i);
                formId = formatFormId(data->getAamdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Aapd_:
                editorId = data->getAapdCollection().getId(i);
                formId = formatFormId(data->getAapdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Achr_:
                editorId = data->getAchrCollection().getId(i);
                formId = formatFormId(data->getAchrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Addn_:
                editorId = data->getAddnCollection().getId(i);
                formId = formatFormId(data->getAddnCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Affe_:
                editorId = data->getAffeCollection().getId(i);
                formId = formatFormId(data->getAffeCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ambs_:
                editorId = data->getAmbsCollection().getId(i);
                formId = formatFormId(data->getAmbsCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Amdl_:
                editorId = data->getAmdlCollection().getId(i);
                formId = formatFormId(data->getAmdlCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Aopf_:
                editorId = data->getAopfCollection().getId(i);
                formId = formatFormId(data->getAopfCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Aops_:
                editorId = data->getAopsCollection().getId(i);
                formId = formatFormId(data->getAopsCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Aoru_:
                editorId = data->getAoruCollection().getId(i);
                formId = formatFormId(data->getAoruCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Arma_:
                editorId = data->getArmaCollection().getId(i);
                formId = formatFormId(data->getArmaCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Arto_:
                editorId = data->getArtoCollection().getId(i);
                formId = formatFormId(data->getArtoCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Aspc_:
                editorId = data->getAspcCollection().getId(i);
                formId = formatFormId(data->getAspcCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Atmo_:
                editorId = data->getAtmoCollection().getId(i);
                formId = formatFormId(data->getAtmoCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Avmd_:
                editorId = data->getAvmdCollection().getId(i);
                formId = formatFormId(data->getAvmdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Biom_:
                editorId = data->getBiomCollection().getId(i);
                formId = formatFormId(data->getBiomCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Bmmo_:
                editorId = data->getBmmoCollection().getId(i);
                formId = formatFormId(data->getBmmoCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Bmod_:
                editorId = data->getBmodCollection().getId(i);
                formId = formatFormId(data->getBmodCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Bnds_:
                editorId = data->getBndsCollection().getId(i);
                formId = formatFormId(data->getBndsCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Bptd_:
                editorId = data->getBptdCollection().getId(i);
                formId = formatFormId(data->getBptdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Cams_:
                editorId = data->getCamsCollection().getId(i);
                formId = formatFormId(data->getCamsCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Chal_:
                editorId = data->getChalCollection().getId(i);
                formId = formatFormId(data->getChalCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Cldf_:
                editorId = data->getCldfCollection().getId(i);
                formId = formatFormId(data->getCldfCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Cndf_:
                editorId = data->getCndfCollection().getId(i);
                formId = formatFormId(data->getCndfCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Coll_:
                editorId = data->getCollCollection().getId(i);
                formId = formatFormId(data->getCollCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Cpth_:
                editorId = data->getCpthCollection().getId(i);
                formId = formatFormId(data->getCpthCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Dlbr_:
                editorId = data->getDlbrCollection().getId(i);
                formId = formatFormId(data->getDlbrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Cur3_:
                editorId = data->getCur3Collection().getId(i);
                formId = formatFormId(data->getCur3Collection().getRecord(i).get().formId);
                break;
            case CkId::Type_Curv_:
                editorId = data->getCurvCollection().getId(i);
                formId = formatFormId(data->getCurvCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Dfob_:
                editorId = data->getDfobCollection().getId(i);
                formId = formatFormId(data->getDfobCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Dmgt_:
                editorId = data->getDmgtCollection().getId(i);
                formId = formatFormId(data->getDmgtCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Dobj_:
                editorId = data->getDobjCollection().getId(i);
                formId = formatFormId(data->getDobjCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Efsq_:
                editorId = data->getEfsqCollection().getId(i);
                formId = formatFormId(data->getEfsqCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Equp_:
                editorId = data->getEqupCollection().getId(i);
                formId = formatFormId(data->getEqupCollection().getRecord(i).get().formId);
            case CkId::Type_Ffkw_:
                editorId = data->getFfkwCollection().getId(i);
                formId = formatFormId(data->getFfkwCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Fogv_:
                editorId = data->getFogvCollection().getId(i);
                formId = formatFormId(data->getFogvCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Forc_:
                editorId = data->getForcCollection().getId(i);
                formId = formatFormId(data->getForcCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Fstp_:
                editorId = data->getFstpCollection().getId(i);
                formId = formatFormId(data->getFstpCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Fsts_:
                editorId = data->getFstsCollection().getId(i);
                formId = formatFormId(data->getFstsCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Fxpd_:
                editorId = data->getFxpdCollection().getId(i);
                formId = formatFormId(data->getFxpdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Gbfm_:
                editorId = data->getGbfmCollection().getId(i);
                formId = formatFormId(data->getGbfmCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Gbft_:
                editorId = data->getGbftCollection().getId(i);
                formId = formatFormId(data->getGbftCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Gcvr_:
                editorId = data->getGcvrCollection().getId(i);
                formId = formatFormId(data->getGcvrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Imad_:
                editorId = data->getImadCollection().getId(i);
                formId = formatFormId(data->getImadCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Innr_:
                editorId = data->getInnrCollection().getId(i);
                formId = formatFormId(data->getInnrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ires_:
                editorId = data->getIresCollection().getId(i);
                formId = formatFormId(data->getIresCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Kssm_:
                editorId = data->getKssmCollection().getId(i);
                formId = formatFormId(data->getKssmCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Layr_:
                editorId = data->getLayrCollection().getId(i);
                formId = formatFormId(data->getLayrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lens_:
                editorId = data->getLensCollection().getId(i);
                formId = formatFormId(data->getLensCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lgdi_:
                editorId = data->getLgdiCollection().getId(i);
                formId = formatFormId(data->getLgdiCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lgtm_:
                editorId = data->getLgtmCollection().getId(i);
                formId = formatFormId(data->getLgtmCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lmsw_:
                editorId = data->getLmswCollection().getId(i);
                formId = formatFormId(data->getLmswCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lvlb_:
                editorId = data->getLvlbCollection().getId(i);
                formId = formatFormId(data->getLvlbCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lvln_:
                editorId = data->getLvlnCollection().getId(i);
                formId = formatFormId(data->getLvlnCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lvlp_:
                editorId = data->getLvlpCollection().getId(i);
                formId = formatFormId(data->getLvlpCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Lvsc_:
                editorId = data->getLvscCollection().getId(i);
                formId = formatFormId(data->getLvscCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Maam_:
                editorId = data->getMaamCollection().getId(i);
                formId = formatFormId(data->getMaamCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Mrhp_:
                editorId = data->getMrhpCollection().getId(i);
                formId = formatFormId(data->getMrhpCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Mtpt_:
                editorId = data->getMtptCollection().getId(i);
                formId = formatFormId(data->getMtptCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Navi_:
                editorId = data->getNaviCollection().getId(i);
                formId = formatFormId(data->getNaviCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Nocm_:
                editorId = data->getNocmCollection().getId(i);
                formId = formatFormId(data->getNocmCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Omod_:
                editorId = data->getOmodCollection().getId(i);
                formId = formatFormId(data->getOmodCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Oswp_:
                editorId = data->getOswpCollection().getId(i);
                formId = formatFormId(data->getOswpCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ovis_:
                editorId = data->getOvisCollection().getId(i);
                formId = formatFormId(data->getOvisCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Pcbn_:
                editorId = data->getPcbnCollection().getId(i);
                formId = formatFormId(data->getPcbnCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Pccn_:
                editorId = data->getPccnCollection().getId(i);
                formId = formatFormId(data->getPccnCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Pcmt_:
                editorId = data->getPcmtCollection().getId(i);
                formId = formatFormId(data->getPcmtCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Pdcl_:
                editorId = data->getPdclCollection().getId(i);
                formId = formatFormId(data->getPdclCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Pgre_:
                editorId = data->getPgreCollection().getId(i);
                formId = formatFormId(data->getPgreCollection().getRecord(i).get().formId);
            case CkId::Type_Phzd_:
                editorId = data->getPhzdCollection().getId(i);
                formId = formatFormId(data->getPhzdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Pkin_:
                editorId = data->getPkinCollection().getId(i);
                formId = formatFormId(data->getPkinCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Pmft_:
                editorId = data->getPmftCollection().getId(i);
                formId = formatFormId(data->getPmftCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Psdc_:
                editorId = data->getPsdcCollection().getId(i);
                formId = formatFormId(data->getPsdcCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Ptst_:
                editorId = data->getPtstCollection().getId(i);
                formId = formatFormId(data->getPtstCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Rfgp_:
                editorId = data->getRfgpCollection().getId(i);
                formId = formatFormId(data->getRfgpCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Rsgd_:
                editorId = data->getRsgdCollection().getId(i);
                formId = formatFormId(data->getRsgdCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Rspj_:
                editorId = data->getRspjCollection().getId(i);
                formId = formatFormId(data->getRspjCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Sdlt_:
                editorId = data->getSdltCollection().getId(i);
                formId = formatFormId(data->getSdltCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Sech_:
                editorId = data->getSechCollection().getId(i);
                formId = formatFormId(data->getSechCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Sfbk_:
                editorId = data->getSfbkCollection().getId(i);
                formId = formatFormId(data->getSfbkCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Sfpc_:
                editorId = data->getSfpcCollection().getId(i);
                formId = formatFormId(data->getSfpcCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Sfpt_:
                editorId = data->getSfptCollection().getId(i);
                formId = formatFormId(data->getSfptCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Sftr_:
                editorId = data->getSftrCollection().getId(i);
                formId = formatFormId(data->getSftrCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Smbn_:
                editorId = data->getSmbnCollection().getId(i);
                formId = formatFormId(data->getSmbnCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Smen_:
                editorId = data->getSmenCollection().getId(i);
                formId = formatFormId(data->getSmenCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Spch_:
                editorId = data->getSpchCollection().getId(i);
                formId = formatFormId(data->getSpchCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Stag_:
                editorId = data->getStagCollection().getId(i);
                formId = formatFormId(data->getStagCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Stbh_:
                editorId = data->getStbhCollection().getId(i);
                formId = formatFormId(data->getStbhCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Stdt_:
                editorId = data->getStdtCollection().getId(i);
                formId = formatFormId(data->getStdtCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Stmp_:
                editorId = data->getStmpCollection().getId(i);
                formId = formatFormId(data->getStmpCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Stnd_:
                editorId = data->getStndCollection().getId(i);
                formId = formatFormId(data->getStndCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Sunp_:
                editorId = data->getSunpCollection().getId(i);
                formId = formatFormId(data->getSunpCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Tmlm_:
                editorId = data->getTmlmCollection().getId(i);
                formId = formatFormId(data->getTmlmCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Todd_:
                editorId = data->getToddCollection().getId(i);
                formId = formatFormId(data->getToddCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Trav_:
                editorId = data->getTravCollection().getId(i);
                formId = formatFormId(data->getTravCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Trns_:
                editorId = data->getTrnsCollection().getId(i);
                formId = formatFormId(data->getTrnsCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Voli_:
                editorId = data->getVoliCollection().getId(i);
                formId = formatFormId(data->getVoliCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Vtyp_:
                editorId = data->getVtypCollection().getId(i);
                formId = formatFormId(data->getVtypCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Wbar_:
                editorId = data->getWbarCollection().getId(i);
                formId = formatFormId(data->getWbarCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Wkmf_:
                editorId = data->getWkmfCollection().getId(i);
                formId = formatFormId(data->getWkmfCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Wths_:
                editorId = data->getWthsCollection().getId(i);
                formId = formatFormId(data->getWthsCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Wwed_:
                editorId = data->getWwedCollection().getId(i);
                formId = formatFormId(data->getWwedCollection().getRecord(i).get().formId);
                break;
            case CkId::Type_Zoom_:
                editorId = data->getZoomCollection().getId(i);
                formId = formatFormId(data->getZoomCollection().getRecord(i).get().formId);
                break;
            default:
                break;
            }

            rec.editorId = editorId;
            rec.formId = formId;
            cat.visibleRecords.append(rec);
        }

        mCategories.append(cat);
    };

    addCategory("NPC", CkId::Type_Npc_);

    addCategory("Armor", CkId::Type_Armor_);
    addCategory("Weapon", CkId::Type_Weap_);
    addCategory("Alchemy", CkId::Type_Alch_);
    addCategory("Ingredient", CkId::Type_Ingr_);
    addCategory("Book", CkId::Type_Book_);
    addCategory("Misc", CkId::Type_Misc_);
    addCategory("Container", CkId::Type_Cont_);
    addCategory("Enchantment", CkId::Type_Ench_);
    addCategory("Spell", CkId::Type_Spel_);
    addCategory("Magic Effect", CkId::Type_Magic_);

    addCategory("Static", CkId::Type_Stat_);
    addCategory("Activator", CkId::Type_Acti_);
    addCategory("Tree", CkId::Type_Tree_);

    addCategory("Quest", CkId::Type_Quest_);
    addCategory("Package", CkId::Type_Pack_);
    addCategory("Global", CkId::Type_Glob_);
    addCategory("Game Setting", CkId::Type_Gmst);
    addCategory("Perk", CkId::Type_PerK_);
    addCategory("Class", CkId::Type_Class_);
    addCategory("Faction", CkId::Type_Fact_);
    addCategory("Race", CkId::Type_Race_);

    addCategory("Sound", CkId::Type_Soun_);

    addCategory("Dialogue", CkId::Type_Dial_);
    addCategory("Info", CkId::Type_Info_);

    addCategory("Weather", CkId::Type_Wthr_);
    addCategory("Land Texture", CkId::Type_Ltex_);
    addCategory("Location Reference Type", CkId::Type_Lcrt_);
    addCategory("Cell", CkId::Type_Cel_);
    addCategory("Worldspace", CkId::Type_WRLD_);
    addCategory("Location", CkId::Type_LOCT_);
    addCategory("Planet", CkId::Type_Plnt_);
    addCategory("Reference", CkId::Type_Refr_);
    addCategory("Material", CkId::Type_Material_);
    addCategory("Landscape", CkId::Type_Land_);

    addCategory("Creature", CkId::Type_Crea_);
    addCategory("Leveled Actor", CkId::Type_Lvlc_);
    addCategory("Leveled NPC", CkId::Type_Lvln_);
    addCategory("Actor Values", CkId::Type_Avif_);
    addCategory("Voice Types", CkId::Type_Vtyp_);

    addCategory("Ammo", CkId::Type_Ammo_);
    addCategory("Key", CkId::Type_Keym_);
    addCategory("Soul Gem", CkId::Type_Slgm_);
    addCategory("Scroll", CkId::Type_Scrl_);
    addCategory("Potion", CkId::Type_Alch_);
    addCategory("Leveled Item", CkId::Type_Lvli_);
    addCategory("Constructible Object", CkId::Type_Cobj_);
    addCategory("Outfit", CkId::Type_Otft_);

    addCategory("Movable Static", CkId::Type_Mstt_);
    addCategory("Static Collection", CkId::Type_Scol_);
    addCategory("Door", CkId::Type_Door_);
    addCategory("Furniture", CkId::Type_Furn_);
    addCategory("Flora", CkId::Type_Flor_);
    addCategory("Grass", CkId::Type_Grass_);
    addCategory("Debris", CkId::Type_Debr_);
    addCategory("Hazard", CkId::Type_Hazd_);
    addCategory("Idle Marker", CkId::Type_Idlm_);
    addCategory("Light", CkId::Type_Ligh_);
    addCategory("Acoustic Space", CkId::Type_Aspc_);
    addCategory("Image Space", CkId::Type_Imgs_);

    addCategory("Combat Style", CkId::Type_Csty_);
    addCategory("Encounter Zone", CkId::Type_Eczn_);
    addCategory("Body Part", CkId::Type_Bptd_);
    addCategory("Head Part", CkId::Type_Hdpt_);
    addCategory("Keyword", CkId::Type_Kywd_);
    addCategory("Camera Path", CkId::Type_Cpth_);
    addCategory("Camera Shot", CkId::Type_Cams_);
    addCategory("Impact Data", CkId::Type_Ipct_);
    addCategory("Lens Flare", CkId::Type_Lens_);
    addCategory("Speech Challenge", CkId::Type_Spch_);

    addCategory("Music Type", CkId::Type_Must_);
    addCategory("Music Track", CkId::Type_Musc_);

    addCategory("Action", CkId::Type_Aact_);
    addCategory("Audio Modifier", CkId::Type_Aamd_);
    addCategory("Animated Prop", CkId::Type_Aapd_);
    addCategory("Actor Reference", CkId::Type_Achr_);
    addCategory("Add-On Node", CkId::Type_Addn_);
    addCategory("Affinity Event", CkId::Type_Affe_);
    addCategory("Ambient Sound", CkId::Type_Ambs_);
    addCategory("Audio Model", CkId::Type_Amdl_);
    addCategory("Ambient Occlusion Float", CkId::Type_Aopf_);
    addCategory("Shader Params", CkId::Type_Aops_);
    addCategory("Audio Reverb Unit", CkId::Type_Aoru_);
    addCategory("Armor Addon", CkId::Type_Arma_);
    addCategory("Art Object 2", CkId::Type_Arto_);
    addCategory("Acoustic Space", CkId::Type_Aspc_);
    addCategory("Atmosphere", CkId::Type_Atmo_);
    addCategory("Audio Visual Data", CkId::Type_Avmd_);
    addCategory("Biome", CkId::Type_Biom_);
    addCategory("Behavior", CkId::Type_Bmmo_);
    addCategory("Behavior Mod", CkId::Type_Bmod_);
    addCategory("Bounds", CkId::Type_Bnds_);
    addCategory("Behavior Pattern", CkId::Type_Bptd_);
    addCategory("Camera Shot", CkId::Type_Cams_);
    addCategory("Challenge", CkId::Type_Chal_);
    addCategory("Character Float Table", CkId::Type_Cldf_);
    addCategory("Character Data", CkId::Type_Cndf_);
    addCategory("Collision", CkId::Type_Coll_);
    addCategory("Camera Path", CkId::Type_Cpth_);
    addCategory("Culling Data", CkId::Type_Dlbr_);
    addCategory("Curve Table 3", CkId::Type_Cur3_);
    addCategory("Curve Table", CkId::Type_Curv_);
    addCategory("Debris Object", CkId::Type_Dfob_);
    addCategory("Damage Type", CkId::Type_Dmgt_);
    addCategory("Default Object", CkId::Type_Dobj_);
    addCategory("Effect Shader 2", CkId::Type_Efsq_);
    addCategory("Equipment Slot", CkId::Type_Equp_);

    addCategory("Furnishing Keyword", CkId::Type_Ffkw_);
    addCategory("Fog Volume", CkId::Type_Fogv_);
    addCategory("Force Field", CkId::Type_Forc_);
    addCategory("Footstep Set", CkId::Type_Fstp_);
    addCategory("Footstep Sound", CkId::Type_Fsts_);
    addCategory("FX Particle", CkId::Type_Fxpd_);
    addCategory("Biome Mask", CkId::Type_Gbfm_);
    addCategory("Biome Type", CkId::Type_Gbft_);
    addCategory("Gravity Camera", CkId::Type_Gcvr_);
    addCategory("Image Space Adapter", CkId::Type_Imad_);
    addCategory("Interior Data", CkId::Type_Innr_);
    addCategory("Resource Property", CkId::Type_Ires_);
    addCategory("Keyword Set", CkId::Type_Kssm_);
    addCategory("Layer", CkId::Type_Layr_);
    addCategory("Lens", CkId::Type_Lens_);
    addCategory("Landscape Grid Data", CkId::Type_Lgdi_);
    addCategory("Light Template", CkId::Type_Lgtm_);
    addCategory("Lens Modifier Set", CkId::Type_Lmsw_);
    addCategory("Leveled Biome", CkId::Type_Lvlb_);
    addCategory("Leveled Node", CkId::Type_Lvln_);
    addCategory("Leveled Perk", CkId::Type_Lvlp_);
    addCategory("Leveled Structure", CkId::Type_Lvsc_);
    addCategory("Material Attachment", CkId::Type_Maam_);
    addCategory("Mesh Renderer", CkId::Type_Mrhp_);
    addCategory("Mount Point", CkId::Type_Mtpt_);
    addCategory("Navigation Island", CkId::Type_Navi_);
    addCategory("Navigation Component", CkId::Type_Nocm_);
    addCategory("Object Mod", CkId::Type_Omod_);
    addCategory("Object Swap Set", CkId::Type_Oswp_);
    addCategory("Object Visual", CkId::Type_Ovis_);
    addCategory("Placement Configuration", CkId::Type_Pcbn_);
    addCategory("Placement Collision Node", CkId::Type_Pccn_);
    addCategory("Placement Material", CkId::Type_Pcmt_);
    addCategory("Particle Decal", CkId::Type_Pdcl_);
    addCategory("Particle Emitter", CkId::Type_Pgre_);
    addCategory("Voice Type", CkId::Type_Vtyp_);
    addCategory("Material Type", CkId::Type_Matt_);
    addCategory("Movement Type", CkId::Type_Movt_);

    addCategory("Animated Object", CkId::Type_Anio_);
    addCategory("Color", CkId::Type_Clfm_);
    addCategory("Relationship", CkId::Type_Rela_);
    addCategory("Reverb", CkId::Type_Revb_);
    addCategory("Shout", CkId::Type_Shou_);

    addCategory("Topic", CkId::Type_Dial_);
    addCategory("Scene", CkId::Type_Scen_);
    addCategory("Message", CkId::Type_Mesg_);
    addCategory("Note", CkId::Type_Note_);
    addCategory("Terminal", CkId::Type_Term_);

    addCategory("Navmesh", CkId::Type_Navm_);
    addCategory("Climate", CkId::Type_Clmt_);

    addCategory("Effect Shader", CkId::Type_Efsh_);
    addCategory("Art Object", CkId::Type_Artv_);
    addCategory("Water Shader", CkId::Type_Wate_);
    addCategory("Weather Shader", CkId::Type_Wths_);
    addCategory("Default Object", CkId::Type_Dobj_);
    addCategory("Biome", CkId::Type_Biom_);
    addCategory("Snap Template", CkId::Type_Stmp_);
    addCategory("Apparatus", CkId::Type_Appa_);
    addCategory("Birthsign", CkId::Type_Bsgn_);
    addCategory("Clothing", CkId::Type_Clot_);
    addCategory("Explosion", CkId::Type_Expl_);
    addCategory("Eyes", CkId::Type_Eyes_);
    addCategory("Form List", CkId::Type_Flst_);
    addCategory("Hair", CkId::Type_Hair_);
    addCategory("Idle Animation", CkId::Type_Idle_);
    addCategory("Leveled Spell", CkId::Type_Lvsp_);
    addCategory("Load Screen", CkId::Type_Lscr_);
    addCategory("Projectile", CkId::Type_Proj_);
    addCategory("Region", CkId::Type_Regn_);
    addCategory("Road", CkId::Type_Road_);
    addCategory("Script", CkId::Type_Scpt_);
    addCategory("Sound Marker", CkId::Type_Smqn_);
    addCategory("Texture Set", CkId::Type_Txst_);
    addCategory("PHZD", CkId::Type_Phzd_);
    addCategory("PKIN", CkId::Type_Pkin_);
    addCategory("PMFT", CkId::Type_Pmft_);
    addCategory("PSDC", CkId::Type_Psdc_);
    addCategory("PTST", CkId::Type_Ptst_);
    addCategory("RFGP", CkId::Type_Rfgp_);
    addCategory("RSGD", CkId::Type_Rsgd_);
    addCategory("RSPJ", CkId::Type_Rspj_);
    addCategory("SDLT", CkId::Type_Sdlt_);
    addCategory("SECH", CkId::Type_Sech_);
    addCategory("SFBK", CkId::Type_Sfbk_);
    addCategory("SFPC", CkId::Type_Sfpc_);
    addCategory("SFPT", CkId::Type_Sfpt_);
    addCategory("SFTR", CkId::Type_Sftr_);
    addCategory("SMBN", CkId::Type_Smbn_);
    addCategory("SMEN", CkId::Type_Smen_);
    addCategory("SPCH", CkId::Type_Spch_);
    addCategory("STAG", CkId::Type_Stag_);
    addCategory("STBH", CkId::Type_Stbh_);
    addCategory("STDT", CkId::Type_Stdt_);
    addCategory("STMP", CkId::Type_Stmp_);
    addCategory("STND", CkId::Type_Stnd_);
    addCategory("SUNP", CkId::Type_Sunp_);
    addCategory("TMLM", CkId::Type_Tmlm_);
    addCategory("TODD", CkId::Type_Todd_);
    addCategory("TRAV", CkId::Type_Trav_);
    addCategory("TRNS", CkId::Type_Trns_);
    addCategory("VOLI", CkId::Type_Voli_);
    addCategory("VTYP", CkId::Type_Vtyp_);
    addCategory("WBAR", CkId::Type_Wbar_);
    addCategory("WKMF", CkId::Type_Wkmf_);
    addCategory("WTHS", CkId::Type_Wths_);
    addCategory("WWED", CkId::Type_Wwed_);
    addCategory("ZOOM", CkId::Type_Zoom_);


    auto addGroupNamed = [this](const QString& name, std::initializer_list<QString> catNames) {
        CategoryGroup group;
        group.name = name;
        for (const QString& n : catNames)
        {
            for (int i = 0; i < mCategories.size(); i++)
            {
                if (mCategories[i].name == n)
                {
                    group.categoryIndices.append(i);
                    break;
                }
            }
        }
        mGroups.append(group);
    };

    // All Forms: flat list of every category
    {
        CategoryGroup allForms;
        allForms.name = "All Forms";
        for (int i = 0; i < mCategories.size(); i++)
            allForms.categoryIndices.append(i);
        mGroups.append(allForms);
    }

    addGroupNamed("Actors", {"NPC", "Creature", "Leveled Actor", "Leveled NPC", "Actor Values", "Voice Types",
                                "Eyes", "Hair", "Idle Animation"});
    addGroupNamed("Items", {"Armor", "Weapon", "Alchemy", "Ingredient", "Book", "Misc", "Container",
                            "Enchantment", "Spell", "Magic Effect", "Ammo", "Key", "Soul Gem", "Scroll",
                            "Potion", "Leveled Item", "Constructible Object", "Outfit",
                            "Apparatus", "Clothing", "Form List", "Leveled Spell"});
    addGroupNamed("World Objects", {"Static", "Activator", "Tree", "Movable Static", "Static Collection",
                                    "Door", "Furniture", "Flora", "Grass", "Debris", "Hazard", "Idle Marker",
                                    "Light", "Acoustic Space", "Image Space", "Explosion", "Projectile",
                                    "Texture Set"});
    addGroupNamed("Gameplay", {"Quest", "Package", "Global", "Game Setting", "Perk", "Class", "Faction",
                               "Race", "Combat Style", "Encounter Zone", "Body Part", "Head Part", "Location",
                               "Keyword", "Camera Path", "Camera Shot", "Impact Data", "Lens Flare",
                               "Speech Challenge", "Birthsign", "Relationship", "Shout", "Movement Type"});
    addGroupNamed("Audio", {"Sound", "Music Type", "Music Track", "Voice Type", "Sound Marker", "Reverb"});
    addGroupNamed("Dialogue", {"Dialogue", "Info", "Topic", "Scene", "Message", "Note", "Terminal"});
    addGroupNamed("World", {"Cell", "Worldspace", "Navmesh", "Landscape", "Reference", "Weather",
                            "Land Texture", "Climate", "Region", "Road"});
    addGroupNamed("Miscellaneous", {"Location Reference Type", "Effect Shader", "Art Object", "Water Shader",
                                    "Weather Shader", "Power", "Default Object", "Association Type",
                                    "Biome", "Snap Template", "Material", "Material Type", "Load Screen", "Script",
                                    "Animated Object", "Color"});
}

QString ObjectWindowModel::formatFormId(quint32 formId) const
{
    return QString("0x%1").arg(formId, 8, 16, QChar('0'));
}

void ObjectWindowModel::applyFilter(const QString& text)
{
    mFilter = text;
    QString lowerFilter = text.toLower();

    for (auto& cat : mCategories)
    {
        if (lowerFilter.isEmpty())
        {
            rebuildAllRecords();
        }
        else
        {
            QVector<VisibleRecord> filtered;
            for (const auto& rec : cat.visibleRecords)
            {
                if (rec.editorId.toLower().contains(lowerFilter) ||
                    rec.formId.toLower().contains(lowerFilter))
                {
                    filtered.append(rec);
                }
            }
            cat.visibleRecords = filtered;
        }
    }

    emit layoutChanged();
}

void ObjectWindowModel::applyObjectFilter(const ObjectWindowFilter& filter)
{
    mActiveObjectFilter = filter;
    rebuildAllRecords();
    for (auto& cat : mCategories)
    {
        QVector<VisibleRecord> filtered;
        for (const auto& rec : cat.visibleRecords)
        {
            QJsonObject record;
            record.insert(QStringLiteral("EditorID"), rec.editorId);
            record.insert(QStringLiteral("FormID"), rec.formId);
            record.insert(QStringLiteral("Type"), cat.name);
            if (filter.matches(record))
                filtered.append(rec);
        }
        cat.visibleRecords = filtered;
    }
    emit layoutChanged();
}

void ObjectWindowModel::rebuildAllRecords()
{
    for (auto& cat : mCategories)
    {
        cat.visibleRecords.clear();
        for (int i = 0; i < cat.totalRecords; i++)
        {
            VisibleRecord rec;
            rec.actualIndex = i;
            rec.editorId = "";
            rec.formId = "";

            switch (static_cast<CkId::Type>(cat.typeId))
            {
                case CkId::Type_Gmst:
                    rec.editorId = mData->getGameSettings().getId(i);
                    break;
                case CkId::Type_Npc_:
                    rec.editorId = mData->getNpcCollection().getId(i);
                    rec.formId = formatFormId(mData->getNpcCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Weap_:
                    rec.editorId = mData->getWeaponCollection().getId(i);
                    rec.formId = formatFormId(mData->getWeaponCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Armor_:
                    rec.editorId = mData->getArmorCollection().getId(i);
                    rec.formId = formatFormId(mData->getArmorCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Spel_:
                    rec.editorId = mData->getSpellCollection().getId(i);
                    rec.formId = formatFormId(mData->getSpellCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Magic_:
                    rec.editorId = mData->getMagicCollection().getId(i);
                    rec.formId = formatFormId(mData->getMagicCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Quest_:
                    rec.editorId = mData->getQuestCollection().getId(i);
                    rec.formId = formatFormId(mData->getQuestCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Dial_:
                    rec.editorId = mData->getDialCollection().getId(i);
                    rec.formId = formatFormId(mData->getDialCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Info_:
                    rec.editorId = mData->getInfoCollection().getId(i);
                    rec.formId = formatFormId(mData->getInfoCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Glob_:
                    rec.editorId = mData->getGlobCollection().getId(i);
                    rec.formId = QString();
                    break;
                case CkId::Type_Lcrt_:
                    rec.editorId = mData->getLcrtCollection().getId(i);
                    rec.formId = QString();
                    break;
                case CkId::Type_Pack_:
                    rec.editorId = mData->getPackCollection().getId(i);
                    rec.formId = formatFormId(mData->getPackCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Tree_:
                    rec.editorId = mData->getTreeCollection().getId(i);
                    rec.formId = formatFormId(mData->getTreeCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Alch_:
                    rec.editorId = mData->getAlchCollection().getId(i);
                    rec.formId = formatFormId(mData->getAlchCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ingr_:
                    rec.editorId = mData->getIngrCollection().getId(i);
                    rec.formId = formatFormId(mData->getIngrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Cont_:
                    rec.editorId = mData->getContCollection().getId(i);
                    rec.formId = formatFormId(mData->getContCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ench_:
                    rec.editorId = mData->getEnchCollection().getId(i);
                    rec.formId = formatFormId(mData->getEnchCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Book_:
                    rec.editorId = mData->getBookCollection().getId(i);
                    rec.formId = formatFormId(mData->getBookCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Misc_:
                    rec.editorId = mData->getMiscCollection().getId(i);
                    rec.formId = formatFormId(mData->getMiscCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Acti_:
                    rec.editorId = mData->getActiCollection().getId(i);
                    rec.formId = formatFormId(mData->getActiCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Stat_:
                    rec.editorId = mData->getStatCollection().getId(i);
                    rec.formId = formatFormId(mData->getStatCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Race_:
                    rec.editorId = mData->getRaceCollection().getId(i);
                    rec.formId = formatFormId(mData->getRaceCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Class_:
                    rec.editorId = mData->getClassCollection().getId(i);
                    rec.formId = formatFormId(mData->getClassCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Fact_:
                    rec.editorId = mData->getFactCollection().getId(i);
                    rec.formId = formatFormId(mData->getFactCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_PerK_:
                    rec.editorId = mData->getPerkCollection().getId(i);
                    rec.formId = formatFormId(mData->getPerkCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Soun_:
                    rec.editorId = mData->getSounCollection().getId(i);
                    rec.formId = formatFormId(mData->getSounCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Wthr_:
                    rec.editorId = mData->getWthrCollection().getId(i);
                    rec.formId = formatFormId(mData->getWthrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ltex_:
                    rec.editorId = mData->getLtexCollection().getId(i);
                    rec.formId = formatFormId(mData->getLtexCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Cel_:
                    rec.editorId = mData->getCellCollection().getId(i);
                    rec.formId = formatFormId(mData->getCellCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_WRLD_:
                    rec.editorId = mData->getWorldspaceCollection().getId(i);
                    rec.formId = formatFormId(mData->getWorldspaceCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_LOCT_:
                    rec.editorId = mData->getLocationCollection().getId(i);
                    rec.formId = formatFormId(mData->getLocationCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Refr_:
                    rec.editorId = mData->getRefrCollection().getId(i);
                    rec.formId = formatFormId(mData->getRefrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Material_:
                    rec.editorId = mData->getMaterialCollection().getId(i);
                    rec.formId = formatFormId(mData->getMaterialCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Land_:
                    rec.editorId = mData->getLandCollection().getId(i);
                    rec.formId = formatFormId(mData->getLandCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Hdpt_:
                    rec.editorId = mData->getHdptCollection().getId(i);
                    rec.formId = formatFormId(mData->getHdptCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Term_:
                    rec.editorId = mData->getTermCollection().getId(i);
                    rec.formId = formatFormId(mData->getTermCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Matt_:
                    rec.editorId = mData->getMattCollection().getId(i);
                    rec.formId = formatFormId(mData->getMattCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Movt_:
                    rec.editorId = mData->getMovtCollection().getId(i);
                    rec.formId = formatFormId(mData->getMovtCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Musc_:
                    rec.editorId = mData->getMuscCollection().getId(i);
                    rec.formId = formatFormId(mData->getMuscCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Aact_:
                    rec.editorId = mData->getAactCollection().getId(i);
                    rec.formId = formatFormId(mData->getAactCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Aamd_:
                    rec.editorId = mData->getAamdCollection().getId(i);
                    rec.formId = formatFormId(mData->getAamdCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Aapd_:
                    rec.editorId = mData->getAapdCollection().getId(i);
                    rec.formId = formatFormId(mData->getAapdCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Achr_:
                    rec.editorId = mData->getAchrCollection().getId(i);
                    rec.formId = formatFormId(mData->getAchrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Addn_:
                    rec.editorId = mData->getAddnCollection().getId(i);
                    rec.formId = formatFormId(mData->getAddnCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Affe_:
                    rec.editorId = mData->getAffeCollection().getId(i);
                    rec.formId = formatFormId(mData->getAffeCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ambs_:
                    rec.editorId = mData->getAmbsCollection().getId(i);
                    rec.formId = formatFormId(mData->getAmbsCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Amdl_:
                    rec.editorId = mData->getAmdlCollection().getId(i);
                    rec.formId = formatFormId(mData->getAmdlCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Aopf_:
                    rec.editorId = mData->getAopfCollection().getId(i);
                    rec.formId = formatFormId(mData->getAopfCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Aops_:
                    rec.editorId = mData->getAopsCollection().getId(i);
                    rec.formId = formatFormId(mData->getAopsCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Aoru_:
                    rec.editorId = mData->getAoruCollection().getId(i);
                    rec.formId = formatFormId(mData->getAoruCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Arma_:
                    rec.editorId = mData->getArmaCollection().getId(i);
                    rec.formId = formatFormId(mData->getArmaCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Arto_:
                    rec.editorId = mData->getArtoCollection().getId(i);
                    rec.formId = formatFormId(mData->getArtoCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Aspc_:
                    rec.editorId = mData->getAspcCollection().getId(i);
                    rec.formId = formatFormId(mData->getAspcCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Atmo_:
                    rec.editorId = mData->getAtmoCollection().getId(i);
                    rec.formId = formatFormId(mData->getAtmoCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Avmd_:
                    rec.editorId = mData->getAvmdCollection().getId(i);
                    rec.formId = formatFormId(mData->getAvmdCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Biom_:
                    rec.editorId = mData->getBiomCollection().getId(i);
                    rec.formId = formatFormId(mData->getBiomCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Bmmo_:
                    rec.editorId = mData->getBmmoCollection().getId(i);
                    rec.formId = formatFormId(mData->getBmmoCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Bmod_:
                    rec.editorId = mData->getBmodCollection().getId(i);
                    rec.formId = formatFormId(mData->getBmodCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Bnds_:
                    rec.editorId = mData->getBndsCollection().getId(i);
                    rec.formId = formatFormId(mData->getBndsCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Bptd_:
                    rec.editorId = mData->getBptdCollection().getId(i);
                    rec.formId = formatFormId(mData->getBptdCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Cams_:
                    rec.editorId = mData->getCamsCollection().getId(i);
                    rec.formId = formatFormId(mData->getCamsCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Chal_:
                    rec.editorId = mData->getChalCollection().getId(i);
                    rec.formId = formatFormId(mData->getChalCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Cldf_:
                    rec.editorId = mData->getCldfCollection().getId(i);
                    rec.formId = formatFormId(mData->getCldfCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Cndf_:
                    rec.editorId = mData->getCndfCollection().getId(i);
                    rec.formId = formatFormId(mData->getCndfCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Coll_:
                    rec.editorId = mData->getCollCollection().getId(i);
                    rec.formId = formatFormId(mData->getCollCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Cpth_:
                    rec.editorId = mData->getCpthCollection().getId(i);
                    rec.formId = formatFormId(mData->getCpthCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Dlbr_:
                    rec.editorId = mData->getDlbrCollection().getId(i);
                    rec.formId = formatFormId(mData->getDlbrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Cur3_:
                    rec.editorId = mData->getCur3Collection().getId(i);
                    rec.formId = formatFormId(mData->getCur3Collection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Curv_:
                    rec.editorId = mData->getCurvCollection().getId(i);
                    rec.formId = formatFormId(mData->getCurvCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Dfob_:
                    rec.editorId = mData->getDfobCollection().getId(i);
                    rec.formId = formatFormId(mData->getDfobCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Dmgt_:
                    rec.editorId = mData->getDmgtCollection().getId(i);
                    rec.formId = formatFormId(mData->getDmgtCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Dobj_:
                    rec.editorId = mData->getDobjCollection().getId(i);
                    rec.formId = formatFormId(mData->getDobjCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Efsq_:
                    rec.editorId = mData->getEfsqCollection().getId(i);
                    rec.formId = formatFormId(mData->getEfsqCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Equp_:
                    rec.editorId = mData->getEqupCollection().getId(i);
                    rec.formId = formatFormId(mData->getEqupCollection().getRecord(i).get().formId);
                case CkId::Type_Ffkw_:
                    rec.editorId = mData->getFfkwCollection().getId(i);
                    rec.formId = formatFormId(mData->getFfkwCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Fogv_:
                    rec.editorId = mData->getFogvCollection().getId(i);
                    rec.formId = formatFormId(mData->getFogvCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Forc_:
                    rec.editorId = mData->getForcCollection().getId(i);
                    rec.formId = formatFormId(mData->getForcCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Fstp_:
                    rec.editorId = mData->getFstpCollection().getId(i);
                    rec.formId = formatFormId(mData->getFstpCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Fsts_:
                    rec.editorId = mData->getFstsCollection().getId(i);
                    rec.formId = formatFormId(mData->getFstsCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Fxpd_:
                    rec.editorId = mData->getFxpdCollection().getId(i);
                    rec.formId = formatFormId(mData->getFxpdCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Gbfm_:
                    rec.editorId = mData->getGbfmCollection().getId(i);
                    rec.formId = formatFormId(mData->getGbfmCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Gbft_:
                    rec.editorId = mData->getGbftCollection().getId(i);
                    rec.formId = formatFormId(mData->getGbftCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Gcvr_:
                    rec.editorId = mData->getGcvrCollection().getId(i);
                    rec.formId = formatFormId(mData->getGcvrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Imad_:
                    rec.editorId = mData->getImadCollection().getId(i);
                    rec.formId = formatFormId(mData->getImadCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Innr_:
                    rec.editorId = mData->getInnrCollection().getId(i);
                    rec.formId = formatFormId(mData->getInnrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ires_:
                    rec.editorId = mData->getIresCollection().getId(i);
                    rec.formId = formatFormId(mData->getIresCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Kssm_:
                    rec.editorId = mData->getKssmCollection().getId(i);
                    rec.formId = formatFormId(mData->getKssmCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Layr_:
                    rec.editorId = mData->getLayrCollection().getId(i);
                    rec.formId = formatFormId(mData->getLayrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Lens_:
                    rec.editorId = mData->getLensCollection().getId(i);
                    rec.formId = formatFormId(mData->getLensCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Lgdi_:
                    rec.editorId = mData->getLgdiCollection().getId(i);
                    rec.formId = formatFormId(mData->getLgdiCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Lgtm_:
                    rec.editorId = mData->getLgtmCollection().getId(i);
                    rec.formId = formatFormId(mData->getLgtmCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Lmsw_:
                    rec.editorId = mData->getLmswCollection().getId(i);
                    rec.formId = formatFormId(mData->getLmswCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Lvlb_:
                    rec.editorId = mData->getLvlbCollection().getId(i);
                    rec.formId = formatFormId(mData->getLvlbCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Lvln_:
                    rec.editorId = mData->getLvlnCollection().getId(i);
                    rec.formId = formatFormId(mData->getLvlnCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Lvlp_:
                    rec.editorId = mData->getLvlpCollection().getId(i);
                    rec.formId = formatFormId(mData->getLvlpCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Lvsc_:
                    rec.editorId = mData->getLvscCollection().getId(i);
                    rec.formId = formatFormId(mData->getLvscCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Maam_:
                    rec.editorId = mData->getMaamCollection().getId(i);
                    rec.formId = formatFormId(mData->getMaamCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Mrhp_:
                    rec.editorId = mData->getMrhpCollection().getId(i);
                    rec.formId = formatFormId(mData->getMrhpCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Mtpt_:
                    rec.editorId = mData->getMtptCollection().getId(i);
                    rec.formId = formatFormId(mData->getMtptCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Navi_:
                    rec.editorId = mData->getNaviCollection().getId(i);
                    rec.formId = formatFormId(mData->getNaviCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Nocm_:
                    rec.editorId = mData->getNocmCollection().getId(i);
                    rec.formId = formatFormId(mData->getNocmCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Omod_:
                    rec.editorId = mData->getOmodCollection().getId(i);
                    rec.formId = formatFormId(mData->getOmodCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Oswp_:
                    rec.editorId = mData->getOswpCollection().getId(i);
                    rec.formId = formatFormId(mData->getOswpCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ovis_:
                    rec.editorId = mData->getOvisCollection().getId(i);
                    rec.formId = formatFormId(mData->getOvisCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Pcbn_:
                    rec.editorId = mData->getPcbnCollection().getId(i);
                    rec.formId = formatFormId(mData->getPcbnCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Pccn_:
                    rec.editorId = mData->getPccnCollection().getId(i);
                    rec.formId = formatFormId(mData->getPccnCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Pcmt_:
                    rec.editorId = mData->getPcmtCollection().getId(i);
                    rec.formId = formatFormId(mData->getPcmtCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Pdcl_:
                    rec.editorId = mData->getPdclCollection().getId(i);
                    rec.formId = formatFormId(mData->getPdclCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Pgre_:
                    rec.editorId = mData->getPgreCollection().getId(i);
                    rec.formId = formatFormId(mData->getPgreCollection().getRecord(i).get().formId);
                case CkId::Type_Phzd_:
                    rec.editorId = mData->getPhzdCollection().getId(i);
                    rec.formId = formatFormId(mData->getPhzdCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Pkin_:
                    rec.editorId = mData->getPkinCollection().getId(i);
                    rec.formId = formatFormId(mData->getPkinCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Pmft_:
                    rec.editorId = mData->getPmftCollection().getId(i);
                    rec.formId = formatFormId(mData->getPmftCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Psdc_:
                    rec.editorId = mData->getPsdcCollection().getId(i);
                    rec.formId = formatFormId(mData->getPsdcCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Ptst_:
                    rec.editorId = mData->getPtstCollection().getId(i);
                    rec.formId = formatFormId(mData->getPtstCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Rfgp_:
                    rec.editorId = mData->getRfgpCollection().getId(i);
                    rec.formId = formatFormId(mData->getRfgpCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Rsgd_:
                    rec.editorId = mData->getRsgdCollection().getId(i);
                    rec.formId = formatFormId(mData->getRsgdCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Rspj_:
                    rec.editorId = mData->getRspjCollection().getId(i);
                    rec.formId = formatFormId(mData->getRspjCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Sdlt_:
                    rec.editorId = mData->getSdltCollection().getId(i);
                    rec.formId = formatFormId(mData->getSdltCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Sech_:
                    rec.editorId = mData->getSechCollection().getId(i);
                    rec.formId = formatFormId(mData->getSechCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Sfbk_:
                    rec.editorId = mData->getSfbkCollection().getId(i);
                    rec.formId = formatFormId(mData->getSfbkCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Sfpc_:
                    rec.editorId = mData->getSfpcCollection().getId(i);
                    rec.formId = formatFormId(mData->getSfpcCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Sfpt_:
                    rec.editorId = mData->getSfptCollection().getId(i);
                    rec.formId = formatFormId(mData->getSfptCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Sftr_:
                    rec.editorId = mData->getSftrCollection().getId(i);
                    rec.formId = formatFormId(mData->getSftrCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Smbn_:
                    rec.editorId = mData->getSmbnCollection().getId(i);
                    rec.formId = formatFormId(mData->getSmbnCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Smen_:
                    rec.editorId = mData->getSmenCollection().getId(i);
                    rec.formId = formatFormId(mData->getSmenCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Spch_:
                    rec.editorId = mData->getSpchCollection().getId(i);
                    rec.formId = formatFormId(mData->getSpchCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Stag_:
                    rec.editorId = mData->getStagCollection().getId(i);
                    rec.formId = formatFormId(mData->getStagCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Stbh_:
                    rec.editorId = mData->getStbhCollection().getId(i);
                    rec.formId = formatFormId(mData->getStbhCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Stdt_:
                    rec.editorId = mData->getStdtCollection().getId(i);
                    rec.formId = formatFormId(mData->getStdtCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Stmp_:
                    rec.editorId = mData->getStmpCollection().getId(i);
                    rec.formId = formatFormId(mData->getStmpCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Stnd_:
                    rec.editorId = mData->getStndCollection().getId(i);
                    rec.formId = formatFormId(mData->getStndCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Sunp_:
                    rec.editorId = mData->getSunpCollection().getId(i);
                    rec.formId = formatFormId(mData->getSunpCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Tmlm_:
                    rec.editorId = mData->getTmlmCollection().getId(i);
                    rec.formId = formatFormId(mData->getTmlmCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Todd_:
                    rec.editorId = mData->getToddCollection().getId(i);
                    rec.formId = formatFormId(mData->getToddCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Trav_:
                    rec.editorId = mData->getTravCollection().getId(i);
                    rec.formId = formatFormId(mData->getTravCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Trns_:
                    rec.editorId = mData->getTrnsCollection().getId(i);
                    rec.formId = formatFormId(mData->getTrnsCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Voli_:
                    rec.editorId = mData->getVoliCollection().getId(i);
                    rec.formId = formatFormId(mData->getVoliCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Vtyp_:
                    rec.editorId = mData->getVtypCollection().getId(i);
                    rec.formId = formatFormId(mData->getVtypCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Wbar_:
                    rec.editorId = mData->getWbarCollection().getId(i);
                    rec.formId = formatFormId(mData->getWbarCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Wkmf_:
                    rec.editorId = mData->getWkmfCollection().getId(i);
                    rec.formId = formatFormId(mData->getWkmfCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Wths_:
                    rec.editorId = mData->getWthsCollection().getId(i);
                    rec.formId = formatFormId(mData->getWthsCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Wwed_:
                    rec.editorId = mData->getWwedCollection().getId(i);
                    rec.formId = formatFormId(mData->getWwedCollection().getRecord(i).get().formId);
                    break;
                case CkId::Type_Zoom_:
                    rec.editorId = mData->getZoomCollection().getId(i);
                    rec.formId = formatFormId(mData->getZoomCollection().getRecord(i).get().formId);
                    break;
                default:
                    break;
                }

                cat.visibleRecords.append(rec);
            }
        }
}

QModelIndex ObjectWindowModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    if (!parent.isValid())
    {
        if (row < 0 || row >= mGroups.size())
            return QModelIndex();
        return createIndex(row, column, kGroupInternalId);
    }

    quintptr parentInternal = parent.internalId();
    if (parentInternal == kGroupInternalId)
    {
        int groupRow = parent.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QModelIndex();
        const auto& group = mGroups[groupRow];
        if (row < 0 || row >= group.categoryIndices.size())
            return QModelIndex();
        return createIndex(row, column, static_cast<quintptr>((groupRow << 16) | row));
    }

    if (!(parentInternal & kRecordBit))
    {
        int groupRow = static_cast<int>((parentInternal >> 16) & 0xFFFF);
        int categoryRow = parent.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QModelIndex();
        const auto& group = mGroups[groupRow];
        if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
            return QModelIndex();
        int flatId = group.categoryIndices[categoryRow];
        if (row < 0 || row >= mCategories[flatId].visibleRecords.size())
            return QModelIndex();
        return createIndex(row, column, kRecordBit | static_cast<quintptr>((groupRow << 16) | categoryRow));
    }

    return QModelIndex();
}

QModelIndex ObjectWindowModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return QModelIndex();

    quintptr internal = child.internalId();
    if (internal == kGroupInternalId)
        return QModelIndex();

    if (!(internal & kRecordBit))
    {
        int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QModelIndex();
        return createIndex(groupRow, 0, kGroupInternalId);
    }

    int groupRow = static_cast<int>(((internal & ~kRecordBit) >> 16) & 0xFFFF);
    int categoryRow = static_cast<int>((internal & ~kRecordBit) & 0xFFFF);
    if (groupRow < 0 || groupRow >= mGroups.size())
        return QModelIndex();
    return createIndex(categoryRow, 0, static_cast<quintptr>((groupRow << 16) | categoryRow));
}

int ObjectWindowModel::rowCount(const QModelIndex& parent) const
{
    if (!mData)
        return 0;

    if (!parent.isValid())
        return mGroups.size();

    quintptr internal = parent.internalId();
    if (internal == kGroupInternalId)
    {
        int groupRow = parent.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return 0;
        return mGroups[groupRow].categoryIndices.size();
    }

    if (!(internal & kRecordBit))
    {
        int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
        int categoryRow = parent.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return 0;
        const auto& group = mGroups[groupRow];
        if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
            return 0;
        int flatId = group.categoryIndices[categoryRow];
        return mCategories[flatId].visibleRecords.size();
    }

    return 0;
}

int ObjectWindowModel::columnCount(const QModelIndex& /*parent*/) const
{
    return 3;
}

bool ObjectWindowModel::canFetchMore(const QModelIndex& parent) const
{
    if (!mData || !parent.isValid())
        return false;

    // Only category nodes fetch (deferred master types); group nodes
    // (internalId 0) and record nodes never do.
    quintptr internal = parent.internalId();
    if (internal == kGroupInternalId || (internal & kRecordBit))
        return false;

    int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
    int categoryRow = parent.row();
    if (groupRow < 0 || groupRow >= mGroups.size())
        return false;
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return false;
    return mCategories[group.categoryIndices[categoryRow]].pendingMaterialize;
}

void ObjectWindowModel::appendMaterializedRows(int groupRow, int categoryRow, int firstNew, const QModelIndex& parent)
{
    if (!mData || !parent.isValid())
        return;
    if (groupRow < 0 || groupRow >= mGroups.size())
        return;
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return;
    Category& cat = mCategories[group.categoryIndices[categoryRow]];

    BaseCollection* coll = mData->getCollectionByType(static_cast<CkId::Type>(cat.typeId));
    if (!coll)
        return;

    const int end = coll->size();
    if (firstNew >= end)
        return;

    QVector<VisibleRecord> added;
    for (int i = firstNew; i < end; ++i)
    {
        VisibleRecord rec;
        rec.actualIndex = i;
        rec.editorId = coll->getId(i);
        rec.formId = formatFormId(coll->getFormId(i));
        added.append(rec);
    }

    // Keep inserted rows consistent with any active filters.
    const QString lowerFilter = mFilter.toLower();
    if (!lowerFilter.isEmpty())
    {
        QVector<VisibleRecord> filtered;
        for (const auto& rec : added)
        {
            if (rec.editorId.toLower().contains(lowerFilter)
                || rec.formId.toLower().contains(lowerFilter))
                filtered.append(rec);
        }
        added = filtered;
    }
    if (mActiveObjectFilter.count() > 0)
    {
        QVector<VisibleRecord> filtered;
        for (const auto& rec : added)
        {
            QJsonObject record;
            record.insert(QStringLiteral("EditorID"), rec.editorId);
            record.insert(QStringLiteral("FormID"), rec.formId);
            record.insert(QStringLiteral("Type"), cat.name);
            if (mActiveObjectFilter.matches(record))
                filtered.append(rec);
        }
        added = filtered;
    }

    if (added.isEmpty())
        return;

    beginInsertRows(parent, firstNew, firstNew + added.size() - 1);
    cat.visibleRecords.append(added);
    endInsertRows();
}

void ObjectWindowModel::startMaterializeTimer()
{
    mMaterializeTimer.stop();
    mMaterializeTimer.start(20);
}

void ObjectWindowModel::materializeTick()
{
    if (!mData || !mJob.active)
    {
        mMaterializeTimer.stop();
        return;
    }

    const int typeId = mJob.typeId;
    const int firstNew = mJob.firstNew;
    mData->materializeNextBatch(typeId, 256);

    const QModelIndex groupNode = index(mJob.groupRow, 0, QModelIndex());
    const QModelIndex parentIdx = index(mJob.categoryRow, 0, groupNode);
    appendMaterializedRows(mJob.groupRow, mJob.categoryRow, firstNew, parentIdx);
    mJob.firstNew = mCategories[mJob.flatId].visibleRecords.size();

    if (!mData->isMaterializing())
    {
        mMaterializeTimer.stop();
        Category& cat = mCategories[mJob.flatId];
        cat.pendingMaterialize = false;
        mJob.active = false;
        if (parentIdx.isValid())
            emit dataChanged(parentIdx, parentIdx);
    }
}

void ObjectWindowModel::fetchMore(const QModelIndex& parent)
{
    if (!mData || !parent.isValid())
        return;

    quintptr internal = parent.internalId();
    if (internal == kGroupInternalId || (internal & kRecordBit))
        return;

    int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
    int categoryRow = parent.row();
    if (groupRow < 0 || groupRow >= mGroups.size())
        return;
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return;

    Category& cat = mCategories[group.categoryIndices[categoryRow]];
    if (!cat.pendingMaterialize)
        return;

    // Small types (or types already fully parsed) load synchronously;
    // anything with deferred master records is materialized in 20 ms
    // slices so expanding a huge category never freezes the UI. Row
    // insertion is still done with beginInsertRows/endInsertRows on the
    // UI thread (a full model reset inside fetchMore crashes QTreeView).
    if (mData->isMaterializing())
        return;
    if (!mData->beginTypeMaterialization(cat.typeId))
    {
        cat.pendingMaterialize = false;
        return;
    }

    mJob.typeId = cat.typeId;
    mJob.groupRow = groupRow;
    mJob.categoryRow = categoryRow;
    mJob.flatId = group.categoryIndices[categoryRow];
    mJob.firstNew = cat.visibleRecords.size();
    mJob.active = true;

    mData->materializeNextBatch(mJob.typeId, 256);
    appendMaterializedRows(mJob.groupRow, mJob.categoryRow, mJob.firstNew, parent);
    mJob.firstNew = mCategories[mJob.flatId].visibleRecords.size();

    if (mData->isMaterializing())
    {
        startMaterializeTimer();
    }
    else
    {
        cat.pendingMaterialize = false;
        mJob.active = false;
    }
}

QVariant ObjectWindowModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    quintptr internal = index.internalId();

    if (internal == kGroupInternalId)
    {
        int groupRow = index.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QVariant();
        switch (index.column())
        {
        case 0:
            return mGroups[groupRow].name;
        default:
            return QString();
        }
    }

    if (!(internal & kRecordBit))
    {
        int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
        int categoryRow = index.row();
        if (groupRow < 0 || groupRow >= mGroups.size())
            return QVariant();
        const auto& group = mGroups[groupRow];
        if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
            return QVariant();
        int flatId = group.categoryIndices[categoryRow];
        const auto& cat = mCategories[flatId];
        switch (index.column())
        {
        case 0:
        {
            QString label = cat.name;
            if (cat.pendingMaterialize)
            {
                label += QStringLiteral(" (%1 deferred)")
                    .arg(cat.totalRecords - cat.parsedCount);
            }
            return label;
        }
        case 1:
            return QString();
        case 2:
        {
            CkId::Type t = static_cast<CkId::Type>(cat.typeId);
            return (t == CkId::Type_None) ? QStringLiteral("-") : CkId(t).getTypeName();
        }
        }
        return QVariant();
    }

    int groupRow = static_cast<int>(((internal & ~kRecordBit) >> 16) & 0xFFFF);
    int categoryRow = static_cast<int>((internal & ~kRecordBit) & 0xFFFF);
    if (groupRow < 0 || groupRow >= mGroups.size())
        return QVariant();
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return QVariant();
    int flatId = group.categoryIndices[categoryRow];
    const auto& visibleRecords = mCategories[flatId].visibleRecords;
    int localRow = index.row();
    if (localRow < 0 || localRow >= visibleRecords.size())
        return QVariant();
    const auto& rec = visibleRecords[localRow];
    switch (index.column())
    {
    case 0:
        return rec.editorId;
    case 1:
        return rec.formId;
    case 2:
    {
        CkId::Type t = static_cast<CkId::Type>(mCategories[flatId].typeId);
        return (t == CkId::Type_None) ? QStringLiteral("-") : CkId(t).getTypeName();
    }
    }
    return QVariant();
}

Qt::ItemFlags ObjectWindowModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemFlag::ItemIsDropEnabled;

    if (isRecordNode(index))
        return QAbstractItemModel::flags(index) | Qt::ItemFlag::ItemIsSelectable;

    return QAbstractItemModel::flags(index) | Qt::ItemFlag::ItemIsEnabled;
}

int ObjectWindowModel::flatCategoryId(int groupRow, int categoryRow) const
{
    if (groupRow < 0 || groupRow >= mGroups.size())
        return -1;
    const auto& group = mGroups[groupRow];
    if (categoryRow < 0 || categoryRow >= group.categoryIndices.size())
        return -1;
    return group.categoryIndices[categoryRow];
}

bool ObjectWindowModel::findCategoryLocation(int flatId, int& groupRow, int& categoryRow) const
{
    for (int g = 0; g < mGroups.size(); g++)
    {
        const auto& group = mGroups[g];
        for (int c = 0; c < group.categoryIndices.size(); c++)
        {
            if (group.categoryIndices[c] == flatId)
            {
                groupRow = g;
                categoryRow = c;
                return true;
            }
        }
    }
    return false;
}

bool ObjectWindowModel::isGroupNode(const QModelIndex& index) const
{
    return index.isValid() && index.internalId() == kGroupInternalId;
}

bool ObjectWindowModel::isCategoryNode(const QModelIndex& index) const
{
    quintptr internal = index.internalId();
    return index.isValid() && internal != kGroupInternalId && !(internal & kRecordBit);
}

bool ObjectWindowModel::isRecordNode(const QModelIndex& index) const
{
    return index.isValid() && (index.internalId() & kRecordBit) != 0;
}

bool ObjectWindowModel::isRecord(const QModelIndex& index) const
{
    return isRecordNode(index);
}

int ObjectWindowModel::getCategoryIndex(const QModelIndex& index) const
{
    if (!index.isValid())
        return -1;

    quintptr internal = index.internalId();
    if (internal == kGroupInternalId)
        return -1;

    if (!(internal & kRecordBit))
    {
        int groupRow = static_cast<int>((internal >> 16) & 0xFFFF);
        int categoryRow = index.row();
        return flatCategoryId(groupRow, categoryRow);
    }

    int groupRow = static_cast<int>(((internal & ~kRecordBit) >> 16) & 0xFFFF);
    int categoryRow = static_cast<int>((internal & ~kRecordBit) & 0xFFFF);
    return flatCategoryId(groupRow, categoryRow);
}

int ObjectWindowModel::getCategoryType(int categoryId) const
{
    if (categoryId < 0 || categoryId >= mCategories.size())
        return static_cast<int>(CkId::Type_None);
    return mCategories[categoryId].typeId;
}

int ObjectWindowModel::getRecordIndex(const QModelIndex& index) const
{
    if (!isRecordNode(index))
        return -1;

    quintptr internal = index.internalId();
    int groupRow = static_cast<int>(((internal & ~kRecordBit) >> 16) & 0xFFFF);
    int categoryRow = static_cast<int>((internal & ~kRecordBit) & 0xFFFF);
    int flatId = flatCategoryId(groupRow, categoryRow);
    if (flatId < 0)
        return -1;

    int localRow = index.row();
    const auto& visibleRecords = mCategories[flatId].visibleRecords;
    if (localRow < 0 || localRow >= visibleRecords.size())
        return -1;

    return visibleRecords[localRow].actualIndex;
}

QModelIndex ObjectWindowModel::getCategoryIndexModel(int categoryId) const
{
    int groupRow = 0;
    int categoryRow = 0;
    if (!findCategoryLocation(categoryId, groupRow, categoryRow))
        return QModelIndex();
    return createIndex(categoryRow, 0, static_cast<quintptr>((groupRow << 16) | categoryRow));
}

QModelIndex ObjectWindowModel::getRecordIndexModel(int categoryId, int recordIndex) const
{
    int groupRow = 0;
    int categoryRow = 0;
    if (!findCategoryLocation(categoryId, groupRow, categoryRow))
        return QModelIndex();

    const auto& visibleRecords = mCategories[categoryId].visibleRecords;
    for (int i = 0; i < visibleRecords.size(); i++)
    {
        if (visibleRecords[i].actualIndex == recordIndex)
        {
            return createIndex(i, 0, kRecordBit | static_cast<quintptr>((groupRow << 16) | categoryRow));
        }
    }
    return QModelIndex();
}

const QString& ObjectWindowModel::getRecordEditorId(int categoryId, int recordIndex) const
{
    static const QString empty;
    if (categoryId < 0 || categoryId >= mCategories.size())
        return empty;

    const auto& visibleRecords = mCategories[categoryId].visibleRecords;
    for (const auto& rec : visibleRecords)
    {
        if (rec.actualIndex == recordIndex)
            return rec.editorId;
    }
    return visibleRecords.isEmpty() ? empty : visibleRecords[0].editorId;
}

const QString& ObjectWindowModel::getRecordFormId(int categoryId, int recordIndex) const
{
    static const QString empty;
    if (categoryId < 0 || categoryId >= mCategories.size())
        return empty;

    const auto& visibleRecords = mCategories[categoryId].visibleRecords;
    for (const auto& rec : visibleRecords)
    {
        if (rec.actualIndex == recordIndex)
            return rec.formId;
    }
    return visibleRecords.isEmpty() ? empty : visibleRecords[0].formId;
}
