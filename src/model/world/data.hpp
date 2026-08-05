#ifndef WORLDDATA_H
#define WORLDDATA_H

#include "idcollection.hpp"
#include "metadata.hpp"
#include "../../../libs/files/filepaths.hpp"
#include "../../../libs/files/esm/esmreader.hpp"
#include "../../../libs/files/esm/gmst.hpp"
#include "../../../libs/files/esm/tes4.hpp"
#include "../../../libs/files/esm/npcrecord.hpp"
#include "../../../libs/files/esm/weaprecord.hpp"
#include "../../../libs/files/esm/armorrecord.hpp"
#include "../../../libs/files/esm/spellrecord.hpp"
#include "../../../libs/files/esm/magicrecord.hpp"
#include "../../../libs/files/esm/questrecord.hpp"
#include "../../../libs/files/esm/dialrecord.hpp"
#include "../../../libs/files/esm/inforecord.hpp"
#include "../../../libs/files/esm/glob.hpp"
#include "../../../libs/files/esm/lcrt.hpp"
#include "../../../libs/files/esm/Packagerecord.hpp"
#include "../../../libs/files/esm/treerecord.hpp"
#include "../../../libs/files/esm/alchrecord.hpp"
#include "../../../libs/files/esm/ingrrecord.hpp"
#include "../../../libs/files/esm/contrecord.hpp"
#include "../../../libs/files/esm/enchrecord.hpp"
#include "../../../libs/files/esm/bookrecord.hpp"
#include "../../../libs/files/esm/miscrecord.hpp"
#include "../../../libs/files/esm/actirecord.hpp"
#include "../../../libs/files/esm/statrecord.hpp"
#include "../../../libs/files/esm/racerecord.hpp"
#include "../../../libs/files/esm/classrecord.hpp"
#include "../../../libs/files/esm/factrecord.hpp"
#include "../../../libs/files/esm/perkrecord.hpp"
#include "../../../libs/files/esm/materialrecord.hpp"
#include "../../../libs/files/esm/cellrecord.hpp"
#include "../../../libs/files/esm/worldspacerecord.hpp"
#include "../../../libs/files/esm/locationrecord.hpp"
#include "../../../libs/files/esm/landrecord.hpp"
#include "../../../libs/files/esm/refrecord.hpp"
#include "../../../libs/files/esm/sounrecord.hpp"
#include "../../../libs/files/esm/wthrrecord.hpp"
#include "../../../libs/files/esm/ltexrecord.hpp"
#include "../../../libs/files/esm/scenrecord.hpp"
#include "../../../libs/files/esm/ammorecord.hpp"
#include "../../../libs/files/esm/apparatusrecord.hpp"
#include "../../../libs/files/esm/actorvalueinforecord.hpp"
#include "../../../libs/files/esm/birthsignrecord.hpp"
#include "../../../libs/files/esm/climaterecord.hpp"
#include "../../../libs/files/esm/clothrecord.hpp"
#include "../../../libs/files/esm/constructibleobjectrecord.hpp"
#include "../../../libs/files/esm/creaturerecord.hpp"
#include "../../../libs/files/esm/combatstylerecord.hpp"
#include "../../../libs/files/esm/doorrecord.hpp"
#include "../../../libs/files/esm/effectshaderrecord.hpp"
#include "../../../libs/files/esm/explosionrecord.hpp"
#include "../../../libs/files/esm/eyesrecord.hpp"
#include "../../../libs/files/esm/florrecord.hpp"
#include "../../../libs/files/esm/formlistrecord.hpp"
#include "../../../libs/files/esm/furnrecord.hpp"
#include "../../../libs/files/esm/grassrecord.hpp"
#include "../../../libs/files/esm/hairrecord.hpp"
#include "../../../libs/files/esm/idleanimationrecord.hpp"
#include "../../../libs/files/esm/idlemarkerrecord.hpp"
#include "../../../libs/files/esm/imagespacerecord.hpp"
#include "../../../libs/files/esm/keymrecord.hpp"
#include "../../../libs/files/esm/keywordrecord.hpp"
#include "../../../libs/files/esm/lighrecord.hpp"
#include "../../../libs/files/esm/loadscreenrecord.hpp"
#include "../../../libs/files/esm/lvlcreaturerecord.hpp"
#include "../../../libs/files/esm/lvlistrecord.hpp"
#include "../../../libs/files/esm/lvspellrecord.hpp"
#include "../../../libs/files/esm/messagerecord.hpp"
#include "../../../libs/files/esm/msttrecord.hpp"
#include "../../../libs/files/esm/navmrecord.hpp"
#include "../../../libs/files/esm/noterecord.hpp"
#include "../../../libs/files/esm/outfitrecord.hpp"
#include "../../../libs/files/esm/projectilerecord.hpp"
#include "../../../libs/files/esm/regionrecord.hpp"
#include "../../../libs/files/esm/roadrecord.hpp"
#include "../../../libs/files/esm/scriptrecord.hpp"
#include "../../../libs/files/esm/scrollrecord.hpp"
#include "../../../libs/files/esm/slgmrecord.hpp"
#include "../../../libs/files/esm/soundmarkerrecord.hpp"
#include "../../../libs/files/esm/shaderparticlerecord.hpp"
#include "../../../libs/files/esm/staticcollectionrecord.hpp"
#include "../../../libs/files/esm/pndrecord.hpp"
#include "../../../libs/files/esm/texturesetrecord.hpp"
#include "../../../libs/files/esm/waterecord.hpp"
#include "../../../libs/files/esm/aniorecord.hpp"
#include "../../../libs/files/esm/artvrecord.hpp"
#include "../../../libs/files/esm/clfmrecord.hpp"
#include "../../../libs/files/esm/debrrecord.hpp"
#include "../../../libs/files/esm/ecznrecord.hpp"
#include "../../../libs/files/esm/hazdrecord.hpp"
#include "../../../libs/files/esm/ipctrecord.hpp"
#include "../../../libs/files/esm/ipdsrecord.hpp"
#include "../../../libs/files/esm/mustrecord.hpp"
#include "../../../libs/files/esm/relarecord.hpp"
#include "../../../libs/files/esm/revbrecord.hpp"
#include "../../../libs/files/esm/shourecord.hpp"
#include "../../../libs/files/esm/hdptrecord.hpp"
#include "../../../libs/files/esm/termrecord.hpp"
#include "../../../libs/files/esm/mattrecord.hpp"
#include "../../../libs/files/esm/movtrecord.hpp"
#include "../../../libs/files/esm/muscrecord.hpp"
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
#include "../../../libs/files/esm/bpttrecord.hpp"
#include "../../../libs/files/esm/camsrecord.hpp"
#include "../../../libs/files/esm/chalrecord.hpp"
#include "../../../libs/files/esm/ciftrecord.hpp"
#include "../../../libs/files/esm/cndarecord.hpp"
#include "../../../libs/files/esm/collrecord.hpp"
#include "../../../libs/files/esm/cpthrecord.hpp"
#include "../../../libs/files/esm/culkrecord.hpp"
#include "../../../libs/files/esm/cur3record.hpp"
#include "../../../libs/files/esm/curvrecord.hpp"
#include "../../../libs/files/esm/dfobrecord.hpp"
#include "../../../libs/files/esm/dmgtrecord.hpp"
#include "../../../libs/files/esm/dobjrecord.hpp"
#include "../../../libs/files/esm/efsrrecord.hpp"
#include "../../../libs/files/esm/equprecord.hpp"

#include <QObject>
#include <QStringList>
#include <QMap>
#include <QVector>

class MacroCommand;

/// \brief Message structure for loading progress and error reporting
class Messages;
/// \brief Qt abstract item model interface
class QAbstractItemModel;
/// \brief Undo stack for managing command history
class UndoStack;

/// \brief Central data management class for all ESM/ESP records
/// 
/// The Data class is the core of OpenCK's data layer. It manages:
/// - Loading and saving ESM/ESP files
/// - 30+ record type collections (NPC_, WEAP, ARMOR, etc.)
/// - Model-view integration for UI display
/// - Undo/redo operations across all plugins
/// - Conflict detection between plugins
/// 
/// Architecture:
/// 1. preload() reads file headers and metadata
/// 2. continueLoading() processes records in chunks
/// 3. getTableModel() provides Qt models for UI binding
/// 4. add*() methods insert new records into collections
/// 
/// Thread Safety: Data operations are not thread-safe. All access
/// must occur on the main thread via Qt's signal/slot mechanism.
class Data : public QObject
{
    Q_OBJECT

public:
    /// \brief Construct Data object and prepare for loading
    /// \param files List of ESM/ESP file paths to load
    /// \param paths File paths configuration (game dirs, config dir)
    /// 
    /// Initializes all record collections with column definitions
    /// and registers Qt models for UI binding. Does not load data
    /// yet — call preload() and continueLoading() to load files.
    Data(const QStringList& files, const FilePaths& paths);
    ~Data();

    /// \brief Get the file paths configuration
    const FilePaths& getPaths() const { return paths; }

    /// \brief Preload a file's header and metadata
    /// \param filename Path to ESM/ESP file
    /// \param base True if this is a master file, false if plugin
    /// \return Number of records found in file
    /// 
    /// Reads TES4 header, master list, and file metadata.
    /// Does not load individual records — call continueLoading() for that.
    int preload(const QString& filename, bool base);

    /// \brief TES4 header of the last preloaded plugin (for flag preservation).
    const Header& getReaderHeader() const { return reader ? reader->getHeader() : m_fallbackHeader; }

    /// \brief Continue loading records from preloaded files
    /// \param messages Reference to messages container for progress reporting
    /// \return True if loading complete, false if more data needed
    /// 
    /// Processes records in chunks to avoid blocking UI.
    /// Each call processes one record type's worth of data.
    /// Call repeatedly until returns true.
    bool continueLoading(Messages& messages);

    /// \brief Register a Qt model for a record type
    /// \param model Pointer to QAbstractItemModel subclass
    /// \param type Record type enum (NPC_, WEAP, etc.)
    /// \param update Whether to trigger model change signals
    /// 
    /// Associates a data model with a record type for UI display.
    /// Models are created automatically from IdCollection objects.
    void addModel(QAbstractItemModel* model, CkId::Type type, bool update = true);
    
    /// \brief Get the Qt model for a specific record type
    /// \param id Record type identifier
    /// \return Pointer to QAbstractItemModel, or nullptr if not found
    /// 
    /// Used by UI components to access data models for tables/views.
    QAbstractItemModel* getTableModel(const CkId& id);
    
    /// \brief Get game settings collection (GMST records)
    /// \return Const reference to the game settings collection
    const IdCollection<GameSetting>& getGameSettings() const;
    /// \brief Get metadata collection
    /// \return Const reference to the metadata collection
    const Collection<MetaData>& getMetaData() const;
    /// \brief Get NPC collection (NPC_ records)
    /// \return Const reference to the NPC record collection
    const IdCollection<NpcRecord>& getNpcCollection() const;
    /// \brief Get weapon collection (WEAP records)
    /// \return Const reference to the weapon record collection
    const IdCollection<WeaponRecord>& getWeaponCollection() const;
    /// \brief Get armor collection (ARMOR records)
    /// \return Const reference to the armor record collection
    const IdCollection<ArmorRecord>& getArmorCollection() const;
    /// \brief Get spell collection (SPEL records)
    /// \return Const reference to the spell record collection
    const IdCollection<SpellRecord>& getSpellCollection() const;
    /// \brief Get magic effect collection (MAGIC records)
    /// \return Const reference to the magic effect record collection
    const IdCollection<MagicRecord>& getMagicCollection() const;
    /// \brief Get quest collection (QUEST records)
    /// \return Const reference to the quest record collection
    const IdCollection<QuestRecord>& getQuestCollection() const;
    /// \brief Get dialogue collection (DIAL records)
    /// \return Const reference to the dialogue record collection
    const IdCollection<DialRecord>& getDialCollection() const;
    /// \brief Get dialogue info collection (INFO records)
    /// \return Const reference to the dialogue info record collection
    const IdCollection<InfoRecord>& getInfoCollection() const;
    /// \brief Get global variable collection (GLOB records)
    /// \return Const reference to the global variable collection
    const IdCollection<GlobalVariable>& getGlobCollection() const;
    /// \brief Get location reference collection (LCRT records)
    /// \return Const reference to the location reference type collection
    const IdCollection<LocationRefType>& getLcrtCollection() const;
    /// \brief Get package collection (PACK records)
    /// \return Const reference to the package record collection
    const IdCollection<PackageRecord>& getPackCollection() const;
    /// \brief Get tree collection (TREE records)
    /// \return Const reference to the tree record collection
    const IdCollection<TreeRecord>& getTreeCollection() const;
    /// \brief Get alchemy collection (ALCH records)
    /// \return Const reference to the alchemy record collection
    const IdCollection<AlchRecord>& getAlchCollection() const;
    /// \brief Get ingredient collection (INGR records)
    /// \return Const reference to the ingredient record collection
    const IdCollection<IngrRecord>& getIngrCollection() const;
    /// \brief Get container collection (CONT records)
    /// \return Const reference to the container record collection
    const IdCollection<ContRecord>& getContCollection() const;
    /// \brief Get enchantment collection (ENCH records)
    /// \return Const reference to the enchantment record collection
    const IdCollection<EnchRecord>& getEnchCollection() const;
    /// \brief Get book collection (BOOK records)
    /// \return Const reference to the book record collection
    const IdCollection<BookRecord>& getBookCollection() const;
    /// \brief Get misc item collection (MISC records)
    /// \return Const reference to the misc item record collection
    const IdCollection<MiscRecord>& getMiscCollection() const;
    /// \brief Get activator collection (ACTI records)
    /// \return Const reference to the activator record collection
    const IdCollection<ActiRecord>& getActiCollection() const;
    /// \brief Get static collection (STAT records)
    /// \return Const reference to the static record collection
    const IdCollection<StatRecord>& getStatCollection() const;
    /// \brief Get race collection (RACE records)
    /// \return Const reference to the race record collection
    const IdCollection<RaceRecord>& getRaceCollection() const;
    /// \brief Get class collection (CLASS records)
    /// \return Const reference to the class record collection
    const IdCollection<ClassRecord>& getClassCollection() const;
    /// \brief Get faction collection (FACT records)
    /// \return Const reference to the faction record collection
    const IdCollection<FactRecord>& getFactCollection() const;
    /// \brief Get perk collection (PERK records)
    /// \return Const reference to the perk record collection
    const IdCollection<PerkRecord>& getPerkCollection() const;
    /// \brief Get cell collection (CELL records)
    /// \return Const reference to the cell record collection
    const IdCollection<CellRecord>& getCellCollection() const;
    /// \brief Get worldspace collection (WRLD records)
    /// \return Const reference to the worldspace record collection
    const IdCollection<WorldspaceRecord>& getWorldspaceCollection() const;
    /// \brief Get location collection (LCTN records)
    /// \return Const reference to the location record collection
    const IdCollection<LocationRecord>& getLocationCollection() const;
    /// \brief Get planet collection (PNDT records)
    /// \return Const reference to the planet record collection
    const IdCollection<PndRecord>& getPlanetCollection() const;
    /// \brief Get reference collection (REFR records)
    /// \return Const reference to the reference record collection
    const IdCollection<RefrRecord>& getRefrCollection() const;
    /// \brief Get material collection (MATL records)
    /// \return Const reference to the material record collection
    const IdCollection<MaterialRecord>& getMaterialCollection() const;
    /// \brief Get landscape collection (LAND records)
    /// \return Const reference to the landscape record collection
    const IdCollection<LandRecord>& getLandCollection() const;
    /// \brief Get sound collection (SOUN records)
    /// \return Const reference to the sound record collection
    const IdCollection<SounRecord>& getSounCollection() const;
    /// \brief Get weather collection (WTHR records)
    /// \return Const reference to the weather record collection
    const IdCollection<WthrRecord>& getWthrCollection() const;
    /// \brief Get land texture collection (LTEX records)
    /// \return Const reference to the land texture record collection
    const IdCollection<LtexRecord>& getLtexCollection() const;
    /// \brief Get SCEN collection
    /// \return Const reference to the SCEN record collection
    const IdCollection<ScenRecord>& getScenCollection() const;
    /// \brief Get AMMO collection
    /// \return Const reference to the AMMO record collection
    const IdCollection<AmmoRecord>& getAmmoCollection() const;
    /// \brief Get APPA collection
    /// \return Const reference to the APPA record collection
    const IdCollection<AppaRecord>& getAppaCollection() const;
    /// \brief Get AVIF collection
    /// \return Const reference to the AVIF record collection
    const IdCollection<ActorValueInfoRecord>& getAvifCollection() const;
    /// \brief Get BSGN collection
    /// \return Const reference to the BSGN record collection
    const IdCollection<BsgnRecord>& getBsgnCollection() const;
    /// \brief Get CLMT collection
    /// \return Const reference to the CLMT record collection
    const IdCollection<ClimateRecord>& getClmtCollection() const;
    /// \brief Get CLOT collection
    /// \return Const reference to the CLOT record collection
    const IdCollection<ClotRecord>& getClotCollection() const;
    /// \brief Get COBJ collection
    /// \return Const reference to the COBJ record collection
    const IdCollection<CobjRecord>& getCobjCollection() const;
    /// \brief Get CREA collection
    /// \return Const reference to the CREA record collection
    const IdCollection<CreatureRecord>& getCreatureCollection() const;
    /// \brief Get CSTY collection
    /// \return Const reference to the CSTY record collection
    const IdCollection<CstyRecord>& getCstyCollection() const;
    /// \brief Get DOOR collection
    /// \return Const reference to the DOOR record collection
    const IdCollection<DoorRecord>& getDoorCollection() const;
    /// \brief Get EFSH collection
    /// \return Const reference to the EFSH record collection
    const IdCollection<EfshRecord>& getEfshCollection() const;
    /// \brief Get EXPL collection
    /// \return Const reference to the EXPL record collection
    const IdCollection<ExplRecord>& getExplCollection() const;
    /// \brief Get EYES collection
    /// \return Const reference to the EYES record collection
    const IdCollection<EyesRecord>& getEyesCollection() const;
    /// \brief Get FLOR collection
    /// \return Const reference to the FLOR record collection
    const IdCollection<FlorRecord>& getFlorCollection() const;
    /// \brief Get FLST collection
    /// \return Const reference to the FLST record collection
    const IdCollection<FormListRecord>& getFlstCollection() const;
    /// \brief Get FURN collection
    /// \return Const reference to the FURN record collection
    const IdCollection<FurnRecord>& getFurnCollection() const;
    /// \brief Get GRAS collection
    /// \return Const reference to the GRAS record collection
    const IdCollection<GrassRecord>& getGrassCollection() const;
    /// \brief Get HAIR collection
    /// \return Const reference to the HAIR record collection
    const IdCollection<HairRecord>& getHairCollection() const;
    /// \brief Get IDLE collection
    /// \return Const reference to the IDLE record collection
    const IdCollection<IdleAnimationRecord>& getIdleCollection() const;
    /// \brief Get IDLM collection
    /// \return Const reference to the IDLM record collection
    const IdCollection<IdleMarkerRecord>& getIdlmCollection() const;
    /// \brief Get IMGS collection
    /// \return Const reference to the IMGS record collection
    const IdCollection<ImgsRecord>& getImgsCollection() const;
    /// \brief Get KEYM collection
    /// \return Const reference to the KEYM record collection
    const IdCollection<KeymRecord>& getKeymCollection() const;
    /// \brief Get KYWD collection
    /// \return Const reference to the KYWD record collection
    const IdCollection<KeywordRecord>& getKywdCollection() const;
    /// \brief Get LIGH collection
    /// \return Const reference to the LIGH record collection
    const IdCollection<LighRecord>& getLighCollection() const;
    /// \brief Get LSCR collection
    /// \return Const reference to the LSCR record collection
    const IdCollection<LoadScreenRecord>& getLscrCollection() const;
    /// \brief Get LVLC collection
    /// \return Const reference to the LVLC record collection
    const IdCollection<LvlcRecord>& getLvlcCollection() const;
    /// \brief Get LVLI collection
    /// \return Const reference to the LVLI record collection
    const IdCollection<LvliRecord>& getLvliCollection() const;
    /// \brief Get LVSP collection
    /// \return Const reference to the LVSP record collection
    const IdCollection<LvspRecord>& getLvspCollection() const;
    /// \brief Get MESG collection
    /// \return Const reference to the MESG record collection
    const IdCollection<MesgRecord>& getMesgCollection() const;
    /// \brief Get MSTT collection
    /// \return Const reference to the MSTT record collection
    const IdCollection<MsttRecord>& getMsttCollection() const;
    /// \brief Get NAVM collection
    /// \return Const reference to the NAVM record collection
    const IdCollection<NavmRecord>& getNavmCollection() const;
    /// \brief Get NOTE collection
    /// \return Const reference to the NOTE record collection
    const IdCollection<NoteRecord>& getNoteCollection() const;
    /// \brief Get OTFT collection
    /// \return Const reference to the OTFT record collection
    const IdCollection<OutfitRecord>& getOtftCollection() const;
    /// \brief Get PROJ collection
    /// \return Const reference to the PROJ record collection
    const IdCollection<ProjRecord>& getProjCollection() const;
    /// \brief Get REGN collection
    /// \return Const reference to the REGN record collection
    const IdCollection<RegionRecord>& getRegnCollection() const;
    /// \brief Get ROAD collection
    /// \return Const reference to the ROAD record collection
    const IdCollection<RoadRecord>& getRoadCollection() const;
    /// \brief Get SCPT collection
    /// \return Const reference to the SCPT record collection
    const IdCollection<ScriptRecord>& getScptCollection() const;
    /// \brief Get SCRL collection
    /// \return Const reference to the SCRL record collection
    const IdCollection<ScrRecord>& getScrlCollection() const;
    /// \brief Get SLGM collection
    /// \return Const reference to the SLGM record collection
    const IdCollection<SlgmRecord>& getSlgmCollection() const;
    /// \brief Get SMQN collection
    /// \return Const reference to the SMQN record collection
    const IdCollection<SmqnRecord>& getSmqnCollection() const;
    /// \brief Get SPGD collection
    /// \return Const reference to the SPGD record collection
    const IdCollection<SpgdRecord>& getSpgdCollection() const;
    /// \brief Get SCOL collection
    /// \return Const reference to the SCOL record collection
    const IdCollection<StaticCollectionRecord>& getScolCollection() const;
    /// \brief Get TXST collection
    /// \return Const reference to the TXST record collection
    const IdCollection<TextureSetRecord>& getTxstCollection() const;
    /// \brief Get WATR collection
    /// \return Const reference to the WATR record collection
    const IdCollection<WateRecord>& getWateCollection() const;
    /// \brief Get ANIO collection
    /// \return Const reference to the ANIO record collection
    const IdCollection<AnioRecord>& getAnioCollection() const;
    /// \brief Get ARTV collection
    /// \return Const reference to the ARTV record collection
    const IdCollection<ArtvRecord>& getArtvCollection() const;
    /// \brief Get CLFM collection
    /// \return Const reference to the CLFM record collection
    const IdCollection<ClfmRecord>& getClfmCollection() const;
    /// \brief Get DEBR collection
    /// \return Const reference to the DEBR record collection
    const IdCollection<DebrRecord>& getDebrCollection() const;
    /// \brief Get ECZN collection
    /// \return Const reference to the ECZN record collection
    const IdCollection<EcznRecord>& getEcznCollection() const;
    /// \brief Get HAZD collection
    /// \return Const reference to the HAZD record collection
    const IdCollection<HazdRecord>& getHazdCollection() const;
    /// \brief Get IPCT collection
    /// \return Const reference to the IPCT record collection
    const IdCollection<IpctRecord>& getIpctCollection() const;
    /// \brief Get IPDS collection
    /// \return Const reference to the IPDS record collection
    const IdCollection<IpdsRecord>& getIpdsCollection() const;
    /// \brief Get MUST collection
    /// \return Const reference to the MUST record collection
    const IdCollection<MustRecord>& getMustCollection() const;
    /// \brief Get RELA collection
    /// \return Const reference to the RELA record collection
    const IdCollection<RelaRecord>& getRelaCollection() const;
    /// \brief Get REVB collection
    /// \return Const reference to the REVB record collection
    const IdCollection<RevbRecord>& getRevbCollection() const;
    /// \brief Get SHOU collection
    /// \return Const reference to the SHOU record collection
    const IdCollection<ShouRecord>& getShouCollection() const;
    /// \brief Get HDPT collection
    /// \return Const reference to the HDPT record collection
    const IdCollection<HdptRecord>& getHdptCollection() const;
    /// \brief Get TERM collection
    /// \return Const reference to the TERM record collection
    const IdCollection<TermRecord>& getTermCollection() const;
    /// \brief Get MATT collection
    /// \return Const reference to the MATT record collection
    const IdCollection<MattRecord>& getMattCollection() const;
    /// \brief Get MOVT collection
    /// \return Const reference to the MOVT record collection
    const IdCollection<MovtRecord>& getMovtCollection() const;
    /// \brief Get MUSC collection
    /// \return Const reference to the MUSC record collection
    const IdCollection<MuscRecord>& getMuscCollection() const;
    /// \brief Get AACT collection
    /// \return Const reference to the AACT record collection
    const IdCollection<AactRecord>& getAactCollection() const;
    /// \brief Get AAMD collection
    /// \return Const reference to the AAMD record collection
    const IdCollection<AamdRecord>& getAamdCollection() const;
    /// \brief Get AAPD collection
    /// \return Const reference to the AAPD record collection
    const IdCollection<AapdRecord>& getAapdCollection() const;
    /// \brief Get ACHR collection
    /// \return Const reference to the ACHR record collection
    const IdCollection<AchrRecord>& getAchrCollection() const;
    /// \brief Get ADDN collection
    /// \return Const reference to the ADDN record collection
    const IdCollection<AddnRecord>& getAddnCollection() const;
    /// \brief Get AFFE collection
    /// \return Const reference to the AFFE record collection
    const IdCollection<AffeRecord>& getAffeCollection() const;
    /// \brief Get AMBS collection
    /// \return Const reference to the AMBS record collection
    const IdCollection<AmbsRecord>& getAmbsCollection() const;
    /// \brief Get AMDL collection
    /// \return Const reference to the AMDL record collection
    const IdCollection<AmdlRecord>& getAmdlCollection() const;
    /// \brief Get AOPF collection
    /// \return Const reference to the AOPF record collection
    const IdCollection<AopfRecord>& getAopfCollection() const;
    /// \brief Get AOPS collection
    /// \return Const reference to the AOPS record collection
    const IdCollection<AopsRecord>& getAopsCollection() const;
    /// \brief Get AORU collection
    /// \return Const reference to the AORU record collection
    const IdCollection<AoruRecord>& getAoruCollection() const;
    /// \brief Get ARMA collection
    /// \return Const reference to the ARMA record collection
    const IdCollection<ArmaRecord>& getArmaCollection() const;
    /// \brief Get ARTO collection
    /// \return Const reference to the ARTO record collection
    const IdCollection<ArtoRecord>& getArtoCollection() const;
    /// \brief Get ASPC collection
    /// \return Const reference to the ASPC record collection
    const IdCollection<AspcRecord>& getAspcCollection() const;
    /// \brief Get ATMO collection
    /// \return Const reference to the ATMO record collection
    const IdCollection<AtmoRecord>& getAtmoCollection() const;
    /// \brief Get AVMD collection
    /// \return Const reference to the AVMD record collection
    const IdCollection<AvmdRecord>& getAvmdCollection() const;
    /// \brief Get BIOM collection
    /// \return Const reference to the BIOM record collection
    const IdCollection<BiomRecord>& getBiomCollection() const;
    /// \brief Get BMMO collection
    /// \return Const reference to the BMMO record collection
    const IdCollection<BmmoRecord>& getBmmoCollection() const;
    /// \brief Get BMOD collection
    /// \return Const reference to the BMOD record collection
    const IdCollection<BmodRecord>& getBmodCollection() const;
    /// \brief Get BNDS collection
    /// \return Const reference to the BNDS record collection
    const IdCollection<BndsRecord>& getBndsCollection() const;
    /// \brief Get BPTT collection
    /// \return Const reference to the BPTT record collection
    const IdCollection<BpttRecord>& getBpttCollection() const;
    /// \brief Get CAMS collection
    /// \return Const reference to the CAMS record collection
    const IdCollection<CamsRecord>& getCamsCollection() const;
    /// \brief Get CHAL collection
    /// \return Const reference to the CHAL record collection
    const IdCollection<ChalRecord>& getChalCollection() const;
    /// \brief Get CIFT collection
    /// \return Const reference to the CIFT record collection
    const IdCollection<CiftRecord>& getCiftCollection() const;
    /// \brief Get CNDA collection
    /// \return Const reference to the CNDA record collection
    const IdCollection<CndaRecord>& getCndaCollection() const;
    /// \brief Get COLL collection
    /// \return Const reference to the COLL record collection
    const IdCollection<CollRecord>& getCollCollection() const;
    /// \brief Get CPTH collection
    /// \return Const reference to the CPTH record collection
    const IdCollection<CpthRecord>& getCpthCollection() const;
    /// \brief Get CULK collection
    /// \return Const reference to the CULK record collection
    const IdCollection<CulkRecord>& getCulkCollection() const;
    /// \brief Get CUR3 collection
    /// \return Const reference to the CUR3 record collection
    const IdCollection<Cur3Record>& getCur3Collection() const;
    /// \brief Get CURV collection
    /// \return Const reference to the CURV record collection
    const IdCollection<CurvRecord>& getCurvCollection() const;
    /// \brief Get DFOB collection
    /// \return Const reference to the DFOB record collection
    const IdCollection<DfobRecord>& getDfobCollection() const;
    /// \brief Get DMGT collection
    /// \return Const reference to the DMGT record collection
    const IdCollection<DmgtRecord>& getDmgtCollection() const;
    /// \brief Get DOBJ collection
    /// \return Const reference to the DOBJ record collection
    const IdCollection<DobjRecord>& getDobjCollection() const;
    /// \brief Get EFSR collection
    /// \return Const reference to the EFSR record collection
    const IdCollection<EfsrRecord>& getEfsrCollection() const;
    /// \brief Get EQUP collection
    /// \return Const reference to the EQUP record collection
    const IdCollection<EqupRecord>& getEqupCollection() const;

    // --- Non-const overloads for mutation ---

    /// \brief Get game settings collection (mutable)
    /// \return Reference to the game settings collection for modification
    IdCollection<GameSetting>& getGameSettings();
    /// \brief Get metadata collection (mutable)
    /// \return Reference to the metadata collection for modification
    Collection<MetaData>& getMetaData();
    /// \brief Get NPC collection (mutable)
    /// \return Reference to the NPC record collection for modification
    IdCollection<NpcRecord>& getNpcCollection();
    /// \brief Get weapon collection (mutable)
    /// \return Reference to the weapon record collection for modification
    IdCollection<WeaponRecord>& getWeaponCollection();
    /// \brief Get armor collection (mutable)
    /// \return Reference to the armor record collection for modification
    IdCollection<ArmorRecord>& getArmorCollection();
    /// \brief Get spell collection (mutable)
    /// \return Reference to the spell record collection for modification
    IdCollection<SpellRecord>& getSpellCollection();
    /// \brief Get magic effect collection (mutable)
    /// \return Reference to the magic effect record collection for modification
    IdCollection<MagicRecord>& getMagicCollection();
    /// \brief Get quest collection (mutable)
    /// \return Reference to the quest record collection for modification
    IdCollection<QuestRecord>& getQuestCollection();
    /// \brief Get dialogue collection (mutable)
    /// \return Reference to the dialogue record collection for modification
    IdCollection<DialRecord>& getDialCollection();
    /// \brief Get dialogue info collection (mutable)
    /// \return Reference to the dialogue info record collection for modification
    IdCollection<InfoRecord>& getInfoCollection();
    /// \brief Get global variable collection (mutable)
    /// \return Reference to the global variable collection for modification
    IdCollection<GlobalVariable>& getGlobCollection();
    /// \brief Get location reference collection (mutable)
    /// \return Reference to the location reference type collection for modification
    IdCollection<LocationRefType>& getLcrtCollection();
    /// \brief Get package collection (mutable)
    /// \return Reference to the package record collection for modification
    IdCollection<PackageRecord>& getPackCollection();
    /// \brief Get tree collection (mutable)
    /// \return Reference to the tree record collection for modification
    IdCollection<TreeRecord>& getTreeCollection();
    /// \brief Get alchemy collection (mutable)
    /// \return Reference to the alchemy record collection for modification
    IdCollection<AlchRecord>& getAlchCollection();
    /// \brief Get ingredient collection (mutable)
    /// \return Reference to the ingredient record collection for modification
    IdCollection<IngrRecord>& getIngrCollection();
    /// \brief Get container collection (mutable)
    /// \return Reference to the container record collection for modification
    IdCollection<ContRecord>& getContCollection();
    /// \brief Get enchantment collection (mutable)
    /// \return Reference to the enchantment record collection for modification
    IdCollection<EnchRecord>& getEnchCollection();
    /// \brief Get book collection (mutable)
    /// \return Reference to the book record collection for modification
    IdCollection<BookRecord>& getBookCollection();
    /// \brief Get misc item collection (mutable)
    /// \return Reference to the misc item record collection for modification
    IdCollection<MiscRecord>& getMiscCollection();
    /// \brief Get activator collection (mutable)
    /// \return Reference to the activator record collection for modification
    IdCollection<ActiRecord>& getActiCollection();
    /// \brief Get static collection (mutable)
    /// \return Reference to the static record collection for modification
    IdCollection<StatRecord>& getStatCollection();
    /// \brief Get race collection (mutable)
    /// \return Reference to the race record collection for modification
    IdCollection<RaceRecord>& getRaceCollection();
    /// \brief Get class collection (mutable)
    /// \return Reference to the class record collection for modification
    IdCollection<ClassRecord>& getClassCollection();
    /// \brief Get faction collection (mutable)
    /// \return Reference to the faction record collection for modification
    IdCollection<FactRecord>& getFactCollection();
    /// \brief Get perk collection (mutable)
    /// \return Reference to the perk record collection for modification
    IdCollection<PerkRecord>& getPerkCollection();
    /// \brief Get cell collection (mutable)
    /// \return Reference to the cell record collection for modification
    IdCollection<CellRecord>& getCellCollection();
    /// \brief Get worldspace collection (mutable)
    /// \return Reference to the worldspace record collection for modification
    IdCollection<WorldspaceRecord>& getWorldspaceCollection();
    /// \brief Get location collection (mutable)
    /// \return Reference to the location record collection for modification
    IdCollection<LocationRecord>& getLocationCollection();
    /// \brief Get planet collection (mutable)
    /// \return Reference to the planet record collection for modification
    IdCollection<PndRecord>& getPlanetCollection();
    /// \brief Get reference collection (mutable)
    /// \return Reference to the reference record collection for modification
    IdCollection<RefrRecord>& getRefrCollection();
    /// \brief Get material collection (mutable)
    /// \return Reference to the material record collection for modification
    IdCollection<MaterialRecord>& getMaterialCollection();
    /// \brief Get landscape collection (mutable)
    /// \return Reference to the landscape record collection for modification
    IdCollection<LandRecord>& getLandCollection();
    /// \brief Get sound collection (mutable)
    /// \return Reference to the sound record collection for modification
    IdCollection<SounRecord>& getSounCollection();
    /// \brief Get weather collection (mutable)
    /// \return Reference to the weather record collection for modification
    IdCollection<WthrRecord>& getWthrCollection();
    /// \brief Get land texture collection (mutable)
    /// \return Reference to the land texture record collection for modification
    IdCollection<LtexRecord>& getLtexCollection();
    /// \brief Get SCEN collection (mutable)
    /// \return Reference to the SCEN record collection for modification
    IdCollection<ScenRecord>& getScenCollection();
    /// \brief Get AMMO collection (mutable)
    /// \return Reference to the AMMO record collection for modification
    IdCollection<AmmoRecord>& getAmmoCollection();
    /// \brief Get APPA collection (mutable)
    /// \return Reference to the APPA record collection for modification
    IdCollection<AppaRecord>& getAppaCollection();
    /// \brief Get AVIF collection (mutable)
    /// \return Reference to the AVIF record collection for modification
    IdCollection<ActorValueInfoRecord>& getAvifCollection();
    /// \brief Get BSGN collection (mutable)
    /// \return Reference to the BSGN record collection for modification
    IdCollection<BsgnRecord>& getBsgnCollection();
    /// \brief Get CLMT collection (mutable)
    /// \return Reference to the CLMT record collection for modification
    IdCollection<ClimateRecord>& getClmtCollection();
    /// \brief Get CLOT collection (mutable)
    /// \return Reference to the CLOT record collection for modification
    IdCollection<ClotRecord>& getClotCollection();
    /// \brief Get COBJ collection (mutable)
    /// \return Reference to the COBJ record collection for modification
    IdCollection<CobjRecord>& getCobjCollection();
    /// \brief Get CREA collection (mutable)
    /// \return Reference to the CREA record collection for modification
    IdCollection<CreatureRecord>& getCreatureCollection();
    /// \brief Get CSTY collection (mutable)
    /// \return Reference to the CSTY record collection for modification
    IdCollection<CstyRecord>& getCstyCollection();
    /// \brief Get DOOR collection (mutable)
    /// \return Reference to the DOOR record collection for modification
    IdCollection<DoorRecord>& getDoorCollection();
    /// \brief Get EFSH collection (mutable)
    /// \return Reference to the EFSH record collection for modification
    IdCollection<EfshRecord>& getEfshCollection();
    /// \brief Get EXPL collection (mutable)
    /// \return Reference to the EXPL record collection for modification
    IdCollection<ExplRecord>& getExplCollection();
    /// \brief Get EYES collection (mutable)
    /// \return Reference to the EYES record collection for modification
    IdCollection<EyesRecord>& getEyesCollection();
    /// \brief Get FLOR collection (mutable)
    /// \return Reference to the FLOR record collection for modification
    IdCollection<FlorRecord>& getFlorCollection();
    /// \brief Get FLST collection (mutable)
    /// \return Reference to the FLST record collection for modification
    IdCollection<FormListRecord>& getFlstCollection();
    /// \brief Get FURN collection (mutable)
    /// \return Reference to the FURN record collection for modification
    IdCollection<FurnRecord>& getFurnCollection();
    /// \brief Get GRAS collection (mutable)
    /// \return Reference to the GRAS record collection for modification
    IdCollection<GrassRecord>& getGrassCollection();
    /// \brief Get HAIR collection (mutable)
    /// \return Reference to the HAIR record collection for modification
    IdCollection<HairRecord>& getHairCollection();
    /// \brief Get IDLE collection (mutable)
    /// \return Reference to the IDLE record collection for modification
    IdCollection<IdleAnimationRecord>& getIdleCollection();
    /// \brief Get IDLM collection (mutable)
    /// \return Reference to the IDLM record collection for modification
    IdCollection<IdleMarkerRecord>& getIdlmCollection();
    /// \brief Get IMGS collection (mutable)
    /// \return Reference to the IMGS record collection for modification
    IdCollection<ImgsRecord>& getImgsCollection();
    /// \brief Get KEYM collection (mutable)
    /// \return Reference to the KEYM record collection for modification
    IdCollection<KeymRecord>& getKeymCollection();
    /// \brief Get KYWD collection (mutable)
    /// \return Reference to the KYWD record collection for modification
    IdCollection<KeywordRecord>& getKywdCollection();
    /// \brief Get LIGH collection (mutable)
    /// \return Reference to the LIGH record collection for modification
    IdCollection<LighRecord>& getLighCollection();
    /// \brief Get LSCR collection (mutable)
    /// \return Reference to the LSCR record collection for modification
    IdCollection<LoadScreenRecord>& getLscrCollection();
    /// \brief Get LVLC collection (mutable)
    /// \return Reference to the LVLC record collection for modification
    IdCollection<LvlcRecord>& getLvlcCollection();
    /// \brief Get LVLI collection (mutable)
    /// \return Reference to the LVLI record collection for modification
    IdCollection<LvliRecord>& getLvliCollection();
    /// \brief Get LVSP collection (mutable)
    /// \return Reference to the LVSP record collection for modification
    IdCollection<LvspRecord>& getLvspCollection();
    /// \brief Get MESG collection (mutable)
    /// \return Reference to the MESG record collection for modification
    IdCollection<MesgRecord>& getMesgCollection();
    /// \brief Get MSTT collection (mutable)
    /// \return Reference to the MSTT record collection for modification
    IdCollection<MsttRecord>& getMsttCollection();
    /// \brief Get NAVM collection (mutable)
    /// \return Reference to the NAVM record collection for modification
    IdCollection<NavmRecord>& getNavmCollection();
    /// \brief Get NOTE collection (mutable)
    /// \return Reference to the NOTE record collection for modification
    IdCollection<NoteRecord>& getNoteCollection();
    /// \brief Get OTFT collection (mutable)
    /// \return Reference to the OTFT record collection for modification
    IdCollection<OutfitRecord>& getOtftCollection();
    /// \brief Get PROJ collection (mutable)
    /// \return Reference to the PROJ record collection for modification
    IdCollection<ProjRecord>& getProjCollection();
    /// \brief Get REGN collection (mutable)
    /// \return Reference to the REGN record collection for modification
    IdCollection<RegionRecord>& getRegnCollection();
    /// \brief Get ROAD collection (mutable)
    /// \return Reference to the ROAD record collection for modification
    IdCollection<RoadRecord>& getRoadCollection();
    /// \brief Get SCPT collection (mutable)
    /// \return Reference to the SCPT record collection for modification
    IdCollection<ScriptRecord>& getScptCollection();
    /// \brief Get SCRL collection (mutable)
    /// \return Reference to the SCRL record collection for modification
    IdCollection<ScrRecord>& getScrlCollection();
    /// \brief Get SLGM collection (mutable)
    /// \return Reference to the SLGM record collection for modification
    IdCollection<SlgmRecord>& getSlgmCollection();
    /// \brief Get SMQN collection (mutable)
    /// \return Reference to the SMQN record collection for modification
    IdCollection<SmqnRecord>& getSmqnCollection();
    /// \brief Get SPGD collection (mutable)
    /// \return Reference to the SPGD record collection for modification
    IdCollection<SpgdRecord>& getSpgdCollection();
    /// \brief Get SCOL collection (mutable)
    /// \return Reference to the SCOL record collection for modification
    IdCollection<StaticCollectionRecord>& getScolCollection();
    /// \brief Get TXST collection (mutable)
    /// \return Reference to the TXST record collection for modification
    IdCollection<TextureSetRecord>& getTxstCollection();
    /// \brief Get WATR collection (mutable)
    /// \return Reference to the WATR record collection for modification
    IdCollection<WateRecord>& getWateCollection();
    /// \brief Get ANIO collection (mutable)
    /// \return Reference to the ANIO record collection for modification
    IdCollection<AnioRecord>& getAnioCollection();
    /// \brief Get ARTV collection (mutable)
    /// \return Reference to the ARTV record collection for modification
    IdCollection<ArtvRecord>& getArtvCollection();
    /// \brief Get CLFM collection (mutable)
    /// \return Reference to the CLFM record collection for modification
    IdCollection<ClfmRecord>& getClfmCollection();
    /// \brief Get DEBR collection (mutable)
    /// \return Reference to the DEBR record collection for modification
    IdCollection<DebrRecord>& getDebrCollection();
    /// \brief Get ECZN collection (mutable)
    /// \return Reference to the ECZN record collection for modification
    IdCollection<EcznRecord>& getEcznCollection();
    /// \brief Get HAZD collection (mutable)
    /// \return Reference to the HAZD record collection for modification
    IdCollection<HazdRecord>& getHazdCollection();
    /// \brief Get IPCT collection (mutable)
    /// \return Reference to the IPCT record collection for modification
    IdCollection<IpctRecord>& getIpctCollection();
    /// \brief Get IPDS collection (mutable)
    /// \return Reference to the IPDS record collection for modification
    IdCollection<IpdsRecord>& getIpdsCollection();
    /// \brief Get MUST collection (mutable)
    /// \return Reference to the MUST record collection for modification
    IdCollection<MustRecord>& getMustCollection();
    /// \brief Get RELA collection (mutable)
    /// \return Reference to the RELA record collection for modification
    IdCollection<RelaRecord>& getRelaCollection();
    /// \brief Get REVB collection (mutable)
    /// \return Reference to the REVB record collection for modification
    IdCollection<RevbRecord>& getRevbCollection();
    /// \brief Get SHOU collection (mutable)
    /// \return Reference to the SHOU record collection for modification
    IdCollection<ShouRecord>& getShouCollection();
    /// \brief Get HDPT collection (mutable)
    /// \return Reference to the HDPT record collection for modification
    IdCollection<HdptRecord>& getHdptCollection();
    /// \brief Get TERM collection (mutable)
    /// \return Reference to the TERM record collection for modification
    IdCollection<TermRecord>& getTermCollection();
    /// \brief Get MATT collection (mutable)
    /// \return Reference to the MATT record collection for modification
    IdCollection<MattRecord>& getMattCollection();
    /// \brief Get MOVT collection (mutable)
    /// \return Reference to the MOVT record collection for modification
    IdCollection<MovtRecord>& getMovtCollection();
    /// \brief Get MUSC collection (mutable)
    /// \return Reference to the MUSC record collection for modification
    IdCollection<MuscRecord>& getMuscCollection();
    IdCollection<AactRecord>& getAactCollection();
    IdCollection<AamdRecord>& getAamdCollection();
    IdCollection<AapdRecord>& getAapdCollection();
    IdCollection<AchrRecord>& getAchrCollection();
    IdCollection<AddnRecord>& getAddnCollection();
    IdCollection<AffeRecord>& getAffeCollection();
    IdCollection<AmbsRecord>& getAmbsCollection();
    IdCollection<AmdlRecord>& getAmdlCollection();
    IdCollection<AopfRecord>& getAopfCollection();
    IdCollection<AopsRecord>& getAopsCollection();
    IdCollection<AoruRecord>& getAoruCollection();
    IdCollection<ArmaRecord>& getArmaCollection();
    IdCollection<ArtoRecord>& getArtoCollection();
    IdCollection<AspcRecord>& getAspcCollection();
    IdCollection<AtmoRecord>& getAtmoCollection();
    IdCollection<AvmdRecord>& getAvmdCollection();
    IdCollection<BiomRecord>& getBiomCollection();
    IdCollection<BmmoRecord>& getBmmoCollection();
    IdCollection<BmodRecord>& getBmodCollection();
    IdCollection<BndsRecord>& getBndsCollection();
    IdCollection<BpttRecord>& getBpttCollection();
    IdCollection<CamsRecord>& getCamsCollection();
    IdCollection<ChalRecord>& getChalCollection();
    IdCollection<CiftRecord>& getCiftCollection();
    IdCollection<CndaRecord>& getCndaCollection();
    IdCollection<CollRecord>& getCollCollection();
    IdCollection<CpthRecord>& getCpthCollection();
    IdCollection<CulkRecord>& getCulkCollection();
    IdCollection<Cur3Record>& getCur3Collection();
    IdCollection<CurvRecord>& getCurvCollection();
    IdCollection<DfobRecord>& getDfobCollection();
    IdCollection<DmgtRecord>& getDmgtCollection();
    IdCollection<DobjRecord>& getDobjCollection();
    IdCollection<EfsrRecord>& getEfsrCollection();
    IdCollection<EqupRecord>& getEqupCollection();

    /// \brief Get collection by record type (const version)
    /// \param type Record type identifier
    /// \return Const pointer to the collection, or nullptr if not found
    const BaseCollection* getCollectionByType(CkId::Type type) const;
    /// \brief Get collection by record type (non-const version)
    /// \param type Record type identifier
    /// \return Pointer to the collection, or nullptr if not found
    BaseCollection* getCollectionByType(CkId::Type type);

    /// \brief Get all collections as a vector
    /// \return Vector of all record collection pointers
    QVector<IRecordCollection*> allCollections();
    /// \brief Get all collections as a vector (const)
    /// \return Vector of all record collection pointers
    QVector<IRecordCollection*> allCollections() const;

    /// \brief Collection type pairing structure
    struct TypedCollection {
        IRecordCollection* collection;  ///< Pointer to the collection
        CkId::Type type;                ///< Record type enum
    };
    /// \brief Get all collections with their types
    /// \return Vector of TypedCollection pairs
    QVector<TypedCollection> allCollectionsWithTypes();
    /// \brief Get all collections with their types (const)
    /// \return Vector of TypedCollection pairs
    QVector<TypedCollection> allCollectionsWithTypes() const;

    /// \brief Clone a record from source to destination
    /// \param type Record type to clone
    /// \param src Editor ID of source record
    /// \param dest Editor ID for cloned record
    /// \return true if clone successful
    bool cloneRecord(CkId::Type type, const QString& src, const QString& dest);

    /// \brief Clone a record with undo support
    /// \param type Record type to clone
    /// \param src Editor ID of source record
    /// \param dest Editor ID for cloned record
    /// \return true if clone successful
    bool cloneRecordWithUndo(CkId::Type type, const QString& src, const QString& dest);

    /// \brief Batch clone multiple records with undo support
    /// \param type Record type to clone
    /// \param srcIds List of source editor IDs
    /// \param destIds List of destination editor IDs (one-to-one with srcIds)
    void batchCloneWithUndo(CkId::Type type, const QVector<QString>& srcIds, const QVector<QString>& destIds);

    /// \brief Batch set editor ID for multiple records with undo support
    /// \param type Record type to modify
    /// \param srcIds List of record editor IDs to modify
    /// \param newEditorId New editor ID to assign to all specified records
    void batchSetEditorIdWithUndo(CkId::Type type, const QVector<QString>& srcIds, const QString& newEditorId);

    /// \brief Remove a record by ID
    /// \param type Record type of the record to remove
    /// \param id Editor ID of the record to remove
    /// \return true if record was found and removed
    bool removeRecord(CkId::Type type, const QString& id);

    /// \brief Create new record with auto-assigned FormID
    /// \param type Record type to create (e.g., CkId::Npc_)
    /// \param editorId Optional editor ID (auto-generated if empty)
    /// \return New FormID assigned to the created record
    quint32 createNewRecord(CkId::Type type, const QString& editorId = "");

    /// \brief Conflict information structure for plugin conflicts
    struct ConflictInfo {
        CkId::Type type;        ///< Record type
        QString editorId;       ///< Editor ID of conflicting record
        int pluginIndexA;       ///< Plugin index A
        int pluginIndexB;       ///< Plugin index B
        QString pluginNameA;    ///< Plugin name A
        QString pluginNameB;    ///< Plugin name B
    };
    /// \brief Detect conflicts between loaded plugins
    /// \return QList of ConflictInfo for each detected conflict
    QList<ConflictInfo> detectConflicts();

    /// \brief Add NPC record to collection
    /// \param record Reference to the NPC record to add
    /// \return true if record was added successfully
    bool addNpc(NpcRecord& record);
    /// \brief Add weapon record to collection
    /// \param record Reference to the weapon record to add
    /// \return true if record was added successfully
    bool addWeapon(WeaponRecord& record);
    /// \brief Add armor record to collection
    /// \param record Reference to the armor record to add
    /// \return true if record was added successfully
    bool addArmor(ArmorRecord& record);
    /// \brief Add spell record to collection
    /// \param record Reference to the spell record to add
    /// \return true if record was added successfully
    bool addSpell(SpellRecord& record);
    /// \brief Add quest record to collection
    /// \param record Reference to the quest record to add
    /// \return true if record was added successfully
    bool addQuest(QuestRecord& record);
    /// \brief Add global variable record to collection
    /// \param record Reference to the global variable record to add
    /// \return true if record was added successfully
    bool addGlobVar(GlobalVariable& record);
    /// \brief Add dialogue record to collection
    /// \param record Reference to the dialogue record to add
    /// \return true if record was added successfully
    bool addDial(DialRecord& record);
    /// \brief Add dialogue info record to collection
    /// \param record Reference to the dialogue info record to add
    /// \return true if record was added successfully
    bool addInfo(InfoRecord& record);
    /// \brief Add tree record to collection
    /// \param record Reference to the tree record to add
    /// \return true if record was added successfully
    bool addTree(TreeRecord& record);
    /// \brief Add static record to collection
    /// \param record Reference to the static record to add
    /// \return true if record was added successfully
    bool addStat(StatRecord& record);
    /// \brief Add activator record to collection
    /// \param record Reference to the activator record to add
    /// \return true if record was added successfully
    bool addActi(ActiRecord& record);
    /// \brief Add misc item record to collection
    /// \param record Reference to the misc item record to add
    /// \return true if record was added successfully
    bool addMisc(MiscRecord& record);
    /// \brief Add alchemy record to collection
    /// \param record Reference to the alchemy record to add
    /// \return true if record was added successfully
    bool addAlch(AlchRecord& record);
    /// \brief Add ingredient record to collection
    /// \param record Reference to the ingredient record to add
    /// \return true if record was added successfully
    bool addIngr(IngrRecord& record);
    /// \brief Add book record to collection
    /// \param record Reference to the book record to add
    /// \return true if record was added successfully
    bool addBook(BookRecord& record);
    /// \brief Add enchantment record to collection
    /// \param record Reference to the enchantment record to add
    /// \return true if record was added successfully
    bool addEnch(EnchRecord& record);
    /// \brief Add container record to collection
    /// \param record Reference to the container record to add
    /// \return true if record was added successfully
    bool addCont(ContRecord& record);
    /// \brief Add race record to collection
    /// \param record Reference to the race record to add
    /// \return true if record was added successfully
    bool addRace(RaceRecord& record);
    /// \brief Add perk record to collection
    /// \param record Reference to the perk record to add
    /// \return true if record was added successfully
    bool addPerk(PerkRecord& record);
    /// \brief Add magic effect record to collection
    /// \param record Reference to the magic effect record to add
    /// \return true if record was added successfully
    bool addMagic(MagicRecord& record);
    /// \brief Add package record to collection
    /// \param record Reference to the package record to add
    /// \return true if record was added successfully
    bool addPack(PackageRecord& record);
    /// \brief Add location reference record to collection
    /// \param record Reference to the location reference type record to add
    /// \return true if record was added successfully
    bool addLcrt(LocationRefType& record);
    /// \brief Add class record to collection
    /// \param record Reference to the class record to add
    /// \return true if record was added successfully
    bool addClass(ClassRecord& record);
    /// \brief Add faction record to collection
    /// \param record Reference to the faction record to add
    /// \return true if record was added successfully
    bool addFact(FactRecord& record);
    /// \brief Add cell record to collection
    /// \param record Reference to the cell record to add
    /// \return true if record was added successfully
    bool addCell(CellRecord& record);
    /// \brief Add worldspace record to collection
    /// \param record Reference to the worldspace record to add
    /// \return true if record was added successfully
    bool addWorldspace(WorldspaceRecord& record);
    /// \brief Add location record to collection
    /// \param record Reference to the location record to add
    /// \return true if record was added successfully
    bool addLocation(LocationRecord& record);
bool addPlanet(PndRecord& record);
    /// \brief Add reference record to collection
    /// \param record Reference to the reference record to add
    /// \return true if record was added successfully
    bool addRef(RefrRecord& record);
    /// \brief Add material record to collection
    /// \param record Reference to the material record to add
    /// \return true if record was added successfully
    bool addMaterial(MaterialRecord& record);
    /// \brief Add landscape record to collection
    /// \param record Reference to the landscape record to add
    /// \return true if record was added successfully
    bool addLand(LandRecord& record);
    /// \brief Add sound record to collection
    /// \param record Reference to the sound record to add
    /// \return true if record was added successfully
    bool addSoun(SounRecord& record);
    /// \brief Add weather record to collection
    /// \param record Reference to the weather record to add
    /// \return true if record was added successfully
    bool addWthr(WthrRecord& record);
    /// \brief Add land texture record to collection
    /// \param record Reference to the land texture record to add
    /// \return true if record was added successfully
    bool addLtex(LtexRecord& record);

    /// \brief Get the main undo stack for all operations
    /// \return Pointer to the global undo stack
    UndoStack* getUndoStack();
    /// \brief Get undo stack for a specific plugin
    /// \param pluginIndex Plugin index (0 = master, 1+ = plugins)
    /// \return Pointer to the plugin's undo stack, or nullptr if not found
    UndoStack* getPluginUndoStack(int pluginIndex);
    /// \brief Set undo stack for a specific plugin
    /// \param pluginIndex Plugin index (0 = master, 1+ = plugins)
    /// \param stack Pointer to the undo stack to assign
    void setPluginUndoStack(int pluginIndex, UndoStack* stack);
    /// \brief Create a macro command for batch operations
    /// \param description Human-readable description of the batch operation
    /// \return Pointer to the newly created MacroCommand
    MacroCommand* createMacroCommand(const QString& description);
    /// \brief Get list of content file paths
    /// \return QStringList of ESM/ESP file paths loaded as content
    QStringList getContentFiles() const;

signals:
    /// \brief Emitted when record IDs change (e.g., after clone/remove)
    void idListChanged();

private:
    std::unique_ptr<ESMReader> reader;
    Header m_fallbackHeader;

    QStringList contentFiles;
    FilePaths paths;
    bool base;
    
    IdCollection<GameSetting> gameSettings;
    Collection<MetaData> metaData;

    IdCollection<NpcRecord> npcCollection;
    IdCollection<WeaponRecord> weaponCollection;
    IdCollection<ArmorRecord> armorCollection;
    IdCollection<SpellRecord> spellCollection;
    IdCollection<MagicRecord> magicCollection;
    IdCollection<QuestRecord> questCollection;
    IdCollection<DialRecord> dialCollection;
    IdCollection<InfoRecord> infoCollection;
    IdCollection<GlobalVariable> globCollection;
    IdCollection<LocationRefType> lcrtCollection;
    IdCollection<PackageRecord> packCollection;
    IdCollection<TreeRecord> treeCollection;
    IdCollection<AlchRecord> alchCollection;
    IdCollection<IngrRecord> ingrCollection;
    IdCollection<ContRecord> contCollection;
    IdCollection<EnchRecord> enchCollection;
    IdCollection<BookRecord> bookCollection;
    IdCollection<MiscRecord> miscCollection;
    IdCollection<ActiRecord> actiCollection;
    IdCollection<StatRecord> statCollection;
    IdCollection<RaceRecord> raceCollection;
    IdCollection<ClassRecord> classCollection;
    IdCollection<FactRecord> factCollection;
    IdCollection<PerkRecord> perkCollection;
    IdCollection<CellRecord> cellCollection;
    IdCollection<WorldspaceRecord> worldspaceCollection;
    IdCollection<LocationRecord> locationCollection;
    IdCollection<PndRecord> planetCollection;
    IdCollection<RefrRecord> refrCollection;
    IdCollection<MaterialRecord> materialCollection;
    IdCollection<LandRecord> landCollection;
    IdCollection<SounRecord> sounCollection;
    IdCollection<WthrRecord> wthrCollection;
    IdCollection<LtexRecord> ltexCollection;
    IdCollection<ScenRecord> scenCollection;
    IdCollection<AmmoRecord> ammoCollection;
    IdCollection<AppaRecord> appaCollection;
    IdCollection<ActorValueInfoRecord> avifCollection;
    IdCollection<BsgnRecord> bsgnCollection;
    IdCollection<ClimateRecord> clmtCollection;
    IdCollection<ClotRecord> clotCollection;
    IdCollection<CobjRecord> cobjCollection;
    IdCollection<CreatureRecord> creatureCollection;
    IdCollection<CstyRecord> cstyCollection;
    IdCollection<DoorRecord> doorCollection;
    IdCollection<EfshRecord> efshCollection;
    IdCollection<ExplRecord> explCollection;
    IdCollection<EyesRecord> eyesCollection;
    IdCollection<FlorRecord> florCollection;
    IdCollection<FormListRecord> flstCollection;
    IdCollection<FurnRecord> furnCollection;
    IdCollection<GrassRecord> grassCollection;
    IdCollection<HairRecord> hairCollection;
    IdCollection<IdleAnimationRecord> idleCollection;
    IdCollection<IdleMarkerRecord> idlmCollection;
    IdCollection<ImgsRecord> imgsCollection;
    IdCollection<KeymRecord> keymCollection;
    IdCollection<KeywordRecord> kywdCollection;
    IdCollection<LighRecord> lighCollection;
    IdCollection<LoadScreenRecord> lscrCollection;
    IdCollection<LvlcRecord> lvlcCollection;
    IdCollection<LvliRecord> lvliCollection;
    IdCollection<LvspRecord> lvspCollection;
    IdCollection<MesgRecord> mesgCollection;
    IdCollection<MsttRecord> msttCollection;
    IdCollection<NavmRecord> navmCollection;
    IdCollection<NoteRecord> noteCollection;
    IdCollection<OutfitRecord> otftCollection;
    IdCollection<ProjRecord> projCollection;
    IdCollection<RegionRecord> regnCollection;
    IdCollection<RoadRecord> roadCollection;
    IdCollection<ScriptRecord> scptCollection;
    IdCollection<ScrRecord> scrlCollection;
    IdCollection<SlgmRecord> slgmCollection;
    IdCollection<SmqnRecord> smqnCollection;
    IdCollection<SpgdRecord> spgdCollection;
    IdCollection<StaticCollectionRecord> scolCollection;
    IdCollection<TextureSetRecord> txstCollection;
    IdCollection<WateRecord> wateCollection;
    IdCollection<AnioRecord> anioCollection;
    IdCollection<ArtvRecord> artvCollection;
    IdCollection<ClfmRecord> clfmCollection;
    IdCollection<DebrRecord> debrCollection;
    IdCollection<EcznRecord> ecznCollection;
    IdCollection<HazdRecord> hazdCollection;
    IdCollection<IpctRecord> ipctCollection;
    IdCollection<IpdsRecord> ipdsCollection;
    IdCollection<MustRecord> mustCollection;
    IdCollection<RelaRecord> relaCollection;
    IdCollection<RevbRecord> revbCollection;
    IdCollection<ShouRecord> shouCollection;
    IdCollection<HdptRecord> hdptCollection;
    IdCollection<TermRecord> termCollection;
    IdCollection<MattRecord> mattCollection;
    IdCollection<MovtRecord> movtCollection;
    IdCollection<MuscRecord> muscCollection;
    IdCollection<AactRecord> aactCollection;
    IdCollection<AamdRecord> aamdCollection;
    IdCollection<AapdRecord> aapdCollection;
    IdCollection<AchrRecord> achrCollection;
    IdCollection<AddnRecord> addnCollection;
    IdCollection<AffeRecord> affeCollection;
    IdCollection<AmbsRecord> ambsCollection;
    IdCollection<AmdlRecord> amdlCollection;
    IdCollection<AopfRecord> aopfCollection;
    IdCollection<AopsRecord> aopsCollection;
    IdCollection<AoruRecord> aoruCollection;
    IdCollection<ArmaRecord> armaCollection;
    IdCollection<ArtoRecord> artoCollection;
    IdCollection<AspcRecord> aspcCollection;
    IdCollection<AtmoRecord> atmoCollection;
    IdCollection<AvmdRecord> avmdCollection;
    IdCollection<BiomRecord> biomCollection;
    IdCollection<BmmoRecord> bmmoCollection;
    IdCollection<BmodRecord> bmodCollection;
    IdCollection<BndsRecord> bndsCollection;
    IdCollection<BpttRecord> bpttCollection;
    IdCollection<CamsRecord> camsCollection;
    IdCollection<ChalRecord> chalCollection;
    IdCollection<CiftRecord> ciftCollection;
    IdCollection<CndaRecord> cndaCollection;
    IdCollection<CollRecord> collCollection;
    IdCollection<CpthRecord> cpthCollection;
    IdCollection<CulkRecord> culkCollection;
    IdCollection<Cur3Record> cur3Collection;
    IdCollection<CurvRecord> curvCollection;
    IdCollection<DfobRecord> dfobCollection;
    IdCollection<DmgtRecord> dmgtCollection;
    IdCollection<DobjRecord> dobjCollection;
    IdCollection<EfsrRecord> efsrCollection;
    IdCollection<EqupRecord> equpCollection;

    QVector<QAbstractItemModel*> models;
    QMap<CkId::Type, QAbstractItemModel*> modelIndexes;
    UndoStack* mUndoStack;
    QMap<int, UndoStack*> mPluginUndoStacks;

private slots:
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
};

#endif // WORLDDATA_H
