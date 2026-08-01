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
    Type_Txst_,
    Type_Wate_,

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