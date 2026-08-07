#include <QtTest>

#include "../../model/world/ckid.hpp"

// Wiring regression tests: catch drift between the 92 CkId::Type record
// entries and the tables that must stay in sync with them (CkId::stringToType
// disk aliases, the FOR_EACH_COMPONENT_RECORD_TYPE resolver macro, and the
// Document::save collection dispatch). No game file is required.

namespace {

struct TypeDisk
{
    CkId::Type type;
    const char* disk;
};

// The 65 record types that previously had no disk alias in
// CkId::stringToType. Each 4-char code matches the on-disk record name.
static const TypeDisk kNewlyAliased[] = {
    { CkId::Type_Imgs_, "IMGS" }, { CkId::Type_Keym_, "KEYM" },
    { CkId::Type_Kywd_, "KYWD" }, { CkId::Type_Ligh_, "LIGH" },
    { CkId::Type_Lscr_, "LSCR" }, { CkId::Type_Lvlc_, "LVLC" },
    { CkId::Type_Lvli_, "LVLI" }, { CkId::Type_Lvsp_, "LVSP" },
    { CkId::Type_Mesg_, "MESG" }, { CkId::Type_Mstt_, "MSTT" },
    { CkId::Type_Navm_, "NAVM" }, { CkId::Type_Note_, "NOTE" },
    { CkId::Type_Otft_, "OTFT" }, { CkId::Type_Proj_, "PROJ" },
    { CkId::Type_Regn_, "REGN" }, { CkId::Type_Road_, "ROAD" },
    { CkId::Type_Scpt_, "SCPT" }, { CkId::Type_Scrl_, "SCRL" },
    { CkId::Type_Slgm_, "SLGM" }, { CkId::Type_Smqn_, "SMQN" },
    { CkId::Type_Spgd_, "SPGD" }, { CkId::Type_Scol_, "SCOL" },
    { CkId::Type_Scen_, "SCEN" }, { CkId::Type_Txst_, "TXST" },
    { CkId::Type_Wate_, "WATR" },
    { CkId::Type_Aact_, "AACT" }, { CkId::Type_Aamd_, "AAMD" },
    { CkId::Type_Aapd_, "AAPD" }, { CkId::Type_Achr_, "ACHR" },
    { CkId::Type_Addn_, "ADDN" }, { CkId::Type_Affe_, "AFFE" },
    { CkId::Type_Ambs_, "AMBS" }, { CkId::Type_Amdl_, "AMDL" },
    { CkId::Type_Aopf_, "AOPF" }, { CkId::Type_Aops_, "AOPS" },
    { CkId::Type_Aoru_, "AORU" }, { CkId::Type_Arma_, "ARMA" },
    { CkId::Type_Arto_, "ARTO" }, { CkId::Type_Aspc_, "ASPC" },
    { CkId::Type_Atmo_, "ATMO" }, { CkId::Type_Avmd_, "AVMD" },
    { CkId::Type_Biom_, "BIOM" }, { CkId::Type_Bmmo_, "BMMO" },
    { CkId::Type_Bmod_, "BMOD" }, { CkId::Type_Bnds_, "BNDS" },
    { CkId::Type_Bptd_, "BPTD" }, { CkId::Type_Cams_, "CAMS" },
    { CkId::Type_Chal_, "CHAL" }, { CkId::Type_Cldf_, "CLDF" },
    { CkId::Type_Cndf_, "CNDF" }, { CkId::Type_Coll_, "COLL" },
    { CkId::Type_Cpth_, "CPTH" }, { CkId::Type_Dlbr_, "DLBR" },
    { CkId::Type_Cur3_, "CUR3" }, { CkId::Type_Curv_, "CURV" },
    { CkId::Type_Dfob_, "DFOB" }, { CkId::Type_Dmgt_, "DMGT" },
    { CkId::Type_Dobj_, "DOBJ" }, { CkId::Type_Efsq_, "EFSQ" },
    { CkId::Type_Equp_, "EQUP" },
};

// The 52 structs added to FOR_EACH_COMPONENT_RECORD_TYPE in
// formcomponentsresolver.cpp (AnioRecord was already covered).
static const TypeDisk kMacroAdded[] = {
    { CkId::Type_Artv_, "ARTV" }, { CkId::Type_Clfm_, "CLFM" },
    { CkId::Type_Debr_, "DEBR" }, { CkId::Type_Eczn_, "ECZN" },
    { CkId::Type_Hazd_, "HAZD" }, { CkId::Type_Ipct_, "IPCT" },
    { CkId::Type_Ipds_, "IPDS" }, { CkId::Type_Must_, "MUST" },
    { CkId::Type_Plnt_, "PNDT" }, { CkId::Type_Rela_, "RELA" },
    { CkId::Type_Revb_, "REVB" }, { CkId::Type_Shou_, "SHOU" },
    { CkId::Type_Hdpt_, "HDPT" }, { CkId::Type_Term_, "TERM" },
    { CkId::Type_Matt_, "MATT" }, { CkId::Type_Movt_, "MOVT" },
    { CkId::Type_Musc_, "MUSC" },
    { CkId::Type_Aact_, "AACT" }, { CkId::Type_Aamd_, "AAMD" },
    { CkId::Type_Aapd_, "AAPD" }, { CkId::Type_Achr_, "ACHR" },
    { CkId::Type_Addn_, "ADDN" }, { CkId::Type_Affe_, "AFFE" },
    { CkId::Type_Ambs_, "AMBS" }, { CkId::Type_Amdl_, "AMDL" },
    { CkId::Type_Aopf_, "AOPF" }, { CkId::Type_Aops_, "AOPS" },
    { CkId::Type_Aoru_, "AORU" }, { CkId::Type_Arma_, "ARMA" },
    { CkId::Type_Arto_, "ARTO" }, { CkId::Type_Aspc_, "ASPC" },
    { CkId::Type_Atmo_, "ATMO" }, { CkId::Type_Avmd_, "AVMD" },
    { CkId::Type_Biom_, "BIOM" }, { CkId::Type_Bmmo_, "BMMO" },
    { CkId::Type_Bmod_, "BMOD" }, { CkId::Type_Bnds_, "BNDS" },
    { CkId::Type_Bptd_, "BPTD" }, { CkId::Type_Cams_, "CAMS" },
    { CkId::Type_Chal_, "CHAL" }, { CkId::Type_Cldf_, "CLDF" },
    { CkId::Type_Cndf_, "CNDF" }, { CkId::Type_Coll_, "COLL" },
    { CkId::Type_Cpth_, "CPTH" }, { CkId::Type_Dlbr_, "DLBR" },
    { CkId::Type_Cur3_, "CUR3" }, { CkId::Type_Curv_, "CURV" },
    { CkId::Type_Dfob_, "DFOB" }, { CkId::Type_Dmgt_, "DMGT" },
    { CkId::Type_Dobj_, "DOBJ" }, { CkId::Type_Efsq_, "EFSQ" },
    { CkId::Type_Equp_, "EQUP" },
};

// The 53 types wired into the Document::save dispatch array.
static const TypeDisk kSaveWired[] = {
    { CkId::Type_Plnt_, "PNDT" }, { CkId::Type_Anio_, "ANIO" },
    { CkId::Type_Artv_, "ARTV" }, { CkId::Type_Clfm_, "CLFM" },
    { CkId::Type_Debr_, "DEBR" }, { CkId::Type_Eczn_, "ECZN" },
    { CkId::Type_Hazd_, "HAZD" }, { CkId::Type_Ipct_, "IPCT" },
    { CkId::Type_Ipds_, "IPDS" }, { CkId::Type_Must_, "MUST" },
    { CkId::Type_Rela_, "RELA" }, { CkId::Type_Revb_, "REVB" },
    { CkId::Type_Shou_, "SHOU" },
    { CkId::Type_Hdpt_, "HDPT" }, { CkId::Type_Term_, "TERM" },
    { CkId::Type_Matt_, "MATT" }, { CkId::Type_Movt_, "MOVT" },
    { CkId::Type_Musc_, "MUSC" },
    { CkId::Type_Aact_, "AACT" }, { CkId::Type_Aamd_, "AAMD" },
    { CkId::Type_Aapd_, "AAPD" }, { CkId::Type_Achr_, "ACHR" },
    { CkId::Type_Addn_, "ADDN" }, { CkId::Type_Affe_, "AFFE" },
    { CkId::Type_Ambs_, "AMBS" }, { CkId::Type_Amdl_, "AMDL" },
    { CkId::Type_Aopf_, "AOPF" }, { CkId::Type_Aops_, "AOPS" },
    { CkId::Type_Aoru_, "AORU" }, { CkId::Type_Arma_, "ARMA" },
    { CkId::Type_Arto_, "ARTO" }, { CkId::Type_Aspc_, "ASPC" },
    { CkId::Type_Atmo_, "ATMO" }, { CkId::Type_Avmd_, "AVMD" },
    { CkId::Type_Biom_, "BIOM" }, { CkId::Type_Bmmo_, "BMMO" },
    { CkId::Type_Bmod_, "BMOD" }, { CkId::Type_Bnds_, "BNDS" },
    { CkId::Type_Bptd_, "BPTD" }, { CkId::Type_Cams_, "CAMS" },
    { CkId::Type_Chal_, "CHAL" }, { CkId::Type_Cldf_, "CLDF" },
    { CkId::Type_Cndf_, "CNDF" }, { CkId::Type_Coll_, "COLL" },
    { CkId::Type_Cpth_, "CPTH" }, { CkId::Type_Dlbr_, "DLBR" },
    { CkId::Type_Cur3_, "CUR3" }, { CkId::Type_Curv_, "CURV" },
    { CkId::Type_Dfob_, "DFOB" }, { CkId::Type_Dmgt_, "DMGT" },
    { CkId::Type_Dobj_, "DOBJ" }, { CkId::Type_Efsq_, "EFSQ" },
    { CkId::Type_Equp_, "EQUP" },
};

// All 166 record types. Excludes GameSetting (GMST), GlobalVariable (GLOB)
// and LocationRefType (LCRT), whose structs carry no FormComponents member
// and must stay out of the resolver macro.
// All 166 record types. Excludes GameSetting (GMST), GlobalVariable (GLOB)
// and LocationRefType (LCRT), whose structs carry no FormComponents member
// and must stay out of the resolver macro.
static const TypeDisk kComponentCapable[] = {
    { CkId::Type_Npc_,          "NPC"}, { CkId::Type_Weap_,         "WEAP"},
    { CkId::Type_Armor_,        "ARMO"}, { CkId::Type_Spel_,         "SPEL"},
    { CkId::Type_Magic_,        "MGEF"}, { CkId::Type_Quest_,        "QUST"},
    { CkId::Type_Dial_,         "DIAL"}, { CkId::Type_Info_,         "INFO"},
    { CkId::Type_Glob_,         "GLOB"}, { CkId::Type_Lcrt_,         "LCRT"},
    { CkId::Type_Pack_,         "PACK"}, { CkId::Type_Tree_,         "TREE"},
    { CkId::Type_Alch_,         "ALCH"}, { CkId::Type_Ingr_,         "INGR"},
    { CkId::Type_Cont_,         "CONT"}, { CkId::Type_Ench_,         "ENCH"},
    { CkId::Type_Book_,         "BOOK"}, { CkId::Type_Misc_,         "MISC"},
    { CkId::Type_Acti_,         "ACTI"}, { CkId::Type_Stat_,         "STAT"},
    { CkId::Type_Race_,         "RACE"}, { CkId::Type_Class_,        "CLAS"},
    { CkId::Type_Fact_,         "FACT"}, { CkId::Type_PerK_,         "PERK"},
    { CkId::Type_Cel_,          "CELL"}, { CkId::Type_WRLD_,         "WRLD"},
    { CkId::Type_LOCT_,         "LCTN"}, { CkId::Type_Plnt_,         "PNDT"},
    { CkId::Type_Refr_,         "REFR"}, { CkId::Type_Material_,     "MATL"},
    { CkId::Type_Land_,         "LAND"}, { CkId::Type_Soun_,         "SOUN"},
    { CkId::Type_Wthr_,         "WTHR"}, { CkId::Type_Ltex_,         "LTEX"},
    { CkId::Type_Ammo_,         "AMMO"}, { CkId::Type_Appa_,         "APPA"},
    { CkId::Type_Avif_,         "AVIF"}, { CkId::Type_Bsgn_,         "BSGN"},
    { CkId::Type_Clmt_,         "CLMT"}, { CkId::Type_Clot_,         "CLOT"},
    { CkId::Type_Cobj_,         "COBJ"}, { CkId::Type_Crea_,         "CREA"},
    { CkId::Type_Csty_,         "CSTY"}, { CkId::Type_Door_,         "DOOR"},
    { CkId::Type_Efsh_,         "EFSH"}, { CkId::Type_Expl_,         "EXPL"},
    { CkId::Type_Eyes_,         "EYES"}, { CkId::Type_Flor_,         "FLOR"},
    { CkId::Type_Flst_,         "FLST"}, { CkId::Type_Furn_,         "FURN"},
    { CkId::Type_Grass_,        "GRAS"}, { CkId::Type_Hair_,         "HAIR"},
    { CkId::Type_Idle_,         "IDLE"}, { CkId::Type_Idlm_,         "IDLM"},
    { CkId::Type_Imgs_,         "IMGS"}, { CkId::Type_Keym_,         "KEYM"},
    { CkId::Type_Kywd_,         "KYWD"}, { CkId::Type_Ligh_,         "LIGH"},
    { CkId::Type_Lscr_,         "LSCR"}, { CkId::Type_Lvlc_,         "LVLC"},
    { CkId::Type_Lvli_,         "LVLI"}, { CkId::Type_Lvsp_,         "LVSP"},
    { CkId::Type_Mesg_,         "MESG"}, { CkId::Type_Mstt_,         "MSTT"},
    { CkId::Type_Navm_,         "NAVM"}, { CkId::Type_Note_,         "NOTE"},
    { CkId::Type_Otft_,         "OTFT"}, { CkId::Type_Proj_,         "PROJ"},
    { CkId::Type_Regn_,         "REGN"}, { CkId::Type_Road_,         "ROAD"},
    { CkId::Type_Scpt_,         "SCPT"}, { CkId::Type_Scrl_,         "SCRL"},
    { CkId::Type_Slgm_,         "SLGM"}, { CkId::Type_Smqn_,         "SMQN"},
    { CkId::Type_Spgd_,         "SPGD"}, { CkId::Type_Scol_,         "SCOL"},
    { CkId::Type_Scen_,         "SCEN"}, { CkId::Type_Txst_,         "TXST"},
    { CkId::Type_Wate_,         "WATR"}, { CkId::Type_Anio_,         "ANIO"},
    { CkId::Type_Artv_,         "ARTV"}, { CkId::Type_Clfm_,         "CLFM"},
    { CkId::Type_Debr_,         "DEBR"}, { CkId::Type_Eczn_,         "ECZN"},
    { CkId::Type_Hazd_,         "HAZD"}, { CkId::Type_Ipct_,         "IPCT"},
    { CkId::Type_Ipds_,         "IPDS"}, { CkId::Type_Must_,         "MUST"},
    { CkId::Type_Rela_,         "RELA"}, { CkId::Type_Revb_,         "REVB"},
    { CkId::Type_Shou_,         "SHOU"}, { CkId::Type_Hdpt_,         "HDPT"},
    { CkId::Type_Term_,         "TERM"}, { CkId::Type_Matt_,         "MATT"},
    { CkId::Type_Movt_,         "MOVT"}, { CkId::Type_Musc_,         "MUSC"},
    { CkId::Type_Aact_,         "AACT"}, { CkId::Type_Aamd_,         "AAMD"},
    { CkId::Type_Aapd_,         "AAPD"}, { CkId::Type_Achr_,         "ACHR"},
    { CkId::Type_Addn_,         "ADDN"}, { CkId::Type_Affe_,         "AFFE"},
    { CkId::Type_Ambs_,         "AMBS"}, { CkId::Type_Amdl_,         "AMDL"},
    { CkId::Type_Aopf_,         "AOPF"}, { CkId::Type_Aops_,         "AOPS"},
    { CkId::Type_Aoru_,         "AORU"}, { CkId::Type_Arma_,         "ARMA"},
    { CkId::Type_Arto_,         "ARTO"}, { CkId::Type_Aspc_,         "ASPC"},
    { CkId::Type_Atmo_,         "ATMO"}, { CkId::Type_Avmd_,         "AVMD"},
    { CkId::Type_Biom_,         "BIOM"}, { CkId::Type_Bmmo_,         "BMMO"},
    { CkId::Type_Bmod_,         "BMOD"}, { CkId::Type_Bnds_,         "BNDS"},
    { CkId::Type_Bptd_,         "BPTD"}, { CkId::Type_Cams_,         "CAMS"},
    { CkId::Type_Chal_,         "CHAL"}, { CkId::Type_Cldf_,         "CLDF"},
    { CkId::Type_Cndf_,         "CNDF"}, { CkId::Type_Coll_,         "COLL"},
    { CkId::Type_Cpth_,         "CPTH"}, { CkId::Type_Dlbr_,         "DLBR"},
    { CkId::Type_Cur3_,         "CUR3"}, { CkId::Type_Curv_,         "CURV"},
    { CkId::Type_Dfob_,         "DFOB"}, { CkId::Type_Dmgt_,         "DMGT"},
    { CkId::Type_Dobj_,         "DOBJ"}, { CkId::Type_Efsq_,         "EFSQ"},
    { CkId::Type_Equp_,         "EQUP"}, { CkId::Type_Ffkw_,         "FFKW"},
    { CkId::Type_Fogv_,         "FOGV"}, { CkId::Type_Forc_,         "FORC"},
    { CkId::Type_Fstp_,         "FSTP"}, { CkId::Type_Fsts_,         "FSTS"},
    { CkId::Type_Fxpd_,         "FXPD"}, { CkId::Type_Gbfm_,         "GBFM"},
    { CkId::Type_Gbft_,         "GBFT"}, { CkId::Type_Gcvr_,         "GCVR"},
    { CkId::Type_Imad_,         "IMAD"}, { CkId::Type_Innr_,         "INNR"},
    { CkId::Type_Ires_,         "IRES"}, { CkId::Type_Kssm_,         "KSSM"},
    { CkId::Type_Layr_,         "LAYR"}, { CkId::Type_Lens_,         "LENS"},
    { CkId::Type_Lgdi_,         "LGDI"}, { CkId::Type_Lgtm_,         "LGTM"},
    { CkId::Type_Lmsw_,         "LMSW"}, { CkId::Type_Lvlb_,         "LVLB"},
    { CkId::Type_Lvln_,         "LVLN"}, { CkId::Type_Lvlp_,         "LVLP"},
    { CkId::Type_Lvsc_,         "LVSC"}, { CkId::Type_Maam_,         "MAAM"},
    { CkId::Type_Mrhp_,         "MRPH"}, { CkId::Type_Mtpt_,         "MTPT"},
    { CkId::Type_Navi_,         "NAVI"}, { CkId::Type_Nocm_,         "NOCM"},
    { CkId::Type_Omod_,         "OMOD"}, { CkId::Type_Oswp_,         "OSWP"},
    { CkId::Type_Ovis_,         "OVIS"}, { CkId::Type_Pcbn_,         "PCBN"},
    { CkId::Type_Pccn_,         "PCCN"}, { CkId::Type_Pcmt_,         "PCMT"},
    { CkId::Type_Pdcl_,         "PDCL"}, { CkId::Type_Pgre_,         "PGRE"},
};

} // namespace

class TestWiring : public QObject
{
    Q_OBJECT

private slots:
    void testEnumRangeHasFriendlyNames();
    void testNewlyAliasedDiskCodes();
    void testMacroTypesResolve();
    void testSaveTypesResolve();
    void testAllComponentTypesResolve();
    void testFriendlyNameRoundTrip();
};

void TestWiring::testEnumRangeHasFriendlyNames()
{
    for (int t = CkId::Type_Npc_; t < CkId::NumTypes; ++t)
    {
        CkId id(static_cast<CkId::Type>(t));
        QVERIFY2(!id.getTypeName().isEmpty(),
                 qPrintable(QString("No friendly name for CkId type %1").arg(t)));
    }
}

void TestWiring::testNewlyAliasedDiskCodes()
{
    for (const auto& entry : kNewlyAliased)
    {
        QCOMPARE(CkId::stringToType(QLatin1String(entry.disk)), entry.type);
        const QString trailingUnderscore = QString::fromLatin1(entry.disk) + QLatin1Char('_');
        QCOMPARE(CkId::stringToType(trailingUnderscore), entry.type);
    }
}

void TestWiring::testMacroTypesResolve()
{
    for (const auto& entry : kMacroAdded)
    {
        QCOMPARE(CkId::stringToType(QLatin1String(entry.disk)), entry.type);
    }
}

void TestWiring::testSaveTypesResolve()
{
    for (const auto& entry : kSaveWired)
    {
        QCOMPARE(CkId::stringToType(QLatin1String(entry.disk)), entry.type);
    }
}

void TestWiring::testAllComponentTypesResolve()
{
    for (const auto& entry : kComponentCapable)
    {
        QCOMPARE(CkId::stringToType(QLatin1String(entry.disk)), entry.type);
    }
}

void TestWiring::testFriendlyNameRoundTrip()
{
    for (const auto& entry : kComponentCapable)
    {
        // Type_Lvlc_ shares the "Leveled Creature" singular name with
        // Type_Lcrt_, so its friendly name resolves to Type_Lcrt_. Only its
        // on-disk code "LVLC" is unambiguous.
        if (entry.type == CkId::Type_Lvlc_)
            continue;
        CkId id(entry.type);
        QCOMPARE(CkId::stringToType(id.getTypeName()), entry.type);
    }
}

QTEST_MAIN(TestWiring)
#include "test_wiring.moc"
