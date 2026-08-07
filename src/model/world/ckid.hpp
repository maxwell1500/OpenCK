#ifndef CKID_H
#define CKID_H

#include <QString>

class CkId
{
public:
    enum ArgumentType
    {
        ArgumentType_None,
        ArgumentType_Id,
        ArgumentType_Index
    };

    enum Type
    {
        Type_None = 0,
        Type_Gmst,
        Type_LoadingLog,
        Type_RunLog,

        Type_Npc_,
        Type_Weap_,
        Type_Armor_,
        Type_Spel_,
        Type_Magic_,
        Type_Quest_,
        Type_Dial_,
        Type_Info_,
        Type_Glob_,
        Type_Lcrt_,
        Type_Pack_,
        Type_Tree_,
        Type_Alch_,
        Type_Ingr_,
        Type_Cont_,
        Type_Ench_,
        Type_Book_,
        Type_Misc_,
        Type_Acti_,
        Type_Stat_,
        Type_Race_,
        Type_Class_,
        Type_Fact_,
        Type_PerK_,
        Type_Cel_,
        Type_WRLD_,
        Type_LOCT_,
        Type_Plnt_,
        Type_Refr_,
    Type_Material_,
    Type_Land_,
    Type_Soun_,
    Type_Wthr_,
    Type_Ltex_,

    Type_Ammo_,
    Type_Appa_,
    Type_Avif_,
    Type_Bsgn_,
    Type_Clmt_,
    Type_Clot_,
    Type_Cobj_,
    Type_Crea_,
    Type_Csty_,
    Type_Door_,
    Type_Efsh_,
    Type_Expl_,
    Type_Eyes_,
    Type_Flor_,
    Type_Flst_,
    Type_Furn_,
    Type_Grass_,
    Type_Hair_,
    Type_Idle_,
    Type_Idlm_,
    Type_Imgs_,
    Type_Keym_,
    Type_Kywd_,
    Type_Ligh_,
    Type_Lscr_,
    Type_Lvlc_,
    Type_Lvli_,
    Type_Lvsp_,
    Type_Mesg_,
    Type_Mstt_,
    Type_Navm_,
    Type_Note_,
    Type_Otft_,
    Type_Proj_,
    Type_Regn_,
    Type_Road_,
    Type_Scpt_,
    Type_Scrl_,
    Type_Slgm_,
    Type_Smqn_,
    Type_Spgd_,
    Type_Scol_,
    Type_Scen_,
    Type_Txst_,
    Type_Wate_,
    Type_Anio_,
    Type_Artv_,
    Type_Clfm_,
    Type_Debr_,
    Type_Eczn_,
    Type_Hazd_,
    Type_Ipct_,
    Type_Ipds_,
    Type_Must_,
    Type_Rela_,
    Type_Revb_,
    Type_Shou_,
    Type_Ffkw_,
    Type_Fogv_,
    Type_Forc_,
    Type_Fstp_,
    Type_Fsts_,
    Type_Fxpd_,
    Type_Gbfm_,
    Type_Gbft_,
    Type_Gcvr_,
    Type_Hdpt_,
    Type_Imad_,
    Type_Innr_,
    Type_Ires_,
    Type_Kssm_,
    Type_Layr_,
    Type_Lens_,
    Type_Lgdi_,
    Type_Lgtm_,
    Type_Lmsw_,
    Type_Lvlb_,
    Type_Lvln_,
    Type_Lvlp_,
    Type_Lvsc_,
    Type_Maam_,
    Type_Matt_,
    Type_Movt_,
    Type_Mrhp_,
    Type_Mtpt_,
    Type_Musc_,
    Type_Aact_,
    Type_Aamd_,
    Type_Aapd_,
    Type_Achr_,
    Type_Addn_,
    Type_Affe_,
    Type_Ambs_,
    Type_Amdl_,
    Type_Aopf_,
    Type_Aops_,
    Type_Aoru_,
    Type_Arma_,
    Type_Arto_,
    Type_Aspc_,
    Type_Atmo_,
    Type_Avmd_,
    Type_Biom_,
    Type_Bmmo_,
    Type_Bmod_,
    Type_Bnds_,
    Type_Bptd_,
    Type_Cams_,
    Type_Chal_,
    Type_Cldf_,
    Type_Cndf_,
    Type_Coll_,
    Type_Cpth_,
    Type_Dlbr_,
    Type_Cur3_,
    Type_Curv_,
    Type_Dfob_,
    Type_Dmgt_,
    Type_Dobj_,
    Type_Efsq_,
    Type_Equp_,
    Type_Navi_,
    Type_Nocm_,
    Type_Omod_,
    Type_Oswp_,
    Type_Ovis_,
    Type_Pcbn_,
    Type_Pccn_,
    Type_Pcmt_,
    Type_Pdcl_,
    Type_Pgre_,
    Type_Term_,
    Type_Phzd_,
    Type_Pkin_,
    Type_Pmft_,
    Type_Psdc_,
    Type_Ptst_,
    Type_Rfgp_,
    Type_Rsgd_,
    Type_Rspj_,
    Type_Sdlt_,
    Type_Sech_,
    Type_Sfbk_,
    Type_Sfpc_,
    Type_Sfpt_,
    Type_Sftr_,
    Type_Smbn_,
    Type_Smen_,
    Type_Spch_,
    Type_Stag_,
    Type_Stbh_,
    Type_Stdt_,
    Type_Stmp_,
    Type_Stnd_,
    Type_Sunp_,
    Type_Tmlm_,
    Type_Todd_,
    Type_Trav_,
    Type_Trns_,
    Type_Voli_,
    Type_Vtyp_,
    Type_Wbar_,
    Type_Wkmf_,
    Type_Wths_,
    Type_Wwed_,
    Type_Zoom_,

    NumTypes
};

    enum { Number = Type_RunLog + 1 };

    CkId(const QString& ckid);
    CkId(Type type = Type_None);
    CkId(Type type, const QString& ckid);
    CkId(Type type, int index);

    ArgumentType getArgumentType() const;
    Type getType() const;
    const QString& getId() const;
    int getIndex() const;
    QString getTypeName() const;
    QString toString() const;


    bool equalTo(const CkId& ckid) const;
    bool lessThan(const CkId& ckid) const;
    
    static Type stringToType(const QString& typeName);

private:
    ArgumentType argumentType;
    Type type;
    QString id;
    int index;
};

bool operator== (const CkId& left, const CkId& right);
bool operator!= (const CkId& left, const CkId& right);
bool operator< (const CkId& left, const CkId& right);

// Static helper for getting type name from enum value
inline QString typeName(CkId::Type t) { return QString("Unknown Type %1").arg(t); }

#endif // CKID_H