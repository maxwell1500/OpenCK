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