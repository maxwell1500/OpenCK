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
#include <QMessageBox>

#include <stdexcept>

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

    if (!base)
    {
        MetaData metaData_;
        metaData_.editorId = "esm::metadata";
        metaData_.load(*reader);

        metaData.appendRecord(Record<MetaData>(State::State_ModifiedOnly, 0, &metaData_));
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
        {&scenCollection,    CkId::Type_Scen_},
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


