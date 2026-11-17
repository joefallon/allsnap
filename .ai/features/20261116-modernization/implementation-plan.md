# allSnap modernization implementation plan

Status: Phase 16 sets release 2.0.0 and completes final automated release checks, retaining the pending x64 icon update. All 311 tests pass in Debug/Release on Win32/x64 (99 unit, 212 acceptance); all eight builds and actual binary metadata/icon/manifest inspections pass. Independent review is complete. Manual global-hook/Explorer, keyboard/help rendering, hardware/DPI, and long-duration checks remain pending; no multi-year reliability claim is established. Keep this living document and `.ai/current-state.md` aligned with actual progress; requirements are in `project-spec.md`.

## Current position

- Completed: source inspection of snapping, confinement, settings/candidates, hook/subclass lifecycle, startup, and sound handling.
- Completed: verified local availability of Microsoft's native C++ testing headers, x86/x64 libraries, and VSTest runner. Approved as the common unit/acceptance framework; no dependencies installed.
- Confirmed with user: root `features/*.feature` files are documentation only; no Gherkin tooling.
- Completed in Phase 7: all app sound functionality removed; historical compiled help is explicitly retained as an archive.
- Completed in Phase 8: root README covering build/run commands, settings, dual-architecture operation, tests, source navigation, and known limitations.
- Completed in Phase 9: folder organization with project/include/filter, installer-input, help-output, and documentation references updated; behavior and project naming preserved.
- Completed in Phase 10: root solution/application project/filter filenames align with the existing `allsnap` project name; references updated without renaming runtime binaries or source symbols.
- Completed in Phase 11: INI save serialization and traversal have separate responsibilities; goto-based save exits replaced by direct returns, with existing behavior preserved by review.
- Completed in Phase 12: INI load dispatch and traversal have separate responsibilities; unused load bookkeeping and the registry-named goto removed, with existing behavior preserved by review.
- Completed in Phase 13: INI numeric parsing extracted from profile reading without changing grammar, bounds, errno behavior, or output assignment.
- Completed in Phase 14: audited low-risk app/engine cleanup inventory, independently cross-reviewed; remaining behavior-sensitive and larger refactoring work deferred explicitly.
- Completed in Phase 15: deferred Settings help/menu/native callback and tray packet/resource/failure repairs with 19 new acceptance regressions and executed full test matrix.
- Completed in Phase 16: 2.0.0 release identity, six version regressions, fresh isolated builds/tests, and inspection of actual EXE/DLL resources for both architectures.
- Native unit/acceptance tests now cover 311 methods (99 unit, 212 acceptance). All passed in Debug and Release on Win32 and x64; the acceptance suite includes 12 isolated-process methods using the same Microsoft framework.
- User handles all commits. Agents do not stage, commit, reset, or amend. Phase boundaries below are review checkpoints, not claims of commits already created.

## Milestones and human commit checkpoints

| Phase | Reviewable scope | Current verification |
| --- | --- | --- |
| 1. Test foundation | Microsoft framework, runner, documentation, production-C fixture seams, initial confinement/snapping contracts | Implemented. Current suite: 311/311 in all four architecture/configuration runs. Later contract review corrected inherited assumptions rather than freezing legacy outputs. |
| 2. Lifecycle reliability | Hook/subclass ownership and failures, safe shutdown bookkeeping, diagnostics | Initial failure-handling checkpoint passed 23 focused tests. Phase 5 supersedes scalar accounting and simulated ownership with a process-aware registry and actual child-process coverage. |
| 3. Monitor/grid and Settings repairs | Workarea/guide separation, validation, Old position, Settings persistence and failure reporting, held-key semantics | Implemented. Geometry, Old position, input, validation reentrancy, Apply acknowledgement/retry, and opening-state regressions demonstrated red before repair; final suite green on both architectures. Remaining Settings paths require desktop validation. |
| 4. Visible frames, spacing, and desktop geometry | DWM/region/manual crop composition, collapsed spacing UI/INI, ordinary/HiDPI coordinates, monitor union confinement | Implemented. Independent GDI coverage oracle plus native framed ordinary/Ctrl resize cases are green in the final matrix. Real mixed-DPI transitions and home-monitor checks remain pending. |
| 5. Isolated process lifetime | Actual child-process termination/detach, concurrent owners, shutdown reservations, callback/module retention | Implemented and independently reviewed. Three product failures demonstrated before repair; 12 isolated-process methods and four new unit tests pass within the 222-test matrix. Global-hook/desktop validation remains separate. |
| 6. Settings behavior audit | Non-sound controls, actual property sheet, tray boundary, persisted validation, MDI and class filtering | Implemented and independently reviewed. Added 55 methods; final 277-test matrix and all eight builds pass. Actual Shell/global-hook delivery and manual UX/hardware checks remain separate. |
| 7. Sound removal | Audio UI/preview, worker/messages/settings/API, audio assets/help/packaging and exclusive dependencies | Implemented and independently reviewed. Two native UI absence regressions failed before removal; final 286-test matrix, eight builds, and actual Release import/export checks pass. |
| 8. Repository README | Build/run/configuration/test instructions, source navigation, and known limitations | Complete and independently reviewed; commands, claims, and local links checked against source. No builds or tests rerun. |
| 9. Folder organization | Separate app, engine, common code, resources, help, packaging, and legacy material | Complete and independently reviewed. Moved C/headers/resources unchanged; all eight solution builds pass, project/local-link checks pass; no tests executed. |
| 10. Solution/project naming | `allsnap.sln`, `allsnap.vcxproj`, matching filters and active references | Complete and independently reviewed; all eight solution builds, reference/metadata checks, and whitespace checks pass. No tests executed. |
| 11. Clean Code: INI save path | Separate per-setting serialization from ordered save traversal in one production file | Complete and independently reviewed; all eight builds and whitespace checks pass. Existing tests inspected, not executed; runtime regression verification remains outstanding. |
| 12. Clean Code: INI load path | Separate per-setting loading from ordered traversal in the same production file | Complete and independently reviewed; all eight builds and whitespace checks pass. Existing tests inspected, not executed; runtime regression verification remains outstanding. |
| 13. Clean Code: INI numeric parsing | Separate text conversion from the existing profile read/length guard | Complete and independently reviewed; all eight builds and whitespace checks pass. Existing tests inspected, not executed. |
| 14. Clean Code: remaining low-risk inventory | Unused private state/declarations/locals, obsolete commented code, equivalent guards/returns, shared page setup | Complete and cross-reviewed; all eight builds and whitespace checks pass. Tests not executed. |
| 15. Deferred Settings repairs | Help dispatch, menu ownership, property-sheet native contracts, tray packets/resources/failure cleanup | Complete and cross-reviewed. New regression failures reproduced before repair; 305/305 in all four Debug/Release architecture runs and all eight builds pass. |
| 16. Version 2.0.0 release checks | Readable startup identity, shared native metadata/About text, retained x64 icon, final fresh binaries | Complete and reviewed; 311/311 across Debug/Release Win32/x64, eight builds, actual binary metadata/manifest/icon checks pass. Manual desktop validation remains separate. |
| Later backlog phases | Deeper Clean Code refactoring and remaining behavior repairs | Settings/tray findings addressed in Phase 15; other deferred candidates remain separate. |
## Execution sequence

### 1. Establish the test foundation and protect confinement

Status: complete. Debug baseline: 29/29 tests passed on Win32 and x64 (12 unit, 17 acceptance). Separate x64 Unit and Acceptance filtering also passed. Tests exercise real production C matching, confinement, and move/resize result application without global hooks or user INI writes.

- Add a native test project in the existing solution, with `tests/unit/`, `tests/acceptance/`, and only necessary shared fixtures. Use the same Microsoft C++ framework and VSTest runner for both suites.
- Compile reused production C modules as C; use C++ only for the test harness. Link real production behavior and avoid including application/DLL entry-point objects in the test harness.
- Add the smallest test seams needed for geometry/settings and deterministic Win32 inputs. Use temporary files and test-owned window handles; prevent default tests from installing global hooks.
- Write `features/desktop-confinement.feature` and `features/window-snapping.feature` as plain behavior documentation with stable scenario IDs, then map initial acceptance tests to those IDs.
- Characterize confinement and add focused geometry unit tests. Include work-area edges, taskbar reservations, negative coordinates, multi-monitor transitions, oversized windows, move versus resize, no candidates, toggles, and threshold boundaries. Do not enshrine an observed defect as intended behavior.
- Document reproducible build/run commands and filtering for Unit versus Acceptance, with explicit x86/x64 execution. Provide one entry point for both suites without downloading tools implicitly.

Verification: discover and run meaningful unit and acceptance tests on both architectures; ensure tests use production code and leave user state untouched. Build the application projects after introducing seams. Record actual commands/results here.

### 2. Repair lifecycle failure handling and establish diagnostics

Status: initial repairs implemented and independently reviewed. The failed-subclass regression was reproduced before repair (12 passed, 1 failed in the Win32 Unit suite). Phase 5 adds actual separate-process termination and native callback/module retention coverage; actual global-hook delivery and desktop use remain unverified.

- Document `features/hook-lifecycle.feature` scenarios and add regressions for ignored hook startup failure and failed subclass registration.
- Add coverage for shared versus process-local ownership, unrelated process detach, repeated cleanup, and shutdown following partial initialization. Use controlled failure injection for deterministic failure paths; add isolated process checks where ownership requires them.
- Fix state transitions and startup unwinding locally in `src/engine/snap_lib.c`/`src/app/snap_App.c`; preserve the existing hook API and DLL boundaries. Do not solve a shared-ownership defect by simply making every shared flag process-local.
- Add small, actionable diagnostics for startup/hook/subclass failures and clean versus abnormal shutdown. Capture errors at the failing API. Avoid synchronous disk logging or blocking IPC in hot hook callbacks. Do not introduce a watchdog or claim the reported intermittent stop is diagnosed.

Verification: demonstrate each targeted regression before/after its repair, run both suites for x86/x64, and check normal startup/shutdown in a controlled environment. Report crash reproduction separately from successful state-transition tests.

### 3. Repair monitor/grid accounting and validate settings

Status: initial repairs implemented and independently reviewed, including property-sheet Apply acknowledgement/retry and preserving runtime enabled state when opening Settings. Final automated matrix passed; remaining actual-desktop Settings coverage is listed below.

- Write `features/monitor-and-grid-settings.feature` scenarios for multiple monitors with many grid lines and malformed persisted settings.
- Separate real work-area enumeration/accounting from artificial snap-guide candidates. Ensure enabling a grid cannot hide later monitors or turn a guide line into a confinement work area.
- Validate grid modes and numeric values before indexing UI arrays or dividing; preserve valid legacy keys and preferences. Set deterministic limits that match actual array capacities. Add unit tests for those boundaries.
- Keep changes to `src/engine/snap_WinRects.c`, grid/settings boundaries, and their immediate callers. Reuse the existing rectangle result structures wherever suitable.

Verification: acceptance scenarios retain all work areas with grids enabled; invalid input is handled safely; baseline confinement and edge-snapping tests pass for x86/x64.

### 4. Diagnose visual gaps and remaining snapping failures

Status: initial repairs implemented and independently reviewed. Configurable spacing, visible-frame conversion, full-rectangle union placement, and owned-window ordinary/Ctrl resizing tests pass the final automated matrix. Hardware checks remain pending.

- Implement the approved single configurable pixel-spacing value (0–1000, default 0) for window/window and work-area edges. Apply the value once between adjacent windows. Keep activation distance separate, and do not pad center/grid guides. Reproduce legacy gap sources before changing frame geometry.
- Inspect DWM visible frame bounds, existing crop adjustments, and DPI coordinate conversions together. Do not change only the threshold or toggle manifest DPI awareness without matching the calculation path.
- Extend documented snapping scenarios and acceptance fixtures for window/window and window/work-area edges, resizing, negative coordinates, scaling, and repeated gestures. Repair only failures demonstrated by those examples.
- Explicitly check candidate ordering and centering paths with tests before changing them; suspicious source patterns alone do not establish the correct repair.

Verification: unit/acceptance results on both architectures plus native test-owned framed windows. Record the remaining interactive matrix for representative windows/scaling separately; native tests do not complete it. Keep confinement results separate and visible. The user explicitly permits Windows compatibility scaling for the UI; no UI DPI-awareness rewrite is required.

## Backlog

The canonical list is [`.ai/backlog.md`](../../backlog.md), with stable item IDs and separate requested/proposed work. Next priorities are B-001 (actual desktop/process-lifetime verification) and B-002 (remaining Settings coverage). Audio removal, the README, folder organization, naming cleanup, and the INI save/load/parser Clean Code increments are complete. B-007 remains open for further bounded refactoring milestones. A single launcher/tray is proposed and would still manage both helper architectures.

## Phase 5 execution

The user authorized isolated process-lifetime tests and repairs after committing the initial checkpoint. No manual desktop testing has been performed. The implemented harness reuses production C lifecycle code and actual DllMain in a dedicated DLL loaded by separate, test-owned hidden-window processes; only global hook installation/removal is substituted. Private per-scenario DLL copies isolate shared PE data from the component test DLL. Named-pipe/event handshakes have deadlines, and owned kill-on-close jobs cover cleanup and constructor failure. Failed I/O cancellation has a bounded drain; only an unconfirmed failed operation retains its small heap context until test-host exit to avoid freeing pending kernel I/O storage.

First actual-process regression run (Debug x64): 8 methods executed, 5 passed and 3 failed. Forced target termination stranded shared shutdown accounting; releasing an ordinary LoadLibrary reference unloaded the DLL with a native subclass installed, including while its callback was executing through the owned window procedure. Normal process detach, unrelated exit, two live owners, reserved native installation, and post-stop admission passed. These product failures drove the repairs below; an earlier helper startup issue was corrected separately and was not counted as product failure evidence.

Repairs: replace the shared scalar with 64 atomic generation-tagged ownership records carrying full process/thread creation identities; reap only confirmed dead/replaced owners, reconcile abandoned local admission, retain live/unknown ownership, and preserve records across host restart. Registration reserves before native installation and rechecks the stopping gate; detach performs only local writes and exact-token atomic release. Pin engine code at host/hook/subclass entry with GetModuleHandleEx(PIN | FROM_ADDRESS), so callback return paths remain mapped until process exit. Normal component tests substitute pinning; isolated process fixtures use the real Windows operation.

Verification: 12/12 focused process acceptance methods passed after repair. Four new unit methods cover pin failure, inaccessible/uncertain ownership, full birth-identity mismatch, and admission after an actual owner thread finishes; the focused Win32 lifecycle run passed 27/27. Supervisor and independent review inspected the actual ownership protocol, loader behavior, cleanup bounds, and test isolation. Contracts map to HL006–HL013 in `features/hook-lifecycle.feature`.

| Platform | Debug full suite | Release full suite |
| --- | --- | --- |
| Win32 | 222/222 | 222/222 |
| x64 | 222/222 | 222/222 |

Fresh solution builds passed Debug, Release, Debug UNICODE, and Release UNICODE on both architectures. Independent Release suite filters passed Unit 82/82 and Acceptance 140/140 on both architectures. TRX results are under ignored `build/test-results/<platform>/<configuration>/`; initial red and focused green records are retained separately. No new global hooks, dependency installations, user INI changes, staging, or commits were involved.

Tradeoffs and limits: pinned binaries remain loaded/locked until every affected process exits (or a different output path is used). The registry refuses admission on capacity/generation exhaustion and keeps shutdown busy when liveness cannot be established. A pre-publication thread-ID reuse can conservatively delay recovery; published IDs use full birth identities. The isolated tests substitute global hook installation/removal, so actual global-hook delivery, manual monitor/DPI behavior, and extended real desktop operation remain pending. The reported intermittent-stop cause is not established.

## Phase 6 execution and coverage inventory

The user committed Phase 5 before authorizing this audit. Production C/Win32 architecture, UI compatibility scaling, legacy key names, and the Microsoft unit/acceptance framework remain unchanged. No sound removal or new Settings UI was included.

| Area | Audited behavior and evidence |
| --- | --- |
| Preferences | Native dirty state, five destinations, enabled, all four toggle choices, threshold numbers/spinners and spacing; Apply/OK/Cancel and tab validation. Spinner maximum now matches the existing 100 endpoint. Contracts ST-010–ST-013, ST-019. |
| Grid | Both axes, all three modes, enabled state, selected/unused edit validation, boundaries and persistence. Disabling an axis retains its mode/value. Guides work independently of Desktop edges for moving/resizing, including native MDI client-relative grids. SINGLE zero is a valid unpadded boundary guide; EVEN zero yields no guides and PIXELS zero is invalid. Contracts ST-014–ST-015, ST-021–ST-022, MG05–MG08. |
| Advanced | All seven live checkboxes, including inverse mappings, plus enabled/disabled crop input and bounds. Native owned-window tests verify MDI admission, ignored/minimized destination filtering, clipping/manual crop consistency, and safe excessive crop retaining one visible row. Contracts ST-016–ST-017, ST-020, WF-001–WF-006. |
| Tray integration | Real tray controller, resource icon/tooltip selection, hide/unhide, Explorer-recovery function and failed-add retry; only Shell_NotifyIcon is substituted. Dialog INI reload skips enabled/icon-hidden setters so recovery state is preserved without transient changes. Contracts TR-001–TR-004, INI-004. |
| INI-only settings | Center/equal-key persistence remains covered; equal/aspect behavior stays compile-disabled. Exact semicolon class lists support full native class names, missing values preserve state and explicit empty values clear lists. Strict full-token numeric loading rejects invalid/overflowing values, retains decimal/legacy hex formats, normalizes booleans, and enforces persisted threshold bounds. Contracts INI-001–INI-003 and WF-001–WF-002/WF-006. |
| Deliberately inactive controls | Auto-lineup stays hidden/disabled; quiet-fast has no current resource control. Audio remains deferred. |

Red/green evidence: the initial native Settings run had seven owned failures plus the recovered-icon regression; the filter/crop run failed 10 of 16 methods; INI validation failed four of five; tray integration failed one of four. A strict-parser hex compatibility regression was caught during review and repaired with its own test. A green intermediate 270-test matrix did not end the audit: four more grid failures exposed Desktop coupling/SINGLE-zero handling, followed by two native MDI guide failures. All demonstrated failures are repaired in the final suite.

The audit adds 55 methods over the committed 222-test baseline: native modeless property-sheet interaction, direct controller/INI/Shell boundary tests, and owned top-level/MDI collection through actual matching/move/resize. Settings fixtures snapshot and restore crop/key/class state. Supervisor and independent review inspected actual changes; no required corrections remain.

| Platform | Debug full suite | Release full suite | Separate Release filters |
| --- | --- | --- | --- |
| Win32 | 277/277 | 277/277 | Unit 94/94; Acceptance 183/183 |
| x64 | 277/277 | 277/277 | Unit 94/94; Acceptance 183/183 |

All eight solution builds passed after the final MDI grid correction. TRX files remain under ignored `build/test-results/<platform>/<configuration>/`; focused red/green records have distinct names. No global hooks, actual Shell icons, user INI changes, dependency installations, staging, or commits were involved.

Limits: native functional controls are covered, but context-help rendering, keyboard focus/tab order, actual Explorer notifications, host tray-menu command dispatch, and real desktop/global-hook/DPI/long-duration behavior still need validation. Apply persistence remains sequential, with no atomic rollback guarantee on save failure. These checks do not diagnose the reported intermittent stop or certify every setting combination.

## Phase 7 execution: remove app sound functionality

The user committed Phase 6 before authorizing this increment. Removed the Sounds property page, preview/browse controller code, audio help/resource IDs and play/stop globals; Settings now contains Preferences, Snap to grid, and Advanced. Updated retained native tests for the new page indices and removed only the obsolete Sounds-page Apply test. Two new native absence tests failed against the old four-page/resource image before deletion.

Removed playback/worker sources, audio message posts/IDs, noisy settings and shared audio thread ID, the hook API's audio argument, and sound-only speed tracking and transition macros. Kept last-result copy/reset and geometry consumers intact. All app/DLL/process-test callers now use SnapHookAll(hwnd, os), so matching rebuilt EXE/DLL pairs are required. Generic thread helpers and shared dialog/help libraries remain; no ownership-registry redesign was included.

Removed audio mappings from INI and guarded registry code, audio defaults from the checked-in INI, WAVs/play-stop/speaker assets, exclusive WinMM linkage, old/current project references, installer media/default rows, explicit app/debug beeps, and audio sections in source help/context files. Old audio INI keys are ignored without altering other preferences; fresh saves create none, and existing unknown/retired keys remain untouched. The dormant unsupported lineup call uses the existing host thread rather than a deleted worker; its absent plugin is not certified. The mixed-content historical snapTo.chm is retained as an archive and was not rebuilt; source help is updated, and the existing compiled-help/installer modernization limits remain.

Regression coverage adds ten methods: two UI/resource absence, three tracking/continued move-resize/reset, two retired API/repeated controlled lifecycle, and three fresh/legacy INI preservation cases. One obsolete Sounds-page-only method is removed, taking the suite from 277 to 286. Focused UI tests passed 48/48, runtime/lifecycle/geometry tests 120/120, and INI removal tests 3/3. Independent review checked actual source, resource, dependency and packaging changes and found no required correction.

| Platform | Debug full suite | Release full suite | Separate Release filters |
| --- | --- | --- | --- |
| Win32 | 286/286 | 286/286 | Unit 97/97; Acceptance 189/189 |
| x64 | 286/286 | 286/286 | Unit 97/97; Acceptance 189/189 |

All eight solution builds passed. Actual Release EXE/DLL imports show no WinMM/multimedia playback dependency, DLL exports contain no retired audio setters/getters, and each EXE imports its matching rebuilt hook entry point (Win32 _SnapHookAll@280, x64 SnapHookAll). Project XML and residual source/build/installer references were reviewed. Artifacts remain under ignored build/ directories. No global hooks, production app launches, audio playback, dependency installations, user INI changes, staging, or commits were performed. Main-host startup/shutdown was inspected and built; runtime tests exercise the controlled hook/subclass boundary rather than an actual global-hook launch.

## Per-increment record

For each implemented increment, record its completed scope, changed behavior, relevant feature/scenario IDs, executed unit/acceptance/build results by architecture, review outcome, and remaining limitations. Update statuses as work occurs; do not treat planned checks as passing checks. The first foundation baseline was 29/29 on each architecture; current results supersede that count without treating the baseline as proof of legacy correctness.

## Additional verified repair evidence

- Settings: actual native controllers/resources/temporary INI round-trips, Cancel, validation, and save failure were exercised. Validation lost its failure result after SetFocus; the repaired ordering passes both spacing and threshold cases. Enabled state and spacing now persist.
- Input: 4 of 17 intended-behavior tests failed before held-key detection was changed to use the high bit; all 17 then passed on Win32.
- Old position composition: 3 of 8 tests failed before separating old-position targets from ordinary window edges. It now works with inside-edge exclusion and does not create a phantom adjacent target; 20 related old-position/input acceptance tests passed on Win32.
- Native visible frames: two test-owned offscreen framed windows verified physical gaps of 0, 8, and 24 pixels through move and resize paths. Eight focused frame/composition tests passed on Release Win32. Manual top crop is a minimum absolute inset, and explicit regions intersect visible frame bounds.
- Independent review found extra confinement defects after the first green suite: a false internal seam, unresolved opposite outside corners, a remote monitor attracting recovery, and unavailable gaps missed by corner-only checks. Full-rectangle union placement now passes an independent GDI-region oracle, including a rectangle exactly coincident with an unavailable gap. The latter failed before the interior-coverage repair. A green intermediate suite did not establish correctness.
- The strict resize solver initially regressed Ctrl center-resizing: seven of eight actual owned-window pipeline cases failed before adding reflected opposite-boundary candidates. Native framed bottom resizing and two ordinary fixed-visible-edge cases also failed before crop composition was repaired. Final opposite-side proximity tests reproduced a confinement escape on both axes before consistent matched-side reflection was added. All 17 center/native acceptance methods now pass.
- Final Settings review reproduced four Apply acknowledgement/retry failures and two enabled-state-on-opening failures before repair. All four page handlers now return handled status and reset successful results; opening Settings preserves runtime enabled state while retaining the legacy INI reload.
- Activation distance is intentionally inclusive. This replaces a legacy exclusive comparison; the corresponding expected behavior was explicitly documented instead of preserving source-derived assumptions.

## Initial checkpoint automated verification and remaining work

| Platform | Debug Unit + Acceptance | Release Unit + Acceptance | Separate Release filters |
| --- | --- | --- | --- |
| Win32 | 206/206 | 206/206 | Unit 78/78; Acceptance 128/128 |
| x64 | 206/206 | 206/206 | Unit 78/78; Acceptance 128/128 |

All eight `ALLSNAPB.sln` builds passed: Debug, Release, Debug UNICODE, and Release UNICODE for both Win32 and x64. Verification used installed VS2022/MSVC v143/SDK 10.0.26100.0. Remaining build warnings are deprecated GetVersionEx calls in application/SDK compatibility code. `git diff --check` passed; generated output remains ignored.

The user also confirmed that Visual Studio Test Explorer discovered and ran the tests successfully; the IDE configuration/platform was not specified.

Commands: `.\tests\run-tests.ps1 -Platform <Win32|x64> -Configuration <Debug|Release>`, followed by independent VSTest FullyQualifiedName Unit/Acceptance filters. Results are under ignored `build/test-results/<platform>/<configuration>/`; no global hooks, unrelated windows, user INI changes, dependency installations, or commits were involved. Native tests use an active Windows desktop. The independent GDI union/difference oracle covers 237 placements, including the exact-gap regression.

Remaining verification and design work:

- Run actual x86/x64 target-application hook startup, repeated gestures, exit, and a long-duration trial. Phase 5 covers isolated termination and callback retention, but does not certify actual global-hook delivery or diagnose the reported intermittent stop.
- Check representative applications at 100% and on the current HiDPI display, then mixed-DPI monitor transitions and the older home monitor. UI compatibility scaling is approved; fractional caller coordinates can round physical spacing to a representable value.
- Phases 6/7 exercise the retained native property-sheet controls, owned MDI/class filtering, and the real tray controller through a substituted Shell boundary. Still validate actual Explorer delivery, host menu dispatch, context help and keyboard navigation. Disabled aspect-ratio/auto-lineup features remain disabled; app audio has been removed.
- Preserve explicit impossible-fit limits: oversized movement uses an accessible-origin fallback; ordinary resizing does not translate an outside fixed edge; centered resizing cannot always fit if its original center is outside the desktop.
- The user committed Phase 14, then authorized deferred Settings repairs and regression test execution in Phase 15. Other/deeper work remains in B-007 and the deferred audit list. The user owns commits.

## Phase 8 execution: repository README

Added root [README.md](../../../README.md) as the developer/user entry point. It documents the installed build toolchain and output layout, both required process architectures, working-directory INI behavior, Settings and collapsed physical spacing, Microsoft unit/acceptance commands, source navigation, and current manual-validation/packaging limitations. Existing license and historical EULA files are linked.

Verification is supervisor and independent documentation review against project files, the test runner, production settings/startup code, and the recorded Phase 7 results, plus local-link and whitespace checks. No application code changed; no builds or tests were rerun. Manual desktop validation remains outstanding.

## Phase 9 execution: organize source into folders

The user committed Phase 8 before authorizing this structural increment. Moved 173 tracked files into `src/app/`, `src/engine/`, `src/common/`, `resources/`, `help/`, `packaging/`, and `legacy/` (inactive `src/`, historical `projects/`, and `archive/`). Existing tests/features retain their folders. Root solution/project names, GUIDs, output directories, debugger working directory, and INI selection are unchanged.

Updated the five modern projects' source/resource paths and include directories, both IDE filter files, historical source lists, installer input/license paths, and documentation links. Resource scripts, headers, manifests, and assets remain siblings without content edits. `help/snapTo.hhp` writes `..\allSnap.chm`, preserving the application's existing root help lookup. Historical projects/installers remain unvalidated archives; their existing missing/obsolete dependencies were not repaired. No compiled help was regenerated.

Verification:

- All eight `ALLSNAPB.sln` builds passed: Debug, Release, Debug UNICODE, and Release UNICODE on Win32/x64. This compiles the test DLL and isolated-process helpers but does not execute tests. Logs are under ignored `build/layout-verification/`; existing deprecated `GetVersionEx` warnings remain.
- Compared moved C/header/resource/assets against HEAD with normal Git text normalization: unchanged. All moved files exist and are not ignored; modern project/filter inputs and local Markdown links resolve. `git diff --check` passed.
- Independent review found two relocation issues, both corrected: archive folder names initially matched ignore rules, and the still-included `WDJSUB.H` belongs with active engine headers even though its old C implementation is inactive. No further required corrections remained.

No tests were executed for this increment; the last results remain Phase 7's 286/286 in Debug/Release on each architecture. No application/global hooks were launched, user INI changed, or Git staging/commits performed. Manual desktop validation, naming, and Clean Code work remain separate.

## Phase 10 execution: solution/project naming cleanup

The user committed Phase 9 before authorizing this naming increment. Renamed `ALLSNAPB.sln` to `allsnap.sln`, and `snapit.vcxproj` / `snapit.vcxproj.filters` to `allsnap.vcxproj` / `allsnap.vcxproj.filters`. These names match the existing application ProjectName and executable naming. The original meaning of the trailing B is unknown; no interpretation was assumed. Historical project/source/installer names are retained.

Updated the solution's app-project path, app RootNamespace metadata (`snapit` to `allsnap`), current build/test instructions, spec references, and backlog. Seven Solution Items still used pre-folder-move HTML paths; they now resolve under `help/`. Project GUIDs, configurations, targets/outputs, debugger working directory, INI behavior, binary names, and runtime identifiers are unchanged. The app project's only content change is RootNamespace; filters are unchanged. No production/resource/test code changed.

Verification: all eight `allsnap.sln` builds passed (Debug, Release, Debug UNICODE, Release UNICODE for Win32/x64), including test-project compilation without executing tests. Logs are under ignored `build/naming-verification/`. Solution project/item paths and local Markdown links resolve, exact project/filter comparisons confirm the metadata-only scope, and `git diff --check` passed. Independent review found no further required corrections. Historical build records above retain the solution filename used at the time.

No tests were executed; the last test results remain Phase 7's 286/286 in Debug/Release on each architecture. No app/global hooks were launched, user settings changed, or Git staging/commits performed. Open `allsnap.sln` in Visual Studio; old recent-solution entries may need replacing. Manual desktop checks and Clean Code refactoring remain separate milestones.

## Phase 11 execution: focused INI save refactoring

The user committed Phase 10 before authorizing the first focused Clean Code milestone. Scope is only the save implementation in `src/app/snap_ini.c`. Extracted file-local `SaveSettingToINI` to serialize one table entry, leaving `SaveSettingsToINI` responsible for path lookup, ordered traversal, and immediate failure returns. Removed save-specific goto labels, status bookkeeping, and an obsolete registry-close comment. No new abstraction layer, module, API, test seam, or behavior requirement was introduced.

Preserved the exact settings table/order, getter types and calls, raw BOOL serialization, UINT-to-int conversion, string writes, successful REGT_MULTI no-op, and unsupported-type diagnostic/failure. A missing path or first failed write still returns FALSE; earlier successful writes remain in place. Public success/failure results remain canonical TRUE/FALSE. Loading, numeric parsing/validation, empty/missing key handling, path caching, runtime preservation, and all other source modules are untouched.

Verification: independent review compared all save branches and getter/write sequencing with HEAD. A prefix comparison confirms the entire file before the save implementation is unchanged. All eight `allsnap.sln` builds passed (Debug, Release, Debug UNICODE, Release UNICODE on Win32/x64), including test-project compilation without test execution. Logs are under ignored `build/refactoring-ini-save-verification/`; `git diff --check` passed.

Existing tests inspected: INI round trips and save failure in `tests/unit/settings_tests.cpp`; Apply failure/retry in `tests/acceptance/settings_acceptance.cpp` and `property_sheet_acceptance.cpp`; fresh/legacy save behavior in `silent_ini_acceptance.cpp`. No tests were added or executed. These checks do not provide a new runtime regression result; the last executed suite remains Phase 7's 286/286 in Debug/Release on each architecture. No app/global hooks were launched, user settings changed, or Git staging/commits performed. Further refactoring stays in separate reviewable milestones under B-007.

## Phase 12 execution: focused INI load refactoring

The user committed Phase 11 before authorizing this increment. Extracted file-local `LoadSettingFromINI` in `src/app/snap_ini.c`; `LoadSettings` retains path lookup, ordered traversal, and runtime enabled/icon-hidden skips. Removed unused load locals and the registry-named goto. Missing/invalid values still skip their setters, BOOL values still normalize, INT threshold/crop checks and signed-to-UINT conversion are unchanged, strings retain their lengths and empty/missing semantics, and REGT_MULTI remains a no-op. Unsupported types still stop traversal while a valid-path load returns TRUE. The helper's continuation result is documented separately from public load success.

Verification: independent semantic review found no required corrections. Prefix/suffix comparisons against HEAD confirm the settings table, parser, path handling, public wrappers, and save implementation are untouched. All eight `allsnap.sln` builds passed (Debug, Release, Debug UNICODE, Release UNICODE on Win32/x64), including test-project compilation without execution. Logs are under ignored `build/refactoring-ini-load-verification/`; `git diff --check` passed.

Existing `tests/unit/ini_validation_tests.cpp` and Settings/tray acceptance coverage were inspected, not executed. No tests were added or run; runtime regression verification remains outstanding and the last executed suite is still Phase 7's 286/286 in Debug/Release on each architecture. No app/global hooks were launched, user settings changed, or Git staging/commits performed. Further refactoring remains in B-007 as separate bounded milestones.

## Phase 13 execution: focused INI numeric parser refactoring

After the user committed Phase 12, extracted file-local `ParseIntegerValue` from `getInt` in `src/app/snap_ini.c`. The existing function retains its exact 64-TCHAR profile read and missing/truncated-value guard. Conversion retains signed decimal/0x detection, leading-zero decimals, whitespace handling, original-input no-digit checks, errno reset/ERANGE checks, INT bounds, trailing-character rejection, and output assignment only on success. The settings table, string/path helpers, load/save implementations, and public APIs are unchanged.

Independent semantic review found no required corrections. Prefix/suffix comparisons confirm the bounded scope. All eight solution builds passed (Debug, Release, Debug UNICODE, Release UNICODE on Win32/x64), including test-project compilation without execution; `git diff --check` passed. Logs are under ignored `build/refactoring-ini-parser-verification/`. Existing `tests/unit/ini_validation_tests.cpp` was inspected, not run. No tests were added or executed; runtime regression verification remains outstanding and the last suite results remain Phase 7. No app/global hooks, user settings changes, staging, or commits were involved. Further refactoring remains in B-007.

## Phase 14 execution: finish the low-risk cleanup inventory

The user authorized continuing without another commit checkpoint. Retained Phase 13 and audited active app/engine/common modules for changes whose equivalence is clear from source. Completed the finite safe inventory in thirteen source files:

- INI: remove orphan/redundant declarations and dead locals, correct stale registry comments, and flatten cached-path guards while preserving override/cache/API order and bounds.
- Settings: share the exact eight existing property-page assignments, flatten context-menu and grid-selection guards, remove the unreferenced app helper `find_index` and orphan declarations, and remove obsolete experiments. Page order, flags, callbacks, unassigned fields, grid selection bounds, notification handling, menu lifetime, and write order are retained.
- App/tray/debug/help: remove unused state/result bookkeeping, unused declarations/macros, dead initialization, and commented examples. Keep all native calls and arguments; return the existing raw notification BOOL directly, and remove only the provably always-true tooltip-array branch.
- Engine: remove verified unread/private or write-only state, unused macros/declaration, comment-only alternatives, and a no-op loop initializer. Flatten stack-pop/window-rectangle guards. Retain cursor queries, public engine symbols, arithmetic, callback flow, capacities, and shared hook/subclass ownership.

Supervisor inspected the diffs; app and engine agents cross-reviewed each other's changes, including INI and the retained parser extraction. No required corrections remain. The audit found no further worthwhile obvious cleanup within these boundaries; small explicit grid adapters and already-compact modern geometry/diagnostic helpers remain as written.

All eight `allsnap.sln` configurations build successfully after the final help cleanup (Debug, Release, Debug UNICODE, Release UNICODE on Win32/x64), including test-project compilation without execution. Logs are under ignored `build/low-risk-cleanup-verification/`; `git diff --check` passed. No tests were added or executed; last suite results remain Phase 7, and runtime regression verification is outstanding. No app/global hooks, user settings changes, staging, or commits were involved.

### Deferred audit findings and stopping boundary

These are source-review candidates, not reproduced failures or diagnoses of the intermittent stops. They require behavioral decisions and regression coverage, so were not changed in this pass:

- Settings `PrefsProc` help-message fallthrough, context-menu handle lifetime, and native structure initialization/callback contracts.
- Tray tooltip character-count/buffer handling and notification failure handling; existing counts/initialization were deliberately preserved.
- Debug clipboard/file-dialog buffer capacities and error paths; legacy multi-string bounds and debug macros.
- Typed INI dispatch, unused externally visible engine functions, registry/auto-lineup retirement, geometry/arithmetic deduplication, and lifecycle/shared-state redesign. These exceed the low-risk cleanup scope.

Keep these separate from the completed behavior-preserving pass. Actual desktop/Explorer, monitor/DPI, and long-running verification remain pending under B-001/B-002.

## Phase 15 execution: deferred Settings/native tray repairs

The user committed the cleanup before authorizing behavior repairs, new regressions, and test execution. Scope is `src/app/snap_settings.c`, `src/app/snap_taskbar.c`, controlled test-boundary adapters, feature documentation, and native acceptance tests. Geometry, hook ownership, INI semantics, Settings selections, and desired hide/unhide preference are unchanged.

### Repairs and contracts

- ST-024: every page acknowledges WM_HELP immediately and forwards the supplied real child control once. Preferences no longer falls through into notification processing. Tests check pending threshold/crop/grid edits, INI contents, and page result are unchanged.
- ST-025: Settings help menus retain the loaded parent and destroy it (and its submenu) after selection/cancel or missing-submenu failure. Load failure avoids tracking. Tests use real menu handles while replacing interactive popup tracking.
- ST-026/027: use the exact int CALLBACK signature without a function-pointer cast; return zero. Read the native template signature and update only its style at standard offset 0 or extended offset 12 using memcpy. Initialize page/header structures. Tests compare native headers, create actual hidden standard/extended dialogs, and inspect optional descriptor fields.
- TR-005–007: initialize Shell packets, load tooltip directly into its TCHAR capacity, reject missing tooltip/icon resources before modification, release owned images after modification, preserve the original failure over cleanup, and attempt deletion if initial add succeeded but refresh failed. Desired visibility preference remains retryable. Cleanup cannot guarantee removal when the Shell also rejects deletion.

New cases are in `tests/acceptance/settings_native_contract_acceptance.cpp` (11) and `tray_native_contract_acceptance.cpp` (8), using the existing Microsoft framework. Small test-only seams control popup interaction/resource failures; shared AppFixture captures help forwarding and per-operation Shell failures. Existing resource-backed native/functional tests remain intact. Test descriptor SDK target/layout matches production C (`stdafx.h` before native headers); an initial test-layout mismatch was corrected before accepting Settings red results. No stack-poison seam or assertions about structure padding were introduced.

### Executed evidence

Existing compiled baseline: 286/286 Debug tests passed on each architecture before new tests were built. Pre-repair combined x64 regression run passed 2/19; eight tray cases included seven failures and one retained hide-state success. The corrected-layout Settings-only red run passed 1/11: three unacknowledged help requests, two menu leaks, two menu failure paths, invalid callback return, corrupted extended template header, and uninitialized optional fields failed; the no-help guard passed. Inactive-field initialization is an additive deterministic-input contract, not proof of a previously observed UI symptom. The stronger help state assertions were added before green verification.

After repairs, focused runs passed 19/19 on Win32/x64. Full results:

| Platform | Debug | Release | Suite composition |
| --- | --- | --- | --- |
| Win32 | 305/305 | 305/305 | 97 unit, 208 acceptance |
| x64 | 305/305 | 305/305 | 97 unit, 208 acceptance |

All eight solution builds passed: Debug, Release, Debug UNICODE, Release UNICODE on both architectures. Build logs are under ignored `build/settings-repairs-verification/`; baseline/red/focused/final TRX and logs are under `build/test-results/<platform>/<configuration>/settings-repairs-*`. The full runs also verify the retained Phase 11–14 refactors and 12 isolated-process cases. Independent app/tray cross-review and supervisor inspection found no required corrections; `git diff --check` passed. No Git staging/commits, global hook installation, actual tray changes, unrelated-window changes, or user INI changes occurred.

Native contract references: [WM_HELP](https://learn.microsoft.com/en-us/windows/win32/shell/wm-help), [property-sheet callback and template formats](https://learn.microsoft.com/en-us/windows/win32/api/prsht/nc-prsht-pfnpropsheetcallback), [menu ownership](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroymenu), and [notification fields](https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataw). These define the expected behavior rather than preserving legacy outputs.

### Remaining limits

Tests suppress interactive popup/help rendering and substitute Explorer/resource-failure delivery. Actual Explorer/host-menu integration, help rendering, keyboard navigation, display hardware/mixed DPI and a long-running trial remain pending. Host/debug menu ownership, debug clipboard/file-dialog capacities, multi-string bounds, debug macros, typed dispatch, and larger architecture work remain outside this Settings increment. These repairs do not establish the cause of intermittent stops; rollback after Shell rejection remains best-effort.

## 2026-11-17 follow-up: x64 icon selection

After committing Phase 15, the user requested `ico00007.ico` for x64. Updated the x64 application and ordinary enabled-tray resource mappings (including resource-editor variants); the XP-style enabled tray mapping already used that asset. Win32 and disabled-state icons are unchanged. Release resource compilation passed for Win32/x64 via the ResourceCompile target; no tests were requested or run, and running executables/DLLs were not replaced. Rebuild/restart x64 to use the new resources. No staging or commits performed.

## Phase 16 execution: 2.0.0 and final release checks

The user requested a clean 2.0.0 release with a readable source version, passing tests, and README/state updates before their own master commit. Retained the existing uncommitted icon/documentation changes. The literal `ALLSNAP_VERSION_TEXT "2.0.0"` is in `src/app/allsnap_version.h`, beside startup. `snap_App.c` exposes it as the human-readable `g_version` TCHAR array and uses it in the About caption; a separate numeric `g_instance_version` preserves the instance-message protocol and distinguishes legacy copies. Mutex/window names, settings, hook behavior, and DLL API are unchanged.

Both active RC files consume shared text/numeric values. Windows fixed versions are 2.0.0.0, string versions are 2.0.0, both About templates no longer say Beta, and Release metadata no longer sets SpecialBuild. Current per-architecture OriginalFilename values replace stale historical names. RC dialog text requires one literal, so the full About label also lives in the obvious version header and is checked against the release text by acceptance coverage. Resource-editor TEXTINCLUDE records retain the shared include. The app manifest's own version is 2.0.0.0; Common Controls6.0.0.0 and asInvoker are unchanged.

Added two unit methods in `tests/unit/version_tests.cpp` and four acceptance methods in `tests/acceptance/version_acceptance.cpp`, using the existing Microsoft framework. They check text/tuple consistency and legacy-token separation, actual compiled fixed/string/flag VERSIONINFO, and real hidden About resources. Version.lib is linked only by tests. Initial Win32 Debug verification passed 310/311 and caught missing `_DEBUG` in test resource compilation; the test project's resource definitions now match its C/C++ configuration. The final matrix then passed:

| Platform | Debug | Release | Suite composition |
| --- | --- | --- | --- |
| Win32 | 311/311 | 311/311 | 99 unit, 212 acceptance |
| x64 | 311/311 | 311/311 | 99 unit, 212 acceptance |

All eight configurations built into isolated `build/releases/2.0.0/<platform>/<configuration>/` directories, including test helpers. The actual EXE/DLL files from every configuration were inspected for fixed/text product/file versions, Debug/Release flags, architecture, and correct original filenames. Actual embedded manifest identities/dependency/execution level passed. X64 group icons109/170/217 use the exact image payloads from `ico00007.ico` (the resource compiler normalizes the ICO directory's plane field). Resource inspection loaded binaries only as data, without executing application/DLL entry points. The final regression matrix retains all 12 isolated-process cases and prior Settings/refactoring coverage.

Build logs and artifact inspection are under ignored `build/releases/2.0.0/`; TRX/runner logs are under `build/test-results/<platform>/<configuration>/release-2.0.0-final/`. Independent review found no required corrections; project/filter paths, local links, new-file whitespace, and `git diff --check` were checked. No staging/commits, installed-process termination, global-hook launch, user INI change, or replacement of locked binaries occurred. Fresh matching Release pairs are ready in the Win32/Release and x64/Release subfolders.

Automated release checks do not establish years of real desktop reliability or diagnose intermittent stops. Actual global-hook/Explorer delivery, keyboard/help rendering, 100%/HiDPI/mixed monitors and the older home monitor, and a long-running trial remain manual validation. The README records these limits and the old-DLL upgrade procedure. Other deferred behavioral/refactoring items remain outside this version release.
