# allSnap modernization implementation plan

Status: Phase 8 adds the root README after the user committed Phase 7. This is a documentation-only milestone; no builds or tests were rerun. The last executable verification remains Phase 7: all 286 tests passed in Debug/Release on Win32/x64, all eight solution builds passed, and Release binary imports/exports were verified. Manual global-hook/desktop validation remains pending. This is a living execution document. Update the relevant step, verification result, and outstanding issue after each increment; update `.ai/current-state.md` when actual architecture or behavior changes. Requirements are in `project-spec.md`.

## Current position

- Completed: source inspection of snapping, confinement, settings/candidates, hook/subclass lifecycle, startup, and sound handling.
- Completed: verified local availability of Microsoft's native C++ testing headers, x86/x64 libraries, and VSTest runner. Approved as the common unit/acceptance framework; no dependencies installed.
- Confirmed with user: root `features/*.feature` files are documentation only; no Gherkin tooling.
- Completed in Phase 7: all app sound functionality removed; historical compiled help is explicitly retained as an archive.
- Completed in Phase 8: root README covering build/run commands, settings, dual-architecture operation, tests, source navigation, and known limitations.
- Native unit/acceptance tests now cover 286 methods (97 unit, 189 acceptance). All passed in Debug and Release on Win32 and x64; the acceptance suite includes 12 isolated-process methods using the same Microsoft framework.
- User handles all commits. Agents do not stage, commit, reset, or amend. Phase boundaries below are review checkpoints, not claims of commits already created.

## Milestones and human commit checkpoints

| Phase | Reviewable scope | Current verification |
| --- | --- | --- |
| 1. Test foundation | Microsoft framework, runner, documentation, production-C fixture seams, initial confinement/snapping contracts | Implemented. Current suite: 286/286 in all four architecture/configuration runs. Later contract review corrected inherited assumptions rather than freezing legacy outputs. |
| 2. Lifecycle reliability | Hook/subclass ownership and failures, safe shutdown bookkeeping, diagnostics | Initial failure-handling checkpoint passed 23 focused tests. Phase 5 supersedes scalar accounting and simulated ownership with a process-aware registry and actual child-process coverage. |
| 3. Monitor/grid and Settings repairs | Workarea/guide separation, validation, Old position, Settings persistence and failure reporting, held-key semantics | Implemented. Geometry, Old position, input, validation reentrancy, Apply acknowledgement/retry, and opening-state regressions demonstrated red before repair; final suite green on both architectures. Remaining Settings paths require desktop validation. |
| 4. Visible frames, spacing, and desktop geometry | DWM/region/manual crop composition, collapsed spacing UI/INI, ordinary/HiDPI coordinates, monitor union confinement | Implemented. Independent GDI coverage oracle plus native framed ordinary/Ctrl resize cases are green in the final matrix. Real mixed-DPI transitions and home-monitor checks remain pending. |
| 5. Isolated process lifetime | Actual child-process termination/detach, concurrent owners, shutdown reservations, callback/module retention | Implemented and independently reviewed. Three product failures demonstrated before repair; 12 isolated-process methods and four new unit tests pass within the 222-test matrix. Global-hook/desktop validation remains separate. |
| 6. Settings behavior audit | Non-sound controls, actual property sheet, tray boundary, persisted validation, MDI and class filtering | Implemented and independently reviewed. Added 55 methods; final 277-test matrix and all eight builds pass. Actual Shell/global-hook delivery and manual UX/hardware checks remain separate. |
| 7. Sound removal | Audio UI/preview, worker/messages/settings/API, audio assets/help/packaging and exclusive dependencies | Implemented and independently reviewed. Two native UI absence regressions failed before removal; final 286-test matrix, eight builds, and actual Release import/export checks pass. |
| 8. Repository README | Build/run/configuration/test instructions, source navigation, and known limitations | Complete and independently reviewed; commands, claims, and local links checked against source. No builds or tests rerun. |
| Later backlog phases | Folder organization; naming; Clean Code refactoring | Not started; keep behavioral, structural, and naming changes separate. |
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
- Fix state transitions and startup unwinding locally in `snap_lib.c`/`snap_App.c`; preserve the existing hook API and DLL boundaries. Do not solve a shared-ownership defect by simply making every shared flag process-local.
- Add small, actionable diagnostics for startup/hook/subclass failures and clean versus abnormal shutdown. Capture errors at the failing API. Avoid synchronous disk logging or blocking IPC in hot hook callbacks. Do not introduce a watchdog or claim the reported intermittent stop is diagnosed.

Verification: demonstrate each targeted regression before/after its repair, run both suites for x86/x64, and check normal startup/shutdown in a controlled environment. Report crash reproduction separately from successful state-transition tests.

### 3. Repair monitor/grid accounting and validate settings

Status: initial repairs implemented and independently reviewed, including property-sheet Apply acknowledgement/retry and preserving runtime enabled state when opening Settings. Final automated matrix passed; remaining actual-desktop Settings coverage is listed below.

- Write `features/monitor-and-grid-settings.feature` scenarios for multiple monitors with many grid lines and malformed persisted settings.
- Separate real work-area enumeration/accounting from artificial snap-guide candidates. Ensure enabling a grid cannot hide later monitors or turn a guide line into a confinement work area.
- Validate grid modes and numeric values before indexing UI arrays or dividing; preserve valid legacy keys and preferences. Set deterministic limits that match actual array capacities. Add unit tests for those boundaries.
- Keep changes to `snap_WinRects.c`, grid/settings boundaries, and their immediate callers. Reuse the existing rectangle result structures wherever suitable.

Verification: acceptance scenarios retain all work areas with grids enabled; invalid input is handled safely; baseline confinement and edge-snapping tests pass for x86/x64.

### 4. Diagnose visual gaps and remaining snapping failures

Status: initial repairs implemented and independently reviewed. Configurable spacing, visible-frame conversion, full-rectangle union placement, and owned-window ordinary/Ctrl resizing tests pass the final automated matrix. Hardware checks remain pending.

- Implement the approved single configurable pixel-spacing value (0–1000, default 0) for window/window and work-area edges. Apply the value once between adjacent windows. Keep activation distance separate, and do not pad center/grid guides. Reproduce legacy gap sources before changing frame geometry.
- Inspect DWM visible frame bounds, existing crop adjustments, and DPI coordinate conversions together. Do not change only the threshold or toggle manifest DPI awareness without matching the calculation path.
- Extend documented snapping scenarios and acceptance fixtures for window/window and window/work-area edges, resizing, negative coordinates, scaling, and repeated gestures. Repair only failures demonstrated by those examples.
- Explicitly check candidate ordering and centering paths with tests before changing them; suspicious source patterns alone do not establish the correct repair.

Verification: unit/acceptance results on both architectures plus native test-owned framed windows. Record the remaining interactive matrix for representative windows/scaling separately; native tests do not complete it. Keep confinement results separate and visible. The user explicitly permits Windows compatibility scaling for the UI; no UI DPI-awareness rewrite is required.

## Backlog

The canonical list is [`.ai/backlog.md`](../../backlog.md), with stable item IDs and separate requested/proposed work. Next priorities are B-001 (actual desktop/process-lifetime verification) and B-002 (remaining Settings coverage). Audio removal and the README are complete; folder organization, naming, and Clean Code work remain separate follow-up milestones. A single launcher/tray is proposed and would still manage both helper architectures.

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
- The user committed Phase 7 and authorized the Phase 8 README. Layout, naming, and refactoring remain separate backlog phases. The user owns commits.

## Phase 8 execution: repository README

Added root [README.md](../../../README.md) as the developer/user entry point. It documents the installed build toolchain and output layout, both required process architectures, working-directory INI behavior, Settings and collapsed physical spacing, Microsoft unit/acceptance commands, source navigation, and current manual-validation/packaging limitations. Existing license and historical EULA files are linked.

Verification is supervisor and independent documentation review against project files, the test runner, production settings/startup code, and the recorded Phase 7 results, plus local-link and whitespace checks. No application code changed; no builds or tests were rerun. Manual desktop validation remains outstanding.
