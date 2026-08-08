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
| A4 | Fix red test `test_specialized_widgets` | `tests/test_specialized_widgets.cpp:72` | ✅ Done — widget creates 9 `QSpinBox` (worldspacedatawidget.cpp:33-47+); test now asserts 9 with correct ordering waterType=1, climateId=2, lightingId=3, map fields=0 (music/terrain have no spin control). Pack(3)/Loc(4) expectations still match. | test exits 0; suite 104/104 |
| A5 | Tests menu | `ui/mainwindow.ui:382-387`, `mainwindow.cpp` | ✅ Done — removed `menuTests` + disabled `actionNotImplementedTests` from the .ui (deliberate F4 candidate to re-add with real actions). | menu absent |
| A6 | Register unregistered tests | `tests/CMakeLists.txt` + 14 exes | ✅ Done — `test_configpaths` (w/ ENVIRONMENT PATH Qt DLL fix), `archivebrowser`, `assetresolver`, `bsawrite` registered unconditionally; `starfieldesm`, `pndrecord`, `worldspacerecord`, `bsaarchive`, `btdterrain`, `hknpphysicssystem`, `xwmadecoder` gated behind `if(EXISTS ...)` for real game data; `dumpesm`/`scanbtd`/`meshprobe` intentionally unregistered (non-QTest diagnostic tools). | `ctest -N` = 103 (101 OpenCK + 2 vendored ogg); gated tests skip cleanly |
| A7 | Remove dead CI configs | `.travis.yml`, `appveyor.yml` | ✅ Done — both deleted; GitHub Actions is the only pipeline. | `git ls-files` clean of both |
| A8 | CTest DLL-path fix | `test_configpaths`, `tests/CMakeLists.txt:112` | ✅ Done — `set_tests_properties(... ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>")`; "manual use only" caveat removed. | `ctest -R configpaths` green |

---

## Phase B — Test suite hygiene & honest coverage ◐

> 111 test sources, 104 built exes, 101 CTest-registered (103 incl. vendored
> ogg's 2), 0 red, 7 orphan sources never built. Make the fleet deterministic
> and truthful.

| # | Task | Where | Action | Verify |
|---|------|-------|--------|--------|
| B1 | Resurrect valuable orphans | `tests/test_groundtruth.cpp`, `test_subrecord_roundtrip.cpp` | **test_groundtruth** (4CC cross-check vs Starfield scan — extend to assert all 180 resolve, then fold into `test_wiring`); **test_subrecord_roundtrip** — build + register. | both in `git ls-files` + CTest |
| B2 | Delete or rebuild the rest | `test_dataexporter.cpp`, `test_loader.cpp`, `test_recordloading.cpp`, `test_undo.cpp`, `test_stubs.cpp` (shim) | Decide per file: rebuild if it tests live code, else remove. `test_stubs` remains a linkage shim (keep, but rename/mark intentional). | no orphan sources in `tests/` |
| B3 | QSKIP audit | `test_archivebrowser`, `test_assetresolver`, `test_ba2dx10`, `test_bsawrite`, `test_gitrepository`, `test_perforcerepository` | Standardize skip messages; ensure skips exit 0 (Qt does by default) and log reason; where a real game install exists locally, run and capture results. | all skip-path tests exit 0 |
| B4 | Deterministic log-file tests | `tests/CMakeLists.txt` | Route per-test log files (`openck_*.log`) to a temp dir via env var or QDir::tempPath; stop tests writing logs next to exe. | no stray logs in `build/bin` after runs |
| B5 | Debug/Release parity check | CI + `build.ps1` | Run the gate loop in Release too (CI does Release); capture `test_specialized_widgets` (A4) and any others that differ per config. | Debug + Release both green |

---

## Phase C — Documentation truth ◐

> Docs claimed "310/310" and "26 tests" while the real suite was 104 exes / 90
> CTest / 1 red. Counts updated through Phase A; remaining stale rows tracked
> below.

| # | Task | Where | Action | Verify |
|---|------|-------|--------|--------|
| C1 | Refresh `STATUS.md` | `docs/STATUS.md:17,102` | ✅ Done — counts corrected to 104 exes / 101 CTest (+2 vendored) / 0 red; date 2026-08-07. | numbers match repo |
| C2 | Correct `UNIFIED_PLAN.md` tracker | `docs/UNIFIED_PLAN.md` tracker + phases 15-24 | ◐ — "310/310 (audited)" retained with caveat note; partials folded into Phase E refs. Remaining: 16.x/17.x/23.4/24.x cells explicitly marked. | tracker no longer overclaims |
| C3 | Fix `ROADMAP.md` S-rows | `docs/ROADMAP.md:19-29` | ✅ Done — S1-S6/M1-M7 rewritten as complete; next candidates → Phase F. | roadmap reflects reality |
| C4 | Update `TECHNICAL_DEBT.md` | H1 note, M rows, L rows | ✅ Done — H1/M9 link to Phase E; H2 marked resolved; date 2026-08-07. | each open row points at this doc |
| C5 | Commit `docs/BUILD.md` | (untracked today) | ⬜ After A1 it's no longer ignored; refresh to use `cmake -G "Visual Studio 17 2022"` and the full test loop incl. Release. | matches CI steps |

---

## Phase D — Kill user-visible stubs & "Not implemented" ◐

| # | Task | Where | Action | Verify |
|---|------|-------|--------|--------|
| D1 | Preferences Network page | `preferencesdialog.cpp:345-353` | ✅ Done — removed the disabled "Not implemented" VC-server field; note now says VC is configured via Tools menu/external tools. | no disabled "Not implemented" text |
| D2 | InfoData "Record" button | `infodatawidget.cpp:151` | ✅ Done — disabled with truthful tooltip "Voice recording is not yet implemented" (no playback backend exists). | wired or honestly labeled |
| D3 | Cell View filter | `cellsdialog.cpp:563` | ✅ Done — real filter implemented: `RefrTableModel` keeps `mAllRows`/`mAllPoints`, `setFilter()` matches EditorID or Form ID (case-insensitive); QLineEdit in toolbar, Filter action focuses it. | filter works |
| D4 | Search "not yet supported" | `searchdialog.cpp:1268` | ⬜ — narrow message to truly-uneditable (GMST) only after D5. | message only for GMST |
| D5 | Export templates gap | `exporttemplatesdialog.cpp:1015` | ⬜ — extend export `fieldsForType`/templates to remaining types; 22 covered done (R19). | no "not yet supported" for wired types |
| D6 | Disabled menu actions | `ui/mainwindow.ui` (PreviewWindow, Lighting, NpcEditor, RaceEditor, ClassEditor, FactionEditor, SaveLayout, LoadLayout) | ⬜ — each action deliberately kept disabled; wire periodically. | all actions reachable or honestly disabled |
| D7 | `collection.hpp` note | `src/model/world/collection.hpp:52` | ✅ Done — copy ctor/assign now `= delete`; "Not implemented" note removed. | note gone |
| D8 | Header comment discipline | `rawsubrecordwidget.hpp:11-14`, tier2_components.hpp:320 | ⬜ — update comments that say "not yet implemented" to point at this plan (D4/E3) so they stay honest. | grep for "Not implemented" → only D2/D4/D5 refs |
| D9 | `cellwithtransitionseditor.cpp:218-240` | M3 | ⬜ — permanent for Skyrim (no TES4 data) — add truthful disabled state + doc note. | documented, no stub text |

---

## Phase E — Technical debt register payoff ◐

| # | Item | Action | Verify |
|---|------|--------|--------|
| E1 | **H1** FormIdCompactor opaque refs | Extend raw-subrecord FormID rewriting to XPRM + Starfield payloads (`formidcompactor.hpp`), add tests. | `test_esl` extended |
| E2 | **M5** QtFormDialog tabs | Add Basic/Components/Keywords/Ingest tabs (Phase 12F partial). | dialog has full tab set |
| E3 | **M9** hknp encoder | Now that Starfield mesh BA2 v2 is readable (`Ba2Archive`), implement hknp polygon encode (`HknpPhysicsSystem`). | round-trip test |
| E4 | **M8** SCEN PHDA binary | Real `ScenRecord` PHDA encode; validate vs real data. | round-trip test |
| E5 | **M7** NavMesh auto-gen voxel tuning | Tune voxel filter against real world geometry; add acceptance signal to test. | `test_navmeshtoolkit` extended |
| E6 | **M6** flat-field mirrors | Keep (audited); document as intentional — no code change. | debt row notes "intentional" |
| E7 | **L1** stale `.bak` files | Remove `src/view/window/armor_editor.cpp.bak`, `objectwindowdialog.cpp.bak`. | `git rm` done |
| E8 | **L2** `NewFindings.md` | Delete or fold into `docs/`; it references navmesheditor as a shell (false). | gone or corrected |
| E9 | **L3** xWMA WMF fallback | Keep ffmpeg primary; mark WMF path as debug-only; optional removal. | documented |
| E10 | Orphan sources cleanup | `bookrecord_new.cpp/_old.cpp`, `dataimporter_new.cpp`, legacy `dataimporter.cpp`/`dataexporter.cpp`, `src/debug_brace.py`, `fix_imp.py`, `fix_npc.py`, `ui/mainwindow.ui.new_section`, `src/view/window/test_new.txt` | Delete or archive all; confirm nothing references them. | `git grep` no refs |

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
| G1 | Full CI run after A-E | Push to origin; watch Actions (Release build + CTest). | workflow green end-to-end |
| G2 | Release packaging check | `cpack` locally + installer artifact from CI (24.4 claims done but NSIS only builds in CI). | installer builds |
| G3 | Living tracker | Keep `docs/STATUS.md` + this plan's checkboxes updated at each phase end. | docs match repo |
| G4 | Worktree cleanup | Remove `wt-a/wt-b/wt-c` worktrees (all merged), delete `types/batch-*` branches, drop `remotes/worktrees/*` refs. | `git worktree list` = 1 |
| G5 | Tag + release | `git tag v0.x.y`; GitHub release with installer artifact. | release published |

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
