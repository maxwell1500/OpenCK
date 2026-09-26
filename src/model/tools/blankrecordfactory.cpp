#include "blankrecordfactory.hpp"

#include "componentrecordtypes.hpp"

#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/gmst.hpp"

#include "../world/basecollection.hpp"
#include "../world/collection.hpp"
#include "../world/record.hpp"

#include "../../../libs/files/esm/actirecord.hpp"
#include "../../../libs/files/esm/actorvalueinforecord.hpp"
#include "../../../libs/files/esm/alchrecord.hpp"
#include "../../../libs/files/esm/ammorecord.hpp"
#include "../../../libs/files/esm/aniorecord.hpp"
#include "../../../libs/files/esm/apparatusrecord.hpp"
#include "../../../libs/files/esm/artvrecord.hpp"
#include "../../../libs/files/esm/armorrecord.hpp"
#include "../../../libs/files/esm/birthsignrecord.hpp"
#include "../../../libs/files/esm/bookrecord.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/classrecord.hpp"
#include "../../../libs/files/esm/clfmrecord.hpp"
#include "../../../libs/files/esm/climaterecord.hpp"
#include "../../../libs/files/esm/clothrecord.hpp"
#include "../../../libs/files/esm/constructibleobjectrecord.hpp"
#include "../../../libs/files/esm/combatstylerecord.hpp"
#include "../../../libs/files/esm/contrecord.hpp"
#include "../../../libs/files/esm/creaturerecord.hpp"
#include "../../../libs/files/esm/debrrecord.hpp"
#include "../../../libs/files/esm/dialrecord.hpp"
#include "../../../libs/files/esm/doorrecord.hpp"
#include "../../../libs/files/esm/effectshaderrecord.hpp"
#include "../../../libs/files/esm/ecznrecord.hpp"
#include "../../../libs/files/esm/enchrecord.hpp"
#include "../../../libs/files/esm/explosionrecord.hpp"
#include "../../../libs/files/esm/eyesrecord.hpp"
#include "../../../libs/files/esm/factrecord.hpp"
#include "../../../libs/files/esm/florrecord.hpp"
#include "../../../libs/files/esm/formlistrecord.hpp"
#include "../../../libs/files/esm/furnrecord.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/grassrecord.hpp"
#include "../../../libs/files/esm/hairrecord.hpp"
#include "../../../libs/files/esm/hazdrecord.hpp"
#include "../../../libs/files/esm/hdptrecord.hpp"
#include "../../../libs/files/esm/idleanimationrecord.hpp"
#include "../../../libs/files/esm/idlemarkerrecord.hpp"
#include "../../../libs/files/esm/imagespacerecord.hpp"
#include "../../../libs/files/esm/inforecord.hpp"
#include "../../../libs/files/esm/ingrrecord.hpp"
#include "../../../libs/files/esm/ipctrecord.hpp"
#include "../../../libs/files/esm/ipdsrecord.hpp"
#include "../../../libs/files/esm/keymrecord.hpp"
#include "../../../libs/files/esm/keywordrecord.hpp"
#include "../../../libs/files/esm/landrecord.hpp"
#include "../../../libs/files/esm/lighrecord.hpp"
#include "../../../libs/files/esm/loadscreenrecord.hpp"
#include "../../../libs/files/esm/locationrecord.hpp"
#include "../../../libs/files/esm/ltexrecord.hpp"
#include "../../../libs/files/esm/lvlcreaturerecord.hpp"
#include "../../../libs/files/esm/lvlistrecord.hpp"
#include "../../../libs/files/esm/lvspellrecord.hpp"
#include "../../../libs/files/esm/magicrecord.hpp"
#include "../../../libs/files/esm/materialrecord.hpp"
#include "../../../libs/files/esm/mattrecord.hpp"
#include "../../../libs/files/esm/movtrecord.hpp"
#include "../../../libs/files/esm/muscrecord.hpp"
#include "../../../libs/files/esm/ffkwrecord.hpp"
#include "../../../libs/files/esm/fogvrecord.hpp"
#include "../../../libs/files/esm/forcrecord.hpp"
#include "../../../libs/files/esm/fstprecord.hpp"
#include "../../../libs/files/esm/fstsrecord.hpp"
#include "../../../libs/files/esm/fxpdrecord.hpp"
#include "../../../libs/files/esm/gbfmrecord.hpp"
#include "../../../libs/files/esm/gbftrecord.hpp"
#include "../../../libs/files/esm/gcvrrecord.hpp"
#include "../../../libs/files/esm/imadrecord.hpp"
#include "../../../libs/files/esm/innrrecord.hpp"
#include "../../../libs/files/esm/iresrecord.hpp"
#include "../../../libs/files/esm/kssmrecord.hpp"
#include "../../../libs/files/esm/layrrecord.hpp"
#include "../../../libs/files/esm/lensrecord.hpp"
#include "../../../libs/files/esm/lgdirecord.hpp"
#include "../../../libs/files/esm/lgtmrecord.hpp"
#include "../../../libs/files/esm/lmswrecord.hpp"
#include "../../../libs/files/esm/lvlbrecord.hpp"
#include "../../../libs/files/esm/lvlnrecord.hpp"
#include "../../../libs/files/esm/lvlprecord.hpp"
#include "../../../libs/files/esm/lvscrecord.hpp"
#include "../../../libs/files/esm/maamrecord.hpp"
#include "../../../libs/files/esm/mrhprecord.hpp"
#include "../../../libs/files/esm/mtptrecord.hpp"
#include "../../../libs/files/esm/navirecord.hpp"
#include "../../../libs/files/esm/nocmrecord.hpp"
#include "../../../libs/files/esm/omodrecord.hpp"
#include "../../../libs/files/esm/oswprecord.hpp"
#include "../../../libs/files/esm/ovisrecord.hpp"
#include "../../../libs/files/esm/pcbnrecord.hpp"
#include "../../../libs/files/esm/pccnrecord.hpp"
#include "../../../libs/files/esm/pcmtrecord.hpp"
#include "../../../libs/files/esm/pdclrecord.hpp"
#include "../../../libs/files/esm/pgrerecord.hpp"
#include "../../../libs/files/esm/messagerecord.hpp"
#include "../../../libs/files/esm/miscrecord.hpp"
#include "../../../libs/files/esm/msttrecord.hpp"
#include "../../../libs/files/esm/mustrecord.hpp"
#include "../../../libs/files/esm/navmrecord.hpp"
#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/noterecord.hpp"
#include "../../../libs/files/esm/outfitrecord.hpp"
#include "../../../libs/files/esm/packagerecord.hpp"
#include "../../../libs/files/esm/perkrecord.hpp"
#include "../../../libs/files/esm/pndrecord.hpp"
#include "../../../libs/files/esm/projectilerecord.hpp"
#include "../../../libs/files/esm/questrecord.hpp"
#include "../../../libs/files/esm/racerecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"
#include "../../../libs/files/esm/regionrecord.hpp"
#include "../../../libs/files/esm/relarecord.hpp"
#include "../../../libs/files/esm/revbrecord.hpp"
#include "../../../libs/files/esm/roadrecord.hpp"
#include "../../../libs/files/esm/scenrecord.hpp"
#include "../../../libs/files/esm/scriptrecord.hpp"
#include "../../../libs/files/esm/scrollrecord.hpp"
#include "../../../libs/files/esm/shaderparticlerecord.hpp"
#include "../../../libs/files/esm/shourecord.hpp"
#include "../../../libs/files/esm/slgmrecord.hpp"
#include "../../../libs/files/esm/sounrecord.hpp"
#include "../../../libs/files/esm/soundmarkerrecord.hpp"
#include "../../../libs/files/esm/spellrecord.hpp"
#include "../../../libs/files/esm/staticcollectionrecord.hpp"
#include "../../../libs/files/esm/statrecord.hpp"
#include "../../../libs/files/esm/Tes3record.hpp"
#include "../../../libs/files/esm/termrecord.hpp"
#include "../../../libs/files/esm/texturesetrecord.hpp"
#include "../../../libs/files/esm/treerecord.hpp"
#include "../../../libs/files/esm/waterecord.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "../../../libs/files/esm/wthrrecord.hpp"
#include "../../../libs/files/esm/aactrecord.hpp"
#include "../../../libs/files/esm/aamdrecord.hpp"
#include "../../../libs/files/esm/aapdrecord.hpp"
#include "../../../libs/files/esm/achrrecord.hpp"
#include "../../../libs/files/esm/addnrecord.hpp"
#include "../../../libs/files/esm/afferecord.hpp"
#include "../../../libs/files/esm/ambsrecord.hpp"
#include "../../../libs/files/esm/amdlrecord.hpp"
#include "../../../libs/files/esm/aopfrecord.hpp"
#include "../../../libs/files/esm/aopsrecord.hpp"
#include "../../../libs/files/esm/aorurecord.hpp"
#include "../../../libs/files/esm/armarecord.hpp"
#include "../../../libs/files/esm/artorecord.hpp"
#include "../../../libs/files/esm/aspcrecord.hpp"
#include "../../../libs/files/esm/atmrecord.hpp"
#include "../../../libs/files/esm/avmdrecord.hpp"
#include "../../../libs/files/esm/biomrecord.hpp"
#include "../../../libs/files/esm/bmmorecord.hpp"
#include "../../../libs/files/esm/bmodrecord.hpp"
#include "../../../libs/files/esm/bndsrecord.hpp"
#include "../../../libs/files/esm/bptdrecord.hpp"
#include "../../../libs/files/esm/camsrecord.hpp"
#include "../../../libs/files/esm/chalrecord.hpp"
#include "../../../libs/files/esm/cldfrecord.hpp"
#include "../../../libs/files/esm/cndfrecord.hpp"
#include "../../../libs/files/esm/collrecord.hpp"
#include "../../../libs/files/esm/cpthrecord.hpp"
#include "../../../libs/files/esm/dlbrrecord.hpp"
#include "../../../libs/files/esm/cur3record.hpp"
#include "../../../libs/files/esm/curvrecord.hpp"
#include "../../../libs/files/esm/dfobrecord.hpp"
#include "../../../libs/files/esm/dmgtrecord.hpp"
#include "../../../libs/files/esm/dobjrecord.hpp"
#include "../../../libs/files/esm/efsqrecord.hpp"
#include "../../../libs/files/esm/equprecord.hpp"
#include "../../../libs/files/esm/phzdrecord.hpp"
#include "../../../libs/files/esm/pkinrecord.hpp"
#include "../../../libs/files/esm/pmftrecord.hpp"
#include "../../../libs/files/esm/psdcrecord.hpp"
#include "../../../libs/files/esm/ptstrecord.hpp"
#include "../../../libs/files/esm/rfgprecord.hpp"
#include "../../../libs/files/esm/rsgdrecord.hpp"
#include "../../../libs/files/esm/rspjrecord.hpp"
#include "../../../libs/files/esm/sdltrecord.hpp"
#include "../../../libs/files/esm/sechrecord.hpp"
#include "../../../libs/files/esm/sfbkrecord.hpp"
#include "../../../libs/files/esm/sfpcrecord.hpp"
#include "../../../libs/files/esm/sfptrecord.hpp"
#include "../../../libs/files/esm/sftrrecord.hpp"
#include "../../../libs/files/esm/smbnrecord.hpp"
#include "../../../libs/files/esm/smenrecord.hpp"
#include "../../../libs/files/esm/spchrecord.hpp"
#include "../../../libs/files/esm/stagrecord.hpp"
#include "../../../libs/files/esm/stbhrecord.hpp"
#include "../../../libs/files/esm/stdtrecord.hpp"
#include "../../../libs/files/esm/stmprecord.hpp"
#include "../../../libs/files/esm/stndrecord.hpp"
#include "../../../libs/files/esm/sunprecord.hpp"
#include "../../../libs/files/esm/tmlmrecord.hpp"
#include "../../../libs/files/esm/toddrecord.hpp"
#include "../../../libs/files/esm/travrecord.hpp"
#include "../../../libs/files/esm/trnsrecord.hpp"
#include "../../../libs/files/esm/volirecord.hpp"
#include "../../../libs/files/esm/vtyprecord.hpp"
#include "../../../libs/files/esm/wbarrecord.hpp"
#include "../../../libs/files/esm/wkmfrecord.hpp"
#include "../../../libs/files/esm/wthsrecord.hpp"
#include "../../../libs/files/esm/wwedrecord.hpp"
#include "../../../libs/files/esm/zoomrecord.hpp"
namespace {

/// A record is creatable when it can be blanked and carries the two identity
/// fields every record shares. Types failing any of these are skipped at compile
/// time rather than breaking the build.
template <typename T, typename = void>
struct IsBlankCreatable : std::false_type {};

template <typename T>
struct IsBlankCreatable<T, std::void_t<
    decltype(std::declval<T&>().blank()),
    decltype(std::declval<T&>().editorId = QString()),
    decltype(std::declval<T&>().formId = quint32(0))>> : std::true_type {};

/// Global variables and game settings sit outside the component architecture,
/// so initComponents() is optional rather than required.
template <typename T, typename = void>
struct HasInitComponents : std::false_type {};

template <typename T>
struct HasInitComponents<T, std::void_t<
    decltype(std::declval<T&>().initComponents())>> : std::true_type {};

template <typename T>
std::unique_ptr<BaseRecord> makeIfMatches(BaseCollection* collection,
    const QString& editorId, quint32 formId)
{
    if constexpr (!IsBlankCreatable<T>::value)
    {
        Q_UNUSED(collection); Q_UNUSED(editorId); Q_UNUSED(formId);
        return nullptr;
    }
    else
    {
        if (!dynamic_cast<Collection<T>*>(collection))
            return nullptr;
        T record;
        record.blank();
        record.editorId = editorId;
        record.formId = formId;
        if constexpr (HasInitComponents<T>::value)
            record.initComponents();
        return std::make_unique<Record<T>>(State_ModifiedOnly, nullptr, &record);
    }
}

} // namespace

bool BlankRecordFactory::supports(BaseCollection* collection)
{
    if (!collection)
        return false;
    // GlobalVariable and GameSetting are not in the component record list.
    if (dynamic_cast<Collection<GlobalVariable>*>(collection)) return true;
    if (dynamic_cast<Collection<GameSetting>*>(collection)) return true;
#define FACTORY_SUPPORTS(recType) \
    if (makeIfMatches<recType>(collection, QString(), 0) != nullptr) return true;
    FOR_EACH_COMPONENT_RECORD_TYPE(FACTORY_SUPPORTS)
#undef FACTORY_SUPPORTS
    return false;
}

std::unique_ptr<BaseRecord> BlankRecordFactory::create(BaseCollection* collection,
    const QString& editorId, quint32 formId)
{
    if (!collection)
        return nullptr;
#define FACTORY_MAKE(recType) \
    if (auto made = makeIfMatches<recType>(collection, editorId, formId)) return made;
    FOR_EACH_COMPONENT_RECORD_TYPE(FACTORY_MAKE)
#undef FACTORY_MAKE
    // GlobalVariable and GameSetting are not in the component record list, so
    // they are tried separately after the shared list.
    if (auto made = makeIfMatches<GlobalVariable>(collection, editorId, formId)) return made;
    if (auto made = makeIfMatches<GameSetting>(collection, editorId, formId)) return made;
    return nullptr;
}
