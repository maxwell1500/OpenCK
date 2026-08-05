#include "formcomponentsresolver.hpp"

#include "../../../libs/components/formcomponents.hpp"

#include "../world/basecollection.hpp"
#include "../world/collection.hpp"

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
#include "../../../libs/files/esm/texturesetrecord.hpp"
#include "../../../libs/files/esm/treerecord.hpp"
#include "../../../libs/files/esm/waterecord.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "../../../libs/files/esm/wthrrecord.hpp"

namespace {
template <typename T>
bool tryResolveComponents(BaseCollection* coll, int recordIndex,
                          openck::FormComponents*& components, void*& recordPtr)
{
    auto* typed = dynamic_cast<Collection<T>*>(coll);
    if (!typed)
        return false;
    if (recordIndex < 0 || recordIndex >= typed->size())
        return false;
    auto& record = typed->getRecord(recordIndex).get();
    components = &record.components;
    recordPtr = &record;
    return true;
}

#define FOR_EACH_COMPONENT_RECORD_TYPE(MACRO) \
    MACRO(ActiRecord) \
    MACRO(ActorValueInfoRecord) \
    MACRO(AlchRecord) \
    MACRO(AmmoRecord) \
    MACRO(AnioRecord) \
    MACRO(AppaRecord) \
    MACRO(ArmorRecord) \
    MACRO(ArtvRecord) \
    MACRO(BookRecord) \
    MACRO(BsgnRecord) \
    MACRO(CellRecord) \
    MACRO(ClassRecord) \
    MACRO(ClfmRecord) \
    MACRO(ClimateRecord) \
    MACRO(ClotRecord) \
    MACRO(CobjRecord) \
    MACRO(ContRecord) \
    MACRO(CreatureRecord) \
    MACRO(CstyRecord) \
    MACRO(DebrRecord) \
    MACRO(DialRecord) \
    MACRO(DoorRecord) \
    MACRO(EcznRecord) \
    MACRO(EfshRecord) \
    MACRO(EnchRecord) \
    MACRO(ExplRecord) \
    MACRO(EyesRecord) \
    MACRO(FactRecord) \
    MACRO(FlorRecord) \
    MACRO(FormListRecord) \
    MACRO(FurnRecord) \
    MACRO(GrassRecord) \
    MACRO(HairRecord) \
    MACRO(HazdRecord) \
    MACRO(IdleAnimationRecord) \
    MACRO(IdleMarkerRecord) \
    MACRO(ImgsRecord) \
    MACRO(InfoRecord) \
    MACRO(IngrRecord) \
    MACRO(IpctRecord) \
    MACRO(IpdsRecord) \
    MACRO(KeymRecord) \
    MACRO(KeywordRecord) \
    MACRO(LandRecord) \
    MACRO(LighRecord) \
    MACRO(LoadScreenRecord) \
    MACRO(LocationRecord) \
    MACRO(LtexRecord) \
    MACRO(LvlcRecord) \
    MACRO(LvliRecord) \
    MACRO(LvspRecord) \
    MACRO(MagicRecord) \
    MACRO(MaterialRecord) \
    MACRO(MesgRecord) \
    MACRO(MiscRecord) \
    MACRO(MsttRecord) \
    MACRO(MustRecord) \
    MACRO(NavmRecord) \
    MACRO(NpcRecord) \
    MACRO(NoteRecord) \
    MACRO(OutfitRecord) \
    MACRO(PackageRecord) \
    MACRO(PerkRecord) \
    MACRO(PndRecord) \
    MACRO(ProjRecord) \
    MACRO(QuestRecord) \
    MACRO(RaceRecord) \
    MACRO(RefrRecord) \
    MACRO(RegionRecord) \
    MACRO(RelaRecord) \
    MACRO(RevbRecord) \
    MACRO(RoadRecord) \
    MACRO(ScriptRecord) \
    MACRO(ScrRecord) \
    MACRO(ShouRecord) \
    MACRO(SlgmRecord) \
    MACRO(SmqnRecord) \
    MACRO(SounRecord) \
    MACRO(SpellRecord) \
    MACRO(SpgdRecord) \
    MACRO(StaticCollectionRecord) \
    MACRO(StatRecord) \
    MACRO(ScenRecord) \
    MACRO(TextureSetRecord) \
    MACRO(TreeRecord) \
    MACRO(WateRecord) \
    MACRO(WeaponRecord) \
    MACRO(WorldspaceRecord) \
    MACRO(WthrRecord)
} // namespace

bool resolveComponents(BaseCollection* coll, int recordIndex,
                       openck::FormComponents*& components, void*& recordPtr)
{
#define RESOLVE_RECORD_TYPE(recType) \
    if (tryResolveComponents<recType>(coll, recordIndex, components, recordPtr)) return true;
    FOR_EACH_COMPONENT_RECORD_TYPE(RESOLVE_RECORD_TYPE)
#undef RESOLVE_RECORD_TYPE
#undef FOR_EACH_COMPONENT_RECORD_TYPE
    return false;
}
