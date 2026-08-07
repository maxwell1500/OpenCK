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
}

void ObjectWindowModel::setData(Data* data)
{
    beginResetModel();

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

        for (int i = 0; i < cat.totalRecords; i++)
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
    addCategory("Leveled NPC", CkId::Type_None);
    addCategory("Actor Values", CkId::Type_Avif_);
    addCategory("Voice Types", CkId::Type_None);

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
    addCategory("Acoustic Space", CkId::Type_None);
    addCategory("Image Space", CkId::Type_Imgs_);

    addCategory("Combat Style", CkId::Type_Csty_);
    addCategory("Encounter Zone", CkId::Type_Eczn_);
    addCategory("Body Part", CkId::Type_None);
    addCategory("Head Part", CkId::Type_Hdpt_);
    addCategory("Keyword", CkId::Type_Kywd_);
    addCategory("Camera Path", CkId::Type_None);
    addCategory("Camera Shot", CkId::Type_None);
    addCategory("Impact Data", CkId::Type_Ipct_);
    addCategory("Lens Flare", CkId::Type_None);
    addCategory("Speech Challenge", CkId::Type_None);

    addCategory("Music Type", CkId::Type_Must_);
    addCategory("Music Track", CkId::Type_Musc_);
    addCategory("Voice Type", CkId::Type_None);
    addCategory("Material Type", CkId::Type_Matt_);
    addCategory("Movement Type", CkId::Type_Movt_);

    addCategory("Animated Object", CkId::Type_Anio_);
    addCategory("Color", CkId::Type_Clfm_);
    addCategory("Relationship", CkId::Type_Rela_);
    addCategory("Reverb", CkId::Type_Revb_);
    addCategory("Shout", CkId::Type_Shou_);

    addCategory("Topic", CkId::Type_None);
    addCategory("Scene", CkId::Type_Scen_);
    addCategory("Message", CkId::Type_Mesg_);
    addCategory("Note", CkId::Type_Note_);
    addCategory("Terminal", CkId::Type_Term_);

    addCategory("Navmesh", CkId::Type_Navm_);
    addCategory("Climate", CkId::Type_Clmt_);

    addCategory("Effect Shader", CkId::Type_Efsh_);
    addCategory("Art Object", CkId::Type_Artv_);
    addCategory("Water Shader", CkId::Type_Wate_);
    addCategory("Weather Shader", CkId::Type_None);
    addCategory("Power", CkId::Type_None);
    addCategory("Default Object", CkId::Type_None);
    addCategory("Association Type", CkId::Type_None);
    addCategory("Biome", CkId::Type_None);
    addCategory("Snap Template", CkId::Type_None);
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
            return cat.name;
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
