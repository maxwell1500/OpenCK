#include "data.hpp"
#include "collection_impl.hpp"

#include "../tools/undostack.hpp"
#include "../tools/macrocommand.hpp"
#include "../tools/deleterecordcommandbase.hpp"
#include "../tools/editrecordcommand.hpp"
#include "columnimp.hpp"
#include "ckid.hpp"
#include "idtable.hpp"
#include "../doc/messages.hpp"
#include "../../../libs/files/esm/esmreader.hpp"
#include "logger.hpp"

#include <QAbstractItemModel>
#include <QElapsedTimer>
#include <QMessageBox>

#include <stdexcept>

#include <algorithm>

#ifdef _WIN32
static void seh_translator(unsigned int code, EXCEPTION_POINTERS*)
{
    throw std::runtime_error("SEH exception");
}
#endif

Data::Data(const QStringList& files, const FilePaths& paths)
    : contentFiles(files), paths(paths), mUndoStack(new UndoStack())
{
    // GMST - Game Settings
    gameSettings.addColumn(new StringIdColumn<GameSetting>());
    gameSettings.addColumn(new RecordStateColumn<GameSetting>());
    gameSettings.addColumn(new StringColumn<GameSetting>("Name", &GameSetting::editorId));
    gameSettings.addColumn(new VariantColumn<GameSetting>("Value", &GameSetting::value));
    gameSettings.addColumn(new VarTypeColumn<GameSetting>(BaseColumn::Display_GmstVarType));
    gameSettings.addColumn(new VarValueColumn<GameSetting>());
    addModel(new IdTable(&gameSettings), CkId::Type_Gmst);

    // NPC_ - NPCs and Actors
    npcCollection.addColumn(new StringIdColumn<NpcRecord>());
    npcCollection.addColumn(new RecordStateColumn<NpcRecord>());
    npcCollection.addColumn(new StringColumn<NpcRecord>("Full Name", &NpcRecord::fullName));
    npcCollection.addColumn(new IntColumn<NpcRecord>("Level", &NpcRecord::level));
    npcCollection.addColumn(new IntColumn<NpcRecord>("Race", &NpcRecord::race));
    npcCollection.addColumn(new IntColumn<NpcRecord>("Class", &NpcRecord::class_));
    npcCollection.addColumn(new IntColumn<NpcRecord>("Faction", &NpcRecord::faction));
    addModel(new IdTable(&npcCollection), CkId::Type_Npc_);

    // WEAP - Weapons
    weaponCollection.addColumn(new StringIdColumn<WeaponRecord>());
    weaponCollection.addColumn(new RecordStateColumn<WeaponRecord>());
    weaponCollection.addColumn(new StringColumn<WeaponRecord>("Name", &WeaponRecord::editorId));
    weaponCollection.addColumn(new FloatColumn<WeaponRecord>("Damage", &WeaponRecord::damage));
    weaponCollection.addColumn(new FloatColumn<WeaponRecord>("Speed", &WeaponRecord::speed));
    weaponCollection.addColumn(new FloatColumn<WeaponRecord>("Weight", &WeaponRecord::weight));
    weaponCollection.addColumn(new IntColumn<WeaponRecord>("Value", &WeaponRecord::value));
    addModel(new IdTable(&weaponCollection), CkId::Type_Weap_);

    // ARMOR - Armor
    armorCollection.addColumn(new StringIdColumn<ArmorRecord>());
    armorCollection.addColumn(new RecordStateColumn<ArmorRecord>());
    armorCollection.addColumn(new StringColumn<ArmorRecord>("Name", &ArmorRecord::editorId));
    armorCollection.addColumn(new IntColumn<ArmorRecord>("Armor Rating", &ArmorRecord::armorRating));
    armorCollection.addColumn(new FloatColumn<ArmorRecord>("Weight", &ArmorRecord::weight));
    armorCollection.addColumn(new IntColumn<ArmorRecord>("Value", &ArmorRecord::value));
    addModel(new IdTable(&armorCollection), CkId::Type_Armor_);

    // SPEL - Spells
    spellCollection.addColumn(new StringIdColumn<SpellRecord>());
    spellCollection.addColumn(new RecordStateColumn<SpellRecord>());
    spellCollection.addColumn(new StringColumn<SpellRecord>("Name", &SpellRecord::editorId));
    spellCollection.addColumn(new IntColumn<SpellRecord>("Cost", &SpellRecord::cost));
    addModel(new IdTable(&spellCollection), CkId::Type_Spel_);

    // MAGIC - Magic Effects
    magicCollection.addColumn(new StringIdColumn<MagicRecord>());
    magicCollection.addColumn(new RecordStateColumn<MagicRecord>());
    magicCollection.addColumn(new StringColumn<MagicRecord>("Name", &MagicRecord::editorId));
    magicCollection.addColumn(new IntColumn<MagicRecord>("Schools", &MagicRecord::schools));
    addModel(new IdTable(&magicCollection), CkId::Type_Magic_);

    // QUEST - Quests
    questCollection.addColumn(new StringIdColumn<QuestRecord>());
    questCollection.addColumn(new RecordStateColumn<QuestRecord>());
    questCollection.addColumn(new StringColumn<QuestRecord>("Name", &QuestRecord::questName));
    questCollection.addColumn(new IntColumn<QuestRecord>("Type", &QuestRecord::questType));
    addModel(new IdTable(&questCollection), CkId::Type_Quest_);

    // DIAL - Dialogue
    dialCollection.addColumn(new StringIdColumn<DialRecord>());
    dialCollection.addColumn(new RecordStateColumn<DialRecord>());
    dialCollection.addColumn(new StringColumn<DialRecord>("Name", &DialRecord::topicName));
    addModel(new IdTable(&dialCollection), CkId::Type_Dial_);

    // INFO - Dialogue Information
    infoCollection.addColumn(new StringIdColumn<InfoRecord>());
    infoCollection.addColumn(new RecordStateColumn<InfoRecord>());
    infoCollection.addColumn(new StringColumn<InfoRecord>("Response", &InfoRecord::responseText));
    addModel(new IdTable(&infoCollection), CkId::Type_Info_);

    // GLOB - Global Variables
    globCollection.addColumn(new StringIdColumn<GlobalVariable>());
    globCollection.addColumn(new RecordStateColumn<GlobalVariable>());
    globCollection.addColumn(new VarValueColumn<GlobalVariable>());
    globCollection.addColumn(new VarTypeColumn<GlobalVariable>(BaseColumn::Display_GmstVarType));
    addModel(new IdTable(&globCollection), CkId::Type_Glob_);

    // LCRT - Location References
    lcrtCollection.addColumn(new StringIdColumn<LocationRefType>());
    lcrtCollection.addColumn(new RecordStateColumn<LocationRefType>());
    addModel(new IdTable(&lcrtCollection), CkId::Type_Lcrt_);

    // PACK - AI Packages
    packCollection.addColumn(new StringIdColumn<PackageRecord>());
    packCollection.addColumn(new RecordStateColumn<PackageRecord>());
    packCollection.addColumn(new StringColumn<PackageRecord>("Name", &PackageRecord::editorId));
    packCollection.addColumn(new IntColumn<PackageRecord>("Type", &PackageRecord::packageType));
    addModel(new IdTable(&packCollection), CkId::Type_Pack_);

    // TREE - Trees
    treeCollection.addColumn(new StringIdColumn<TreeRecord>());
    treeCollection.addColumn(new RecordStateColumn<TreeRecord>());
    treeCollection.addColumn(new StringColumn<TreeRecord>("Model Path", &TreeRecord::modelPath));
    addModel(new IdTable(&treeCollection), CkId::Type_Tree_);

    // ALCH - Alchemy
    alchCollection.addColumn(new StringIdColumn<AlchRecord>());
    alchCollection.addColumn(new RecordStateColumn<AlchRecord>());
    alchCollection.addColumn(new FloatColumn<AlchRecord>("Weight", &AlchRecord::weight));
    alchCollection.addColumn(new IntColumn<AlchRecord>("Value", &AlchRecord::value));
    addModel(new IdTable(&alchCollection), CkId::Type_Alch_);

    // INGR - Ingredients
    ingrCollection.addColumn(new StringIdColumn<IngrRecord>());
    ingrCollection.addColumn(new RecordStateColumn<IngrRecord>());
    ingrCollection.addColumn(new FloatColumn<IngrRecord>("Weight", &IngrRecord::weight));
    ingrCollection.addColumn(new IntColumn<IngrRecord>("Value", &IngrRecord::value));
    addModel(new IdTable(&ingrCollection), CkId::Type_Ingr_);

    // CONT - Containers
    contCollection.addColumn(new StringIdColumn<ContRecord>());
    contCollection.addColumn(new RecordStateColumn<ContRecord>());
    contCollection.addColumn(new FloatColumn<ContRecord>("Weight", &ContRecord::weight));
    contCollection.addColumn(new IntColumn<ContRecord>("Value", &ContRecord::value));
    addModel(new IdTable(&contCollection), CkId::Type_Cont_);

    // ENCH - Enchantments
    enchCollection.addColumn(new StringIdColumn<EnchRecord>());
    enchCollection.addColumn(new RecordStateColumn<EnchRecord>());
    enchCollection.addColumn(new StringColumn<EnchRecord>("Name", &EnchRecord::name));
    enchCollection.addColumn(new IntColumn<EnchRecord>("Cost", &EnchRecord::costLimit));
    enchCollection.addColumn(new IntColumn<EnchRecord>("Charges", &EnchRecord::charges));
    addModel(new IdTable(&enchCollection), CkId::Type_Ench_);

    // BOOK - Books
    bookCollection.addColumn(new StringIdColumn<BookRecord>());
    bookCollection.addColumn(new RecordStateColumn<BookRecord>());
    bookCollection.addColumn(new IntColumn<BookRecord>("Page Count", &BookRecord::pageCount));
    bookCollection.addColumn(new StringColumn<BookRecord>("Pages", &BookRecord::pages));
    addModel(new IdTable(&bookCollection), CkId::Type_Book_);

    // MISC - Miscellaneous Items
    miscCollection.addColumn(new StringIdColumn<MiscRecord>());
    miscCollection.addColumn(new RecordStateColumn<MiscRecord>());
    miscCollection.addColumn(new StringColumn<MiscRecord>("Name", &MiscRecord::editorId));
    miscCollection.addColumn(new FloatColumn<MiscRecord>("Weight", &MiscRecord::weight));
    miscCollection.addColumn(new IntColumn<MiscRecord>("Value", &MiscRecord::value));
    addModel(new IdTable(&miscCollection), CkId::Type_Misc_);

    // ACTI - Activators
    actiCollection.addColumn(new StringIdColumn<ActiRecord>());
    actiCollection.addColumn(new RecordStateColumn<ActiRecord>());
    addModel(new IdTable(&actiCollection), CkId::Type_Acti_);

    // STAT - Static Objects
    statCollection.addColumn(new StringIdColumn<StatRecord>());
    statCollection.addColumn(new RecordStateColumn<StatRecord>());
    statCollection.addColumn(new StringColumn<StatRecord>("Model Path", &StatRecord::modelPath));
    addModel(new IdTable(&statCollection), CkId::Type_Stat_);

    // RACE - Races
    raceCollection.addColumn(new StringIdColumn<RaceRecord>());
    raceCollection.addColumn(new RecordStateColumn<RaceRecord>());
    raceCollection.addColumn(new StringColumn<RaceRecord>("Name", &RaceRecord::editorId));
    addModel(new IdTable(&raceCollection), CkId::Type_Race_);

    // CLASS - Classes
    classCollection.addColumn(new StringIdColumn<ClassRecord>());
    classCollection.addColumn(new RecordStateColumn<ClassRecord>());
    classCollection.addColumn(new StringColumn<ClassRecord>("Name", &ClassRecord::className));
    addModel(new IdTable(&classCollection), CkId::Type_Class_);

    // FACT - Factions
    factCollection.addColumn(new StringIdColumn<FactRecord>());
    factCollection.addColumn(new RecordStateColumn<FactRecord>());
    factCollection.addColumn(new StringColumn<FactRecord>("Name", &FactRecord::factionName));
    addModel(new IdTable(&factCollection), CkId::Type_Fact_);

    // PERK - Perks
    perkCollection.addColumn(new StringIdColumn<PerkRecord>());
    perkCollection.addColumn(new RecordStateColumn<PerkRecord>());
    perkCollection.addColumn(new StringColumn<PerkRecord>("Name", &PerkRecord::editorId));
    perkCollection.addColumn(new StringColumn<PerkRecord>("Description", &PerkRecord::description));
    addModel(new IdTable(&perkCollection), CkId::Type_PerK_);

    // CELL - Interior Cells
    cellCollection.addColumn(new StringIdColumn<CellRecord>());
    cellCollection.addColumn(new RecordStateColumn<CellRecord>());
    cellCollection.addColumn(new IntColumn<CellRecord>("Cell X", &CellRecord::cellX));
    cellCollection.addColumn(new IntColumn<CellRecord>("Cell Y", &CellRecord::cellY));
    cellCollection.addColumn(new StringColumn<CellRecord>("Cell Name", &CellRecord::cellName));
    cellCollection.addColumn(new IntColumn<CellRecord>("Owner", &CellRecord::owner));
    addModel(new IdTable(&cellCollection), CkId::Type_Cel_);

    // WRLD - Worldspaces
    worldspaceCollection.addColumn(new StringIdColumn<WorldspaceRecord>());
    worldspaceCollection.addColumn(new RecordStateColumn<WorldspaceRecord>());
    worldspaceCollection.addColumn(new StringColumn<WorldspaceRecord>("Name", &WorldspaceRecord::name));
    worldspaceCollection.addColumn(new IntColumn<WorldspaceRecord>("Water Type", &WorldspaceRecord::waterType));
    addModel(new IdTable(&worldspaceCollection), CkId::Type_WRLD_);

    // LOCT - Locations
    locationCollection.addColumn(new StringIdColumn<LocationRecord>());
    locationCollection.addColumn(new RecordStateColumn<LocationRecord>());
    locationCollection.addColumn(new IntColumn<LocationRecord>("Parent ID", &LocationRecord::parentId));
    locationCollection.addColumn(new IntColumn<LocationRecord>("X", &LocationRecord::x));
    locationCollection.addColumn(new IntColumn<LocationRecord>("Y", &LocationRecord::y));
    locationCollection.addColumn(new IntColumn<LocationRecord>("Z", &LocationRecord::z));
    addModel(new IdTable(&locationCollection), CkId::Type_LOCT_);

    // PNDT - Planets
    planetCollection.addColumn(new StringIdColumn<PndRecord>());
    planetCollection.addColumn(new RecordStateColumn<PndRecord>());
    planetCollection.addColumn(new IntColumn<PndRecord>("Flags", &PndRecord::flags));
    planetCollection.addColumn(new StringColumn<PndRecord>("Star System", &PndRecord::starSystem));
    planetCollection.addColumn(new FloatColumn<PndRecord>("Temperature", &PndRecord::temperature));
    planetCollection.addColumn(new FloatColumn<PndRecord>("Density", &PndRecord::density));
    planetCollection.addColumn(new FloatColumn<PndRecord>("Phase", &PndRecord::phase));
    planetCollection.addColumn(new IntColumn<PndRecord>("Resources", &PndRecord::resources));
    addModel(new IdTable(&planetCollection), CkId::Type_Plnt_);

    // REFR - References
    refrCollection.addColumn(new StringIdColumn<RefrRecord>());
    refrCollection.addColumn(new RecordStateColumn<RefrRecord>());
    refrCollection.addColumn(new IntColumn<RefrRecord>("Base ID", &RefrRecord::baseId));
    refrCollection.addColumn(new FloatColumn<RefrRecord>("Pos X", &RefrRecord::posX));
    refrCollection.addColumn(new FloatColumn<RefrRecord>("Pos Y", &RefrRecord::posY));
    refrCollection.addColumn(new FloatColumn<RefrRecord>("Pos Z", &RefrRecord::posZ));
    refrCollection.addColumn(new FloatColumn<RefrRecord>("Rot X", &RefrRecord::rotX));
    refrCollection.addColumn(new FloatColumn<RefrRecord>("Rot Y", &RefrRecord::rotY));
    refrCollection.addColumn(new FloatColumn<RefrRecord>("Rot Z", &RefrRecord::rotZ));
    refrCollection.addColumn(new FloatColumn<RefrRecord>("Scale", &RefrRecord::scale));
    addModel(new IdTable(&refrCollection), CkId::Type_Refr_);

    // MATERIAL - Materials
    materialCollection.addColumn(new StringIdColumn<MaterialRecord>());
    materialCollection.addColumn(new RecordStateColumn<MaterialRecord>());
    materialCollection.addColumn(new StringColumn<MaterialRecord>("Material Name", &MaterialRecord::materialName));
    materialCollection.addColumn(new StringColumn<MaterialRecord>("Texture Path", &MaterialRecord::texturePath));
    addModel(new IdTable(&materialCollection), CkId::Type_Material_);

    // LAND - Landscape/Terrain
    landCollection.addColumn(new StringIdColumn<LandRecord>());
    landCollection.addColumn(new RecordStateColumn<LandRecord>());
    addModel(new IdTable(&landCollection), CkId::Type_Land_);

    // SOUN - Sounds
    sounCollection.addColumn(new StringIdColumn<SounRecord>());
    sounCollection.addColumn(new RecordStateColumn<SounRecord>());
    sounCollection.addColumn(new StringColumn<SounRecord>("Sound File", &SounRecord::soundFile));
    addModel(new IdTable(&sounCollection), CkId::Type_Soun_);

    // WTHR - Weather
    wthrCollection.addColumn(new StringIdColumn<WthrRecord>());
    wthrCollection.addColumn(new RecordStateColumn<WthrRecord>());
    wthrCollection.addColumn(new StringColumn<WthrRecord>("Sun Texture", &WthrRecord::sunTexture));
    addModel(new IdTable(&wthrCollection), CkId::Type_Wthr_);

    // LTEX - Land Textures
    ltexCollection.addColumn(new StringIdColumn<LtexRecord>());
    ltexCollection.addColumn(new RecordStateColumn<LtexRecord>());
    ltexCollection.addColumn(new StringColumn<LtexRecord>("Icon Path", &LtexRecord::iconPath));
    ltexCollection.addColumn(new IntColumn<LtexRecord>("Havok Material", &LtexRecord::havokMaterial));
    addModel(new IdTable(&ltexCollection), CkId::Type_Ltex_);

    // SCEN - Scenes
    scenCollection.addColumn(new StringIdColumn<ScenRecord>());
    scenCollection.addColumn(new RecordStateColumn<ScenRecord>());
    scenCollection.addColumn(new StringColumn<ScenRecord>("Name", &ScenRecord::editorId));
    addModel(new IdTable(&scenCollection), CkId::Type_Scen_);

    // AMMO - Ammo_s
    ammoCollection.addColumn(new StringIdColumn<AmmoRecord>());
    ammoCollection.addColumn(new RecordStateColumn<AmmoRecord>());
    ammoCollection.addColumn(new StringColumn<AmmoRecord>("Name", &AmmoRecord::editorId));
    addModel(new IdTable(&ammoCollection), CkId::Type_Ammo_);

    // APPA - Appa_s
    appaCollection.addColumn(new StringIdColumn<AppaRecord>());
    appaCollection.addColumn(new RecordStateColumn<AppaRecord>());
    appaCollection.addColumn(new StringColumn<AppaRecord>("Name", &AppaRecord::editorId));
    addModel(new IdTable(&appaCollection), CkId::Type_Appa_);

    // AVIF - Avif_s
    avifCollection.addColumn(new StringIdColumn<ActorValueInfoRecord>());
    avifCollection.addColumn(new RecordStateColumn<ActorValueInfoRecord>());
    avifCollection.addColumn(new StringColumn<ActorValueInfoRecord>("Name", &ActorValueInfoRecord::editorId));
    addModel(new IdTable(&avifCollection), CkId::Type_Avif_);

    // BSGN - Bsgn_s
    bsgnCollection.addColumn(new StringIdColumn<BsgnRecord>());
    bsgnCollection.addColumn(new RecordStateColumn<BsgnRecord>());
    bsgnCollection.addColumn(new StringColumn<BsgnRecord>("Name", &BsgnRecord::editorId));
    addModel(new IdTable(&bsgnCollection), CkId::Type_Bsgn_);

    // CLMT - Clmt_s
    clmtCollection.addColumn(new StringIdColumn<ClimateRecord>());
    clmtCollection.addColumn(new RecordStateColumn<ClimateRecord>());
    clmtCollection.addColumn(new StringColumn<ClimateRecord>("Name", &ClimateRecord::editorId));
    addModel(new IdTable(&clmtCollection), CkId::Type_Clmt_);

    // CLOT - Clot_s
    clotCollection.addColumn(new StringIdColumn<ClotRecord>());
    clotCollection.addColumn(new RecordStateColumn<ClotRecord>());
    clotCollection.addColumn(new StringColumn<ClotRecord>("Name", &ClotRecord::editorId));
    addModel(new IdTable(&clotCollection), CkId::Type_Clot_);

    // COBJ - Cobj_s
    cobjCollection.addColumn(new StringIdColumn<CobjRecord>());
    cobjCollection.addColumn(new RecordStateColumn<CobjRecord>());
    cobjCollection.addColumn(new StringColumn<CobjRecord>("Name", &CobjRecord::editorId));
    addModel(new IdTable(&cobjCollection), CkId::Type_Cobj_);

    // CREA - Crea_s
    creatureCollection.addColumn(new StringIdColumn<CreatureRecord>());
    creatureCollection.addColumn(new RecordStateColumn<CreatureRecord>());
    creatureCollection.addColumn(new StringColumn<CreatureRecord>("Name", &CreatureRecord::editorId));
    addModel(new IdTable(&creatureCollection), CkId::Type_Crea_);

    // CSTY - Csty_s
    cstyCollection.addColumn(new StringIdColumn<CstyRecord>());
    cstyCollection.addColumn(new RecordStateColumn<CstyRecord>());
    cstyCollection.addColumn(new StringColumn<CstyRecord>("Name", &CstyRecord::editorId));
    addModel(new IdTable(&cstyCollection), CkId::Type_Csty_);

    // DOOR - Door_s
    doorCollection.addColumn(new StringIdColumn<DoorRecord>());
    doorCollection.addColumn(new RecordStateColumn<DoorRecord>());
    doorCollection.addColumn(new StringColumn<DoorRecord>("Name", &DoorRecord::editorId));
    addModel(new IdTable(&doorCollection), CkId::Type_Door_);

    // EFSH - Efsh_s
    efshCollection.addColumn(new StringIdColumn<EfshRecord>());
    efshCollection.addColumn(new RecordStateColumn<EfshRecord>());
    efshCollection.addColumn(new StringColumn<EfshRecord>("Name", &EfshRecord::editorId));
    addModel(new IdTable(&efshCollection), CkId::Type_Efsh_);

    // EXPL - Expl_s
    explCollection.addColumn(new StringIdColumn<ExplRecord>());
    explCollection.addColumn(new RecordStateColumn<ExplRecord>());
    explCollection.addColumn(new StringColumn<ExplRecord>("Name", &ExplRecord::editorId));
    addModel(new IdTable(&explCollection), CkId::Type_Expl_);

    // EYES - Eyes_s
    eyesCollection.addColumn(new StringIdColumn<EyesRecord>());
    eyesCollection.addColumn(new RecordStateColumn<EyesRecord>());
    eyesCollection.addColumn(new StringColumn<EyesRecord>("Name", &EyesRecord::editorId));
    addModel(new IdTable(&eyesCollection), CkId::Type_Eyes_);

    // FLOR - Flor_s
    florCollection.addColumn(new StringIdColumn<FlorRecord>());
    florCollection.addColumn(new RecordStateColumn<FlorRecord>());
    florCollection.addColumn(new StringColumn<FlorRecord>("Name", &FlorRecord::editorId));
    addModel(new IdTable(&florCollection), CkId::Type_Flor_);

    // FLST - Flst_s
    flstCollection.addColumn(new StringIdColumn<FormListRecord>());
    flstCollection.addColumn(new RecordStateColumn<FormListRecord>());
    flstCollection.addColumn(new StringColumn<FormListRecord>("Name", &FormListRecord::editorId));
    addModel(new IdTable(&flstCollection), CkId::Type_Flst_);

    // FURN - Furn_s
    furnCollection.addColumn(new StringIdColumn<FurnRecord>());
    furnCollection.addColumn(new RecordStateColumn<FurnRecord>());
    furnCollection.addColumn(new StringColumn<FurnRecord>("Name", &FurnRecord::editorId));
    addModel(new IdTable(&furnCollection), CkId::Type_Furn_);

    // GRAS - Grass_s
    grassCollection.addColumn(new StringIdColumn<GrassRecord>());
    grassCollection.addColumn(new RecordStateColumn<GrassRecord>());
    grassCollection.addColumn(new StringColumn<GrassRecord>("Name", &GrassRecord::editorId));
    addModel(new IdTable(&grassCollection), CkId::Type_Grass_);

    // HAIR - Hair_s
    hairCollection.addColumn(new StringIdColumn<HairRecord>());
    hairCollection.addColumn(new RecordStateColumn<HairRecord>());
    hairCollection.addColumn(new StringColumn<HairRecord>("Name", &HairRecord::editorId));
    addModel(new IdTable(&hairCollection), CkId::Type_Hair_);

    // IDLE - Idle_s
    idleCollection.addColumn(new StringIdColumn<IdleAnimationRecord>());
    idleCollection.addColumn(new RecordStateColumn<IdleAnimationRecord>());
    idleCollection.addColumn(new StringColumn<IdleAnimationRecord>("Name", &IdleAnimationRecord::editorId));
    addModel(new IdTable(&idleCollection), CkId::Type_Idle_);

    // IDLM - Idlm_s
    idlmCollection.addColumn(new StringIdColumn<IdleMarkerRecord>());
    idlmCollection.addColumn(new RecordStateColumn<IdleMarkerRecord>());
    idlmCollection.addColumn(new StringColumn<IdleMarkerRecord>("Name", &IdleMarkerRecord::editorId));
    addModel(new IdTable(&idlmCollection), CkId::Type_Idlm_);

    // IMGS - Imgs_s
    imgsCollection.addColumn(new StringIdColumn<ImgsRecord>());
    imgsCollection.addColumn(new RecordStateColumn<ImgsRecord>());
    imgsCollection.addColumn(new StringColumn<ImgsRecord>("Name", &ImgsRecord::editorId));
    addModel(new IdTable(&imgsCollection), CkId::Type_Imgs_);

    // KEYM - Keym_s
    keymCollection.addColumn(new StringIdColumn<KeymRecord>());
    keymCollection.addColumn(new RecordStateColumn<KeymRecord>());
    keymCollection.addColumn(new StringColumn<KeymRecord>("Name", &KeymRecord::editorId));
    addModel(new IdTable(&keymCollection), CkId::Type_Keym_);

    // KYWD - Kywd_s
    kywdCollection.addColumn(new StringIdColumn<KeywordRecord>());
    kywdCollection.addColumn(new RecordStateColumn<KeywordRecord>());
    kywdCollection.addColumn(new StringColumn<KeywordRecord>("Name", &KeywordRecord::editorId));
    addModel(new IdTable(&kywdCollection), CkId::Type_Kywd_);

    // LIGH - Ligh_s
    lighCollection.addColumn(new StringIdColumn<LighRecord>());
    lighCollection.addColumn(new RecordStateColumn<LighRecord>());
    lighCollection.addColumn(new StringColumn<LighRecord>("Name", &LighRecord::editorId));
    addModel(new IdTable(&lighCollection), CkId::Type_Ligh_);

    // LSCR - Lscr_s
    lscrCollection.addColumn(new StringIdColumn<LoadScreenRecord>());
    lscrCollection.addColumn(new RecordStateColumn<LoadScreenRecord>());
    lscrCollection.addColumn(new StringColumn<LoadScreenRecord>("Name", &LoadScreenRecord::editorId));
    addModel(new IdTable(&lscrCollection), CkId::Type_Lscr_);

    // LVLC - Lvlc_s
    lvlcCollection.addColumn(new StringIdColumn<LvlcRecord>());
    lvlcCollection.addColumn(new RecordStateColumn<LvlcRecord>());
    lvlcCollection.addColumn(new StringColumn<LvlcRecord>("Name", &LvlcRecord::editorId));
    addModel(new IdTable(&lvlcCollection), CkId::Type_Lvlc_);

    // LVLI - Lvli_s
    lvliCollection.addColumn(new StringIdColumn<LvliRecord>());
    lvliCollection.addColumn(new RecordStateColumn<LvliRecord>());
    lvliCollection.addColumn(new StringColumn<LvliRecord>("Name", &LvliRecord::editorId));
    addModel(new IdTable(&lvliCollection), CkId::Type_Lvli_);

    // LVSP - Lvsp_s
    lvspCollection.addColumn(new StringIdColumn<LvspRecord>());
    lvspCollection.addColumn(new RecordStateColumn<LvspRecord>());
    lvspCollection.addColumn(new StringColumn<LvspRecord>("Name", &LvspRecord::editorId));
    addModel(new IdTable(&lvspCollection), CkId::Type_Lvsp_);

    // MESG - Mesg_s
    mesgCollection.addColumn(new StringIdColumn<MesgRecord>());
    mesgCollection.addColumn(new RecordStateColumn<MesgRecord>());
    mesgCollection.addColumn(new StringColumn<MesgRecord>("Name", &MesgRecord::editorId));
    addModel(new IdTable(&mesgCollection), CkId::Type_Mesg_);

    // MSTT - Mstt_s
    msttCollection.addColumn(new StringIdColumn<MsttRecord>());
    msttCollection.addColumn(new RecordStateColumn<MsttRecord>());
    msttCollection.addColumn(new StringColumn<MsttRecord>("Name", &MsttRecord::editorId));
    addModel(new IdTable(&msttCollection), CkId::Type_Mstt_);

    // NAVM - Navm_s
    navmCollection.addColumn(new StringIdColumn<NavmRecord>());
    navmCollection.addColumn(new RecordStateColumn<NavmRecord>());
    navmCollection.addColumn(new StringColumn<NavmRecord>("Name", &NavmRecord::editorId));
    addModel(new IdTable(&navmCollection), CkId::Type_Navm_);

    // NOTE - Note_s
    noteCollection.addColumn(new StringIdColumn<NoteRecord>());
    noteCollection.addColumn(new RecordStateColumn<NoteRecord>());
    noteCollection.addColumn(new StringColumn<NoteRecord>("Name", &NoteRecord::editorId));
    addModel(new IdTable(&noteCollection), CkId::Type_Note_);

    // OTFT - Otft_s
    otftCollection.addColumn(new StringIdColumn<OutfitRecord>());
    otftCollection.addColumn(new RecordStateColumn<OutfitRecord>());
    otftCollection.addColumn(new StringColumn<OutfitRecord>("Name", &OutfitRecord::editorId));
    addModel(new IdTable(&otftCollection), CkId::Type_Otft_);

    // PROJ - Proj_s
    projCollection.addColumn(new StringIdColumn<ProjRecord>());
    projCollection.addColumn(new RecordStateColumn<ProjRecord>());
    projCollection.addColumn(new StringColumn<ProjRecord>("Name", &ProjRecord::editorId));
    addModel(new IdTable(&projCollection), CkId::Type_Proj_);

    // REGN - Regn_s
    regnCollection.addColumn(new StringIdColumn<RegionRecord>());
    regnCollection.addColumn(new RecordStateColumn<RegionRecord>());
    regnCollection.addColumn(new StringColumn<RegionRecord>("Name", &RegionRecord::editorId));
    addModel(new IdTable(&regnCollection), CkId::Type_Regn_);

    // ROAD - Road_s
    roadCollection.addColumn(new StringIdColumn<RoadRecord>());
    roadCollection.addColumn(new RecordStateColumn<RoadRecord>());
    roadCollection.addColumn(new StringColumn<RoadRecord>("Name", &RoadRecord::editorId));
    addModel(new IdTable(&roadCollection), CkId::Type_Road_);

    // SCPT - Scpt_s
    scptCollection.addColumn(new StringIdColumn<ScriptRecord>());
    scptCollection.addColumn(new RecordStateColumn<ScriptRecord>());
    scptCollection.addColumn(new StringColumn<ScriptRecord>("Name", &ScriptRecord::editorId));
    addModel(new IdTable(&scptCollection), CkId::Type_Scpt_);

    // SCRL - Scrl_s
    scrlCollection.addColumn(new StringIdColumn<ScrRecord>());
    scrlCollection.addColumn(new RecordStateColumn<ScrRecord>());
    scrlCollection.addColumn(new StringColumn<ScrRecord>("Name", &ScrRecord::editorId));
    addModel(new IdTable(&scrlCollection), CkId::Type_Scrl_);

    // SLGM - Slgm_s
    slgmCollection.addColumn(new StringIdColumn<SlgmRecord>());
    slgmCollection.addColumn(new RecordStateColumn<SlgmRecord>());
    slgmCollection.addColumn(new StringColumn<SlgmRecord>("Name", &SlgmRecord::editorId));
    addModel(new IdTable(&slgmCollection), CkId::Type_Slgm_);

    // SMQN - Smqn_s
    smqnCollection.addColumn(new StringIdColumn<SmqnRecord>());
    smqnCollection.addColumn(new RecordStateColumn<SmqnRecord>());
    smqnCollection.addColumn(new StringColumn<SmqnRecord>("Name", &SmqnRecord::editorId));
    addModel(new IdTable(&smqnCollection), CkId::Type_Smqn_);

    // SPGD - Spgd_s
    spgdCollection.addColumn(new StringIdColumn<SpgdRecord>());
    spgdCollection.addColumn(new RecordStateColumn<SpgdRecord>());
    spgdCollection.addColumn(new StringColumn<SpgdRecord>("Name", &SpgdRecord::editorId));
    addModel(new IdTable(&spgdCollection), CkId::Type_Spgd_);

    // SCOL - Scol_s
    scolCollection.addColumn(new StringIdColumn<StaticCollectionRecord>());
    scolCollection.addColumn(new RecordStateColumn<StaticCollectionRecord>());
    scolCollection.addColumn(new StringColumn<StaticCollectionRecord>("Name", &StaticCollectionRecord::editorId));
    addModel(new IdTable(&scolCollection), CkId::Type_Scol_);

    // TXST - Txst_s
    txstCollection.addColumn(new StringIdColumn<TextureSetRecord>());
    txstCollection.addColumn(new RecordStateColumn<TextureSetRecord>());
    txstCollection.addColumn(new StringColumn<TextureSetRecord>("Name", &TextureSetRecord::editorId));
    addModel(new IdTable(&txstCollection), CkId::Type_Txst_);

    // WATR - Wate_s
    wateCollection.addColumn(new StringIdColumn<WateRecord>());
    wateCollection.addColumn(new RecordStateColumn<WateRecord>());
    wateCollection.addColumn(new StringColumn<WateRecord>("Name", &WateRecord::editorId));
    addModel(new IdTable(&wateCollection), CkId::Type_Wate_);

    // ANIO - Animated Object
    anioCollection.addColumn(new StringIdColumn<AnioRecord>());
    anioCollection.addColumn(new RecordStateColumn<AnioRecord>());
    anioCollection.addColumn(new StringColumn<AnioRecord>("Name", &AnioRecord::editorId));
    addModel(new IdTable(&anioCollection), CkId::Type_Anio_);

    // ARTV - Art Object
    artvCollection.addColumn(new StringIdColumn<ArtvRecord>());
    artvCollection.addColumn(new RecordStateColumn<ArtvRecord>());
    artvCollection.addColumn(new StringColumn<ArtvRecord>("Name", &ArtvRecord::editorId));
    addModel(new IdTable(&artvCollection), CkId::Type_Artv_);

    // CLFM - Color
    clfmCollection.addColumn(new StringIdColumn<ClfmRecord>());
    clfmCollection.addColumn(new RecordStateColumn<ClfmRecord>());
    clfmCollection.addColumn(new StringColumn<ClfmRecord>("Name", &ClfmRecord::editorId));
    addModel(new IdTable(&clfmCollection), CkId::Type_Clfm_);

    // DEBR - Debris
    debrCollection.addColumn(new StringIdColumn<DebrRecord>());
    debrCollection.addColumn(new RecordStateColumn<DebrRecord>());
    debrCollection.addColumn(new StringColumn<DebrRecord>("Name", &DebrRecord::editorId));
    addModel(new IdTable(&debrCollection), CkId::Type_Debr_);

    // ECZN - Encounter Zone
    ecznCollection.addColumn(new StringIdColumn<EcznRecord>());
    ecznCollection.addColumn(new RecordStateColumn<EcznRecord>());
    ecznCollection.addColumn(new StringColumn<EcznRecord>("Name", &EcznRecord::editorId));
    addModel(new IdTable(&ecznCollection), CkId::Type_Eczn_);

    // HAZD - Hazard
    hazdCollection.addColumn(new StringIdColumn<HazdRecord>());
    hazdCollection.addColumn(new RecordStateColumn<HazdRecord>());
    hazdCollection.addColumn(new StringColumn<HazdRecord>("Name", &HazdRecord::editorId));
    addModel(new IdTable(&hazdCollection), CkId::Type_Hazd_);

    // IPCT - Impact
    ipctCollection.addColumn(new StringIdColumn<IpctRecord>());
    ipctCollection.addColumn(new RecordStateColumn<IpctRecord>());
    ipctCollection.addColumn(new StringColumn<IpctRecord>("Name", &IpctRecord::editorId));
    addModel(new IdTable(&ipctCollection), CkId::Type_Ipct_);

    // IPDS - Impact Data Set
    ipdsCollection.addColumn(new StringIdColumn<IpdsRecord>());
    ipdsCollection.addColumn(new RecordStateColumn<IpdsRecord>());
    ipdsCollection.addColumn(new StringColumn<IpdsRecord>("Name", &IpdsRecord::editorId));
    addModel(new IdTable(&ipdsCollection), CkId::Type_Ipds_);

    // MUST - Music Type
    mustCollection.addColumn(new StringIdColumn<MustRecord>());
    mustCollection.addColumn(new RecordStateColumn<MustRecord>());
    mustCollection.addColumn(new StringColumn<MustRecord>("Name", &MustRecord::editorId));
    addModel(new IdTable(&mustCollection), CkId::Type_Must_);

    // RELA - Relationship
    relaCollection.addColumn(new StringIdColumn<RelaRecord>());
    relaCollection.addColumn(new RecordStateColumn<RelaRecord>());
    relaCollection.addColumn(new StringColumn<RelaRecord>("Name", &RelaRecord::editorId));
    addModel(new IdTable(&relaCollection), CkId::Type_Rela_);

    // REVB - Reverb Parameters
    revbCollection.addColumn(new StringIdColumn<RevbRecord>());
    revbCollection.addColumn(new RecordStateColumn<RevbRecord>());
    revbCollection.addColumn(new StringColumn<RevbRecord>("Name", &RevbRecord::editorId));
    addModel(new IdTable(&revbCollection), CkId::Type_Revb_);

    // SHOU - Shout
    shouCollection.addColumn(new StringIdColumn<ShouRecord>());
    shouCollection.addColumn(new RecordStateColumn<ShouRecord>());
    shouCollection.addColumn(new StringColumn<ShouRecord>("Name", &ShouRecord::editorId));
    addModel(new IdTable(&shouCollection), CkId::Type_Shou_);

    // HDPT - Head Part
    hdptCollection.addColumn(new StringIdColumn<HdptRecord>());
    hdptCollection.addColumn(new RecordStateColumn<HdptRecord>());
    hdptCollection.addColumn(new StringColumn<HdptRecord>("Name", &HdptRecord::editorId));
    addModel(new IdTable(&hdptCollection), CkId::Type_Hdpt_);

    // TERM - Terminal
    termCollection.addColumn(new StringIdColumn<TermRecord>());
    termCollection.addColumn(new RecordStateColumn<TermRecord>());
    termCollection.addColumn(new StringColumn<TermRecord>("Name", &TermRecord::editorId));
    addModel(new IdTable(&termCollection), CkId::Type_Term_);

    // MATT - Material Type
    mattCollection.addColumn(new StringIdColumn<MattRecord>());
    mattCollection.addColumn(new RecordStateColumn<MattRecord>());
    mattCollection.addColumn(new StringColumn<MattRecord>("Name", &MattRecord::editorId));
    addModel(new IdTable(&mattCollection), CkId::Type_Matt_);

    // MOVT - Movement Type
    movtCollection.addColumn(new StringIdColumn<MovtRecord>());
    movtCollection.addColumn(new RecordStateColumn<MovtRecord>());
    movtCollection.addColumn(new StringColumn<MovtRecord>("Name", &MovtRecord::editorId));
    addModel(new IdTable(&movtCollection), CkId::Type_Movt_);

    // MUSC - Music Track
    muscCollection.addColumn(new StringIdColumn<MuscRecord>());
    muscCollection.addColumn(new RecordStateColumn<MuscRecord>());
    muscCollection.addColumn(new StringColumn<MuscRecord>("Name", &MuscRecord::editorId));
    addModel(new IdTable(&muscCollection), CkId::Type_Musc_);

    // AACT - Action
    aactCollection.addColumn(new StringIdColumn<AactRecord>());
    aactCollection.addColumn(new RecordStateColumn<AactRecord>());
    aactCollection.addColumn(new StringColumn<AactRecord>("Name", &AactRecord::editorId));
    addModel(new IdTable(&aactCollection), CkId::Type_Aact_);
    // AAMD - Audio Modifier
    aamdCollection.addColumn(new StringIdColumn<AamdRecord>());
    aamdCollection.addColumn(new RecordStateColumn<AamdRecord>());
    aamdCollection.addColumn(new StringColumn<AamdRecord>("Name", &AamdRecord::editorId));
    addModel(new IdTable(&aamdCollection), CkId::Type_Aamd_);
    // AAPD - Animated Prop
    aapdCollection.addColumn(new StringIdColumn<AapdRecord>());
    aapdCollection.addColumn(new RecordStateColumn<AapdRecord>());
    aapdCollection.addColumn(new StringColumn<AapdRecord>("Name", &AapdRecord::editorId));
    addModel(new IdTable(&aapdCollection), CkId::Type_Aapd_);
    // ACHR - Actor Reference
    achrCollection.addColumn(new StringIdColumn<AchrRecord>());
    achrCollection.addColumn(new RecordStateColumn<AchrRecord>());
    achrCollection.addColumn(new StringColumn<AchrRecord>("Name", &AchrRecord::editorId));
    addModel(new IdTable(&achrCollection), CkId::Type_Achr_);
    // ADDN - Add-On Node
    addnCollection.addColumn(new StringIdColumn<AddnRecord>());
    addnCollection.addColumn(new RecordStateColumn<AddnRecord>());
    addnCollection.addColumn(new StringColumn<AddnRecord>("Name", &AddnRecord::editorId));
    addModel(new IdTable(&addnCollection), CkId::Type_Addn_);
    // AFFE - Affinity Event
    affeCollection.addColumn(new StringIdColumn<AffeRecord>());
    affeCollection.addColumn(new RecordStateColumn<AffeRecord>());
    affeCollection.addColumn(new StringColumn<AffeRecord>("Name", &AffeRecord::editorId));
    addModel(new IdTable(&affeCollection), CkId::Type_Affe_);
    // AMBS - Ambient Sound
    ambsCollection.addColumn(new StringIdColumn<AmbsRecord>());
    ambsCollection.addColumn(new RecordStateColumn<AmbsRecord>());
    ambsCollection.addColumn(new StringColumn<AmbsRecord>("Name", &AmbsRecord::editorId));
    addModel(new IdTable(&ambsCollection), CkId::Type_Ambs_);
    // AMDL - Audio Model
    amdlCollection.addColumn(new StringIdColumn<AmdlRecord>());
    amdlCollection.addColumn(new RecordStateColumn<AmdlRecord>());
    amdlCollection.addColumn(new StringColumn<AmdlRecord>("Name", &AmdlRecord::editorId));
    addModel(new IdTable(&amdlCollection), CkId::Type_Amdl_);
    // AOPF - Ambient Occlusion Float
    aopfCollection.addColumn(new StringIdColumn<AopfRecord>());
    aopfCollection.addColumn(new RecordStateColumn<AopfRecord>());
    aopfCollection.addColumn(new StringColumn<AopfRecord>("Name", &AopfRecord::editorId));
    addModel(new IdTable(&aopfCollection), CkId::Type_Aopf_);
    // AOPS - Shader Params
    aopsCollection.addColumn(new StringIdColumn<AopsRecord>());
    aopsCollection.addColumn(new RecordStateColumn<AopsRecord>());
    aopsCollection.addColumn(new StringColumn<AopsRecord>("Name", &AopsRecord::editorId));
    addModel(new IdTable(&aopsCollection), CkId::Type_Aops_);
    // AORU - Audio Reverb Unit
    aoruCollection.addColumn(new StringIdColumn<AoruRecord>());
    aoruCollection.addColumn(new RecordStateColumn<AoruRecord>());
    aoruCollection.addColumn(new StringColumn<AoruRecord>("Name", &AoruRecord::editorId));
    addModel(new IdTable(&aoruCollection), CkId::Type_Aoru_);
    // ARMA - Armor Addon
    armaCollection.addColumn(new StringIdColumn<ArmaRecord>());
    armaCollection.addColumn(new RecordStateColumn<ArmaRecord>());
    armaCollection.addColumn(new StringColumn<ArmaRecord>("Name", &ArmaRecord::editorId));
    addModel(new IdTable(&armaCollection), CkId::Type_Arma_);
    // ARTO - Art Object 2
    artoCollection.addColumn(new StringIdColumn<ArtoRecord>());
    artoCollection.addColumn(new RecordStateColumn<ArtoRecord>());
    artoCollection.addColumn(new StringColumn<ArtoRecord>("Name", &ArtoRecord::editorId));
    addModel(new IdTable(&artoCollection), CkId::Type_Arto_);
    // ASPC - Acoustic Space
    aspcCollection.addColumn(new StringIdColumn<AspcRecord>());
    aspcCollection.addColumn(new RecordStateColumn<AspcRecord>());
    aspcCollection.addColumn(new StringColumn<AspcRecord>("Name", &AspcRecord::editorId));
    addModel(new IdTable(&aspcCollection), CkId::Type_Aspc_);
    // ATMO - Atmosphere
    atmoCollection.addColumn(new StringIdColumn<AtmoRecord>());
    atmoCollection.addColumn(new RecordStateColumn<AtmoRecord>());
    atmoCollection.addColumn(new StringColumn<AtmoRecord>("Name", &AtmoRecord::editorId));
    addModel(new IdTable(&atmoCollection), CkId::Type_Atmo_);
    // AVMD - Audio Visual Data
    avmdCollection.addColumn(new StringIdColumn<AvmdRecord>());
    avmdCollection.addColumn(new RecordStateColumn<AvmdRecord>());
    avmdCollection.addColumn(new StringColumn<AvmdRecord>("Name", &AvmdRecord::editorId));
    addModel(new IdTable(&avmdCollection), CkId::Type_Avmd_);
    // BIOM - Biome
    biomCollection.addColumn(new StringIdColumn<BiomRecord>());
    biomCollection.addColumn(new RecordStateColumn<BiomRecord>());
    biomCollection.addColumn(new StringColumn<BiomRecord>("Name", &BiomRecord::editorId));
    addModel(new IdTable(&biomCollection), CkId::Type_Biom_);
    // BMMO - Behavior
    bmmoCollection.addColumn(new StringIdColumn<BmmoRecord>());
    bmmoCollection.addColumn(new RecordStateColumn<BmmoRecord>());
    bmmoCollection.addColumn(new StringColumn<BmmoRecord>("Name", &BmmoRecord::editorId));
    addModel(new IdTable(&bmmoCollection), CkId::Type_Bmmo_);
    // BMOD - Behavior Mod
    bmodCollection.addColumn(new StringIdColumn<BmodRecord>());
    bmodCollection.addColumn(new RecordStateColumn<BmodRecord>());
    bmodCollection.addColumn(new StringColumn<BmodRecord>("Name", &BmodRecord::editorId));
    addModel(new IdTable(&bmodCollection), CkId::Type_Bmod_);
    // BNDS - Bounds
    bndsCollection.addColumn(new StringIdColumn<BndsRecord>());
    bndsCollection.addColumn(new RecordStateColumn<BndsRecord>());
    bndsCollection.addColumn(new StringColumn<BndsRecord>("Name", &BndsRecord::editorId));
    addModel(new IdTable(&bndsCollection), CkId::Type_Bnds_);
    // BPTD - Behavior Pattern
    bptdCollection.addColumn(new StringIdColumn<BptdRecord>());
    bptdCollection.addColumn(new RecordStateColumn<BptdRecord>());
    bptdCollection.addColumn(new StringColumn<BptdRecord>("Name", &BptdRecord::editorId));
    addModel(new IdTable(&bptdCollection), CkId::Type_Bptd_);
    // CAMS - Camera Shot
    camsCollection.addColumn(new StringIdColumn<CamsRecord>());
    camsCollection.addColumn(new RecordStateColumn<CamsRecord>());
    camsCollection.addColumn(new StringColumn<CamsRecord>("Name", &CamsRecord::editorId));
    addModel(new IdTable(&camsCollection), CkId::Type_Cams_);
    // CHAL - Challenge
    chalCollection.addColumn(new StringIdColumn<ChalRecord>());
    chalCollection.addColumn(new RecordStateColumn<ChalRecord>());
    chalCollection.addColumn(new StringColumn<ChalRecord>("Name", &ChalRecord::editorId));
    addModel(new IdTable(&chalCollection), CkId::Type_Chal_);
    // CLDF - Character Float Table
    cldfCollection.addColumn(new StringIdColumn<CldfRecord>());
    cldfCollection.addColumn(new RecordStateColumn<CldfRecord>());
    cldfCollection.addColumn(new StringColumn<CldfRecord>("Name", &CldfRecord::editorId));
    addModel(new IdTable(&cldfCollection), CkId::Type_Cldf_);
    // CNDF - Character Data
    cndfCollection.addColumn(new StringIdColumn<CndfRecord>());
    cndfCollection.addColumn(new RecordStateColumn<CndfRecord>());
    cndfCollection.addColumn(new StringColumn<CndfRecord>("Name", &CndfRecord::editorId));
    addModel(new IdTable(&cndfCollection), CkId::Type_Cndf_);
    // COLL - Collision
    collCollection.addColumn(new StringIdColumn<CollRecord>());
    collCollection.addColumn(new RecordStateColumn<CollRecord>());
    collCollection.addColumn(new StringColumn<CollRecord>("Name", &CollRecord::editorId));
    addModel(new IdTable(&collCollection), CkId::Type_Coll_);
    // CPTH - Camera Path
    cpthCollection.addColumn(new StringIdColumn<CpthRecord>());
    cpthCollection.addColumn(new RecordStateColumn<CpthRecord>());
    cpthCollection.addColumn(new StringColumn<CpthRecord>("Name", &CpthRecord::editorId));
    addModel(new IdTable(&cpthCollection), CkId::Type_Cpth_);
    // DLBR - Culling Data
    dlbrCollection.addColumn(new StringIdColumn<DlbrRecord>());
    dlbrCollection.addColumn(new RecordStateColumn<DlbrRecord>());
    dlbrCollection.addColumn(new StringColumn<DlbrRecord>("Name", &DlbrRecord::editorId));
    addModel(new IdTable(&dlbrCollection), CkId::Type_Dlbr_);
    // CUR3 - Curve Table 3
    cur3Collection.addColumn(new StringIdColumn<Cur3Record>());
    cur3Collection.addColumn(new RecordStateColumn<Cur3Record>());
    cur3Collection.addColumn(new StringColumn<Cur3Record>("Name", &Cur3Record::editorId));
    addModel(new IdTable(&cur3Collection), CkId::Type_Cur3_);
    // CURV - Curve Table
    curvCollection.addColumn(new StringIdColumn<CurvRecord>());
    curvCollection.addColumn(new RecordStateColumn<CurvRecord>());
    curvCollection.addColumn(new StringColumn<CurvRecord>("Name", &CurvRecord::editorId));
    addModel(new IdTable(&curvCollection), CkId::Type_Curv_);
    // DFOB - Debris Object
    dfobCollection.addColumn(new StringIdColumn<DfobRecord>());
    dfobCollection.addColumn(new RecordStateColumn<DfobRecord>());
    dfobCollection.addColumn(new StringColumn<DfobRecord>("Name", &DfobRecord::editorId));
    addModel(new IdTable(&dfobCollection), CkId::Type_Dfob_);
    // DMGT - Damage Type
    dmgtCollection.addColumn(new StringIdColumn<DmgtRecord>());
    dmgtCollection.addColumn(new RecordStateColumn<DmgtRecord>());
    dmgtCollection.addColumn(new StringColumn<DmgtRecord>("Name", &DmgtRecord::editorId));
    addModel(new IdTable(&dmgtCollection), CkId::Type_Dmgt_);
    // DOBJ - Default Object
    dobjCollection.addColumn(new StringIdColumn<DobjRecord>());
    dobjCollection.addColumn(new RecordStateColumn<DobjRecord>());
    dobjCollection.addColumn(new StringColumn<DobjRecord>("Name", &DobjRecord::editorId));
    addModel(new IdTable(&dobjCollection), CkId::Type_Dobj_);
    // EFSQ - Effect Shader 2
    efsqCollection.addColumn(new StringIdColumn<EfsqRecord>());
    efsqCollection.addColumn(new RecordStateColumn<EfsqRecord>());
    efsqCollection.addColumn(new StringColumn<EfsqRecord>("Name", &EfsqRecord::editorId));
    addModel(new IdTable(&efsqCollection), CkId::Type_Efsq_);
    // EQUP - Equipment Slot
    equpCollection.addColumn(new StringIdColumn<EqupRecord>());
    equpCollection.addColumn(new RecordStateColumn<EqupRecord>());
    equpCollection.addColumn(new StringColumn<EqupRecord>("Name", &EqupRecord::editorId));
    addModel(new IdTable(&equpCollection), CkId::Type_Equp_);
    // FFKW - Furnishing Keyword
    ffkwCollection.addColumn(new StringIdColumn<FfkwRecord>());
    ffkwCollection.addColumn(new RecordStateColumn<FfkwRecord>());
    ffkwCollection.addColumn(new StringColumn<FfkwRecord>("Name", &FfkwRecord::editorId));
    addModel(new IdTable(&ffkwCollection), CkId::Type_Ffkw_);
    // FOGV - Fog Volume
    fogvCollection.addColumn(new StringIdColumn<FogvRecord>());
    fogvCollection.addColumn(new RecordStateColumn<FogvRecord>());
    fogvCollection.addColumn(new StringColumn<FogvRecord>("Name", &FogvRecord::editorId));
    addModel(new IdTable(&fogvCollection), CkId::Type_Fogv_);
    // FORC - Force Field
    forcCollection.addColumn(new StringIdColumn<ForcRecord>());
    forcCollection.addColumn(new RecordStateColumn<ForcRecord>());
    forcCollection.addColumn(new StringColumn<ForcRecord>("Name", &ForcRecord::editorId));
    addModel(new IdTable(&forcCollection), CkId::Type_Forc_);
    // FSTP - Footstep Set
    fstpCollection.addColumn(new StringIdColumn<FstpRecord>());
    fstpCollection.addColumn(new RecordStateColumn<FstpRecord>());
    fstpCollection.addColumn(new StringColumn<FstpRecord>("Name", &FstpRecord::editorId));
    addModel(new IdTable(&fstpCollection), CkId::Type_Fstp_);
    // FSTS - Footstep Sound
    fstsCollection.addColumn(new StringIdColumn<FstsRecord>());
    fstsCollection.addColumn(new RecordStateColumn<FstsRecord>());
    fstsCollection.addColumn(new StringColumn<FstsRecord>("Name", &FstsRecord::editorId));
    addModel(new IdTable(&fstsCollection), CkId::Type_Fsts_);
    // FXPD - FX Particle
    fxpdCollection.addColumn(new StringIdColumn<FxpdRecord>());
    fxpdCollection.addColumn(new RecordStateColumn<FxpdRecord>());
    fxpdCollection.addColumn(new StringColumn<FxpdRecord>("Name", &FxpdRecord::editorId));
    addModel(new IdTable(&fxpdCollection), CkId::Type_Fxpd_);
    // GBFM - Biome Mask
    gbfmCollection.addColumn(new StringIdColumn<GbfmRecord>());
    gbfmCollection.addColumn(new RecordStateColumn<GbfmRecord>());
    gbfmCollection.addColumn(new StringColumn<GbfmRecord>("Name", &GbfmRecord::editorId));
    addModel(new IdTable(&gbfmCollection), CkId::Type_Gbfm_);
    // GBFT - Biome Type
    gbftCollection.addColumn(new StringIdColumn<GbftRecord>());
    gbftCollection.addColumn(new RecordStateColumn<GbftRecord>());
    gbftCollection.addColumn(new StringColumn<GbftRecord>("Name", &GbftRecord::editorId));
    addModel(new IdTable(&gbftCollection), CkId::Type_Gbft_);
    // GCVR - Gravity Camera
    gcvrCollection.addColumn(new StringIdColumn<GcvrRecord>());
    gcvrCollection.addColumn(new RecordStateColumn<GcvrRecord>());
    gcvrCollection.addColumn(new StringColumn<GcvrRecord>("Name", &GcvrRecord::editorId));
    addModel(new IdTable(&gcvrCollection), CkId::Type_Gcvr_);
    // IMAD - Image Space Adapter
    imadCollection.addColumn(new StringIdColumn<ImadRecord>());
    imadCollection.addColumn(new RecordStateColumn<ImadRecord>());
    imadCollection.addColumn(new StringColumn<ImadRecord>("Name", &ImadRecord::editorId));
    addModel(new IdTable(&imadCollection), CkId::Type_Imad_);
    // INNR - Interior Data
    innrCollection.addColumn(new StringIdColumn<InnrRecord>());
    innrCollection.addColumn(new RecordStateColumn<InnrRecord>());
    innrCollection.addColumn(new StringColumn<InnrRecord>("Name", &InnrRecord::editorId));
    addModel(new IdTable(&innrCollection), CkId::Type_Innr_);
    // IRES - Resource Property
    iresCollection.addColumn(new StringIdColumn<IresRecord>());
    iresCollection.addColumn(new RecordStateColumn<IresRecord>());
    iresCollection.addColumn(new StringColumn<IresRecord>("Name", &IresRecord::editorId));
    addModel(new IdTable(&iresCollection), CkId::Type_Ires_);
    // KSSM - Keyword Set
    kssmCollection.addColumn(new StringIdColumn<KssmRecord>());
    kssmCollection.addColumn(new RecordStateColumn<KssmRecord>());
    kssmCollection.addColumn(new StringColumn<KssmRecord>("Name", &KssmRecord::editorId));
    addModel(new IdTable(&kssmCollection), CkId::Type_Kssm_);
    // LAYR - Layer
    layrCollection.addColumn(new StringIdColumn<LayrRecord>());
    layrCollection.addColumn(new RecordStateColumn<LayrRecord>());
    layrCollection.addColumn(new StringColumn<LayrRecord>("Name", &LayrRecord::editorId));
    addModel(new IdTable(&layrCollection), CkId::Type_Layr_);
    // LENS - Lens
    lensCollection.addColumn(new StringIdColumn<LensRecord>());
    lensCollection.addColumn(new RecordStateColumn<LensRecord>());
    lensCollection.addColumn(new StringColumn<LensRecord>("Name", &LensRecord::editorId));
    addModel(new IdTable(&lensCollection), CkId::Type_Lens_);
    // LGDI - Landscape Grid Data
    lgdiCollection.addColumn(new StringIdColumn<LgdiRecord>());
    lgdiCollection.addColumn(new RecordStateColumn<LgdiRecord>());
    lgdiCollection.addColumn(new StringColumn<LgdiRecord>("Name", &LgdiRecord::editorId));
    addModel(new IdTable(&lgdiCollection), CkId::Type_Lgdi_);
    // LGTM - Light Template
    lgtmCollection.addColumn(new StringIdColumn<LgtmRecord>());
    lgtmCollection.addColumn(new RecordStateColumn<LgtmRecord>());
    lgtmCollection.addColumn(new StringColumn<LgtmRecord>("Name", &LgtmRecord::editorId));
    addModel(new IdTable(&lgtmCollection), CkId::Type_Lgtm_);
    // LMSW - Lens Modifier Set
    lmswCollection.addColumn(new StringIdColumn<LmswRecord>());
    lmswCollection.addColumn(new RecordStateColumn<LmswRecord>());
    lmswCollection.addColumn(new StringColumn<LmswRecord>("Name", &LmswRecord::editorId));
    addModel(new IdTable(&lmswCollection), CkId::Type_Lmsw_);
    // LVLB - Leveled Biome
    lvlbCollection.addColumn(new StringIdColumn<LvlbRecord>());
    lvlbCollection.addColumn(new RecordStateColumn<LvlbRecord>());
    lvlbCollection.addColumn(new StringColumn<LvlbRecord>("Name", &LvlbRecord::editorId));
    addModel(new IdTable(&lvlbCollection), CkId::Type_Lvlb_);
    // LVLN - Leveled Node
    lvlnCollection.addColumn(new StringIdColumn<LvlnRecord>());
    lvlnCollection.addColumn(new RecordStateColumn<LvlnRecord>());
    lvlnCollection.addColumn(new StringColumn<LvlnRecord>("Name", &LvlnRecord::editorId));
    addModel(new IdTable(&lvlnCollection), CkId::Type_Lvln_);
    // LVLP - Leveled Perk
    lvlpCollection.addColumn(new StringIdColumn<LvlpRecord>());
    lvlpCollection.addColumn(new RecordStateColumn<LvlpRecord>());
    lvlpCollection.addColumn(new StringColumn<LvlpRecord>("Name", &LvlpRecord::editorId));
    addModel(new IdTable(&lvlpCollection), CkId::Type_Lvlp_);
    // LVSC - Leveled Structure
    lvscCollection.addColumn(new StringIdColumn<LvscRecord>());
    lvscCollection.addColumn(new RecordStateColumn<LvscRecord>());
    lvscCollection.addColumn(new StringColumn<LvscRecord>("Name", &LvscRecord::editorId));
    addModel(new IdTable(&lvscCollection), CkId::Type_Lvsc_);
    // MAAM - Material Attachment
    maamCollection.addColumn(new StringIdColumn<MaamRecord>());
    maamCollection.addColumn(new RecordStateColumn<MaamRecord>());
    maamCollection.addColumn(new StringColumn<MaamRecord>("Name", &MaamRecord::editorId));
    addModel(new IdTable(&maamCollection), CkId::Type_Maam_);
    // MRPH - Morph
    mrhpCollection.addColumn(new StringIdColumn<MrhpRecord>());
    mrhpCollection.addColumn(new RecordStateColumn<MrhpRecord>());
    mrhpCollection.addColumn(new StringColumn<MrhpRecord>("Name", &MrhpRecord::editorId));
    addModel(new IdTable(&mrhpCollection), CkId::Type_Mrhp_);
    // MTPT - Mount Point
    mtptCollection.addColumn(new StringIdColumn<MtptRecord>());
    mtptCollection.addColumn(new RecordStateColumn<MtptRecord>());
    mtptCollection.addColumn(new StringColumn<MtptRecord>("Name", &MtptRecord::editorId));
    addModel(new IdTable(&mtptCollection), CkId::Type_Mtpt_);
    // NAVI - Navigation Island
    naviCollection.addColumn(new StringIdColumn<NaviRecord>());
    naviCollection.addColumn(new RecordStateColumn<NaviRecord>());
    naviCollection.addColumn(new StringColumn<NaviRecord>("Name", &NaviRecord::editorId));
    addModel(new IdTable(&naviCollection), CkId::Type_Navi_);
    // NOCM - Navigation Component
    nocmCollection.addColumn(new StringIdColumn<NocmRecord>());
    nocmCollection.addColumn(new RecordStateColumn<NocmRecord>());
    nocmCollection.addColumn(new StringColumn<NocmRecord>("Name", &NocmRecord::editorId));
    addModel(new IdTable(&nocmCollection), CkId::Type_Nocm_);
    // OMOD - Object Mod
    omodCollection.addColumn(new StringIdColumn<OmodRecord>());
    omodCollection.addColumn(new RecordStateColumn<OmodRecord>());
    omodCollection.addColumn(new StringColumn<OmodRecord>("Name", &OmodRecord::editorId));
    addModel(new IdTable(&omodCollection), CkId::Type_Omod_);
    // OSWP - Object Swap Set
    oswpCollection.addColumn(new StringIdColumn<OswpRecord>());
    oswpCollection.addColumn(new RecordStateColumn<OswpRecord>());
    oswpCollection.addColumn(new StringColumn<OswpRecord>("Name", &OswpRecord::editorId));
    addModel(new IdTable(&oswpCollection), CkId::Type_Oswp_);
    // OVIS - Object Visual
    ovisCollection.addColumn(new StringIdColumn<OvisRecord>());
    ovisCollection.addColumn(new RecordStateColumn<OvisRecord>());
    ovisCollection.addColumn(new StringColumn<OvisRecord>("Name", &OvisRecord::editorId));
    addModel(new IdTable(&ovisCollection), CkId::Type_Ovis_);
    // PCBN - Placement Configuration
    pcbnCollection.addColumn(new StringIdColumn<PcbnRecord>());
    pcbnCollection.addColumn(new RecordStateColumn<PcbnRecord>());
    pcbnCollection.addColumn(new StringColumn<PcbnRecord>("Name", &PcbnRecord::editorId));
    addModel(new IdTable(&pcbnCollection), CkId::Type_Pcbn_);
    // PCCN - Placement Collision Node
    pccnCollection.addColumn(new StringIdColumn<PccnRecord>());
    pccnCollection.addColumn(new RecordStateColumn<PccnRecord>());
    pccnCollection.addColumn(new StringColumn<PccnRecord>("Name", &PccnRecord::editorId));
    addModel(new IdTable(&pccnCollection), CkId::Type_Pccn_);
    // PCMT - Placement Material
    pcmtCollection.addColumn(new StringIdColumn<PcmtRecord>());
    pcmtCollection.addColumn(new RecordStateColumn<PcmtRecord>());
    pcmtCollection.addColumn(new StringColumn<PcmtRecord>("Name", &PcmtRecord::editorId));
    addModel(new IdTable(&pcmtCollection), CkId::Type_Pcmt_);
    // PDCL - Particle Decal
    pdclCollection.addColumn(new StringIdColumn<PdclRecord>());
    pdclCollection.addColumn(new RecordStateColumn<PdclRecord>());
    pdclCollection.addColumn(new StringColumn<PdclRecord>("Name", &PdclRecord::editorId));
    addModel(new IdTable(&pdclCollection), CkId::Type_Pdcl_);
    // PGRE - Particle Emitter
    pgreCollection.addColumn(new StringIdColumn<PgreRecord>());
    pgreCollection.addColumn(new RecordStateColumn<PgreRecord>());
    pgreCollection.addColumn(new StringColumn<PgreRecord>("Name", &PgreRecord::editorId));
    addModel(new IdTable(&pgreCollection), CkId::Type_Pgre_);
    // PHZD
    phzdCollection.addColumn(new StringIdColumn<PhzdRecord>());
    phzdCollection.addColumn(new RecordStateColumn<PhzdRecord>());
    phzdCollection.addColumn(new StringColumn<PhzdRecord>("Name", &PhzdRecord::editorId));
    addModel(new IdTable(&phzdCollection), CkId::Type_Phzd_);

    // PKIN
    pkinCollection.addColumn(new StringIdColumn<PkinRecord>());
    pkinCollection.addColumn(new RecordStateColumn<PkinRecord>());
    pkinCollection.addColumn(new StringColumn<PkinRecord>("Name", &PkinRecord::editorId));
    addModel(new IdTable(&pkinCollection), CkId::Type_Pkin_);

    // PMFT
    pmftCollection.addColumn(new StringIdColumn<PmftRecord>());
    pmftCollection.addColumn(new RecordStateColumn<PmftRecord>());
    pmftCollection.addColumn(new StringColumn<PmftRecord>("Name", &PmftRecord::editorId));
    addModel(new IdTable(&pmftCollection), CkId::Type_Pmft_);

    // PSDC
    psdcCollection.addColumn(new StringIdColumn<PsdcRecord>());
    psdcCollection.addColumn(new RecordStateColumn<PsdcRecord>());
    psdcCollection.addColumn(new StringColumn<PsdcRecord>("Name", &PsdcRecord::editorId));
    addModel(new IdTable(&psdcCollection), CkId::Type_Psdc_);

    // PTST
    ptstCollection.addColumn(new StringIdColumn<PtstRecord>());
    ptstCollection.addColumn(new RecordStateColumn<PtstRecord>());
    ptstCollection.addColumn(new StringColumn<PtstRecord>("Name", &PtstRecord::editorId));
    addModel(new IdTable(&ptstCollection), CkId::Type_Ptst_);

    // RFGP
    rfgpCollection.addColumn(new StringIdColumn<RfgpRecord>());
    rfgpCollection.addColumn(new RecordStateColumn<RfgpRecord>());
    rfgpCollection.addColumn(new StringColumn<RfgpRecord>("Name", &RfgpRecord::editorId));
    addModel(new IdTable(&rfgpCollection), CkId::Type_Rfgp_);

    // RSGD
    rsgdCollection.addColumn(new StringIdColumn<RsgdRecord>());
    rsgdCollection.addColumn(new RecordStateColumn<RsgdRecord>());
    rsgdCollection.addColumn(new StringColumn<RsgdRecord>("Name", &RsgdRecord::editorId));
    addModel(new IdTable(&rsgdCollection), CkId::Type_Rsgd_);

    // RSPJ
    rspjCollection.addColumn(new StringIdColumn<RspjRecord>());
    rspjCollection.addColumn(new RecordStateColumn<RspjRecord>());
    rspjCollection.addColumn(new StringColumn<RspjRecord>("Name", &RspjRecord::editorId));
    addModel(new IdTable(&rspjCollection), CkId::Type_Rspj_);

    // SDLT
    sdltCollection.addColumn(new StringIdColumn<SdltRecord>());
    sdltCollection.addColumn(new RecordStateColumn<SdltRecord>());
    sdltCollection.addColumn(new StringColumn<SdltRecord>("Name", &SdltRecord::editorId));
    addModel(new IdTable(&sdltCollection), CkId::Type_Sdlt_);

    // SECH
    sechCollection.addColumn(new StringIdColumn<SechRecord>());
    sechCollection.addColumn(new RecordStateColumn<SechRecord>());
    sechCollection.addColumn(new StringColumn<SechRecord>("Name", &SechRecord::editorId));
    addModel(new IdTable(&sechCollection), CkId::Type_Sech_);

    // SFBK
    sfbkCollection.addColumn(new StringIdColumn<SfbkRecord>());
    sfbkCollection.addColumn(new RecordStateColumn<SfbkRecord>());
    sfbkCollection.addColumn(new StringColumn<SfbkRecord>("Name", &SfbkRecord::editorId));
    addModel(new IdTable(&sfbkCollection), CkId::Type_Sfbk_);

    // SFPC
    sfpcCollection.addColumn(new StringIdColumn<SfpcRecord>());
    sfpcCollection.addColumn(new RecordStateColumn<SfpcRecord>());
    sfpcCollection.addColumn(new StringColumn<SfpcRecord>("Name", &SfpcRecord::editorId));
    addModel(new IdTable(&sfpcCollection), CkId::Type_Sfpc_);

    // SFPT
    sfptCollection.addColumn(new StringIdColumn<SfptRecord>());
    sfptCollection.addColumn(new RecordStateColumn<SfptRecord>());
    sfptCollection.addColumn(new StringColumn<SfptRecord>("Name", &SfptRecord::editorId));
    addModel(new IdTable(&sfptCollection), CkId::Type_Sfpt_);

    // SFTR
    sftrCollection.addColumn(new StringIdColumn<SftrRecord>());
    sftrCollection.addColumn(new RecordStateColumn<SftrRecord>());
    sftrCollection.addColumn(new StringColumn<SftrRecord>("Name", &SftrRecord::editorId));
    addModel(new IdTable(&sftrCollection), CkId::Type_Sftr_);

    // SMBN
    smbnCollection.addColumn(new StringIdColumn<SmbnRecord>());
    smbnCollection.addColumn(new RecordStateColumn<SmbnRecord>());
    smbnCollection.addColumn(new StringColumn<SmbnRecord>("Name", &SmbnRecord::editorId));
    addModel(new IdTable(&smbnCollection), CkId::Type_Smbn_);

    // SMEN
    smenCollection.addColumn(new StringIdColumn<SmenRecord>());
    smenCollection.addColumn(new RecordStateColumn<SmenRecord>());
    smenCollection.addColumn(new StringColumn<SmenRecord>("Name", &SmenRecord::editorId));
    addModel(new IdTable(&smenCollection), CkId::Type_Smen_);

    // SPCH
    spchCollection.addColumn(new StringIdColumn<SpchRecord>());
    spchCollection.addColumn(new RecordStateColumn<SpchRecord>());
    spchCollection.addColumn(new StringColumn<SpchRecord>("Name", &SpchRecord::editorId));
    addModel(new IdTable(&spchCollection), CkId::Type_Spch_);

    // STAG
    stagCollection.addColumn(new StringIdColumn<StagRecord>());
    stagCollection.addColumn(new RecordStateColumn<StagRecord>());
    stagCollection.addColumn(new StringColumn<StagRecord>("Name", &StagRecord::editorId));
    addModel(new IdTable(&stagCollection), CkId::Type_Stag_);

    // STBH
    stbhCollection.addColumn(new StringIdColumn<StbhRecord>());
    stbhCollection.addColumn(new RecordStateColumn<StbhRecord>());
    stbhCollection.addColumn(new StringColumn<StbhRecord>("Name", &StbhRecord::editorId));
    addModel(new IdTable(&stbhCollection), CkId::Type_Stbh_);

    // STDT
    stdtCollection.addColumn(new StringIdColumn<StdtRecord>());
    stdtCollection.addColumn(new RecordStateColumn<StdtRecord>());
    stdtCollection.addColumn(new StringColumn<StdtRecord>("Name", &StdtRecord::editorId));
    addModel(new IdTable(&stdtCollection), CkId::Type_Stdt_);

    // STMP
    stmpCollection.addColumn(new StringIdColumn<StmpRecord>());
    stmpCollection.addColumn(new RecordStateColumn<StmpRecord>());
    stmpCollection.addColumn(new StringColumn<StmpRecord>("Name", &StmpRecord::editorId));
    addModel(new IdTable(&stmpCollection), CkId::Type_Stmp_);

    // STND
    stndCollection.addColumn(new StringIdColumn<StndRecord>());
    stndCollection.addColumn(new RecordStateColumn<StndRecord>());
    stndCollection.addColumn(new StringColumn<StndRecord>("Name", &StndRecord::editorId));
    addModel(new IdTable(&stndCollection), CkId::Type_Stnd_);

    // SUNP
    sunpCollection.addColumn(new StringIdColumn<SunpRecord>());
    sunpCollection.addColumn(new RecordStateColumn<SunpRecord>());
    sunpCollection.addColumn(new StringColumn<SunpRecord>("Name", &SunpRecord::editorId));
    addModel(new IdTable(&sunpCollection), CkId::Type_Sunp_);

    // TMLM
    tmlmCollection.addColumn(new StringIdColumn<TmlmRecord>());
    tmlmCollection.addColumn(new RecordStateColumn<TmlmRecord>());
    tmlmCollection.addColumn(new StringColumn<TmlmRecord>("Name", &TmlmRecord::editorId));
    addModel(new IdTable(&tmlmCollection), CkId::Type_Tmlm_);

    // TODD
    toddCollection.addColumn(new StringIdColumn<ToddRecord>());
    toddCollection.addColumn(new RecordStateColumn<ToddRecord>());
    toddCollection.addColumn(new StringColumn<ToddRecord>("Name", &ToddRecord::editorId));
    addModel(new IdTable(&toddCollection), CkId::Type_Todd_);

    // TRAV
    travCollection.addColumn(new StringIdColumn<TravRecord>());
    travCollection.addColumn(new RecordStateColumn<TravRecord>());
    travCollection.addColumn(new StringColumn<TravRecord>("Name", &TravRecord::editorId));
    addModel(new IdTable(&travCollection), CkId::Type_Trav_);

    // TRNS
    trnsCollection.addColumn(new StringIdColumn<TrnsRecord>());
    trnsCollection.addColumn(new RecordStateColumn<TrnsRecord>());
    trnsCollection.addColumn(new StringColumn<TrnsRecord>("Name", &TrnsRecord::editorId));
    addModel(new IdTable(&trnsCollection), CkId::Type_Trns_);

    // VOLI
    voliCollection.addColumn(new StringIdColumn<VoliRecord>());
    voliCollection.addColumn(new RecordStateColumn<VoliRecord>());
    voliCollection.addColumn(new StringColumn<VoliRecord>("Name", &VoliRecord::editorId));
    addModel(new IdTable(&voliCollection), CkId::Type_Voli_);

    // VTYP
    vtypCollection.addColumn(new StringIdColumn<VtypRecord>());
    vtypCollection.addColumn(new RecordStateColumn<VtypRecord>());
    vtypCollection.addColumn(new StringColumn<VtypRecord>("Name", &VtypRecord::editorId));
    addModel(new IdTable(&vtypCollection), CkId::Type_Vtyp_);

    // WBAR
    wbarCollection.addColumn(new StringIdColumn<WbarRecord>());
    wbarCollection.addColumn(new RecordStateColumn<WbarRecord>());
    wbarCollection.addColumn(new StringColumn<WbarRecord>("Name", &WbarRecord::editorId));
    addModel(new IdTable(&wbarCollection), CkId::Type_Wbar_);

    // WKMF
    wkmfCollection.addColumn(new StringIdColumn<WkmfRecord>());
    wkmfCollection.addColumn(new RecordStateColumn<WkmfRecord>());
    wkmfCollection.addColumn(new StringColumn<WkmfRecord>("Name", &WkmfRecord::editorId));
    addModel(new IdTable(&wkmfCollection), CkId::Type_Wkmf_);

    // WTHS
    wthsCollection.addColumn(new StringIdColumn<WthsRecord>());
    wthsCollection.addColumn(new RecordStateColumn<WthsRecord>());
    wthsCollection.addColumn(new StringColumn<WthsRecord>("Name", &WthsRecord::editorId));
    addModel(new IdTable(&wthsCollection), CkId::Type_Wths_);

    // WWED
    wwedCollection.addColumn(new StringIdColumn<WwedRecord>());
    wwedCollection.addColumn(new RecordStateColumn<WwedRecord>());
    wwedCollection.addColumn(new StringColumn<WwedRecord>("Name", &WwedRecord::editorId));
    addModel(new IdTable(&wwedCollection), CkId::Type_Wwed_);

    // ZOOM
    zoomCollection.addColumn(new StringIdColumn<ZoomRecord>());
    zoomCollection.addColumn(new RecordStateColumn<ZoomRecord>());
    zoomCollection.addColumn(new StringColumn<ZoomRecord>("Name", &ZoomRecord::editorId));
    addModel(new IdTable(&zoomCollection), CkId::Type_Zoom_);

    // Loading Log (metadata)
    metaData.addColumn(new StringIdColumn<MetaData>());
    metaData.addColumn(new RecordStateColumn<MetaData>());
    metaData.addColumn(new StringColumn<MetaData>("Author", &MetaData::author));
    metaData.addColumn(new StringColumn<MetaData>("Description", &MetaData::description));
    addModel(new IdTable(&metaData), CkId::Type_LoadingLog);
}

Data::~Data()
{
    for (auto* model : models)
    {
        delete model;
    }
    models.clear();

    for (auto stack : mPluginUndoStacks)
    {
        delete stack;
    }
    delete mUndoStack;
}

int Data::preload(const QString& filename, bool base_)
{
    LOG_INFO(QString("Data::preload: filename='%1' base=%2 length=%3")
        .arg(filename).arg(base_ ? "true" : "false").arg(filename.length()));
    // Hex dump of the filename to find hidden characters
    QString hex;
    for (int i = 0; i < filename.length(); i++) {
        hex += QString("%1 ").arg(filename[i].unicode(), 4, 16, QChar('0'));
    }
    LOG_INFO(QString("Data::preload: filename hex=[%1]").arg(hex));
    QString fullPath = paths.dataDir.path() + "/" + filename;
    // Strip trailing whitespace/nulls from fullPath to avoid "Broken filename" errors
    while (!fullPath.isEmpty() && (fullPath.at(fullPath.length()-1) < QChar(0x20) || fullPath.at(fullPath.length()-1) == QChar(0x7F))) {
        fullPath.chop(1);
    }
    LOG_INFO(QString("Data::preload: fullPath='%1' (length=%2)")
        .arg(fullPath).arg(fullPath.length()));
    QString hexPath;
    for (int i = 0; i < fullPath.length(); i++) {
        hexPath += QString("%1 ").arg(fullPath[i].unicode(), 4, 16, QChar('0'));
    }
    LOG_INFO(QString("Data::preload: fullPath hex=[%1]").arg(hexPath));
    reader.reset(new ESMReader(fullPath, paths));
    reader->open();
    base = base_;
    m_lastPreloadPath = fullPath;

    if (!base)
    {
        MetaData metaData_;
        metaData_.editorId = "esm::metadata";
        metaData_.load(*reader);

        metaData.appendRecord(Record<MetaData>(State::State_ModifiedOnly, 0, &metaData_));
    }
    else
    {
        // Masters are indexed (headers only) instead of parsed. The Loader
        // skips the eager record pass for them; records materialize on
        // demand via ensureTypeLoaded(). This is what makes opening a
        // plugin instant even when its master is a multi-GB game file.
        m_deferredMasterFiles.append(fullPath);
        const int masterFileIdx = m_deferredMasterFiles.size() - 1;

        QElapsedTimer t;
        t.start();
        QVector<RecordIndexEntry> fileIndex;
        reader->buildRecordIndex(fileIndex);
        for (const RecordIndexEntry& entry : fileIndex)
        {
            m_masterIndex.push_back({ entry.type, entry.formId, entry.offset, masterFileIdx });
        }
        LOG_INFO(QString("Data::preload: indexed master %1: %2 records in %3 ms")
            .arg(filename)
            .arg(fileIndex.size())
            .arg(t.elapsed()));
    }

    return reader->recordCount();
}

bool Data::continueLoading(Messages& messages)
{
    if (!reader->isLeft())
    {
        return true;
    }
    else
    {
        NAME name = 0;
        qint64 pos = reader->filePos();

#ifdef _WIN32
        _set_se_translator(seh_translator);
#endif
        try
        {
            // Peek the next 4 bytes to detect compression before reading
            quint32 firstBytes = reader->peekType<quint32>();
            LOG_DEBUG(QString("continueLoading: filePos=0x%1 peek=0x%2")
                .arg(pos, 0, 16)
                .arg(firstBytes, 8, 16, QChar('0')));
            name = reader->readName();

            LOG_DEBUG(QString("Loading record type: %1%2%3%4")
                .arg(QChar(static_cast<char>((name >> 24) & 0xFF)))
                .arg(QChar(static_cast<char>((name >> 16) & 0xFF)))
                .arg(QChar(static_cast<char>((name >> 8) & 0xFF)))
                .arg(QChar(static_cast<char>(name & 0xFF))));

            switch (name)
            {
            case 'GRUP': reader->skipGrupHeader();              break;
            case 'GMST': gameSettings.load(*reader, base);     break;
            case 'NPC_': npcCollection.load(*reader, base);    break;
            case 'WEAP': weaponCollection.load(*reader, base); break;
            case 'ARMO': armorCollection.load(*reader, base); break;
            case 'SPEL': spellCollection.load(*reader, base);  break;
            case 'MGEF': magicCollection.load(*reader, base); break;
            case 'QUST': questCollection.load(*reader, base); break;
            case 'DIAL': dialCollection.load(*reader, base);   break;
            case 'INFO': infoCollection.load(*reader, base);   break;
            case 'GLOB': globCollection.load(*reader, base);   break;
            case 'LCRT': lcrtCollection.load(*reader, base);   break;
            case 'PACK': packCollection.load(*reader, base);   break;
            case 'TREE': treeCollection.load(*reader, base);   break;
            case 'ALCH': alchCollection.load(*reader, base);   break;
            case 'INGR': ingrCollection.load(*reader, base);   break;
            case 'CONT': contCollection.load(*reader, base);   break;
            case 'ENCH': enchCollection.load(*reader, base);   break;
            case 'BOOK': bookCollection.load(*reader, base);   break;
            case 'MISC': miscCollection.load(*reader, base);   break;
            case 'ACTI': actiCollection.load(*reader, base);   break;
            case 'STAT': statCollection.load(*reader, base);   break;
            case 'RACE': raceCollection.load(*reader, base);   break;
            case 'CLAS': classCollection.load(*reader, base); break;
            case 'FACT': factCollection.load(*reader, base);   break;
            case 'PERK': perkCollection.load(*reader, base);   break;
            case 'CELL': cellCollection.load(*reader, base);   break;
            case 'WRLD': worldspaceCollection.load(*reader, base); break;
            case 'LCTN': locationCollection.load(*reader, base); break;
            case 'PNDT': planetCollection.load(*reader, base); break;
            case 'REFR': refrCollection.load(*reader, base);   break;
            case 'MATL': materialCollection.load(*reader, base); break;
            case 'LAND': landCollection.load(*reader, base); break;
            case 'SOUN': sounCollection.load(*reader, base); break;
            case 'WTHR': wthrCollection.load(*reader, base); break;
            case 'LTEX': ltexCollection.load(*reader, base); break;
            case 'SCEN': scenCollection.load(*reader, base); break;
            case 'AMMO': ammoCollection.load(*reader, base); break;
            case 'APPA': appaCollection.load(*reader, base); break;
            case 'AVIF': avifCollection.load(*reader, base); break;
            case 'BSGN': bsgnCollection.load(*reader, base); break;
            case 'CLMT': clmtCollection.load(*reader, base); break;
            case 'CLOT': clotCollection.load(*reader, base); break;
            case 'COBJ': cobjCollection.load(*reader, base); break;
            case 'CREA': creatureCollection.load(*reader, base); break;
            case 'CSTY': cstyCollection.load(*reader, base); break;
            case 'DOOR': doorCollection.load(*reader, base); break;
            case 'EFSH': efshCollection.load(*reader, base); break;
            case 'EXPL': explCollection.load(*reader, base); break;
            case 'EYES': eyesCollection.load(*reader, base); break;
            case 'FLOR': florCollection.load(*reader, base); break;
            case 'FLST': flstCollection.load(*reader, base); break;
            case 'FURN': furnCollection.load(*reader, base); break;
            case 'GRAS': grassCollection.load(*reader, base); break;
            case 'HAIR': hairCollection.load(*reader, base); break;
            case 'IDLE': idleCollection.load(*reader, base); break;
            case 'IDLM': idlmCollection.load(*reader, base); break;
            case 'IMGS': imgsCollection.load(*reader, base); break;
            case 'KEYM': keymCollection.load(*reader, base); break;
            case 'KYWD': kywdCollection.load(*reader, base); break;
            case 'LIGH': lighCollection.load(*reader, base); break;
            case 'LSCR': lscrCollection.load(*reader, base); break;
            case 'LVLC': lvlcCollection.load(*reader, base); break;
            case 'LVLI': lvliCollection.load(*reader, base); break;
            case 'LVSP': lvspCollection.load(*reader, base); break;
            case 'MESG': mesgCollection.load(*reader, base); break;
            case 'MSTT': msttCollection.load(*reader, base); break;
            case 'NAVM': navmCollection.load(*reader, base); break;
            case 'NOTE': noteCollection.load(*reader, base); break;
            case 'OTFT': otftCollection.load(*reader, base); break;
            case 'PROJ': projCollection.load(*reader, base); break;
            case 'REGN': regnCollection.load(*reader, base); break;
            case 'ROAD': roadCollection.load(*reader, base); break;
            case 'SCPT': scptCollection.load(*reader, base); break;
            case 'SCRL': scrlCollection.load(*reader, base); break;
            case 'SLGM': slgmCollection.load(*reader, base); break;
            case 'SMQN': smqnCollection.load(*reader, base); break;
            case 'SPGD': spgdCollection.load(*reader, base); break;
            case 'SCOL': scolCollection.load(*reader, base); break;
            case 'TXST': txstCollection.load(*reader, base); break;
            case 'WATR': wateCollection.load(*reader, base); break;
    case 'ANIO': anioCollection.load(*reader, base); break;
    case 'ARTV': artvCollection.load(*reader, base); break;
    case 'CLFM': clfmCollection.load(*reader, base); break;
    case 'DEBR': debrCollection.load(*reader, base); break;
    case 'ECZN': ecznCollection.load(*reader, base); break;
    case 'HAZD': hazdCollection.load(*reader, base); break;
    case 'IPCT': ipctCollection.load(*reader, base); break;
    case 'IPDS': ipdsCollection.load(*reader, base); break;
    case 'MUST': mustCollection.load(*reader, base); break;
    case 'RELA': relaCollection.load(*reader, base); break;
    case 'REVB': revbCollection.load(*reader, base); break;
    case 'SHOU': shouCollection.load(*reader, base); break;
    case 'HDPT': hdptCollection.load(*reader, base); break;
    case 'TERM': termCollection.load(*reader, base); break;
    case 'MATT': mattCollection.load(*reader, base); break;
    case 'MOVT': movtCollection.load(*reader, base); break;
    case 'MUSC': muscCollection.load(*reader, base); break;
            case 'AACT': aactCollection.load(*reader, base); break;
            case 'AAMD': aamdCollection.load(*reader, base); break;
            case 'AAPD': aapdCollection.load(*reader, base); break;
            case 'ACHR': achrCollection.load(*reader, base); break;
            case 'ADDN': addnCollection.load(*reader, base); break;
            case 'AFFE': affeCollection.load(*reader, base); break;
            case 'AMBS': ambsCollection.load(*reader, base); break;
            case 'AMDL': amdlCollection.load(*reader, base); break;
            case 'AOPF': aopfCollection.load(*reader, base); break;
            case 'AOPS': aopsCollection.load(*reader, base); break;
            case 'AORU': aoruCollection.load(*reader, base); break;
            case 'ARMA': armaCollection.load(*reader, base); break;
            case 'ARTO': artoCollection.load(*reader, base); break;
            case 'ASPC': aspcCollection.load(*reader, base); break;
            case 'ATMO': atmoCollection.load(*reader, base); break;
            case 'AVMD': avmdCollection.load(*reader, base); break;
            case 'BIOM': biomCollection.load(*reader, base); break;
            case 'BMMO': bmmoCollection.load(*reader, base); break;
            case 'BMOD': bmodCollection.load(*reader, base); break;
            case 'BNDS': bndsCollection.load(*reader, base); break;
            case 'BPTD': bptdCollection.load(*reader, base); break;
            case 'CAMS': camsCollection.load(*reader, base); break;
            case 'CHAL': chalCollection.load(*reader, base); break;
            case 'CLDF': cldfCollection.load(*reader, base); break;
            case 'CNDF': cndfCollection.load(*reader, base); break;
            case 'COLL': collCollection.load(*reader, base); break;
            case 'CPTH': cpthCollection.load(*reader, base); break;
            case 'DLBR': dlbrCollection.load(*reader, base); break;
            case 'CUR3': cur3Collection.load(*reader, base); break;
            case 'CURV': curvCollection.load(*reader, base); break;
            case 'DFOB': dfobCollection.load(*reader, base); break;
            case 'DMGT': dmgtCollection.load(*reader, base); break;
            case 'DOBJ': dobjCollection.load(*reader, base); break;
            case 'EFSQ': efsqCollection.load(*reader, base); break;
            case 'EQUP': equpCollection.load(*reader, base); break;
    case 'FFKW': ffkwCollection.load(*reader, base); break;
    case 'FOGV': fogvCollection.load(*reader, base); break;
    case 'FORC': forcCollection.load(*reader, base); break;
    case 'FSTP': fstpCollection.load(*reader, base); break;
    case 'FSTS': fstsCollection.load(*reader, base); break;
    case 'FXPD': fxpdCollection.load(*reader, base); break;
    case 'GBFM': gbfmCollection.load(*reader, base); break;
    case 'GBFT': gbftCollection.load(*reader, base); break;
    case 'GCVR': gcvrCollection.load(*reader, base); break;
    case 'IMAD': imadCollection.load(*reader, base); break;
    case 'INNR': innrCollection.load(*reader, base); break;
    case 'IRES': iresCollection.load(*reader, base); break;
    case 'KSSM': kssmCollection.load(*reader, base); break;
    case 'LAYR': layrCollection.load(*reader, base); break;
    case 'LENS': lensCollection.load(*reader, base); break;
    case 'LGDI': lgdiCollection.load(*reader, base); break;
    case 'LGTM': lgtmCollection.load(*reader, base); break;
    case 'LMSW': lmswCollection.load(*reader, base); break;
    case 'LVLB': lvlbCollection.load(*reader, base); break;
    case 'LVLN': lvlnCollection.load(*reader, base); break;
    case 'LVLP': lvlpCollection.load(*reader, base); break;
    case 'LVSC': lvscCollection.load(*reader, base); break;
    case 'MAAM': maamCollection.load(*reader, base); break;
    case 'MRPH': mrhpCollection.load(*reader, base); break;
    case 'MTPT': mtptCollection.load(*reader, base); break;
    case 'NAVI': naviCollection.load(*reader, base); break;
    case 'NOCM': nocmCollection.load(*reader, base); break;
    case 'OMOD': omodCollection.load(*reader, base); break;
    case 'OSWP': oswpCollection.load(*reader, base); break;
    case 'OVIS': ovisCollection.load(*reader, base); break;
    case 'PCBN': pcbnCollection.load(*reader, base); break;
    case 'PCCN': pccnCollection.load(*reader, base); break;
    case 'PCMT': pcmtCollection.load(*reader, base); break;
    case 'PDCL': pdclCollection.load(*reader, base); break;
    case 'PGRE': pgreCollection.load(*reader, base); break;
    case 'PHZD': phzdCollection.load(*reader, base); break;
    case 'PKIN': pkinCollection.load(*reader, base); break;
    case 'PMFT': pmftCollection.load(*reader, base); break;
    case 'PSDC': psdcCollection.load(*reader, base); break;
    case 'PTST': ptstCollection.load(*reader, base); break;
    case 'RFGP': rfgpCollection.load(*reader, base); break;
    case 'RSGD': rsgdCollection.load(*reader, base); break;
    case 'RSPJ': rspjCollection.load(*reader, base); break;
    case 'SDLT': sdltCollection.load(*reader, base); break;
    case 'SECH': sechCollection.load(*reader, base); break;
    case 'SFBK': sfbkCollection.load(*reader, base); break;
    case 'SFPC': sfpcCollection.load(*reader, base); break;
    case 'SFPT': sfptCollection.load(*reader, base); break;
    case 'SFTR': sftrCollection.load(*reader, base); break;
    case 'SMBN': smbnCollection.load(*reader, base); break;
    case 'SMEN': smenCollection.load(*reader, base); break;
    case 'SPCH': spchCollection.load(*reader, base); break;
    case 'STAG': stagCollection.load(*reader, base); break;
    case 'STBH': stbhCollection.load(*reader, base); break;
    case 'STDT': stdtCollection.load(*reader, base); break;
    case 'STMP': stmpCollection.load(*reader, base); break;
    case 'STND': stndCollection.load(*reader, base); break;
    case 'SUNP': sunpCollection.load(*reader, base); break;
    case 'TMLM': tmlmCollection.load(*reader, base); break;
    case 'TODD': toddCollection.load(*reader, base); break;
    case 'TRAV': travCollection.load(*reader, base); break;
    case 'TRNS': trnsCollection.load(*reader, base); break;
    case 'VOLI': voliCollection.load(*reader, base); break;
    case 'VTYP': vtypCollection.load(*reader, base); break;
    case 'WBAR': wbarCollection.load(*reader, base); break;
    case 'WKMF': wkmfCollection.load(*reader, base); break;
    case 'WTHS': wthsCollection.load(*reader, base); break;
    case 'WWED': wwedCollection.load(*reader, base); break;
    case 'ZOOM': zoomCollection.load(*reader, base); break;
            default:
            {
                if (name == 0)
                {
                    return true;
                }
                char buf[5] = {};
                memcpy(buf, &name, 4);
                LOG_WARNING(QString("Unknown record: %1 (0x%2)").arg(buf).arg(name, 8, 16, QChar('0')));
                reader->skipRecord();
                break;
            }
            }
        }
        catch (const std::exception& e)
        {
            char buf[5] = {};
            buf[0] = static_cast<char>((name >> 24) & 0xFF);
            buf[1] = static_cast<char>((name >> 16) & 0xFF);
            buf[2] = static_cast<char>((name >> 8) & 0xFF);
            buf[3] = static_cast<char>(name & 0xFF);
            LOG_ERROR(QString("Error loading record %1: %2")
                .arg(buf).arg(e.what()));

            reader->seekTo(pos);
            reader->skipRecord();

            return false;
        }

        return false;
    }
}

NAME Data::typeNameFor(int typeId)
{
    switch (static_cast<CkId::Type>(typeId))
    {
    case CkId::Type_Gmst: return NAME('GMST');
    case CkId::Type_Npc_: return NAME('NPC_');
    case CkId::Type_Weap_: return NAME('WEAP');
    case CkId::Type_Armor_: return NAME('ARMO');
    case CkId::Type_Spel_: return NAME('SPEL');
    case CkId::Type_Magic_: return NAME('MGEF');
    case CkId::Type_Quest_: return NAME('QUST');
    case CkId::Type_Dial_: return NAME('DIAL');
    case CkId::Type_Info_: return NAME('INFO');
    case CkId::Type_Glob_: return NAME('GLOB');
    case CkId::Type_Lcrt_: return NAME('LCRT');
    case CkId::Type_Pack_: return NAME('PACK');
    case CkId::Type_Tree_: return NAME('TREE');
    case CkId::Type_Alch_: return NAME('ALCH');
    case CkId::Type_Ingr_: return NAME('INGR');
    case CkId::Type_Cont_: return NAME('CONT');
    case CkId::Type_Ench_: return NAME('ENCH');
    case CkId::Type_Book_: return NAME('BOOK');
    case CkId::Type_Misc_: return NAME('MISC');
    case CkId::Type_Acti_: return NAME('ACTI');
    case CkId::Type_Stat_: return NAME('STAT');
    case CkId::Type_Race_: return NAME('RACE');
    case CkId::Type_Class_: return NAME('CLAS');
    case CkId::Type_Fact_: return NAME('FACT');
    case CkId::Type_PerK_: return NAME('PERK');
    case CkId::Type_Cel_: return NAME('CELL');
    case CkId::Type_WRLD_: return NAME('WRLD');
    case CkId::Type_LOCT_: return NAME('LCTN');
    case CkId::Type_Plnt_: return NAME('PNDT');
    case CkId::Type_Refr_: return NAME('REFR');
    case CkId::Type_Material_: return NAME('MATL');
    case CkId::Type_Land_: return NAME('LAND');
    case CkId::Type_Soun_: return NAME('SOUN');
    case CkId::Type_Wthr_: return NAME('WTHR');
    case CkId::Type_Ltex_: return NAME('LTEX');
    case CkId::Type_Ammo_: return NAME('AMMO');
    case CkId::Type_Appa_: return NAME('APPA');
    case CkId::Type_Avif_: return NAME('AVIF');
    case CkId::Type_Bsgn_: return NAME('BSGN');
    case CkId::Type_Clmt_: return NAME('CLMT');
    case CkId::Type_Clot_: return NAME('CLOT');
    case CkId::Type_Cobj_: return NAME('COBJ');
    case CkId::Type_Crea_: return NAME('CREA');
    case CkId::Type_Csty_: return NAME('CSTY');
    case CkId::Type_Door_: return NAME('DOOR');
    case CkId::Type_Efsh_: return NAME('EFSH');
    case CkId::Type_Expl_: return NAME('EXPL');
    case CkId::Type_Eyes_: return NAME('EYES');
    case CkId::Type_Flor_: return NAME('FLOR');
    case CkId::Type_Flst_: return NAME('FLST');
    case CkId::Type_Furn_: return NAME('FURN');
    case CkId::Type_Grass_: return NAME('GRAS');
    case CkId::Type_Hair_: return NAME('HAIR');
    case CkId::Type_Idle_: return NAME('IDLE');
    case CkId::Type_Idlm_: return NAME('IDLM');
    case CkId::Type_Imgs_: return NAME('IMGS');
    case CkId::Type_Keym_: return NAME('KEYM');
    case CkId::Type_Kywd_: return NAME('KYWD');
    case CkId::Type_Ligh_: return NAME('LIGH');
    case CkId::Type_Lscr_: return NAME('LSCR');
    case CkId::Type_Lvlc_: return NAME('LVLC');
    case CkId::Type_Lvli_: return NAME('LVLI');
    case CkId::Type_Lvsp_: return NAME('LVSP');
    case CkId::Type_Mesg_: return NAME('MESG');
    case CkId::Type_Mstt_: return NAME('MSTT');
    case CkId::Type_Navm_: return NAME('NAVM');
    case CkId::Type_Note_: return NAME('NOTE');
    case CkId::Type_Otft_: return NAME('OTFT');
    case CkId::Type_Proj_: return NAME('PROJ');
    case CkId::Type_Regn_: return NAME('REGN');
    case CkId::Type_Road_: return NAME('ROAD');
    case CkId::Type_Scpt_: return NAME('SCPT');
    case CkId::Type_Scrl_: return NAME('SCRL');
    case CkId::Type_Slgm_: return NAME('SLGM');
    case CkId::Type_Smqn_: return NAME('SMQN');
    case CkId::Type_Spgd_: return NAME('SPGD');
    case CkId::Type_Scol_: return NAME('SCOL');
    case CkId::Type_Scen_: return NAME('SCEN');
    case CkId::Type_Txst_: return NAME('TXST');
    case CkId::Type_Wate_: return NAME('WATR');
    case CkId::Type_Anio_: return NAME('ANIO');
    case CkId::Type_Artv_: return NAME('ARTV');
    case CkId::Type_Clfm_: return NAME('CLFM');
    case CkId::Type_Debr_: return NAME('DEBR');
    case CkId::Type_Eczn_: return NAME('ECZN');
    case CkId::Type_Hazd_: return NAME('HAZD');
    case CkId::Type_Ipct_: return NAME('IPCT');
    case CkId::Type_Ipds_: return NAME('IPDS');
    case CkId::Type_Must_: return NAME('MUST');
    case CkId::Type_Rela_: return NAME('RELA');
    case CkId::Type_Revb_: return NAME('REVB');
    case CkId::Type_Shou_: return NAME('SHOU');
    case CkId::Type_Hdpt_: return NAME('HDPT');
    case CkId::Type_Term_: return NAME('TERM');
    case CkId::Type_Matt_: return NAME('MATT');
    case CkId::Type_Movt_: return NAME('MOVT');
    case CkId::Type_Musc_: return NAME('MUSC');
    case CkId::Type_Aact_: return NAME('AACT');
    case CkId::Type_Aamd_: return NAME('AAMD');
    case CkId::Type_Aapd_: return NAME('AAPD');
    case CkId::Type_Achr_: return NAME('ACHR');
    case CkId::Type_Addn_: return NAME('ADDN');
    case CkId::Type_Affe_: return NAME('AFFE');
    case CkId::Type_Ambs_: return NAME('AMBS');
    case CkId::Type_Amdl_: return NAME('AMDL');
    case CkId::Type_Aopf_: return NAME('AOPF');
    case CkId::Type_Aops_: return NAME('AOPS');
    case CkId::Type_Aoru_: return NAME('AORU');
    case CkId::Type_Arma_: return NAME('ARMA');
    case CkId::Type_Arto_: return NAME('ARTO');
    case CkId::Type_Aspc_: return NAME('ASPC');
    case CkId::Type_Atmo_: return NAME('ATMO');
    case CkId::Type_Avmd_: return NAME('AVMD');
    case CkId::Type_Biom_: return NAME('BIOM');
    case CkId::Type_Bmmo_: return NAME('BMMO');
    case CkId::Type_Bmod_: return NAME('BMOD');
    case CkId::Type_Bnds_: return NAME('BNDS');
    case CkId::Type_Bptd_: return NAME('BPTD');
    case CkId::Type_Cams_: return NAME('CAMS');
    case CkId::Type_Chal_: return NAME('CHAL');
    case CkId::Type_Cldf_: return NAME('CLDF');
    case CkId::Type_Cndf_: return NAME('CNDF');
    case CkId::Type_Coll_: return NAME('COLL');
    case CkId::Type_Cpth_: return NAME('CPTH');
    case CkId::Type_Dlbr_: return NAME('DLBR');
    case CkId::Type_Cur3_: return NAME('CUR3');
    case CkId::Type_Curv_: return NAME('CURV');
    case CkId::Type_Dfob_: return NAME('DFOB');
    case CkId::Type_Dmgt_: return NAME('DMGT');
    case CkId::Type_Dobj_: return NAME('DOBJ');
    case CkId::Type_Efsq_: return NAME('EFSQ');
    case CkId::Type_Equp_: return NAME('EQUP');
    case CkId::Type_Ffkw_: return NAME('FFKW');
    case CkId::Type_Fogv_: return NAME('FOGV');
    case CkId::Type_Forc_: return NAME('FORC');
    case CkId::Type_Fstp_: return NAME('FSTP');
    case CkId::Type_Fsts_: return NAME('FSTS');
    case CkId::Type_Fxpd_: return NAME('FXPD');
    case CkId::Type_Gbfm_: return NAME('GBFM');
    case CkId::Type_Gbft_: return NAME('GBFT');
    case CkId::Type_Gcvr_: return NAME('GCVR');
    case CkId::Type_Imad_: return NAME('IMAD');
    case CkId::Type_Innr_: return NAME('INNR');
    case CkId::Type_Ires_: return NAME('IRES');
    case CkId::Type_Kssm_: return NAME('KSSM');
    case CkId::Type_Layr_: return NAME('LAYR');
    case CkId::Type_Lens_: return NAME('LENS');
    case CkId::Type_Lgdi_: return NAME('LGDI');
    case CkId::Type_Lgtm_: return NAME('LGTM');
    case CkId::Type_Lmsw_: return NAME('LMSW');
    case CkId::Type_Lvlb_: return NAME('LVLB');
    case CkId::Type_Lvln_: return NAME('LVLN');
    case CkId::Type_Lvlp_: return NAME('LVLP');
    case CkId::Type_Lvsc_: return NAME('LVSC');
    case CkId::Type_Maam_: return NAME('MAAM');
    case CkId::Type_Mrhp_: return NAME('MRPH');
    case CkId::Type_Mtpt_: return NAME('MTPT');
    case CkId::Type_Navi_: return NAME('NAVI');
    case CkId::Type_Nocm_: return NAME('NOCM');
    case CkId::Type_Omod_: return NAME('OMOD');
    case CkId::Type_Oswp_: return NAME('OSWP');
    case CkId::Type_Ovis_: return NAME('OVIS');
    case CkId::Type_Pcbn_: return NAME('PCBN');
    case CkId::Type_Pccn_: return NAME('PCCN');
    case CkId::Type_Pcmt_: return NAME('PCMT');
    case CkId::Type_Pdcl_: return NAME('PDCL');
    case CkId::Type_Pgre_: return NAME('PGRE');
    case CkId::Type_Phzd_: return NAME('PHZD');
    case CkId::Type_Pkin_: return NAME('PKIN');
    case CkId::Type_Pmft_: return NAME('PMFT');
    case CkId::Type_Psdc_: return NAME('PSDC');
    case CkId::Type_Ptst_: return NAME('PTST');
    case CkId::Type_Rfgp_: return NAME('RFGP');
    case CkId::Type_Rsgd_: return NAME('RSGD');
    case CkId::Type_Rspj_: return NAME('RSPJ');
    case CkId::Type_Sdlt_: return NAME('SDLT');
    case CkId::Type_Sech_: return NAME('SECH');
    case CkId::Type_Sfbk_: return NAME('SFBK');
    case CkId::Type_Sfpc_: return NAME('SFPC');
    case CkId::Type_Sfpt_: return NAME('SFPT');
    case CkId::Type_Sftr_: return NAME('SFTR');
    case CkId::Type_Smbn_: return NAME('SMBN');
    case CkId::Type_Smen_: return NAME('SMEN');
    case CkId::Type_Spch_: return NAME('SPCH');
    case CkId::Type_Stag_: return NAME('STAG');
    case CkId::Type_Stbh_: return NAME('STBH');
    case CkId::Type_Stdt_: return NAME('STDT');
    case CkId::Type_Stmp_: return NAME('STMP');
    case CkId::Type_Stnd_: return NAME('STND');
    case CkId::Type_Sunp_: return NAME('SUNP');
    case CkId::Type_Tmlm_: return NAME('TMLM');
    case CkId::Type_Todd_: return NAME('TODD');
    case CkId::Type_Trav_: return NAME('TRAV');
    case CkId::Type_Trns_: return NAME('TRNS');
    case CkId::Type_Voli_: return NAME('VOLI');
    case CkId::Type_Vtyp_: return NAME('VTYP');
    case CkId::Type_Wbar_: return NAME('WBAR');
    case CkId::Type_Wkmf_: return NAME('WKMF');
    case CkId::Type_Wths_: return NAME('WTHS');
    case CkId::Type_Wwed_: return NAME('WWED');
    case CkId::Type_Zoom_: return NAME('ZOOM');
    default: return 0;
    }
}

int Data::masterIndexCount(int typeId) const
{
    const NAME name = typeNameFor(typeId);
    if (name == 0)
        return 0;
    int count = 0;
    for (const MasterIndexEntry& entry : m_masterIndex)
    {
        if (entry.type == name)
            ++count;
    }
    return count;
}

int Data::ensureTypeLoaded(int typeId)
{
    const NAME name = typeNameFor(typeId);
    if (name == 0)
        return 0;

    int loaded = 0;
    Messages messages(Message::Error);
    const bool prevBase = base;
    base = true;

    for (int f = 0; f < m_deferredMasterFiles.size(); ++f)
    {
        bool hasEntries = false;
        for (const MasterIndexEntry& entry : m_masterIndex)
        {
            if (entry.fileIndex == f && entry.type == name)
            {
                hasEntries = true;
                break;
            }
        }
        if (!hasEntries)
            continue;

        // Reopen this master (header parse only) so its records can be read.
        reader.reset(new ESMReader(m_deferredMasterFiles[f], paths));
        reader->open();

        for (const MasterIndexEntry& entry : m_masterIndex)
        {
            if (entry.fileIndex != f || entry.type != name)
                continue;
            reader->seekTo(entry.offset);
            if (continueLoading(messages))
                break;
            ++loaded;
        }
    }

    // Restore the reader to the edited file so post-load state matches
    // the pre-materialization state.
    if (!m_lastPreloadPath.isEmpty())
    {
        reader.reset(new ESMReader(m_lastPreloadPath, paths));
        reader->open();
    }
    base = prevBase;

    // Consume the materialized entries so repeated calls are no-ops.
    m_masterIndex.erase(
        std::remove_if(m_masterIndex.begin(), m_masterIndex.end(),
            [name](const MasterIndexEntry& e) { return e.type == name; }),
        m_masterIndex.end());

    LOG_INFO(QString("Data::ensureTypeLoaded: %1 -> %2 record(s)")
        .arg(CkId(static_cast<CkId::Type>(typeId)).getTypeName())
        .arg(loaded));
    return loaded;
}

const IdCollection<GameSetting>& Data::getGameSettings() const
{
    return gameSettings;
}

const Collection<MetaData>& Data::getMetaData() const
{
    return metaData;
}

const IdCollection<NpcRecord>& Data::getNpcCollection() const
{
    return npcCollection;
}

const IdCollection<WeaponRecord>& Data::getWeaponCollection() const
{
    return weaponCollection;
}

const IdCollection<ArmorRecord>& Data::getArmorCollection() const
{
    return armorCollection;
}

const IdCollection<SpellRecord>& Data::getSpellCollection() const
{
    return spellCollection;
}

const IdCollection<MagicRecord>& Data::getMagicCollection() const
{
    return magicCollection;
}

const IdCollection<QuestRecord>& Data::getQuestCollection() const
{
    return questCollection;
}

const IdCollection<DialRecord>& Data::getDialCollection() const
{
    return dialCollection;
}

const IdCollection<InfoRecord>& Data::getInfoCollection() const
{
    return infoCollection;
}

const IdCollection<GlobalVariable>& Data::getGlobCollection() const
{
    return globCollection;
}

const IdCollection<LocationRefType>& Data::getLcrtCollection() const
{
    return lcrtCollection;
}

const IdCollection<PackageRecord>& Data::getPackCollection() const
{
    return packCollection;
}

const IdCollection<TreeRecord>& Data::getTreeCollection() const
{
    return treeCollection;
}

const IdCollection<AlchRecord>& Data::getAlchCollection() const
{
    return alchCollection;
}

const IdCollection<IngrRecord>& Data::getIngrCollection() const
{
    return ingrCollection;
}

const IdCollection<ContRecord>& Data::getContCollection() const
{
    return contCollection;
}

const IdCollection<EnchRecord>& Data::getEnchCollection() const
{
    return enchCollection;
}

const IdCollection<BookRecord>& Data::getBookCollection() const
{
    return bookCollection;
}

const IdCollection<MiscRecord>& Data::getMiscCollection() const
{
    return miscCollection;
}

const IdCollection<ActiRecord>& Data::getActiCollection() const
{
    return actiCollection;
}

const IdCollection<StatRecord>& Data::getStatCollection() const
{
    return statCollection;
}

const IdCollection<RaceRecord>& Data::getRaceCollection() const
{
    return raceCollection;
}

const IdCollection<ClassRecord>& Data::getClassCollection() const
{
    return classCollection;
}

const IdCollection<FactRecord>& Data::getFactCollection() const
{
    return factCollection;
}

const IdCollection<PerkRecord>& Data::getPerkCollection() const
{
    return perkCollection;
}

const IdCollection<CellRecord>& Data::getCellCollection() const
{
    return cellCollection;
}

const IdCollection<WorldspaceRecord>& Data::getWorldspaceCollection() const
{
    return worldspaceCollection;
}

const IdCollection<LocationRecord>& Data::getLocationCollection() const
{
    return locationCollection;
}

const IdCollection<PndRecord>& Data::getPlanetCollection() const
{
    return planetCollection;
}

const IdCollection<RefrRecord>& Data::getRefrCollection() const
{
    return refrCollection;
}

const BaseCollection* Data::getCollectionByType(CkId::Type type) const
{
    switch (type)
    {
    case CkId::Type_Gmst:    return &gameSettings;
    case CkId::Type_Npc_:     return &npcCollection;
    case CkId::Type_Weap_:    return &weaponCollection;
    case CkId::Type_Armor_:   return &armorCollection;
    case CkId::Type_Spel_:    return &spellCollection;
    case CkId::Type_Magic_:   return &magicCollection;
    case CkId::Type_Quest_:   return &questCollection;
    case CkId::Type_Dial_:    return &dialCollection;
    case CkId::Type_Info_:    return &infoCollection;
    case CkId::Type_Glob_:    return &globCollection;
    case CkId::Type_Lcrt_:    return &lcrtCollection;
    case CkId::Type_Pack_:    return &packCollection;
    case CkId::Type_Tree_:    return &treeCollection;
    case CkId::Type_Alch_:    return &alchCollection;
    case CkId::Type_Ingr_:    return &ingrCollection;
    case CkId::Type_Cont_:    return &contCollection;
    case CkId::Type_Ench_:    return &enchCollection;
    case CkId::Type_Book_:    return &bookCollection;
    case CkId::Type_Misc_:    return &miscCollection;
    case CkId::Type_Acti_:    return &actiCollection;
    case CkId::Type_Stat_:    return &statCollection;
    case CkId::Type_Race_:    return &raceCollection;
    case CkId::Type_Class_:   return &classCollection;
    case CkId::Type_Fact_:    return &factCollection;
    case CkId::Type_PerK_:    return &perkCollection;
    case CkId::Type_Cel_:     return &cellCollection;
    case CkId::Type_WRLD_:    return &worldspaceCollection;
    case CkId::Type_LOCT_:    return &locationCollection;
    case CkId::Type_Plnt_:    return &planetCollection;
    case CkId::Type_Refr_:    return &refrCollection;
    case CkId::Type_Material_: return &materialCollection;
    case CkId::Type_Land_:     return &landCollection;
    case CkId::Type_Soun_:     return &sounCollection;
    case CkId::Type_Wthr_:     return &wthrCollection;
    case CkId::Type_Ltex_:     return &ltexCollection;
    case CkId::Type_Scen_:     return &scenCollection;
    case CkId::Type_Ammo_:    return &ammoCollection;
    case CkId::Type_Appa_:    return &appaCollection;
    case CkId::Type_Avif_:    return &avifCollection;
    case CkId::Type_Bsgn_:    return &bsgnCollection;
    case CkId::Type_Clmt_:    return &clmtCollection;
    case CkId::Type_Clot_:    return &clotCollection;
    case CkId::Type_Cobj_:    return &cobjCollection;
    case CkId::Type_Crea_:    return &creatureCollection;
    case CkId::Type_Csty_:    return &cstyCollection;
    case CkId::Type_Door_:    return &doorCollection;
    case CkId::Type_Efsh_:    return &efshCollection;
    case CkId::Type_Expl_:    return &explCollection;
    case CkId::Type_Eyes_:    return &eyesCollection;
    case CkId::Type_Flor_:    return &florCollection;
    case CkId::Type_Flst_:    return &flstCollection;
    case CkId::Type_Furn_:    return &furnCollection;
    case CkId::Type_Grass_:    return &grassCollection;
    case CkId::Type_Hair_:    return &hairCollection;
    case CkId::Type_Idle_:    return &idleCollection;
    case CkId::Type_Idlm_:    return &idlmCollection;
    case CkId::Type_Imgs_:    return &imgsCollection;
    case CkId::Type_Keym_:    return &keymCollection;
    case CkId::Type_Kywd_:    return &kywdCollection;
    case CkId::Type_Ligh_:    return &lighCollection;
    case CkId::Type_Lscr_:    return &lscrCollection;
    case CkId::Type_Lvlc_:    return &lvlcCollection;
    case CkId::Type_Lvli_:    return &lvliCollection;
    case CkId::Type_Lvsp_:    return &lvspCollection;
    case CkId::Type_Mesg_:    return &mesgCollection;
    case CkId::Type_Mstt_:    return &msttCollection;
    case CkId::Type_Navm_:    return &navmCollection;
    case CkId::Type_Note_:    return &noteCollection;
    case CkId::Type_Otft_:    return &otftCollection;
    case CkId::Type_Proj_:    return &projCollection;
    case CkId::Type_Regn_:    return &regnCollection;
    case CkId::Type_Road_:    return &roadCollection;
    case CkId::Type_Scpt_:    return &scptCollection;
    case CkId::Type_Scrl_:    return &scrlCollection;
    case CkId::Type_Slgm_:    return &slgmCollection;
    case CkId::Type_Smqn_:    return &smqnCollection;
    case CkId::Type_Spgd_:    return &spgdCollection;
    case CkId::Type_Scol_:    return &scolCollection;
    case CkId::Type_Txst_:    return &txstCollection;
    case CkId::Type_Wate_:    return &wateCollection;
    case CkId::Type_Anio_:    return &anioCollection;
    case CkId::Type_Artv_:    return &artvCollection;
    case CkId::Type_Clfm_:    return &clfmCollection;
    case CkId::Type_Debr_:    return &debrCollection;
    case CkId::Type_Eczn_:    return &ecznCollection;
    case CkId::Type_Hazd_:    return &hazdCollection;
    case CkId::Type_Ipct_:    return &ipctCollection;
    case CkId::Type_Ipds_:    return &ipdsCollection;
    case CkId::Type_Must_:    return &mustCollection;
    case CkId::Type_Rela_:    return &relaCollection;
    case CkId::Type_Revb_:    return &revbCollection;
    case CkId::Type_Shou_:    return &shouCollection;
    case CkId::Type_Hdpt_:    return &hdptCollection;
    case CkId::Type_Term_:    return &termCollection;
    case CkId::Type_Matt_:    return &mattCollection;
    case CkId::Type_Movt_:    return &movtCollection;
    case CkId::Type_Musc_:    return &muscCollection;
    case CkId::Type_Aact_:    return &aactCollection;
    case CkId::Type_Aamd_:    return &aamdCollection;
    case CkId::Type_Aapd_:    return &aapdCollection;
    case CkId::Type_Achr_:    return &achrCollection;
    case CkId::Type_Addn_:    return &addnCollection;
    case CkId::Type_Affe_:    return &affeCollection;
    case CkId::Type_Ambs_:    return &ambsCollection;
    case CkId::Type_Amdl_:    return &amdlCollection;
    case CkId::Type_Aopf_:    return &aopfCollection;
    case CkId::Type_Aops_:    return &aopsCollection;
    case CkId::Type_Aoru_:    return &aoruCollection;
    case CkId::Type_Arma_:    return &armaCollection;
    case CkId::Type_Arto_:    return &artoCollection;
    case CkId::Type_Aspc_:    return &aspcCollection;
    case CkId::Type_Atmo_:    return &atmoCollection;
    case CkId::Type_Avmd_:    return &avmdCollection;
    case CkId::Type_Biom_:    return &biomCollection;
    case CkId::Type_Bmmo_:    return &bmmoCollection;
    case CkId::Type_Bmod_:    return &bmodCollection;
    case CkId::Type_Bnds_:    return &bndsCollection;
    case CkId::Type_Bptd_:    return &bptdCollection;
    case CkId::Type_Cams_:    return &camsCollection;
    case CkId::Type_Chal_:    return &chalCollection;
    case CkId::Type_Cldf_:    return &cldfCollection;
    case CkId::Type_Cndf_:    return &cndfCollection;
    case CkId::Type_Coll_:    return &collCollection;
    case CkId::Type_Cpth_:    return &cpthCollection;
    case CkId::Type_Dlbr_:    return &dlbrCollection;
    case CkId::Type_Cur3_:    return &cur3Collection;
    case CkId::Type_Curv_:    return &curvCollection;
    case CkId::Type_Dfob_:    return &dfobCollection;
    case CkId::Type_Dmgt_:    return &dmgtCollection;
    case CkId::Type_Dobj_:    return &dobjCollection;
    case CkId::Type_Efsq_:    return &efsqCollection;
    case CkId::Type_Equp_:    return &equpCollection;
    case CkId::Type_Ffkw_:    return &ffkwCollection;
    case CkId::Type_Fogv_:    return &fogvCollection;
    case CkId::Type_Forc_:    return &forcCollection;
    case CkId::Type_Fstp_:    return &fstpCollection;
    case CkId::Type_Fsts_:    return &fstsCollection;
    case CkId::Type_Fxpd_:    return &fxpdCollection;
    case CkId::Type_Gbfm_:    return &gbfmCollection;
    case CkId::Type_Gbft_:    return &gbftCollection;
    case CkId::Type_Gcvr_:    return &gcvrCollection;
    case CkId::Type_Imad_:    return &imadCollection;
    case CkId::Type_Innr_:    return &innrCollection;
    case CkId::Type_Ires_:    return &iresCollection;
    case CkId::Type_Kssm_:    return &kssmCollection;
    case CkId::Type_Layr_:    return &layrCollection;
    case CkId::Type_Lens_:    return &lensCollection;
    case CkId::Type_Lgdi_:    return &lgdiCollection;
    case CkId::Type_Lgtm_:    return &lgtmCollection;
    case CkId::Type_Lmsw_:    return &lmswCollection;
    case CkId::Type_Lvlb_:    return &lvlbCollection;
    case CkId::Type_Lvln_:    return &lvlnCollection;
    case CkId::Type_Lvlp_:    return &lvlpCollection;
    case CkId::Type_Lvsc_:    return &lvscCollection;
    case CkId::Type_Maam_:    return &maamCollection;
    case CkId::Type_Mrhp_:    return &mrhpCollection;
    case CkId::Type_Mtpt_:    return &mtptCollection;
    case CkId::Type_Navi_:    return &naviCollection;
    case CkId::Type_Nocm_:    return &nocmCollection;
    case CkId::Type_Omod_:    return &omodCollection;
    case CkId::Type_Oswp_:    return &oswpCollection;
    case CkId::Type_Ovis_:    return &ovisCollection;
    case CkId::Type_Pcbn_:    return &pcbnCollection;
    case CkId::Type_Pccn_:    return &pccnCollection;
    case CkId::Type_Pcmt_:    return &pcmtCollection;
    case CkId::Type_Pdcl_:    return &pdclCollection;
    case CkId::Type_Pgre_:    return &pgreCollection;
    case CkId::Type_Phzd_:    return &phzdCollection;
    case CkId::Type_Pkin_:    return &pkinCollection;
    case CkId::Type_Pmft_:    return &pmftCollection;
    case CkId::Type_Psdc_:    return &psdcCollection;
    case CkId::Type_Ptst_:    return &ptstCollection;
    case CkId::Type_Rfgp_:    return &rfgpCollection;
    case CkId::Type_Rsgd_:    return &rsgdCollection;
    case CkId::Type_Rspj_:    return &rspjCollection;
    case CkId::Type_Sdlt_:    return &sdltCollection;
    case CkId::Type_Sech_:    return &sechCollection;
    case CkId::Type_Sfbk_:    return &sfbkCollection;
    case CkId::Type_Sfpc_:    return &sfpcCollection;
    case CkId::Type_Sfpt_:    return &sfptCollection;
    case CkId::Type_Sftr_:    return &sftrCollection;
    case CkId::Type_Smbn_:    return &smbnCollection;
    case CkId::Type_Smen_:    return &smenCollection;
    case CkId::Type_Spch_:    return &spchCollection;
    case CkId::Type_Stag_:    return &stagCollection;
    case CkId::Type_Stbh_:    return &stbhCollection;
    case CkId::Type_Stdt_:    return &stdtCollection;
    case CkId::Type_Stmp_:    return &stmpCollection;
    case CkId::Type_Stnd_:    return &stndCollection;
    case CkId::Type_Sunp_:    return &sunpCollection;
    case CkId::Type_Tmlm_:    return &tmlmCollection;
    case CkId::Type_Todd_:    return &toddCollection;
    case CkId::Type_Trav_:    return &travCollection;
    case CkId::Type_Trns_:    return &trnsCollection;
    case CkId::Type_Voli_:    return &voliCollection;
    case CkId::Type_Vtyp_:    return &vtypCollection;
    case CkId::Type_Wbar_:    return &wbarCollection;
    case CkId::Type_Wkmf_:    return &wkmfCollection;
    case CkId::Type_Wths_:    return &wthsCollection;
    case CkId::Type_Wwed_:    return &wwedCollection;
    case CkId::Type_Zoom_:    return &zoomCollection;
    default:                  return nullptr;
    }
}

BaseCollection* Data::getCollectionByType(CkId::Type type)
{
    switch (type)
    {
    case CkId::Type_Gmst:    return &gameSettings;
    case CkId::Type_Npc_:     return &npcCollection;
    case CkId::Type_Weap_:    return &weaponCollection;
    case CkId::Type_Armor_:   return &armorCollection;
    case CkId::Type_Spel_:    return &spellCollection;
    case CkId::Type_Magic_:   return &magicCollection;
    case CkId::Type_Quest_:   return &questCollection;
    case CkId::Type_Dial_:    return &dialCollection;
    case CkId::Type_Info_:    return &infoCollection;
    case CkId::Type_Glob_:    return &globCollection;
    case CkId::Type_Lcrt_:    return &lcrtCollection;
    case CkId::Type_Pack_:    return &packCollection;
    case CkId::Type_Tree_:    return &treeCollection;
    case CkId::Type_Alch_:    return &alchCollection;
    case CkId::Type_Ingr_:    return &ingrCollection;
    case CkId::Type_Cont_:    return &contCollection;
    case CkId::Type_Ench_:    return &enchCollection;
    case CkId::Type_Book_:    return &bookCollection;
    case CkId::Type_Misc_:    return &miscCollection;
    case CkId::Type_Acti_:    return &actiCollection;
    case CkId::Type_Stat_:    return &statCollection;
    case CkId::Type_Race_:    return &raceCollection;
    case CkId::Type_Class_:   return &classCollection;
    case CkId::Type_Fact_:    return &factCollection;
    case CkId::Type_PerK_:    return &perkCollection;
    case CkId::Type_Cel_:     return &cellCollection;
    case CkId::Type_WRLD_:    return &worldspaceCollection;
    case CkId::Type_LOCT_:    return &locationCollection;
    case CkId::Type_Plnt_:    return &planetCollection;
    case CkId::Type_Refr_:    return &refrCollection;
    case CkId::Type_Material_: return &materialCollection;
    case CkId::Type_Land_:     return &landCollection;
    case CkId::Type_Soun_:     return &sounCollection;
    case CkId::Type_Wthr_:     return &wthrCollection;
    case CkId::Type_Ltex_:     return &ltexCollection;
    case CkId::Type_Scen_:     return &scenCollection;
    case CkId::Type_Ammo_:    return &ammoCollection;
    case CkId::Type_Appa_:    return &appaCollection;
    case CkId::Type_Avif_:    return &avifCollection;
    case CkId::Type_Bsgn_:    return &bsgnCollection;
    case CkId::Type_Clmt_:    return &clmtCollection;
    case CkId::Type_Clot_:    return &clotCollection;
    case CkId::Type_Cobj_:    return &cobjCollection;
    case CkId::Type_Crea_:    return &creatureCollection;
    case CkId::Type_Csty_:    return &cstyCollection;
    case CkId::Type_Door_:    return &doorCollection;
    case CkId::Type_Efsh_:    return &efshCollection;
    case CkId::Type_Expl_:    return &explCollection;
    case CkId::Type_Eyes_:    return &eyesCollection;
    case CkId::Type_Flor_:    return &florCollection;
    case CkId::Type_Flst_:    return &flstCollection;
    case CkId::Type_Furn_:    return &furnCollection;
    case CkId::Type_Grass_:    return &grassCollection;
    case CkId::Type_Hair_:    return &hairCollection;
    case CkId::Type_Idle_:    return &idleCollection;
    case CkId::Type_Idlm_:    return &idlmCollection;
    case CkId::Type_Imgs_:    return &imgsCollection;
    case CkId::Type_Keym_:    return &keymCollection;
    case CkId::Type_Kywd_:    return &kywdCollection;
    case CkId::Type_Ligh_:    return &lighCollection;
    case CkId::Type_Lscr_:    return &lscrCollection;
    case CkId::Type_Lvlc_:    return &lvlcCollection;
    case CkId::Type_Lvli_:    return &lvliCollection;
    case CkId::Type_Lvsp_:    return &lvspCollection;
    case CkId::Type_Mesg_:    return &mesgCollection;
    case CkId::Type_Mstt_:    return &msttCollection;
    case CkId::Type_Navm_:    return &navmCollection;
    case CkId::Type_Note_:    return &noteCollection;
    case CkId::Type_Otft_:    return &otftCollection;
    case CkId::Type_Proj_:    return &projCollection;
    case CkId::Type_Regn_:    return &regnCollection;
    case CkId::Type_Road_:    return &roadCollection;
    case CkId::Type_Scpt_:    return &scptCollection;
    case CkId::Type_Scrl_:    return &scrlCollection;
    case CkId::Type_Slgm_:    return &slgmCollection;
    case CkId::Type_Smqn_:    return &smqnCollection;
    case CkId::Type_Spgd_:    return &spgdCollection;
    case CkId::Type_Scol_:    return &scolCollection;
    case CkId::Type_Txst_:    return &txstCollection;
    case CkId::Type_Wate_:    return &wateCollection;
    case CkId::Type_Anio_:    return &anioCollection;
    case CkId::Type_Artv_:    return &artvCollection;
    case CkId::Type_Clfm_:    return &clfmCollection;
    case CkId::Type_Debr_:    return &debrCollection;
    case CkId::Type_Eczn_:    return &ecznCollection;
    case CkId::Type_Hazd_:    return &hazdCollection;
    case CkId::Type_Ipct_:    return &ipctCollection;
    case CkId::Type_Ipds_:    return &ipdsCollection;
    case CkId::Type_Must_:    return &mustCollection;
    case CkId::Type_Rela_:    return &relaCollection;
    case CkId::Type_Revb_:    return &revbCollection;
    case CkId::Type_Shou_:    return &shouCollection;
    case CkId::Type_Hdpt_:    return &hdptCollection;
    case CkId::Type_Term_:    return &termCollection;
    case CkId::Type_Matt_:    return &mattCollection;
    case CkId::Type_Movt_:    return &movtCollection;
    case CkId::Type_Musc_:    return &muscCollection;
    case CkId::Type_Aact_:    return &aactCollection;
    case CkId::Type_Aamd_:    return &aamdCollection;
    case CkId::Type_Aapd_:    return &aapdCollection;
    case CkId::Type_Achr_:    return &achrCollection;
    case CkId::Type_Addn_:    return &addnCollection;
    case CkId::Type_Affe_:    return &affeCollection;
    case CkId::Type_Ambs_:    return &ambsCollection;
    case CkId::Type_Amdl_:    return &amdlCollection;
    case CkId::Type_Aopf_:    return &aopfCollection;
    case CkId::Type_Aops_:    return &aopsCollection;
    case CkId::Type_Aoru_:    return &aoruCollection;
    case CkId::Type_Arma_:    return &armaCollection;
    case CkId::Type_Arto_:    return &artoCollection;
    case CkId::Type_Aspc_:    return &aspcCollection;
    case CkId::Type_Atmo_:    return &atmoCollection;
    case CkId::Type_Avmd_:    return &avmdCollection;
    case CkId::Type_Biom_:    return &biomCollection;
    case CkId::Type_Bmmo_:    return &bmmoCollection;
    case CkId::Type_Bmod_:    return &bmodCollection;
    case CkId::Type_Bnds_:    return &bndsCollection;
    case CkId::Type_Bptd_:    return &bptdCollection;
    case CkId::Type_Cams_:    return &camsCollection;
    case CkId::Type_Chal_:    return &chalCollection;
    case CkId::Type_Cldf_:    return &cldfCollection;
    case CkId::Type_Cndf_:    return &cndfCollection;
    case CkId::Type_Coll_:    return &collCollection;
    case CkId::Type_Cpth_:    return &cpthCollection;
    case CkId::Type_Dlbr_:    return &dlbrCollection;
    case CkId::Type_Cur3_:    return &cur3Collection;
    case CkId::Type_Curv_:    return &curvCollection;
    case CkId::Type_Dfob_:    return &dfobCollection;
    case CkId::Type_Dmgt_:    return &dmgtCollection;
    case CkId::Type_Dobj_:    return &dobjCollection;
    case CkId::Type_Efsq_:    return &efsqCollection;
    case CkId::Type_Equp_:    return &equpCollection;
    case CkId::Type_Ffkw_:    return &ffkwCollection;
    case CkId::Type_Fogv_:    return &fogvCollection;
    case CkId::Type_Forc_:    return &forcCollection;
    case CkId::Type_Fstp_:    return &fstpCollection;
    case CkId::Type_Fsts_:    return &fstsCollection;
    case CkId::Type_Fxpd_:    return &fxpdCollection;
    case CkId::Type_Gbfm_:    return &gbfmCollection;
    case CkId::Type_Gbft_:    return &gbftCollection;
    case CkId::Type_Gcvr_:    return &gcvrCollection;
    case CkId::Type_Imad_:    return &imadCollection;
    case CkId::Type_Innr_:    return &innrCollection;
    case CkId::Type_Ires_:    return &iresCollection;
    case CkId::Type_Kssm_:    return &kssmCollection;
    case CkId::Type_Layr_:    return &layrCollection;
    case CkId::Type_Lens_:    return &lensCollection;
    case CkId::Type_Lgdi_:    return &lgdiCollection;
    case CkId::Type_Lgtm_:    return &lgtmCollection;
    case CkId::Type_Lmsw_:    return &lmswCollection;
    case CkId::Type_Lvlb_:    return &lvlbCollection;
    case CkId::Type_Lvln_:    return &lvlnCollection;
    case CkId::Type_Lvlp_:    return &lvlpCollection;
    case CkId::Type_Lvsc_:    return &lvscCollection;
    case CkId::Type_Maam_:    return &maamCollection;
    case CkId::Type_Mrhp_:    return &mrhpCollection;
    case CkId::Type_Mtpt_:    return &mtptCollection;
    case CkId::Type_Navi_:    return &naviCollection;
    case CkId::Type_Nocm_:    return &nocmCollection;
    case CkId::Type_Omod_:    return &omodCollection;
    case CkId::Type_Oswp_:    return &oswpCollection;
    case CkId::Type_Ovis_:    return &ovisCollection;
    case CkId::Type_Pcbn_:    return &pcbnCollection;
    case CkId::Type_Pccn_:    return &pccnCollection;
    case CkId::Type_Pcmt_:    return &pcmtCollection;
    case CkId::Type_Pdcl_:    return &pdclCollection;
    case CkId::Type_Pgre_:    return &pgreCollection;
    case CkId::Type_Phzd_:    return &phzdCollection;
    case CkId::Type_Pkin_:    return &pkinCollection;
    case CkId::Type_Pmft_:    return &pmftCollection;
    case CkId::Type_Psdc_:    return &psdcCollection;
    case CkId::Type_Ptst_:    return &ptstCollection;
    case CkId::Type_Rfgp_:    return &rfgpCollection;
    case CkId::Type_Rsgd_:    return &rsgdCollection;
    case CkId::Type_Rspj_:    return &rspjCollection;
    case CkId::Type_Sdlt_:    return &sdltCollection;
    case CkId::Type_Sech_:    return &sechCollection;
    case CkId::Type_Sfbk_:    return &sfbkCollection;
    case CkId::Type_Sfpc_:    return &sfpcCollection;
    case CkId::Type_Sfpt_:    return &sfptCollection;
    case CkId::Type_Sftr_:    return &sftrCollection;
    case CkId::Type_Smbn_:    return &smbnCollection;
    case CkId::Type_Smen_:    return &smenCollection;
    case CkId::Type_Spch_:    return &spchCollection;
    case CkId::Type_Stag_:    return &stagCollection;
    case CkId::Type_Stbh_:    return &stbhCollection;
    case CkId::Type_Stdt_:    return &stdtCollection;
    case CkId::Type_Stmp_:    return &stmpCollection;
    case CkId::Type_Stnd_:    return &stndCollection;
    case CkId::Type_Sunp_:    return &sunpCollection;
    case CkId::Type_Tmlm_:    return &tmlmCollection;
    case CkId::Type_Todd_:    return &toddCollection;
    case CkId::Type_Trav_:    return &travCollection;
    case CkId::Type_Trns_:    return &trnsCollection;
    case CkId::Type_Voli_:    return &voliCollection;
    case CkId::Type_Vtyp_:    return &vtypCollection;
    case CkId::Type_Wbar_:    return &wbarCollection;
    case CkId::Type_Wkmf_:    return &wkmfCollection;
    case CkId::Type_Wths_:    return &wthsCollection;
    case CkId::Type_Wwed_:    return &wwedCollection;
    case CkId::Type_Zoom_:    return &zoomCollection;
    default:                  return nullptr;
    }
}

// --- Non-const collection getters ---

IdCollection<GameSetting>& Data::getGameSettings() { return gameSettings; }
Collection<MetaData>& Data::getMetaData() { return metaData; }
IdCollection<NpcRecord>& Data::getNpcCollection() { return npcCollection; }
IdCollection<WeaponRecord>& Data::getWeaponCollection() { return weaponCollection; }
IdCollection<ArmorRecord>& Data::getArmorCollection() { return armorCollection; }
IdCollection<SpellRecord>& Data::getSpellCollection() { return spellCollection; }
IdCollection<MagicRecord>& Data::getMagicCollection() { return magicCollection; }
IdCollection<QuestRecord>& Data::getQuestCollection() { return questCollection; }
IdCollection<DialRecord>& Data::getDialCollection() { return dialCollection; }
IdCollection<InfoRecord>& Data::getInfoCollection() { return infoCollection; }
IdCollection<GlobalVariable>& Data::getGlobCollection() { return globCollection; }
IdCollection<LocationRefType>& Data::getLcrtCollection() { return lcrtCollection; }
IdCollection<PackageRecord>& Data::getPackCollection() { return packCollection; }
IdCollection<TreeRecord>& Data::getTreeCollection() { return treeCollection; }
IdCollection<AlchRecord>& Data::getAlchCollection() { return alchCollection; }
IdCollection<IngrRecord>& Data::getIngrCollection() { return ingrCollection; }
IdCollection<ContRecord>& Data::getContCollection() { return contCollection; }
IdCollection<EnchRecord>& Data::getEnchCollection() { return enchCollection; }
IdCollection<BookRecord>& Data::getBookCollection() { return bookCollection; }
IdCollection<MiscRecord>& Data::getMiscCollection() { return miscCollection; }
IdCollection<ActiRecord>& Data::getActiCollection() { return actiCollection; }
IdCollection<StatRecord>& Data::getStatCollection() { return statCollection; }
IdCollection<RaceRecord>& Data::getRaceCollection() { return raceCollection; }
IdCollection<ClassRecord>& Data::getClassCollection() { return classCollection; }
IdCollection<FactRecord>& Data::getFactCollection() { return factCollection; }
IdCollection<PerkRecord>& Data::getPerkCollection() { return perkCollection; }
IdCollection<CellRecord>& Data::getCellCollection() { return cellCollection; }
IdCollection<WorldspaceRecord>& Data::getWorldspaceCollection() { return worldspaceCollection; }
IdCollection<LocationRecord>& Data::getLocationCollection() { return locationCollection; }
IdCollection<PndRecord>& Data::getPlanetCollection() { return planetCollection; }
IdCollection<RefrRecord>& Data::getRefrCollection() { return refrCollection; }
IdCollection<MaterialRecord>& Data::getMaterialCollection() { return materialCollection; }
IdCollection<LandRecord>& Data::getLandCollection() { return landCollection; }
IdCollection<SounRecord>& Data::getSounCollection() { return sounCollection; }
IdCollection<WthrRecord>& Data::getWthrCollection() { return wthrCollection; }
IdCollection<LtexRecord>& Data::getLtexCollection() { return ltexCollection; }
IdCollection<ScenRecord>& Data::getScenCollection() { return scenCollection; }
IdCollection<AmmoRecord>& Data::getAmmoCollection() { return ammoCollection; }
IdCollection<AppaRecord>& Data::getAppaCollection() { return appaCollection; }
IdCollection<ActorValueInfoRecord>& Data::getAvifCollection() { return avifCollection; }
IdCollection<BsgnRecord>& Data::getBsgnCollection() { return bsgnCollection; }
IdCollection<ClimateRecord>& Data::getClmtCollection() { return clmtCollection; }
IdCollection<ClotRecord>& Data::getClotCollection() { return clotCollection; }
IdCollection<CobjRecord>& Data::getCobjCollection() { return cobjCollection; }
IdCollection<CreatureRecord>& Data::getCreatureCollection() { return creatureCollection; }
IdCollection<CstyRecord>& Data::getCstyCollection() { return cstyCollection; }
IdCollection<DoorRecord>& Data::getDoorCollection() { return doorCollection; }
IdCollection<EfshRecord>& Data::getEfshCollection() { return efshCollection; }
IdCollection<ExplRecord>& Data::getExplCollection() { return explCollection; }
IdCollection<EyesRecord>& Data::getEyesCollection() { return eyesCollection; }
IdCollection<FlorRecord>& Data::getFlorCollection() { return florCollection; }
IdCollection<FormListRecord>& Data::getFlstCollection() { return flstCollection; }
IdCollection<FurnRecord>& Data::getFurnCollection() { return furnCollection; }
IdCollection<GrassRecord>& Data::getGrassCollection() { return grassCollection; }
IdCollection<HairRecord>& Data::getHairCollection() { return hairCollection; }
IdCollection<IdleAnimationRecord>& Data::getIdleCollection() { return idleCollection; }
IdCollection<IdleMarkerRecord>& Data::getIdlmCollection() { return idlmCollection; }
IdCollection<ImgsRecord>& Data::getImgsCollection() { return imgsCollection; }
IdCollection<KeymRecord>& Data::getKeymCollection() { return keymCollection; }
IdCollection<KeywordRecord>& Data::getKywdCollection() { return kywdCollection; }
IdCollection<LighRecord>& Data::getLighCollection() { return lighCollection; }
IdCollection<LoadScreenRecord>& Data::getLscrCollection() { return lscrCollection; }
IdCollection<LvlcRecord>& Data::getLvlcCollection() { return lvlcCollection; }
IdCollection<LvliRecord>& Data::getLvliCollection() { return lvliCollection; }
IdCollection<LvspRecord>& Data::getLvspCollection() { return lvspCollection; }
IdCollection<MesgRecord>& Data::getMesgCollection() { return mesgCollection; }
IdCollection<MsttRecord>& Data::getMsttCollection() { return msttCollection; }
IdCollection<NavmRecord>& Data::getNavmCollection() { return navmCollection; }
IdCollection<NoteRecord>& Data::getNoteCollection() { return noteCollection; }
IdCollection<OutfitRecord>& Data::getOtftCollection() { return otftCollection; }
IdCollection<ProjRecord>& Data::getProjCollection() { return projCollection; }
IdCollection<RegionRecord>& Data::getRegnCollection() { return regnCollection; }
IdCollection<RoadRecord>& Data::getRoadCollection() { return roadCollection; }
IdCollection<ScriptRecord>& Data::getScptCollection() { return scptCollection; }
IdCollection<ScrRecord>& Data::getScrlCollection() { return scrlCollection; }
IdCollection<SlgmRecord>& Data::getSlgmCollection() { return slgmCollection; }
IdCollection<SmqnRecord>& Data::getSmqnCollection() { return smqnCollection; }
IdCollection<SpgdRecord>& Data::getSpgdCollection() { return spgdCollection; }
IdCollection<StaticCollectionRecord>& Data::getScolCollection() { return scolCollection; }
IdCollection<TextureSetRecord>& Data::getTxstCollection() { return txstCollection; }
IdCollection<WateRecord>& Data::getWateCollection() { return wateCollection; }
IdCollection<AnioRecord>& Data::getAnioCollection() { return anioCollection; }
IdCollection<ArtvRecord>& Data::getArtvCollection() { return artvCollection; }
IdCollection<ClfmRecord>& Data::getClfmCollection() { return clfmCollection; }
IdCollection<DebrRecord>& Data::getDebrCollection() { return debrCollection; }
IdCollection<EcznRecord>& Data::getEcznCollection() { return ecznCollection; }
IdCollection<HazdRecord>& Data::getHazdCollection() { return hazdCollection; }
IdCollection<IpctRecord>& Data::getIpctCollection() { return ipctCollection; }
IdCollection<IpdsRecord>& Data::getIpdsCollection() { return ipdsCollection; }
IdCollection<MustRecord>& Data::getMustCollection() { return mustCollection; }
IdCollection<RelaRecord>& Data::getRelaCollection() { return relaCollection; }
IdCollection<RevbRecord>& Data::getRevbCollection() { return revbCollection; }
IdCollection<ShouRecord>& Data::getShouCollection() { return shouCollection; }
IdCollection<HdptRecord>& Data::getHdptCollection() { return hdptCollection; }
IdCollection<TermRecord>& Data::getTermCollection() { return termCollection; }
IdCollection<MattRecord>& Data::getMattCollection() { return mattCollection; }
IdCollection<MovtRecord>& Data::getMovtCollection() { return movtCollection; }
IdCollection<MuscRecord>& Data::getMuscCollection() { return muscCollection; }
IdCollection<AactRecord>& Data::getAactCollection() { return aactCollection; }
IdCollection<AamdRecord>& Data::getAamdCollection() { return aamdCollection; }
IdCollection<AapdRecord>& Data::getAapdCollection() { return aapdCollection; }
IdCollection<AchrRecord>& Data::getAchrCollection() { return achrCollection; }
IdCollection<AddnRecord>& Data::getAddnCollection() { return addnCollection; }
IdCollection<AffeRecord>& Data::getAffeCollection() { return affeCollection; }
IdCollection<AmbsRecord>& Data::getAmbsCollection() { return ambsCollection; }
IdCollection<AmdlRecord>& Data::getAmdlCollection() { return amdlCollection; }
IdCollection<AopfRecord>& Data::getAopfCollection() { return aopfCollection; }
IdCollection<AopsRecord>& Data::getAopsCollection() { return aopsCollection; }
IdCollection<AoruRecord>& Data::getAoruCollection() { return aoruCollection; }
IdCollection<ArmaRecord>& Data::getArmaCollection() { return armaCollection; }
IdCollection<ArtoRecord>& Data::getArtoCollection() { return artoCollection; }
IdCollection<AspcRecord>& Data::getAspcCollection() { return aspcCollection; }
IdCollection<AtmoRecord>& Data::getAtmoCollection() { return atmoCollection; }
IdCollection<AvmdRecord>& Data::getAvmdCollection() { return avmdCollection; }
IdCollection<BiomRecord>& Data::getBiomCollection() { return biomCollection; }
IdCollection<BmmoRecord>& Data::getBmmoCollection() { return bmmoCollection; }
IdCollection<BmodRecord>& Data::getBmodCollection() { return bmodCollection; }
IdCollection<BndsRecord>& Data::getBndsCollection() { return bndsCollection; }
IdCollection<BptdRecord>& Data::getBptdCollection() { return bptdCollection; }
IdCollection<CamsRecord>& Data::getCamsCollection() { return camsCollection; }
IdCollection<ChalRecord>& Data::getChalCollection() { return chalCollection; }
IdCollection<CldfRecord>& Data::getCldfCollection() { return cldfCollection; }
IdCollection<CndfRecord>& Data::getCndfCollection() { return cndfCollection; }
IdCollection<CollRecord>& Data::getCollCollection() { return collCollection; }
IdCollection<CpthRecord>& Data::getCpthCollection() { return cpthCollection; }
IdCollection<DlbrRecord>& Data::getDlbrCollection() { return dlbrCollection; }
IdCollection<Cur3Record>& Data::getCur3Collection() { return cur3Collection; }
IdCollection<CurvRecord>& Data::getCurvCollection() { return curvCollection; }
IdCollection<DfobRecord>& Data::getDfobCollection() { return dfobCollection; }
IdCollection<DmgtRecord>& Data::getDmgtCollection() { return dmgtCollection; }
IdCollection<DobjRecord>& Data::getDobjCollection() { return dobjCollection; }
IdCollection<EfsqRecord>& Data::getEfsqCollection() { return efsqCollection; }
IdCollection<EqupRecord>& Data::getEqupCollection() { return equpCollection; }
IdCollection<FfkwRecord>& Data::getFfkwCollection() { return ffkwCollection; }
IdCollection<FogvRecord>& Data::getFogvCollection() { return fogvCollection; }
IdCollection<ForcRecord>& Data::getForcCollection() { return forcCollection; }
IdCollection<FstpRecord>& Data::getFstpCollection() { return fstpCollection; }
IdCollection<FstsRecord>& Data::getFstsCollection() { return fstsCollection; }
IdCollection<FxpdRecord>& Data::getFxpdCollection() { return fxpdCollection; }
IdCollection<GbfmRecord>& Data::getGbfmCollection() { return gbfmCollection; }
IdCollection<GbftRecord>& Data::getGbftCollection() { return gbftCollection; }
IdCollection<GcvrRecord>& Data::getGcvrCollection() { return gcvrCollection; }
IdCollection<ImadRecord>& Data::getImadCollection() { return imadCollection; }
IdCollection<InnrRecord>& Data::getInnrCollection() { return innrCollection; }
IdCollection<IresRecord>& Data::getIresCollection() { return iresCollection; }
IdCollection<KssmRecord>& Data::getKssmCollection() { return kssmCollection; }
IdCollection<LayrRecord>& Data::getLayrCollection() { return layrCollection; }
IdCollection<LensRecord>& Data::getLensCollection() { return lensCollection; }
IdCollection<LgdiRecord>& Data::getLgdiCollection() { return lgdiCollection; }
IdCollection<LgtmRecord>& Data::getLgtmCollection() { return lgtmCollection; }
IdCollection<LmswRecord>& Data::getLmswCollection() { return lmswCollection; }
IdCollection<LvlbRecord>& Data::getLvlbCollection() { return lvlbCollection; }
IdCollection<LvlnRecord>& Data::getLvlnCollection() { return lvlnCollection; }
IdCollection<LvlpRecord>& Data::getLvlpCollection() { return lvlpCollection; }
IdCollection<LvscRecord>& Data::getLvscCollection() { return lvscCollection; }
IdCollection<MaamRecord>& Data::getMaamCollection() { return maamCollection; }
IdCollection<MrhpRecord>& Data::getMrhpCollection() { return mrhpCollection; }
IdCollection<MtptRecord>& Data::getMtptCollection() { return mtptCollection; }
IdCollection<NaviRecord>& Data::getNaviCollection() { return naviCollection; }
IdCollection<NocmRecord>& Data::getNocmCollection() { return nocmCollection; }
IdCollection<OmodRecord>& Data::getOmodCollection() { return omodCollection; }
IdCollection<OswpRecord>& Data::getOswpCollection() { return oswpCollection; }
IdCollection<OvisRecord>& Data::getOvisCollection() { return ovisCollection; }
IdCollection<PcbnRecord>& Data::getPcbnCollection() { return pcbnCollection; }
IdCollection<PccnRecord>& Data::getPccnCollection() { return pccnCollection; }
IdCollection<PcmtRecord>& Data::getPcmtCollection() { return pcmtCollection; }
IdCollection<PdclRecord>& Data::getPdclCollection() { return pdclCollection; }
IdCollection<PgreRecord>& Data::getPgreCollection() { return pgreCollection; }
IdCollection<PhzdRecord>& Data::getPhzdCollection() { return phzdCollection; }
IdCollection<PkinRecord>& Data::getPkinCollection() { return pkinCollection; }
IdCollection<PmftRecord>& Data::getPmftCollection() { return pmftCollection; }
IdCollection<PsdcRecord>& Data::getPsdcCollection() { return psdcCollection; }
IdCollection<PtstRecord>& Data::getPtstCollection() { return ptstCollection; }
IdCollection<RfgpRecord>& Data::getRfgpCollection() { return rfgpCollection; }
IdCollection<RsgdRecord>& Data::getRsgdCollection() { return rsgdCollection; }
IdCollection<RspjRecord>& Data::getRspjCollection() { return rspjCollection; }
IdCollection<SdltRecord>& Data::getSdltCollection() { return sdltCollection; }
IdCollection<SechRecord>& Data::getSechCollection() { return sechCollection; }
IdCollection<SfbkRecord>& Data::getSfbkCollection() { return sfbkCollection; }
IdCollection<SfpcRecord>& Data::getSfpcCollection() { return sfpcCollection; }
IdCollection<SfptRecord>& Data::getSfptCollection() { return sfptCollection; }
IdCollection<SftrRecord>& Data::getSftrCollection() { return sftrCollection; }
IdCollection<SmbnRecord>& Data::getSmbnCollection() { return smbnCollection; }
IdCollection<SmenRecord>& Data::getSmenCollection() { return smenCollection; }
IdCollection<SpchRecord>& Data::getSpchCollection() { return spchCollection; }
IdCollection<StagRecord>& Data::getStagCollection() { return stagCollection; }
IdCollection<StbhRecord>& Data::getStbhCollection() { return stbhCollection; }
IdCollection<StdtRecord>& Data::getStdtCollection() { return stdtCollection; }
IdCollection<StmpRecord>& Data::getStmpCollection() { return stmpCollection; }
IdCollection<StndRecord>& Data::getStndCollection() { return stndCollection; }
IdCollection<SunpRecord>& Data::getSunpCollection() { return sunpCollection; }
IdCollection<TmlmRecord>& Data::getTmlmCollection() { return tmlmCollection; }
IdCollection<ToddRecord>& Data::getToddCollection() { return toddCollection; }
IdCollection<TravRecord>& Data::getTravCollection() { return travCollection; }
IdCollection<TrnsRecord>& Data::getTrnsCollection() { return trnsCollection; }
IdCollection<VoliRecord>& Data::getVoliCollection() { return voliCollection; }
IdCollection<VtypRecord>& Data::getVtypCollection() { return vtypCollection; }
IdCollection<WbarRecord>& Data::getWbarCollection() { return wbarCollection; }
IdCollection<WkmfRecord>& Data::getWkmfCollection() { return wkmfCollection; }
IdCollection<WthsRecord>& Data::getWthsCollection() { return wthsCollection; }
IdCollection<WwedRecord>& Data::getWwedCollection() { return wwedCollection; }
IdCollection<ZoomRecord>& Data::getZoomCollection() { return zoomCollection; }

QVector<IRecordCollection*> Data::allCollections()
{
    return {
        &gameSettings,
        &npcCollection,
        &weaponCollection,
        &armorCollection,
        &spellCollection,
        &magicCollection,
        &questCollection,
        &dialCollection,
        &infoCollection,
        &globCollection,
        &lcrtCollection,
        &packCollection,
        &treeCollection,
        &alchCollection,
        &ingrCollection,
        &contCollection,
        &enchCollection,
        &bookCollection,
        &miscCollection,
        &actiCollection,
        &statCollection,
        &raceCollection,
        &classCollection,
        &factCollection,
        &perkCollection,
        &cellCollection,
        &worldspaceCollection,
        &locationCollection,
        &planetCollection,
        &refrCollection,
        &materialCollection,
        &landCollection,
        &sounCollection,
        &wthrCollection,
        &ltexCollection,
        &ammoCollection,
        &appaCollection,
        &avifCollection,
        &bsgnCollection,
        &clmtCollection,
        &clotCollection,
        &cobjCollection,
        &creatureCollection,
        &cstyCollection,
        &doorCollection,
        &efshCollection,
        &explCollection,
        &eyesCollection,
        &florCollection,
        &flstCollection,
        &furnCollection,
        &grassCollection,
        &hairCollection,
        &idleCollection,
        &idlmCollection,
        &imgsCollection,
        &keymCollection,
        &kywdCollection,
        &lighCollection,
        &lscrCollection,
        &lvlcCollection,
        &lvliCollection,
        &lvspCollection,
        &mesgCollection,
        &msttCollection,
        &navmCollection,
        &noteCollection,
        &otftCollection,
        &projCollection,
        &regnCollection,
        &roadCollection,
        &scptCollection,
        &scrlCollection,
        &slgmCollection,
        &smqnCollection,
        &spgdCollection,
        &scolCollection,
        &txstCollection,
        &wateCollection,
        &scenCollection,
        &hdptCollection,
        &termCollection,
        &mattCollection,
        &movtCollection,
        &muscCollection,
        &aactCollection,
        &aamdCollection,
        &aapdCollection,
        &achrCollection,
        &addnCollection,
        &affeCollection,
        &ambsCollection,
        &amdlCollection,
        &aopfCollection,
        &aopsCollection,
        &aoruCollection,
        &armaCollection,
        &artoCollection,
        &aspcCollection,
        &atmoCollection,
        &avmdCollection,
        &biomCollection,
        &bmmoCollection,
        &bmodCollection,
        &bndsCollection,
        &bptdCollection,
        &camsCollection,
        &chalCollection,
        &cldfCollection,
        &cndfCollection,
        &collCollection,
        &cpthCollection,
        &dlbrCollection,
        &cur3Collection,
        &curvCollection,
        &dfobCollection,
        &dmgtCollection,
        &dobjCollection,
        &efsqCollection,
        &equpCollection,
        &ffkwCollection,
        &fogvCollection,
        &forcCollection,
        &fstpCollection,
        &fstsCollection,
        &fxpdCollection,
        &gbfmCollection,
        &gbftCollection,
        &gcvrCollection,
        &imadCollection,
        &innrCollection,
        &iresCollection,
        &kssmCollection,
        &layrCollection,
        &lensCollection,
        &lgdiCollection,
        &lgtmCollection,
        &lmswCollection,
        &lvlbCollection,
        &lvlnCollection,
        &lvlpCollection,
        &lvscCollection,
        &maamCollection,
        &mrhpCollection,
        &mtptCollection,
        &naviCollection,
        &nocmCollection,
        &omodCollection,
        &oswpCollection,
        &ovisCollection,
        &pcbnCollection,
        &pccnCollection,
        &pcmtCollection,
        &pdclCollection,
        &pgreCollection,
        &phzdCollection,
        &pkinCollection,
        &pmftCollection,
        &psdcCollection,
        &ptstCollection,
        &rfgpCollection,
        &rsgdCollection,
        &rspjCollection,
        &sdltCollection,
        &sechCollection,
        &sfbkCollection,
        &sfpcCollection,
        &sfptCollection,
        &sftrCollection,
        &smbnCollection,
        &smenCollection,
        &spchCollection,
        &stagCollection,
        &stbhCollection,
        &stdtCollection,
        &stmpCollection,
        &stndCollection,
        &sunpCollection,
        &tmlmCollection,
        &toddCollection,
        &travCollection,
        &trnsCollection,
        &voliCollection,
        &vtypCollection,
        &wbarCollection,
        &wkmfCollection,
        &wthsCollection,
        &wwedCollection,
        &zoomCollection,
    };
}

QVector<Data::TypedCollection> Data::allCollectionsWithTypes()
{
    return {
        {&gameSettings,      CkId::Type_Gmst},
        {&npcCollection,     CkId::Type_Npc_},
        {&weaponCollection,  CkId::Type_Weap_},
        {&armorCollection,   CkId::Type_Armor_},
        {&spellCollection,   CkId::Type_Spel_},
        {&magicCollection,   CkId::Type_Magic_},
        {&questCollection,   CkId::Type_Quest_},
        {&dialCollection,    CkId::Type_Dial_},
        {&infoCollection,    CkId::Type_Info_},
        {&globCollection,    CkId::Type_Glob_},
        {&lcrtCollection,    CkId::Type_Lcrt_},
        {&packCollection,    CkId::Type_Pack_},
        {&treeCollection,    CkId::Type_Tree_},
        {&alchCollection,    CkId::Type_Alch_},
        {&ingrCollection,    CkId::Type_Ingr_},
        {&contCollection,    CkId::Type_Cont_},
        {&enchCollection,    CkId::Type_Ench_},
        {&bookCollection,    CkId::Type_Book_},
        {&miscCollection,    CkId::Type_Misc_},
        {&actiCollection,    CkId::Type_Acti_},
        {&statCollection,    CkId::Type_Stat_},
        {&raceCollection,    CkId::Type_Race_},
        {&classCollection,   CkId::Type_Class_},
        {&factCollection,    CkId::Type_Fact_},
        {&perkCollection,    CkId::Type_PerK_},
        {&cellCollection,    CkId::Type_Cel_},
        {&worldspaceCollection, CkId::Type_WRLD_},
        {&locationCollection, CkId::Type_LOCT_},
        {&planetCollection,    CkId::Type_Plnt_},
        {&refrCollection,    CkId::Type_Refr_},
        {&materialCollection, CkId::Type_Material_},
        {&landCollection,    CkId::Type_Land_},
        {&sounCollection,    CkId::Type_Soun_},
        {&wthrCollection,    CkId::Type_Wthr_},
        {&ltexCollection,    CkId::Type_Ltex_},
        {&ammoCollection,    CkId::Type_Ammo_},
        {&appaCollection,    CkId::Type_Appa_},
        {&avifCollection,    CkId::Type_Avif_},
        {&bsgnCollection,    CkId::Type_Bsgn_},
        {&clmtCollection,    CkId::Type_Clmt_},
        {&clotCollection,    CkId::Type_Clot_},
        {&cobjCollection,    CkId::Type_Cobj_},
        {&creatureCollection,    CkId::Type_Crea_},
        {&cstyCollection,    CkId::Type_Csty_},
        {&doorCollection,    CkId::Type_Door_},
        {&efshCollection,    CkId::Type_Efsh_},
        {&explCollection,    CkId::Type_Expl_},
        {&eyesCollection,    CkId::Type_Eyes_},
        {&florCollection,    CkId::Type_Flor_},
        {&flstCollection,    CkId::Type_Flst_},
        {&furnCollection,    CkId::Type_Furn_},
        {&grassCollection,    CkId::Type_Grass_},
        {&hairCollection,    CkId::Type_Hair_},
        {&idleCollection,    CkId::Type_Idle_},
        {&idlmCollection,    CkId::Type_Idlm_},
        {&imgsCollection,    CkId::Type_Imgs_},
        {&keymCollection,    CkId::Type_Keym_},
        {&kywdCollection,    CkId::Type_Kywd_},
        {&lighCollection,    CkId::Type_Ligh_},
        {&lscrCollection,    CkId::Type_Lscr_},
        {&lvlcCollection,    CkId::Type_Lvlc_},
        {&lvliCollection,    CkId::Type_Lvli_},
        {&lvspCollection,    CkId::Type_Lvsp_},
        {&mesgCollection,    CkId::Type_Mesg_},
        {&msttCollection,    CkId::Type_Mstt_},
        {&navmCollection,    CkId::Type_Navm_},
        {&noteCollection,    CkId::Type_Note_},
        {&otftCollection,    CkId::Type_Otft_},
        {&projCollection,    CkId::Type_Proj_},
        {&regnCollection,    CkId::Type_Regn_},
        {&roadCollection,    CkId::Type_Road_},
        {&scptCollection,    CkId::Type_Scpt_},
        {&scrlCollection,    CkId::Type_Scrl_},
        {&slgmCollection,    CkId::Type_Slgm_},
        {&smqnCollection,    CkId::Type_Smqn_},
        {&spgdCollection,    CkId::Type_Spgd_},
        {&scolCollection,    CkId::Type_Scol_},
        {&txstCollection,    CkId::Type_Txst_},
        {&wateCollection,    CkId::Type_Wate_},
        {&anioCollection,    CkId::Type_Anio_},
        {&artvCollection,    CkId::Type_Artv_},
        {&clfmCollection,    CkId::Type_Clfm_},
        {&debrCollection,    CkId::Type_Debr_},
        {&ecznCollection,    CkId::Type_Eczn_},
        {&hazdCollection,    CkId::Type_Hazd_},
        {&ipctCollection,    CkId::Type_Ipct_},
        {&ipdsCollection,    CkId::Type_Ipds_},
        {&mustCollection,    CkId::Type_Must_},
        {&relaCollection,    CkId::Type_Rela_},
        {&revbCollection,    CkId::Type_Revb_},
        {&shouCollection,    CkId::Type_Shou_},
        {&hdptCollection,    CkId::Type_Hdpt_},
        {&termCollection,    CkId::Type_Term_},
        {&mattCollection,    CkId::Type_Matt_},
        {&movtCollection,    CkId::Type_Movt_},
        {&muscCollection,    CkId::Type_Musc_},
        {&aactCollection,    CkId::Type_Aact_},
        {&aamdCollection,    CkId::Type_Aamd_},
        {&aapdCollection,    CkId::Type_Aapd_},
        {&achrCollection,    CkId::Type_Achr_},
        {&addnCollection,    CkId::Type_Addn_},
        {&affeCollection,    CkId::Type_Affe_},
        {&ambsCollection,    CkId::Type_Ambs_},
        {&amdlCollection,    CkId::Type_Amdl_},
        {&aopfCollection,    CkId::Type_Aopf_},
        {&aopsCollection,    CkId::Type_Aops_},
        {&aoruCollection,    CkId::Type_Aoru_},
        {&armaCollection,    CkId::Type_Arma_},
        {&artoCollection,    CkId::Type_Arto_},
        {&aspcCollection,    CkId::Type_Aspc_},
        {&atmoCollection,    CkId::Type_Atmo_},
        {&avmdCollection,    CkId::Type_Avmd_},
        {&biomCollection,    CkId::Type_Biom_},
        {&bmmoCollection,    CkId::Type_Bmmo_},
        {&bmodCollection,    CkId::Type_Bmod_},
        {&bndsCollection,    CkId::Type_Bnds_},
        {&bptdCollection,    CkId::Type_Bptd_},
        {&camsCollection,    CkId::Type_Cams_},
        {&chalCollection,    CkId::Type_Chal_},
        {&cldfCollection,    CkId::Type_Cldf_},
        {&cndfCollection,    CkId::Type_Cndf_},
        {&collCollection,    CkId::Type_Coll_},
        {&cpthCollection,    CkId::Type_Cpth_},
        {&dlbrCollection,    CkId::Type_Dlbr_},
        {&cur3Collection,    CkId::Type_Cur3_},
        {&curvCollection,    CkId::Type_Curv_},
        {&dfobCollection,    CkId::Type_Dfob_},
        {&dmgtCollection,    CkId::Type_Dmgt_},
        {&dobjCollection,    CkId::Type_Dobj_},
        {&efsqCollection,    CkId::Type_Efsq_},
        {&equpCollection,    CkId::Type_Equp_},
        {&ffkwCollection,    CkId::Type_Ffkw_},
        {&fogvCollection,    CkId::Type_Fogv_},
        {&forcCollection,    CkId::Type_Forc_},
        {&fstpCollection,    CkId::Type_Fstp_},
        {&fstsCollection,    CkId::Type_Fsts_},
        {&fxpdCollection,    CkId::Type_Fxpd_},
        {&gbfmCollection,    CkId::Type_Gbfm_},
        {&gbftCollection,    CkId::Type_Gbft_},
        {&gcvrCollection,    CkId::Type_Gcvr_},
        {&imadCollection,    CkId::Type_Imad_},
        {&innrCollection,    CkId::Type_Innr_},
        {&iresCollection,    CkId::Type_Ires_},
        {&kssmCollection,    CkId::Type_Kssm_},
        {&layrCollection,    CkId::Type_Layr_},
        {&lensCollection,    CkId::Type_Lens_},
        {&lgdiCollection,    CkId::Type_Lgdi_},
        {&lgtmCollection,    CkId::Type_Lgtm_},
        {&lmswCollection,    CkId::Type_Lmsw_},
        {&lvlbCollection,    CkId::Type_Lvlb_},
        {&lvlnCollection,    CkId::Type_Lvln_},
        {&lvlpCollection,    CkId::Type_Lvlp_},
        {&lvscCollection,    CkId::Type_Lvsc_},
        {&maamCollection,    CkId::Type_Maam_},
        {&mrhpCollection,    CkId::Type_Mrhp_},
        {&mtptCollection,    CkId::Type_Mtpt_},
        {&naviCollection,    CkId::Type_Navi_},
        {&nocmCollection,    CkId::Type_Nocm_},
        {&omodCollection,    CkId::Type_Omod_},
        {&oswpCollection,    CkId::Type_Oswp_},
        {&ovisCollection,    CkId::Type_Ovis_},
        {&pcbnCollection,    CkId::Type_Pcbn_},
        {&pccnCollection,    CkId::Type_Pccn_},
        {&pcmtCollection,    CkId::Type_Pcmt_},
        {&pdclCollection,    CkId::Type_Pdcl_},
        {&pgreCollection,    CkId::Type_Pgre_},
        {&scenCollection,    CkId::Type_Scen_},
        {&phzdCollection,    CkId::Type_Phzd_},
        {&pkinCollection,    CkId::Type_Pkin_},
        {&pmftCollection,    CkId::Type_Pmft_},
        {&psdcCollection,    CkId::Type_Psdc_},
        {&ptstCollection,    CkId::Type_Ptst_},
        {&rfgpCollection,    CkId::Type_Rfgp_},
        {&rsgdCollection,    CkId::Type_Rsgd_},
        {&rspjCollection,    CkId::Type_Rspj_},
        {&sdltCollection,    CkId::Type_Sdlt_},
        {&sechCollection,    CkId::Type_Sech_},
        {&sfbkCollection,    CkId::Type_Sfbk_},
        {&sfpcCollection,    CkId::Type_Sfpc_},
        {&sfptCollection,    CkId::Type_Sfpt_},
        {&sftrCollection,    CkId::Type_Sftr_},
        {&smbnCollection,    CkId::Type_Smbn_},
        {&smenCollection,    CkId::Type_Smen_},
        {&spchCollection,    CkId::Type_Spch_},
        {&stagCollection,    CkId::Type_Stag_},
        {&stbhCollection,    CkId::Type_Stbh_},
        {&stdtCollection,    CkId::Type_Stdt_},
        {&stmpCollection,    CkId::Type_Stmp_},
        {&stndCollection,    CkId::Type_Stnd_},
        {&sunpCollection,    CkId::Type_Sunp_},
        {&tmlmCollection,    CkId::Type_Tmlm_},
        {&toddCollection,    CkId::Type_Todd_},
        {&travCollection,    CkId::Type_Trav_},
        {&trnsCollection,    CkId::Type_Trns_},
        {&voliCollection,    CkId::Type_Voli_},
        {&vtypCollection,    CkId::Type_Vtyp_},
        {&wbarCollection,    CkId::Type_Wbar_},
        {&wkmfCollection,    CkId::Type_Wkmf_},
        {&wthsCollection,    CkId::Type_Wths_},
        {&wwedCollection,    CkId::Type_Wwed_},
        {&zoomCollection,    CkId::Type_Zoom_},
    };
}

QVector<IRecordCollection*> Data::allCollections() const
{
    return const_cast<Data*>(this)->allCollections();
}

QVector<Data::TypedCollection> Data::allCollectionsWithTypes() const
{
    return const_cast<Data*>(this)->allCollectionsWithTypes();
}

void Data::addModel(QAbstractItemModel* model, CkId::Type type, bool update)
{
    models.push_back(model);
    modelIndexes.insert(type, model);

    auto* idTable = qobject_cast<IdTable*>(model);
    if (idTable)
    {
        idTable->setUndoStack(mUndoStack);
    }

    if (update)
    {
        connect(model, &QAbstractItemModel::dataChanged,
            this, &Data::dataChanged);
    }
}

QAbstractItemModel* Data::getTableModel(const CkId& id)
{
    auto it = modelIndexes.find(id.getType());

    if (it == modelIndexes.end())
    {
        throw std::logic_error("No table model available for " + id.toString().toStdString());
    }

    return it.value();
}

bool Data::cloneRecord(CkId::Type type, const QString& src, const QString& dest)
{
    BaseCollection* col = getCollectionByType(type);
    if (!col)
    {
        LOG_ERROR(QString("cloneRecord failed: no collection for type %1").arg(static_cast<int>(type)));
        QMessageBox::critical(nullptr, tr("Error"), tr("Clone failed: no collection for this record type."));
        return false;
    }

    int srcIndex = col->searchId(src);
    if (srcIndex == -1)
    {
        LOG_ERROR(QString("cloneRecord failed: source record '%1' not found").arg(src));
        QMessageBox::critical(nullptr, tr("Error"), tr("Clone failed: source record '%1' not found.").arg(src));
        return false;
    }

    try
    {
        col->cloneRecord(src, dest, type);
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(QString("cloneRecord failed: %1").arg(e.what()));
        QMessageBox::critical(nullptr, tr("Error"), tr("Clone failed: %1").arg(e.what()));
        return false;
    }
}

bool Data::cloneRecordWithUndo(CkId::Type type, const QString& src, const QString& dest)
{
    BaseCollection* col = getCollectionByType(type);
    if (!col)
    {
        LOG_ERROR(QString("cloneRecordWithUndo failed: no collection for type %1").arg(static_cast<int>(type)));
        return false;
    }
    return col->cloneRecordWithUndo(src, dest, mUndoStack);
}

void Data::batchCloneWithUndo(CkId::Type type, const QVector<QString>& srcIds, const QVector<QString>& destIds)
{
    BaseCollection* col = getCollectionByType(type);
    if (!col) return;
    col->batchCloneWithUndo(srcIds, destIds, mUndoStack);
}

void Data::batchSetEditorIdWithUndo(CkId::Type type, const QVector<QString>& srcIds, const QString& newEditorId)
{
    BaseCollection* col = getCollectionByType(type);
    if (!col) return;
    col->batchSetEditorIdWithUndo(srcIds, newEditorId, mUndoStack);
}

bool Data::removeRecord(CkId::Type type, const QString& id)
{
    BaseCollection* col = getCollectionByType(type);
    if (!col) return false;
    return col->removeRecordWithUndo(id, mUndoStack);
}

bool Data::addNpc(NpcRecord& record)
{
    if (npcCollection.searchId(record.editorId) >= 0)
        return false;
    npcCollection.add(record);
    return true;
}

bool Data::addWeapon(WeaponRecord& record)
{
    if (weaponCollection.searchId(record.editorId) >= 0)
        return false;
    weaponCollection.add(record);
    return true;
}

bool Data::addArmor(ArmorRecord& record)
{
    if (armorCollection.searchId(record.editorId) >= 0)
        return false;
    armorCollection.add(record);
    return true;
}

bool Data::addSpell(SpellRecord& record)
{
    if (spellCollection.searchId(record.editorId) >= 0)
        return false;
    spellCollection.add(record);
    return true;
}

bool Data::addQuest(QuestRecord& record)
{
    if (questCollection.searchId(record.editorId) >= 0)
        return false;
    questCollection.add(record);
    return true;
}

bool Data::addDial(DialRecord& record)
{
    if (dialCollection.searchId(record.editorId) >= 0)
        return false;
    dialCollection.add(record);
    return true;
}

bool Data::addInfo(InfoRecord& record)
{
    if (infoCollection.searchId(record.editorId) >= 0)
        return false;
    infoCollection.add(record);
    return true;
}

bool Data::addGlobVar(GlobalVariable& record)
{
    if (globCollection.searchId(record.editorId) >= 0)
        return false;
    globCollection.add(record);
    return true;
}

bool Data::addTree(TreeRecord& record)
{
    if (treeCollection.searchId(record.editorId) >= 0)
        return false;
    treeCollection.add(record);
    return true;
}

bool Data::addStat(StatRecord& record)
{
    if (statCollection.searchId(record.editorId) >= 0)
        return false;
    statCollection.add(record);
    return true;
}

bool Data::addActi(ActiRecord& record)
{
    if (actiCollection.searchId(record.editorId) >= 0)
        return false;
    actiCollection.add(record);
    return true;
}

bool Data::addMisc(MiscRecord& record)
{
    if (miscCollection.searchId(record.editorId) >= 0)
        return false;
    miscCollection.add(record);
    return true;
}

bool Data::addAlch(AlchRecord& record)
{
    if (alchCollection.searchId(record.editorId) >= 0)
        return false;
    alchCollection.add(record);
    return true;
}

bool Data::addIngr(IngrRecord& record)
{
    if (ingrCollection.searchId(record.editorId) >= 0)
        return false;
    ingrCollection.add(record);
    return true;
}

bool Data::addBook(BookRecord& record)
{
    if (bookCollection.searchId(record.editorId) >= 0)
        return false;
    bookCollection.add(record);
    return true;
}

bool Data::addEnch(EnchRecord& record)
{
    if (enchCollection.searchId(record.editorId) >= 0)
        return false;
    enchCollection.add(record);
    return true;
}

bool Data::addCont(ContRecord& record)
{
    if (contCollection.searchId(record.editorId) >= 0)
        return false;
    contCollection.add(record);
    return true;
}

bool Data::addRace(RaceRecord& record)
{
    if (raceCollection.searchId(record.editorId) >= 0)
        return false;
    raceCollection.add(record);
    return true;
}

bool Data::addPerk(PerkRecord& record)
{
    if (perkCollection.searchId(record.editorId) >= 0)
        return false;
    perkCollection.add(record);
    return true;
}

bool Data::addMagic(MagicRecord& record)
{
    if (magicCollection.searchId(record.editorId) >= 0)
        return false;
    magicCollection.add(record);
    return true;
}

bool Data::addPack(PackageRecord& record)
{
    if (packCollection.searchId(record.editorId) >= 0)
        return false;
    packCollection.add(record);
    return true;
}

bool Data::addLcrt(LocationRefType& record)
{
    if (lcrtCollection.searchId(record.editorId) >= 0)
        return false;
    lcrtCollection.add(record);
    return true;
}

bool Data::addClass(ClassRecord& record)
{
    if (classCollection.searchId(record.editorId) >= 0)
        return false;
    classCollection.add(record);
    return true;
}

bool Data::addFact(FactRecord& record)
{
    if (factCollection.searchId(record.editorId) >= 0)
        return false;
    factCollection.add(record);
    return true;
}

bool Data::addCell(CellRecord& record)
{
    if (cellCollection.searchId(record.editorId) >= 0)
        return false;
    cellCollection.add(record);
    return true;
}

bool Data::addWorldspace(WorldspaceRecord& record)
{
    if (worldspaceCollection.searchId(record.editorId) >= 0)
        return false;
    worldspaceCollection.add(record);
    return true;
}

bool Data::addLocation(LocationRecord& record)
{
    if (locationCollection.searchId(record.editorId) >= 0)
        return false;
    locationCollection.add(record);
    return true;
}

bool Data::addPlanet(PndRecord& record)
{
    if (planetCollection.searchId(record.editorId) >= 0)
        return false;
    planetCollection.add(record);
    return true;
}

bool Data::addRef(RefrRecord& record)
{
    if (refrCollection.searchId(record.editorId) >= 0)
        return false;
    refrCollection.add(record);
    return true;
}

const IdCollection<MaterialRecord>& Data::getMaterialCollection() const
{
    return materialCollection;
}

const IdCollection<LandRecord>& Data::getLandCollection() const
{
    return landCollection;
}

const IdCollection<SounRecord>& Data::getSounCollection() const
{
    return sounCollection;
}

const IdCollection<WthrRecord>& Data::getWthrCollection() const
{
    return wthrCollection;
}

const IdCollection<LtexRecord>& Data::getLtexCollection() const
{
    return ltexCollection;
}

const IdCollection<ScenRecord>& Data::getScenCollection() const
{
    return scenCollection;
}

const IdCollection<AmmoRecord>& Data::getAmmoCollection() const
{
    return ammoCollection;
}

const IdCollection<AppaRecord>& Data::getAppaCollection() const
{
    return appaCollection;
}

const IdCollection<ActorValueInfoRecord>& Data::getAvifCollection() const
{
    return avifCollection;
}

const IdCollection<BsgnRecord>& Data::getBsgnCollection() const
{
    return bsgnCollection;
}

const IdCollection<ClimateRecord>& Data::getClmtCollection() const
{
    return clmtCollection;
}

const IdCollection<ClotRecord>& Data::getClotCollection() const
{
    return clotCollection;
}

const IdCollection<CobjRecord>& Data::getCobjCollection() const
{
    return cobjCollection;
}

const IdCollection<CreatureRecord>& Data::getCreatureCollection() const
{
    return creatureCollection;
}

const IdCollection<CstyRecord>& Data::getCstyCollection() const
{
    return cstyCollection;
}

const IdCollection<DoorRecord>& Data::getDoorCollection() const
{
    return doorCollection;
}

const IdCollection<EfshRecord>& Data::getEfshCollection() const
{
    return efshCollection;
}

const IdCollection<ExplRecord>& Data::getExplCollection() const
{
    return explCollection;
}

const IdCollection<EyesRecord>& Data::getEyesCollection() const
{
    return eyesCollection;
}

const IdCollection<FlorRecord>& Data::getFlorCollection() const
{
    return florCollection;
}

const IdCollection<FormListRecord>& Data::getFlstCollection() const
{
    return flstCollection;
}

const IdCollection<FurnRecord>& Data::getFurnCollection() const
{
    return furnCollection;
}

const IdCollection<GrassRecord>& Data::getGrassCollection() const
{
    return grassCollection;
}

const IdCollection<HairRecord>& Data::getHairCollection() const
{
    return hairCollection;
}

const IdCollection<IdleAnimationRecord>& Data::getIdleCollection() const
{
    return idleCollection;
}

const IdCollection<IdleMarkerRecord>& Data::getIdlmCollection() const
{
    return idlmCollection;
}

const IdCollection<ImgsRecord>& Data::getImgsCollection() const
{
    return imgsCollection;
}

const IdCollection<KeymRecord>& Data::getKeymCollection() const
{
    return keymCollection;
}

const IdCollection<KeywordRecord>& Data::getKywdCollection() const
{
    return kywdCollection;
}

const IdCollection<LighRecord>& Data::getLighCollection() const
{
    return lighCollection;
}

const IdCollection<LoadScreenRecord>& Data::getLscrCollection() const
{
    return lscrCollection;
}

const IdCollection<LvlcRecord>& Data::getLvlcCollection() const
{
    return lvlcCollection;
}

const IdCollection<LvliRecord>& Data::getLvliCollection() const
{
    return lvliCollection;
}

const IdCollection<LvspRecord>& Data::getLvspCollection() const
{
    return lvspCollection;
}

const IdCollection<MesgRecord>& Data::getMesgCollection() const
{
    return mesgCollection;
}

const IdCollection<MsttRecord>& Data::getMsttCollection() const
{
    return msttCollection;
}

const IdCollection<NavmRecord>& Data::getNavmCollection() const
{
    return navmCollection;
}

const IdCollection<NoteRecord>& Data::getNoteCollection() const
{
    return noteCollection;
}

const IdCollection<OutfitRecord>& Data::getOtftCollection() const
{
    return otftCollection;
}

const IdCollection<ProjRecord>& Data::getProjCollection() const
{
    return projCollection;
}

const IdCollection<RegionRecord>& Data::getRegnCollection() const
{
    return regnCollection;
}

const IdCollection<RoadRecord>& Data::getRoadCollection() const
{
    return roadCollection;
}

const IdCollection<ScriptRecord>& Data::getScptCollection() const
{
    return scptCollection;
}

const IdCollection<ScrRecord>& Data::getScrlCollection() const
{
    return scrlCollection;
}

const IdCollection<SlgmRecord>& Data::getSlgmCollection() const
{
    return slgmCollection;
}

const IdCollection<SmqnRecord>& Data::getSmqnCollection() const
{
    return smqnCollection;
}

const IdCollection<SpgdRecord>& Data::getSpgdCollection() const
{
    return spgdCollection;
}

const IdCollection<StaticCollectionRecord>& Data::getScolCollection() const
{
    return scolCollection;
}

const IdCollection<TextureSetRecord>& Data::getTxstCollection() const
{
    return txstCollection;
}

const IdCollection<WateRecord>& Data::getWateCollection() const
{
    return wateCollection;
}
const IdCollection<AnioRecord>& Data::getAnioCollection() const
{
    return anioCollection;
}
const IdCollection<ArtvRecord>& Data::getArtvCollection() const
{
    return artvCollection;
}
const IdCollection<ClfmRecord>& Data::getClfmCollection() const
{
    return clfmCollection;
}
const IdCollection<DebrRecord>& Data::getDebrCollection() const
{
    return debrCollection;
}
const IdCollection<EcznRecord>& Data::getEcznCollection() const
{
    return ecznCollection;
}
const IdCollection<HazdRecord>& Data::getHazdCollection() const
{
    return hazdCollection;
}
const IdCollection<IpctRecord>& Data::getIpctCollection() const
{
    return ipctCollection;
}
const IdCollection<IpdsRecord>& Data::getIpdsCollection() const
{
    return ipdsCollection;
}
const IdCollection<MustRecord>& Data::getMustCollection() const
{
    return mustCollection;
}
const IdCollection<RelaRecord>& Data::getRelaCollection() const
{
    return relaCollection;
}
const IdCollection<RevbRecord>& Data::getRevbCollection() const
{
    return revbCollection;
}
const IdCollection<ShouRecord>& Data::getShouCollection() const
{
    return shouCollection;
}
const IdCollection<HdptRecord>& Data::getHdptCollection() const
{
    return hdptCollection;
}
const IdCollection<TermRecord>& Data::getTermCollection() const
{
    return termCollection;
}
const IdCollection<MattRecord>& Data::getMattCollection() const
{
    return mattCollection;
}
const IdCollection<MovtRecord>& Data::getMovtCollection() const
{
    return movtCollection;
}
const IdCollection<MuscRecord>& Data::getMuscCollection() const
{
    return muscCollection;
}
const IdCollection<AactRecord>& Data::getAactCollection() const
{
    return aactCollection;
}
const IdCollection<AamdRecord>& Data::getAamdCollection() const
{
    return aamdCollection;
}
const IdCollection<AapdRecord>& Data::getAapdCollection() const
{
    return aapdCollection;
}
const IdCollection<AchrRecord>& Data::getAchrCollection() const
{
    return achrCollection;
}
const IdCollection<AddnRecord>& Data::getAddnCollection() const
{
    return addnCollection;
}
const IdCollection<AffeRecord>& Data::getAffeCollection() const
{
    return affeCollection;
}
const IdCollection<AmbsRecord>& Data::getAmbsCollection() const
{
    return ambsCollection;
}
const IdCollection<AmdlRecord>& Data::getAmdlCollection() const
{
    return amdlCollection;
}
const IdCollection<AopfRecord>& Data::getAopfCollection() const
{
    return aopfCollection;
}
const IdCollection<AopsRecord>& Data::getAopsCollection() const
{
    return aopsCollection;
}
const IdCollection<AoruRecord>& Data::getAoruCollection() const
{
    return aoruCollection;
}
const IdCollection<ArmaRecord>& Data::getArmaCollection() const
{
    return armaCollection;
}
const IdCollection<ArtoRecord>& Data::getArtoCollection() const
{
    return artoCollection;
}
const IdCollection<AspcRecord>& Data::getAspcCollection() const
{
    return aspcCollection;
}
const IdCollection<AtmoRecord>& Data::getAtmoCollection() const
{
    return atmoCollection;
}
const IdCollection<AvmdRecord>& Data::getAvmdCollection() const
{
    return avmdCollection;
}
const IdCollection<BiomRecord>& Data::getBiomCollection() const
{
    return biomCollection;
}
const IdCollection<BmmoRecord>& Data::getBmmoCollection() const
{
    return bmmoCollection;
}
const IdCollection<BmodRecord>& Data::getBmodCollection() const
{
    return bmodCollection;
}
const IdCollection<BndsRecord>& Data::getBndsCollection() const
{
    return bndsCollection;
}
const IdCollection<BptdRecord>& Data::getBptdCollection() const
{
    return bptdCollection;
}
const IdCollection<CamsRecord>& Data::getCamsCollection() const
{
    return camsCollection;
}
const IdCollection<ChalRecord>& Data::getChalCollection() const
{
    return chalCollection;
}
const IdCollection<CldfRecord>& Data::getCldfCollection() const
{
    return cldfCollection;
}
const IdCollection<CndfRecord>& Data::getCndfCollection() const
{
    return cndfCollection;
}
const IdCollection<CollRecord>& Data::getCollCollection() const
{
    return collCollection;
}
const IdCollection<CpthRecord>& Data::getCpthCollection() const
{
    return cpthCollection;
}
const IdCollection<DlbrRecord>& Data::getDlbrCollection() const
{
    return dlbrCollection;
}
const IdCollection<Cur3Record>& Data::getCur3Collection() const
{
    return cur3Collection;
}
const IdCollection<CurvRecord>& Data::getCurvCollection() const
{
    return curvCollection;
}
const IdCollection<DfobRecord>& Data::getDfobCollection() const
{
    return dfobCollection;
}
const IdCollection<DmgtRecord>& Data::getDmgtCollection() const
{
    return dmgtCollection;
}
const IdCollection<DobjRecord>& Data::getDobjCollection() const
{
    return dobjCollection;
}
const IdCollection<EfsqRecord>& Data::getEfsqCollection() const
{
    return efsqCollection;
}
const IdCollection<EqupRecord>& Data::getEqupCollection() const
{
    return equpCollection;
}
const IdCollection<FfkwRecord>& Data::getFfkwCollection() const
{
    return ffkwCollection;
}

const IdCollection<FogvRecord>& Data::getFogvCollection() const
{
    return fogvCollection;
}

const IdCollection<ForcRecord>& Data::getForcCollection() const
{
    return forcCollection;
}

const IdCollection<FstpRecord>& Data::getFstpCollection() const
{
    return fstpCollection;
}

const IdCollection<FstsRecord>& Data::getFstsCollection() const
{
    return fstsCollection;
}

const IdCollection<FxpdRecord>& Data::getFxpdCollection() const
{
    return fxpdCollection;
}

const IdCollection<GbfmRecord>& Data::getGbfmCollection() const
{
    return gbfmCollection;
}

const IdCollection<GbftRecord>& Data::getGbftCollection() const
{
    return gbftCollection;
}

const IdCollection<GcvrRecord>& Data::getGcvrCollection() const
{
    return gcvrCollection;
}

const IdCollection<ImadRecord>& Data::getImadCollection() const
{
    return imadCollection;
}

const IdCollection<InnrRecord>& Data::getInnrCollection() const
{
    return innrCollection;
}

const IdCollection<IresRecord>& Data::getIresCollection() const
{
    return iresCollection;
}

const IdCollection<KssmRecord>& Data::getKssmCollection() const
{
    return kssmCollection;
}

const IdCollection<LayrRecord>& Data::getLayrCollection() const
{
    return layrCollection;
}

const IdCollection<LensRecord>& Data::getLensCollection() const
{
    return lensCollection;
}

const IdCollection<LgdiRecord>& Data::getLgdiCollection() const
{
    return lgdiCollection;
}

const IdCollection<LgtmRecord>& Data::getLgtmCollection() const
{
    return lgtmCollection;
}

const IdCollection<LmswRecord>& Data::getLmswCollection() const
{
    return lmswCollection;
}

const IdCollection<LvlbRecord>& Data::getLvlbCollection() const
{
    return lvlbCollection;
}

const IdCollection<LvlnRecord>& Data::getLvlnCollection() const
{
    return lvlnCollection;
}

const IdCollection<LvlpRecord>& Data::getLvlpCollection() const
{
    return lvlpCollection;
}

const IdCollection<LvscRecord>& Data::getLvscCollection() const
{
    return lvscCollection;
}

const IdCollection<MaamRecord>& Data::getMaamCollection() const
{
    return maamCollection;
}

const IdCollection<MrhpRecord>& Data::getMrhpCollection() const
{
    return mrhpCollection;
}

const IdCollection<MtptRecord>& Data::getMtptCollection() const
{
    return mtptCollection;
}

const IdCollection<NaviRecord>& Data::getNaviCollection() const
{
    return naviCollection;
}

const IdCollection<NocmRecord>& Data::getNocmCollection() const
{
    return nocmCollection;
}

const IdCollection<OmodRecord>& Data::getOmodCollection() const
{
    return omodCollection;
}

const IdCollection<OswpRecord>& Data::getOswpCollection() const
{
    return oswpCollection;
}

const IdCollection<OvisRecord>& Data::getOvisCollection() const
{
    return ovisCollection;
}

const IdCollection<PcbnRecord>& Data::getPcbnCollection() const
{
    return pcbnCollection;
}

const IdCollection<PccnRecord>& Data::getPccnCollection() const
{
    return pccnCollection;
}

const IdCollection<PcmtRecord>& Data::getPcmtCollection() const
{
    return pcmtCollection;
}

const IdCollection<PdclRecord>& Data::getPdclCollection() const
{
    return pdclCollection;
}

const IdCollection<PgreRecord>& Data::getPgreCollection() const
{
    return pgreCollection;
}


const IdCollection<PhzdRecord>& Data::getPhzdCollection() const
{
    return phzdCollection;
}
const IdCollection<PkinRecord>& Data::getPkinCollection() const
{
    return pkinCollection;
}
const IdCollection<PmftRecord>& Data::getPmftCollection() const
{
    return pmftCollection;
}
const IdCollection<PsdcRecord>& Data::getPsdcCollection() const
{
    return psdcCollection;
}
const IdCollection<PtstRecord>& Data::getPtstCollection() const
{
    return ptstCollection;
}
const IdCollection<RfgpRecord>& Data::getRfgpCollection() const
{
    return rfgpCollection;
}
const IdCollection<RsgdRecord>& Data::getRsgdCollection() const
{
    return rsgdCollection;
}
const IdCollection<RspjRecord>& Data::getRspjCollection() const
{
    return rspjCollection;
}
const IdCollection<SdltRecord>& Data::getSdltCollection() const
{
    return sdltCollection;
}
const IdCollection<SechRecord>& Data::getSechCollection() const
{
    return sechCollection;
}
const IdCollection<SfbkRecord>& Data::getSfbkCollection() const
{
    return sfbkCollection;
}
const IdCollection<SfpcRecord>& Data::getSfpcCollection() const
{
    return sfpcCollection;
}
const IdCollection<SfptRecord>& Data::getSfptCollection() const
{
    return sfptCollection;
}
const IdCollection<SftrRecord>& Data::getSftrCollection() const
{
    return sftrCollection;
}
const IdCollection<SmbnRecord>& Data::getSmbnCollection() const
{
    return smbnCollection;
}
const IdCollection<SmenRecord>& Data::getSmenCollection() const
{
    return smenCollection;
}
const IdCollection<SpchRecord>& Data::getSpchCollection() const
{
    return spchCollection;
}
const IdCollection<StagRecord>& Data::getStagCollection() const
{
    return stagCollection;
}
const IdCollection<StbhRecord>& Data::getStbhCollection() const
{
    return stbhCollection;
}
const IdCollection<StdtRecord>& Data::getStdtCollection() const
{
    return stdtCollection;
}
const IdCollection<StmpRecord>& Data::getStmpCollection() const
{
    return stmpCollection;
}
const IdCollection<StndRecord>& Data::getStndCollection() const
{
    return stndCollection;
}
const IdCollection<SunpRecord>& Data::getSunpCollection() const
{
    return sunpCollection;
}
const IdCollection<TmlmRecord>& Data::getTmlmCollection() const
{
    return tmlmCollection;
}
const IdCollection<ToddRecord>& Data::getToddCollection() const
{
    return toddCollection;
}
const IdCollection<TravRecord>& Data::getTravCollection() const
{
    return travCollection;
}
const IdCollection<TrnsRecord>& Data::getTrnsCollection() const
{
    return trnsCollection;
}
const IdCollection<VoliRecord>& Data::getVoliCollection() const
{
    return voliCollection;
}
const IdCollection<VtypRecord>& Data::getVtypCollection() const
{
    return vtypCollection;
}
const IdCollection<WbarRecord>& Data::getWbarCollection() const
{
    return wbarCollection;
}
const IdCollection<WkmfRecord>& Data::getWkmfCollection() const
{
    return wkmfCollection;
}
const IdCollection<WthsRecord>& Data::getWthsCollection() const
{
    return wthsCollection;
}
const IdCollection<WwedRecord>& Data::getWwedCollection() const
{
    return wwedCollection;
}
const IdCollection<ZoomRecord>& Data::getZoomCollection() const
{
    return zoomCollection;
}

bool Data::addMaterial(MaterialRecord& record)
{
    if (materialCollection.searchId(record.editorId) >= 0)
        return false;
    materialCollection.add(record);
    return true;
}

bool Data::addLand(LandRecord& record)
{
    if (landCollection.searchId(record.editorId) >= 0)
        return false;
    landCollection.add(record);
    return true;
}

bool Data::addSoun(SounRecord& record)
{
    if (sounCollection.searchId(record.editorId) >= 0)
        return false;
    sounCollection.add(record);
    return true;
}

bool Data::addWthr(WthrRecord& record)
{
    if (wthrCollection.searchId(record.editorId) >= 0)
        return false;
    wthrCollection.add(record);
    return true;
}

bool Data::addLtex(LtexRecord& record)
{
    if (ltexCollection.searchId(record.editorId) >= 0)
        return false;
    ltexCollection.add(record);
    return true;
}

void Data::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    if (topLeft.column() <= 0)
    {
        emit idListChanged();
    }
}

UndoStack* Data::getUndoStack()
{
    return mUndoStack;
}

UndoStack* Data::getPluginUndoStack(int pluginIndex)
{
    auto it = mPluginUndoStacks.find(pluginIndex);
    if (it != mPluginUndoStacks.end())
    {
        return it.value();
    }
    return mUndoStack;
}

void Data::setPluginUndoStack(int pluginIndex, UndoStack* stack)
{
    mPluginUndoStacks[pluginIndex] = stack;
}

MacroCommand* Data::createMacroCommand(const QString& description)
{
    return new MacroCommand(description);
}

quint32 Data::createNewRecord(CkId::Type type, const QString& editorId)
{
    int baseRange = 0x4000 + static_cast<int>(type);
    QString finalEditorId = editorId.isEmpty() ? "new" : editorId.toLower();

    auto collections = allCollections();
    QSet<quint32> usedFormIds;
    for (IRecordCollection* col : collections) {
        if (!col) continue;
        for (int i = 0; i < col->count(); ++i) {
            usedFormIds.insert(col->getFormId(i));
        }
    }

    for (int i = baseRange; i < baseRange + 32768; ++i) {
        quint32 formId = static_cast<quint32>(i);
        if (!usedFormIds.contains(formId)) {
            LOG_INFO(QString("Created new record '%1' with FormID 0x%2")
                     .arg(finalEditorId, QString::number(formId, 16).toUpper()));
            return formId;
        }
    }

    throw std::runtime_error("No valid FormID available");
}
QList<Data::ConflictInfo> Data::detectConflicts()
{
    QList<ConflictInfo> conflicts;
    QStringList files = getContentFiles();

    for (const auto& tc : allCollectionsWithTypes())
    {
        QVector<QString> ids = tc.collection->getAllIds(false);
        if (ids.size() <= 1) continue;

        QMap<int, QStringList> pluginMap;

        for (const QString& id : ids) {
            int idx = tc.collection->searchId(id);
            if (idx < 0) continue;

            quint32 formId = tc.collection->getFormId(idx);
            int pluginIndex = (formId >> 16) & 0xFFFF;
            pluginMap[pluginIndex].append(id);
        }

        if (pluginMap.size() > 1) {
            QList<int> pluginIndices = pluginMap.keys();
            for (int i = 0; i < pluginIndices.size(); ++i) {
                for (int j = i + 1; j < pluginIndices.size(); ++j) {
                    int idxA = pluginIndices[i];
                    int idxB = pluginIndices[j];

                    QStringList commonIds;
                    for (const QString& id : pluginMap[idxA]) {
                        if (pluginMap[idxB].contains(id))
                            commonIds.append(id);
                    }

                    for (const QString& editorId : commonIds) {
                        ConflictInfo info;
                        info.type = tc.type;
                        info.editorId = editorId;
                        info.pluginIndexA = idxA;
                        info.pluginIndexB = idxB;
                        info.pluginNameA = (idxA - 1 >= 0 && idxA - 1 < files.size()) ? files[idxA - 1] : QString("Plugin %1").arg(idxA);
                        info.pluginNameB = (idxB - 1 >= 0 && idxB - 1 < files.size()) ? files[idxB - 1] : QString("Plugin %1").arg(idxB);
                        conflicts.append(info);
                    }
                }
            }
        }
    }

    LOG_INFO(QString("Conflict detection complete: %1 conflict(s) found").arg(conflicts.size()));
    return conflicts;
}

QStringList Data::getContentFiles() const
{
    return contentFiles;
}


