# OpenCK — Remaining Work Plan

> Living plan for closing out the *actual* remaining gaps identified in the
> 2026-08-07 deep-research sweep (see `docs/TECHNICAL_DEBT.md`, `docs/ROADMAP.md`).
> This is the work list; it supersedes the earlier "310/310 complete" claim
> in `docs/UNIFIED_PLAN.md` until that tracker is corrected (Phase C).
> Tracker audited 2026-08-07 — see the "(audited)" note added there.
>
> Status key: ✅ done · ◐ partial · ⬜ not started
>
> **Ground rule — every phase ends with a clean build and a green gate set:**
> ```powershell
> cmake --build build --config Debug
> $env:Path = "C:\Qt\6.5.3\msvc2019_64\bin;C:\Users\max\Projects\OpenCK\openck\build\bin\Debug;" + $env:Path
> Get-ChildItem build\bin\Debug\test_*.exe | ForEach-Object { & $_.FullName; "exit=$LASTEXITCODE $($_.Name)" }
> ```
> CI parity: `ctest --test-dir build -C Debug --output-on-failure` must stay green.

---

## Phase A — CI/CD: make the correctness gate real ✅

> **Finding:** the GitHub Actions workflow `.github/workflows/windows-build.yml`
> was *not in version control* — `.gitignore:29` (`*build*`) ignored it (and
> `docs/BUILD.md`) because of "build" in the filename. `all_tests` depended on
> two targets (`test_bitwise`, `test_framing`) that don't exist, so the CI test
> step could never have run. There was no enforced green gate today.
> **Resolution:** workflow fixed (Qt path via `QT_ROOT_DIR`, correct `bin/`
> artifact paths, `all_tests` target) and committed; `.gitignore` precise;
> dead CI configs removed. Gate verified: `104/104` exes exit 0, `ctest`
> `103/103` green.

| # | Task | Where | Action | Verify |
|---|------|-------|--------|--------|
| A1 | Fix over-broad `.gitignore` | `.gitignore:29` | ✅ Done — `*build*` replaced with `/build/`, `/build_ninja/`, `build.ps1`, `*.vcxproj`, `*.vcxproj.filters`. `git status` shows `.github/workflows/` + `docs/BUILD.md` as untracked candidates. | verified via `git status` |
| A2 | Commit CI workflow | `.github/workflows/windows-build.yml` | ✅ Done — workflow committed with three blockers fixed (Qt path now `$env:QT_ROOT_DIR` from install-qt-action; artifact paths `build/bin/Release/`; `all_tests` target). | file in `git ls-files`; pushed to origin |
| A3 | Repair `all_tests` | `tests/CMakeLists.txt:689-701` | ✅ Done — dangling `test_bitwise`/`test_framing` deps removed; DEPENDS list now exactly the 104 real exes (verified by scripted compare). | `cmake --build build --config Debug --target all_tests` succeeds |
| A4 | Fix red test `test_specialized_widgets` | `tests/test_specialized_widgets.cpp:72` | ✅ Done — widget creates 9 `QSpinBox` (worldspacedatawidget.cpp:33-47+); test now asserts 9 with correct ordering waterType=1, climateId=2, lightingId=3, map fields=0 (music/terrain have no spin control). Pack(3)/Loc(4) expectations still match. | test exits 0; suite 107/107 |
| A5 | Tests menu | `ui/mainwindow.ui:382-387`, `mainwindow.cpp` | ✅ Done — removed `menuTests` + disabled `actionNotImplementedTests` from the .ui (deliberate F4 candidate to re-add with real actions). | menu absent |
| A6 | Register unregistered tests | `tests/CMakeLists.txt` + 14 exes | ✅ Done — `test_configpaths` (w/ ENVIRONMENT PATH Qt DLL fix), `archivebrowser`, `assetresolver`, `bsawrite` registered unconditionally; `starfieldesm`, `pndrecord`, `worldspacerecord`, `bsaarchive`, `btdterrain`, `hknpphysicssystem`, `xwmadecoder` gated behind `if(EXISTS ...)` for real game data; `dumpesm`/`scanbtd`/`meshprobe` intentionally unregistered (non-QTest diagnostic tools). | `ctest -N` = 106 (104 OpenCK + 2 vendored ogg); gated tests skip cleanly |
| A7 | Remove dead CI configs | `.travis.yml`, `appveyor.yml` | ✅ Done — both deleted; GitHub Actions is the only pipeline. | `git ls-files` clean of both |
| A8 | CTest DLL-path fix | `test_configpaths`, `tests/CMakeLists.txt:112` | ✅ Done — `set_tests_properties(... ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>")`; "manual use only" caveat removed. | `ctest -R configpaths` green |

---

## Phase B — Test suite hygiene & honest coverage ✅

> 108 test sources, 107 built exes, 104 CTest-registered (106 incl. vendored
> ogg's 2), 0 red, 0 orphan sources. Fleet deterministic — all green as of 2026-08-08 (107/107 exes, 106/106 CTest).
> and truthful.

| # | Task | Where | Action | Verify |
|---|------|-------|--------|--------|
| B1 | Resurrect valuable orphans | `tests/test_groundtruth.cpp`, `test_subrecord_roundtrip.cpp` | ✅ Done — **test_subrecord_roundtrip** rebuilt + registered (105-level subrecord round-trip coverage); **test_groundtruth** rebuilt with a flat compressed-record-safe scan (`skipRecord()` restores the stream after decompression) and registered; passes vs the real Starfield.esm. | both built + green; 107/107 gate |
| B2 | Delete or rebuild the rest | `test_dataexporter.cpp`, `test_loader.cpp`, `test_recordloading.cpp`, `test_undo.cpp`, `test_stubs.cpp` (shim) | ✅ Done — `test_loader` rebuilt (QTimer moved off the loader thread — see commits 25a5b83/3818daa) + registered w/ `OPENCK_LOG_DIR`; `test_dataexporter`/`test_recordloading`/`test_undo` deleted (superseded); `test_stubs` kept as intentional static-lib shim. | 0 orphan sources in `tests/` |
| B3 | QSKIP audit | `test_archivebrowser`, `test_assetresolver`, `test_ba2dx10`, `test_bsawrite`, `test_gitrepository`, `test_perforcerepository` | ✅ Done — all QSKIPs use consistent "… not found / not on PATH" wording; skips exit 0 (QTest default); local game data present so the game-data tests actually ran and passed in the 107/107 gate; `test_archivebrowser` got `QTEST_FUNCTION_TIMEOUT=1800000` (75k-entry BSA filter legitimately exceeds the 5-min watchdog in Debug). | all skip-path tests exit 0 |
| B4 | Deterministic log-file tests | `tests/CMakeLists.txt` | ✅ Done — `openck_add_test` sets `OPENCK_LOG_DIR=${CMAKE_BINARY_DIR}/test-logs` (logger.hpp + main.cpp read it), so all registered tests write logs out-of-tree. | no stray logs in `build/bin` after runs |
| B5 | Debug/Release parity check | CI + `build.ps1` | ✅ Done — Debug gate 107/107 locally; Release gate green on CI (100%, 97/97 — the 10 game-data-gated tests aren't registered there; local release spot-checks of the same tests pass). Fixed along the way: ads DLL not deployed next to binaries (0xc0000135), ogg test_bitwise/framing not built into all_tests, test_compressedrecord hardcoded local paths. | Debug + Release both green |

---

## Phase C — Documentation truth ◐

> Docs claimed "310/310" and "26 tests" while the real suite was 104 exes / 90
> CTest / 1 red. Counts updated through Phase A; remaining stale rows tracked
> below.

| # | Task | Where | Action | Verify |
|---|------|-------|--------|--------|
| C1 | Refresh `STATUS.md` | `docs/STATUS.md:17,102` | ✅ Done — counts corrected to 107 exes / 104 CTest (+2 vendored) / 0 red / 0 orphans; date 2026-08-08. | numbers match repo |
| C2 | Correct `UNIFIED_PLAN.md` tracker | `docs/UNIFIED_PLAN.md` tracker + phases 15-24 | ✅ Done — "310/310 (audited)" retained with caveat note (2026-08-08 audit block); all partial cells carry ◐ + Phase E pointer (16.1/16.2/16.3/16.5/16.8, 17.6, 23.4); summary table marks 16/17/23 as ◐; 24.x cells verified against code (24.7 layout save/load wired, 24.10 Galaxy/Packin menus gone — menubar re-surveyed 2026-08-10). STATUS.md menu-belt claim corrected. | tracker no longer overclaims |
| C3 | Fix `ROADMAP.md` S-rows | `docs/ROADMAP.md:19-29` | ✅ Done — S1-S6/M1-M7 rewritten as complete; next candidates → Phase F. | roadmap reflects reality |
| C4 | Update `TECHNICAL_DEBT.md` | H1 note, M rows, L rows | ✅ Done — H1/M9 link to Phase E; H2 marked resolved; date 2026-08-07. | each open row points at this doc |
| C5 | Commit `docs/BUILD.md` | (was untracked) | ✅ Done — refreshed for the VS17 generator form, ads DLL copy step, `OPENCK_LOG_DIR`, Release gate + CI details (windows-2022 pin, qt3d module), NSIS local build caveat; committed. | matches CI steps |

---

## Phase D — Kill user-visible stubs & "Not implemented" ◐

| # | Task | Where | Action | Verify |
|---|------|-------|--------|--------|
| D1 | Preferences Network page | `preferencesdialog.cpp:345-353` | ✅ Done — removed the disabled "Not implemented" VC-server field; note now says VC is configured via Tools menu/external tools. | no disabled "Not implemented" text |
| D2 | InfoData "Record" button | `infodatawidget.cpp:151` | ✅ Done — disabled with truthful tooltip "Voice recording is not yet implemented" (no playback backend exists). | wired or honestly labeled |
| D3 | Cell View filter | `cellsdialog.cpp:563` | ✅ Done — real filter implemented: `RefrTableModel` keeps `mAllRows`/`mAllPoints`, `setFilter()` matches EditorID or Form ID (case-insensitive); QLineEdit in toolbar, Filter action focuses it. | filter works |
| D4 | Search "not yet supported" | `searchdialog.cpp:1268` | ✅ Done — guard narrowed to truly-uneditable kinds (GMST + internal log pseudo-types); message reworded honestly ("no form editor", grid for GMST) — no "yet" implying future support. | message only for GMST |
| D5 | Export templates gap | `exporttemplatesdialog.cpp:1015` | ✅ Done (2026-08-10) — 8 more record types wired end-to-end (fieldsForType + recordFieldValue overload + collection dispatch + editor combo): KEYM, AMMO, LIGH, FURN, TREE, SLGM, DOOR, KYWD. 30 types total (was 22). "not defined yet" message now reachable only via hand-authored JSON templates with exotic types. | no "not yet supported" for wired types |
| D6 | Disabled menu actions | `ui/mainwindow.ui` (PreviewWindow, Lighting, NpcEditor, RaceEditor, ClassEditor, FactionEditor, SaveLayout, LoadLayout) | ✅ Done (2026-08-10) — SaveLayout/LoadLayout were already wired (24.7). Now wired: Preview Window opens a NIF file in the render window; NPC/Race/Class/Faction editors create a new record (input Editor ID, assigned FormID, undo-safe `addNpc/addRace/addClass/addFact`) and open it in the QtFormDialog (`createAndOpenRecord` in mainwindow.cpp). Lighting stays honestly disabled ("not available" tooltip). | all actions reachable or honestly disabled |
| D7 | `collection.hpp` note | `src/model/world/collection.hpp:52` | ✅ Done — copy ctor/assign now `= delete`; "Not implemented" note removed. | note gone |
| D8 | Header comment discipline | `rawsubrecordwidget.hpp:11-14`, tier2_components.hpp:320 | ✅ Done — audit on 2026-08-10: only remaining "not yet implemented" text is the honest D2 tooltip in `infodatawidget.cpp:151`; everything else updated. | grep for "Not implemented" → only D2 refs |
| D9 | `celltransitionseditor.cpp:87-89,214-234` | M3 | ✅ Done — Add/Edit/Delete permanently disabled with truthful comments (plugin format stores no cell-connection data) + plan refs. | documented, no stub text |

---

## Phase E — Technical debt register payoff ◐

| # | Item | Action | Verify |
|---|------|--------|--------|
| E1 | **H1** FormIdCompactor opaque refs | Extend raw-subrecord FormID rewriting to XPRM + Starfield payloads (`formidcompactor.hpp`), add tests. | `test_esl` extended |
| E2 | **M5** QtFormDialog tabs | ✅ Done (2026-08-10) — Basic (universal: TESFullName/TESModel/TESTexture/TESHealth/TESValue/TESWeight/TESDescription) / Components (record-specific) / Keywords (only when BGSKeywordForm present) / Data tabs; `test_qtformdialog` extended (T1b). | dialog has full tab set |
| E3 | **M9** hknp encoder | Now that Starfield mesh BA2 v2 is readable (`Ba2Archive`), implement hknp polygon encode (`HknpPhysicsSystem`). | round-trip test |
| E4 | **M8** SCEN PHDA binary | ⬜ — real-data audit 2026-08-10: Skyrim.esm (1706 SCEN records) and Starfield.esm (7613 SCEN records) contain **zero** PHDA subrecords — both shipped games use the new-generation scene schema (FNAM/VNAM/ALID/LNAM/DNAM/ANAM groups). PHDA is the classic-Skyrim-mod / FO4 format; validating an encoder needs a FO4 install or a sample mod. Current raw-subrecord round-trip is byte-exact, so keep it until a sample is available (risk-register item). The `test_dumpesm` diagnostic gained a path + dump-count parameter for future audits. | round-trip test vs real data |
| E5 | **M7** NavMesh auto-gen voxel tuning | Tune voxel filter against real world geometry; add acceptance signal to test. | `test_navmeshtoolkit` extended |
| E6 | **M6** flat-field mirrors | ✅ Done (audit 2026-08-10) — documented as intentional; TECHNICAL_DEBT.md R11/5E.4 record "most fields intentionally kept". | debt row notes "intentional" |
| E7 | **L1** stale `.bak` files | ✅ Done — no `*.bak` remains in the tree (verified 2026-08-10). | `git rm` done |
| E8 | **L2** `NewFindings.md` | ✅ Done — deleted in the Phase B/C/D/E hygiene commit (06bc824); nothing references it. | gone or corrected |
| E9 | **L3** xWMA WMF fallback | ✅ Done (2026-08-10) — ffmpeg stays primary; WMF MFT path explicitly commented "debug-only diagnostics" with TECHNICAL_DEBT.md L3 pointer. | documented |
| E10 | Orphan sources cleanup | ✅ Done — all listed files deleted in 06bc824; re-verified 2026-08-10 (no orphan sources remain). | `git grep` no refs |

---

## Phase F — Feature surface: next release candidates ◐ (pick 1–2)

| # | Candidate | Where | Scope | Priority |
|---|-----------|-------|-------|----------|
| F1 | Render preview polish | `nifviewportwidget` | Mesh picker, pivot display, floor grid, camera presets — high demo value, near-term. | High |
| F2 | Layout save/load UI | `windowlayout.cpp` | Wire `actionSaveLayout`/`actionLoadLayout` (file dialog already exists). One-session item. | High (cheap) |
| F3 | Specialized editor wizards | `src/view/window/*datawidget*` | CREA (exists), RACE, CLAS, FACT, WTHR, HAZARD, REGN widgets as `QtFormDialog` factories. | Medium |
| F4 | `Tests` menu (A5) + headless self-test | `mainwindow.cpp`, CLI | "Run All Tests" invoking the headless CLI; test log viewer. | Medium |
| F5 | Behavior graph editor breadth | `NodeGraphWidget` | More node types + saved graphs. | Low |

---

## Phase G — Closing & release gate ◐

| # | Task | Action | Verify |
|---|------|--------|--------|
| G1 | Full CI run after A-E | ✅ Done — run 31363153316 (master @ 3f6448e) green end-to-end: configure + Release openck + all_tests + 100% ctest + installer. | workflow green end-to-end |
| G2 | Release packaging check | ✅ Done — CI run 31363153316 built and uploaded the NSIS installer (`openck-installer` artifact). Local cpack not possible (NSIS not installed on this machine — documented in BUILD.md). | installer builds |
| G3 | Living tracker | ✅ Done — `docs/STATUS.md` refreshed (2026-08-10): Release CI green (97/97), G1/G2/G4 recorded here. | docs match repo |
| G4 | Worktree cleanup | ✅ Done — `wt-a/wt-b/wt-c` removed (all fully merged, verified via merge-base), `types/batch-*` branches deleted, `refs/remotes/worktrees/*` dropped. | `git worktree list` = 1 |
| G5 | Tag + release | ⬜ — `git tag v0.x.y`; GitHub release with installer artifact. | release published |

---

## Risk register

| Risk | Impact | Mitigation |
|------|--------|-----------|
| CI never ran — workflow may have latent env bugs | False green | After A1-A3, do a trial Actions run on a branch before master |
| Real-game-data tests depend on personal installs (C:/XboxGames, Skyrim SE) | Unregistered tests never exercised | Keep `OPENCK_TEST_STARFIELD_ESM`/`OPENCK_TEST_BA2_DIR` gating; document in CI README |
| Overscoping Phase F | Release slips | Pick 1–2 candidates only; rest stay in roadmap |
| Doc drift returns | Misleading tracker | G3 (living tracker) enforced each phase |
| FormIdCompactor opaque payloads (H1/E1) may be intractable without samples | Partial fix | Document + ship; keep `test_esl` green |

---

*Started 2026-08-07. Every phase ends with a green build + green gate set (see Ground rule).*
