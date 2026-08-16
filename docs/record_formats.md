# OpenCK Record-Format Audit Table

> Generated mechanically from the loaders in `libs/files/esm/` by
> 	ools/gen_record_audit.ps1 on 2026-08-16.
>
> Legend for the **Status** column:
> - `OK` â€” the loader consumes exactly its declared record size against
>   real master data (see the W1 reader diagnostics); unknown/foreign
>   subrecords are preserved verbatim in `rawSubRecords` so the
>   round-trip is lossless.
> - `raw-only` â€” the loader is a pure raw-preservation pass (no typed
>   fields); acceptable, subrecords round-trip untouched.
> - `parsed-wrong` â€” a typed parser disagrees with the documented or
>   observed layout (tracked in RealFixes.md section 9).
>
> `Subrecords handled` lists the on-disk tags the loader dispatches on
> inside `load()` (NAME/XNAME literals). All other
> subrecords are preserved raw. `Type` is the on-disk record tag.

| Type | Loader | Subrecords handled | Status |
|---|---|---|---|
| AACT | `aactrecord.cpp` | EDID | OK |
| AAMD | `aamdrecord.cpp` | EDID | OK |
| AAPD | `aapdrecord.cpp` | EDID | OK |
| ACHR | `achrrecord.cpp` | EDID | OK |
| ACTI | `Actirecord.cpp` | EDID, FNAM, FLAG | OK |
| ADDN | `addnrecord.cpp` | EDID | OK |
| AFFE | `afferecord.cpp` | EDID | OK |
| ALCH | `Alchrecord.cpp` | EDID, FNAM, FLAG, DATA | OK |
| AMBS | `ambsrecord.cpp` | EDID | OK |
| AMDL | `amdlrecord.cpp` | EDID | OK |
| AMMO | `ammorecord.cpp` | EDID, FNAM, FLAG, DATA | OK |
| ANIO | `aniorecord.cpp` | EDID, DNAM | OK |
| AOPF | `aopfrecord.cpp` | EDID | OK |
| AOPS | `aopsrecord.cpp` | EDID | OK |
| AORU | `aorurecord.cpp` | EDID | OK |
| APPA | `apparatusrecord.cpp` | EDID, DATA | OK |
| ARMA | `armarecord.cpp` | EDID | OK |
| ARMO | `Armorrecord.cpp` | EDID, FLAG, DNAM, DATA | OK |
| ARTO | `artorecord.cpp` | EDID | OK |
| ARTV | `artvrecord.cpp` | EDID, DATA | OK |
| ASPC | `aspcrecord.cpp` | EDID | OK |
| ATMO | `atmrecord.cpp` | EDID | OK |
| AVIF | `actorvalueinforecord.cpp` | EDID | OK |
| AVMD | `avmdrecord.cpp` | EDID | OK |
| BIOM | `biomrecord.cpp` | EDID | OK |
| BMMO | `bmmorecord.cpp` | EDID | OK |
| BMOD | `bmodrecord.cpp` | EDID | OK |
| BNDS | `bndsrecord.cpp` | EDID | OK |
| BOOK | `Bookrecord.cpp` | EDID, FNAM, FLAG, DATA | OK |
| BPTD | `bptdrecord.cpp` | EDID | OK |
| BSGN | `birthsignrecord.cpp` | EDID, TNAM | OK |
| CAMS | `camsrecord.cpp` | EDID | OK |
| CELL | `cellrecord.cpp` | EDID, DATA, XCLC, XOWN, XLOC, XCLW | OK |
| CHAL | `chalrecord.cpp` | EDID | OK |
| CLAS | `Classrecord.cpp` | EDID, FNAM, FLAG, FULL, DESC, CNAM | OK |
| CLDF | `cldfrecord.cpp` | EDID | OK |
| CLFM | `clfmrecord.cpp` | EDID, CNAM, FNAM | OK |
| CLMT | `climaterecord.cpp` | EDID | OK |
| CLOT | `clothrecord.cpp` | EDID | OK |
| CNDF | `cndfrecord.cpp` | EDID | OK |
| COBJ | `constructibleobjectrecord.cpp` | EDID | OK |
| COLL | `collrecord.cpp` | EDID | OK |
| CONT | `Contrecord.cpp` | EDID, FNAM, FLAG, DATA, COCT | OK |
| CPTH | `cpthrecord.cpp` | EDID | OK |
| CREA | `creaturerecord.cpp` | EDID, DATA | OK |
| CSTY | `combatstylerecord.cpp` | EDID | OK |
| CUR3 | `cur3record.cpp` | EDID | OK |
| CURV | `curvrecord.cpp` | EDID | OK |
| DEBR | `debrrecord.cpp` | EDID, DATA | OK |
| DFOB | `dfobrecord.cpp` | EDID | OK |
| DIAL | `Dialrecord.cpp` | EDID, FULL | OK |
| DLBR | `dlbrrecord.cpp` | EDID | OK |
| DMGT | `dmgtrecord.cpp` | EDID | OK |
| DOBJ | `dobjrecord.cpp` | EDID | OK |
| DOOR | `doorrecord.cpp` | EDID, FNAM, FLAG, SNAM | OK |
| ECZN | `ecznrecord.cpp` | EDID, DATA | OK |
| EFSH | `effectshaderrecord.cpp` | EDID, DATA | OK |
| EFSQ | `efsqrecord.cpp` | EDID | OK |
| ENCH | `Enchrecord.cpp` | EDID, FNAM, FLAG, ENIT | OK |
| EQUP | `equprecord.cpp` | EDID | OK |
| EXPL | `explosionrecord.cpp` | EDID | OK |
| EYES | `eyesrecord.cpp` | EDID | OK |
| FACT | `Factrecord.cpp` | EDID, FNAM, FLAG, FULL, XNAM, RNAM | OK |
| FFKW | `ffkwrecord.cpp` | EDID | OK |
| FLOR | `florrecord.cpp` | EDID, FNAM, FLAG, PFIG | OK |
| FLST | `formlistrecord.cpp` | EDID, LNAM | OK |
| FOGV | `fogvrecord.cpp` | EDID | OK |
| FORC | `forcrecord.cpp` | EDID | OK |
| FSTP | `fstprecord.cpp` | EDID | OK |
| FSTS | `fstsrecord.cpp` | EDID | OK |
| FURN | `furnrecord.cpp` | EDID, FNAM, FLAG | OK |
| FXPD | `fxpdrecord.cpp` | EDID | OK |
| GBFM | `gbfmrecord.cpp` | EDID | OK |
| GBFT | `gbftrecord.cpp` | EDID | OK |
| GCVR | `gcvrrecord.cpp` | EDID | OK |
| GRAS | `grassrecord.cpp` | EDID | OK |
| HAIR | `hairrecord.cpp` | EDID | OK |
| HAZD | `hazdrecord.cpp` | EDID, DATA | OK |
| HDPT | `hdptrecord.cpp` | EDID | OK |
| IDLE | `idleanimationrecord.cpp` | EDID | OK |
| IDLM | `idlemarkerrecord.cpp` | EDID | OK |
| IMAD | `imadrecord.cpp` | EDID | OK |
| IMGS | `imagespacerecord.cpp` | EDID, DATA | OK |
| INFO | `Inforecord.cpp` | CNAM, CTDA, TLOI | OK |
| INGR | `Ingrrecord.cpp` | EDID, FNAM, FLAG, DATA | OK |
| INNR | `innrrecord.cpp` | EDID | OK |
| IPCT | `ipctrecord.cpp` | EDID, DATA | OK |
| IPDS | `ipdsrecord.cpp` | EDID, IPDS | OK |
| IRES | `iresrecord.cpp` | EDID | OK |
| KEYM | `keymrecord.cpp` | EDID, FNAM, FLAG, DATA | OK |
| KSSM | `kssmrecord.cpp` | EDID | OK |
| KYWD | `keywordrecord.cpp` | EDID | OK |
| LAND | `landrecord.cpp` | EDID, VHGT, VNML, VCLR, VTEX | OK |
| LAYR | `layrrecord.cpp` | EDID | OK |
| LCTN | `locationrecord.cpp` | EDID, FNAM, FLAG, PNAM, XNAM, LNAM, DATA | OK |
| LENS | `lensrecord.cpp` | EDID | OK |
| LGDI | `lgdirecord.cpp` | EDID | OK |
| LGTM | `lgtmrecord.cpp` | EDID | OK |
| LIGH | `lighrecord.cpp` | EDID, FNAM, FLAG, FNDS, DATA | OK |
| LMSW | `lmswrecord.cpp` | EDID | OK |
| LSCR | `loadscreenrecord.cpp` | EDID | OK |
| LTEX | `ltexrecord.cpp` | EDID, FNAM, FLAG, HNAM, SNAM | OK |
| LVLB | `lvlbrecord.cpp` | EDID | OK |
| LVLC | `lvlcreaturerecord.cpp` | EDID, LVLD, LVLF, LVLO | OK |
| LVLI | `lvlistrecord.cpp` | EDID, LVLD, LVLF, LVLO | OK |
| LVLN | `lvlnrecord.cpp` | EDID | OK |
| LVLP | `lvlprecord.cpp` | EDID | OK |
| LVSC | `lvscrecord.cpp` | EDID | OK |
| LVSP | `lvspellrecord.cpp` | EDID, LVLD, LVLF, LVLO | OK |
| MAAM | `maamrecord.cpp` | EDID | OK |
| MATL | `materialrecord.cpp` | EDID, BKMN, BNAM, CNAM, MNAM | OK |
| MATT | `mattrecord.cpp` | EDID | OK |
| MESG | `messagerecord.cpp` | EDID | OK |
| MGEF | `Magicrecord.cpp` | EDID, FNAM, FLAG, MDOB, SNAM | OK |
| MISC | `Miscrecord.cpp` | EDID, FNAM, FLAG, DATA | OK |
| MOVT | `movtrecord.cpp` | EDID | OK |
| MRPH | `mrhprecord.cpp` | EDID | OK |
| MSTT | `msttrecord.cpp` | EDID, FNAM, FLAG, DATA | OK |
| MTPT | `mtptrecord.cpp` | EDID | OK |
| MUSC | `muscrecord.cpp` | EDID | OK |
| MUST | `mustrecord.cpp` | EDID, CNAM, FNAM | OK |
| NAVI | `navirecord.cpp` | EDID | OK |
| NAVM | `navmrecord.cpp` | EDID, NVMV, NVTR, NVCA, NVDP, NVCV, NVMI | OK |
| NOCM | `nocmrecord.cpp` | EDID | OK |
| NOTE | `noterecord.cpp` | EDID | OK |
| NPC_ | `npcrecord.cpp` | EDID, RNAM, CNAM, ANAM, CNTO | OK |
| OMOD | `omodrecord.cpp` | EDID | OK |
| OSWP | `oswprecord.cpp` | EDID | OK |
| OTFT | `outfitrecord.cpp` | EDID, INAM, DATA | OK |
| OVIS | `ovisrecord.cpp` | EDID | OK |
| PACK | `Packagerecord.cpp` | EDID, PKDT, PLDT, PTDT, CTDA | OK |
| PCBN | `pcbnrecord.cpp` | EDID | OK |
| PCCN | `pccnrecord.cpp` | EDID | OK |
| PCMT | `pcmtrecord.cpp` | EDID | OK |
| PDCL | `pdclrecord.cpp` | EDID | OK |
| PERK | `Perkrecord.cpp` | EDID, FNAM, FLAG, DESC, CTDA | OK |
| PGRE | `pgrerecord.cpp` | EDID | OK |
| PHZD | `phzdrecord.cpp` | EDID | OK |
| PKIN | `pkinrecord.cpp` | EDID | OK |
| PMFT | `pmftrecord.cpp` | EDID | OK |
| PNDT | `pndrecord.cpp` | EDID, FNAM, ANAM, TEMP, DENS, PHLA, RSCS | OK |
| PROJ | `projectilerecord.cpp` | EDID | OK |
| PSDC | `psdcrecord.cpp` | EDID | OK |
| PTST | `ptstrecord.cpp` | EDID | OK |
| QUST | `Questrecord.cpp` | EDID, DNAM, CTDA, INDX, QSDT, CNAM, SCHR, SCDA, SCRO, SCTX, SLSD, QOBJ, NAM1, NAM2, ALST, ALID | OK |
| RACE | `Racerecord.cpp` | EDID | OK |
| REFR | `refrecord.cpp` | EDID | OK |
| REGN | `regionrecord.cpp` | EDID | OK |
| RELA | `relarecord.cpp` | EDID, DATA | OK |
| REVB | `revbrecord.cpp` | EDID, DATA | OK |
| RFGP | `rfgprecord.cpp` | EDID | OK |
| ROAD | `roadrecord.cpp` | EDID | OK |
| RSGD | `rsgdrecord.cpp` | EDID | OK |
| RSPJ | `rspjrecord.cpp` | EDID | OK |
| SCEN | `scenrecord.cpp` | EDID, CTDA | OK |
| SCOL | `staticcollectionrecord.cpp` | EDID | OK |
| SCPT | `scriptrecord.cpp` | EDID, SCHD, SCDA, SCTX, SCRO | OK |
| SCRL | `scrollrecord.cpp` | EDID | OK |
| SDLT | `sdltrecord.cpp` | EDID | OK |
| SECH | `sechrecord.cpp` | EDID | OK |
| SFBK | `sfbkrecord.cpp` | EDID | OK |
| SFPC | `sfpcrecord.cpp` | EDID | OK |
| SFPT | `sfptrecord.cpp` | EDID | OK |
| SFTR | `sftrrecord.cpp` | EDID | OK |
| SHOU | `shourecord.cpp` | EDID, FULL, SNAM | OK |
| SLGM | `slgmrecord.cpp` | EDID, FNAM, FLAG, DATA, SOUL, SLCP | OK |
| SMBN | `smbnrecord.cpp` | EDID | OK |
| SMEN | `smenrecord.cpp` | EDID | OK |
| SMQN | `soundmarkerrecord.cpp` | EDID | OK |
| SOUN | `sounrecord.cpp` | EDID | OK |
| SPCH | `spchrecord.cpp` | EDID | OK |
| SPEL | `Spellrecord.cpp` | EDID, FNAM, FLAG, SPIT, SNAM, SPDT | OK |
| SPGD | `shaderparticlerecord.cpp` | EDID | OK |
| STAG | `stagrecord.cpp` | EDID | OK |
| STAT | `Statrecord.cpp` | EDID, FNAM, FLAG, RNAM | OK |
| STBH | `stbhrecord.cpp` | EDID | OK |
| STDT | `stdtrecord.cpp` | EDID | OK |
| STMP | `stmprecord.cpp` | EDID | OK |
| STND | `stndrecord.cpp` | EDID | OK |
| SUNP | `sunprecord.cpp` | EDID | OK |
| TERM | `termrecord.cpp` | EDID | OK |
| TMLM | `tmlmrecord.cpp` | EDID | OK |
| TODD | `toddrecord.cpp` | EDID | OK |
| TRAV | `travrecord.cpp` | EDID | OK |
| TREE | `Treerecord.cpp` | EDID, FNAM, FLAG, DATA, PFIG | OK |
| TRNS | `trnsrecord.cpp` | EDID | OK |
| TXST | `texturesetrecord.cpp` | EDID | OK |
| VOLI | `volirecord.cpp` | EDID | OK |
| VTYP | `vtyprecord.cpp` | EDID | OK |
| WATR | `waterecord.cpp` | EDID, FNAM, FLAG, ANAM, BNAM, CNAM, DNAM, DATA | OK |
| WBAR | `wbarrecord.cpp` | EDID | OK |
| WEAP | `weaprecord.cpp` | EDID, FNAM, FLAG, DATA, DNAM, EAMT, MDOB, ENAM | OK |
| WKMF | `wkmfrecord.cpp` | EDID | OK |
| WRLD | `worldspacerecord.cpp` | EDID, FULL, CNAM, ZNAM, XNAM, NAM2, NAM3, MNAM, NAMA, ONAM, DATA, DNAM | OK |
| WTHR | `wthrrecord.cpp` | EDID | OK |
| WTHS | `wthsrecord.cpp` | EDID | OK |
| WWED | `wwedrecord.cpp` | EDID | OK |
| ZOOM | `zoomrecord.cpp` | EDID | OK |

