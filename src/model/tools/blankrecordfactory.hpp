#ifndef BLANKRECORDFACTORY_H
#define BLANKRECORDFACTORY_H

#include "../world/ckid.hpp"
#include "../world/record.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/gmst.hpp"
#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/racerecord.hpp"
#include "../../../libs/files/esm/classrecord.hpp"
#include "../../../libs/files/esm/factrecord.hpp"

#include <memory>
#include <type_traits>

class BlankRecordFactory
{
public:
    static bool supports(CkId::Type type)
    {
        switch (type)
        {
        case CkId::Type_Glob_:
        case CkId::Type_Gmst:
        case CkId::Type_Npc_:
        case CkId::Type_Race_:
        case CkId::Type_Class_:
        case CkId::Type_Fact_:
            return true;
        default:
            return false;
        }
    }

    static std::unique_ptr<BaseRecord> create(CkId::Type type,
        const QString& editorId, quint32 formId)
    {
        switch (type)
        {
        case CkId::Type_Glob_: return make<GlobalVariable>(editorId, formId);
        case CkId::Type_Gmst: return make<GameSetting>(editorId, formId);
        case CkId::Type_Npc_: return make<NpcRecord>(editorId, formId);
        case CkId::Type_Race_: return make<RaceRecord>(editorId, formId);
        case CkId::Type_Class_: return make<ClassRecord>(editorId, formId);
        case CkId::Type_Fact_: return make<FactRecord>(editorId, formId);
        default: return nullptr;
        }
    }

private:
    template<typename T, typename = void>
    struct HasInitComponents : std::false_type {};

    template<typename T>
    struct HasInitComponents<T, std::void_t<decltype(std::declval<T&>().initComponents())>>
        : std::true_type {};

    template<typename T>
    static std::unique_ptr<BaseRecord> make(const QString& editorId, quint32 formId)
    {
        T record;
        record.blank();
        record.editorId = editorId;
        record.formId = formId;
        if constexpr (HasInitComponents<T>::value)
            record.initComponents();
        return std::make_unique<Record<T>>(State_ModifiedOnly, nullptr, &record);
    }
};

#endif // BLANKRECORDFACTORY_H
