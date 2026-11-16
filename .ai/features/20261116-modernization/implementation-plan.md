# allSnap modernization implementation plan

Status: initial repair checkpoint implemented and independently reviewed; full Debug/Release test matrix and all eight solution builds passed. Desktop/hardware and remaining lifecycle work are listed below. This is a living execution document. Update the relevant step, verification result, and outstanding issue after each increment; update `.ai/current-state.md` when actual architecture or behavior changes. Requirements are in `project-spec.md`.

## Current position

- Completed: source inspection of snapping, confinement, settings/candidates, hook/subclass lifecycle, startup, and sound handling.
- Completed: verified local availability of Microsoft's native C++ testing headers, x86/x64 libraries, and VSTest runner. Approved as the common unit/acceptance framework; no dependencies installed.
- Confirmed with user: root `features/*.feature` files are documentation only; no Gherkin tooling.
- Confirmed with user: all sound functionality is to be removed in a future backlog increment.
- Native unit/acceptance tests now cover 206 methods (78 unit, 128 acceptance). All passed in Debug and Release on Win32 and x64; both Release suite filters passed independently on each architecture.
- User handles all commits. Agents do not stage, commit, reset, or amend. Phase boundaries below are review checkpoints, not claims of commits already created.

## Milestones and human commit checkpoints

| Phase | Reviewable scope | Current verification |
| --- | --- | --- |
| 1. Test foundation | Microsoft framework, runner, documentation, production-C fixture seams, initial confinement/snapping contracts | Implemented. Current suite: 206/206 in all four architecture/configuration runs; separate filters verify 78 Unit and 128 Acceptance methods. Later contract review corrected an inherited threshold assumption. |
| 2. Lifecycle reliability | Hook/subclass ownership and failures, safe shutdown bookkeeping, diagnostics | Failed-subclass regression reproduced red; 23/23 focused Win32 lifecycle tests passed. Shared-count forced-termination and real cross-process lifetime remain limitations. |
| 3. Monitor/grid and Settings repairs | Workarea/guide separation, validation, Old position, Settings persistence and failure reporting, held-key semantics | Implemented. Geometry, Old position, input, validation reentrancy, Apply acknowledgement/retry, and opening-state regressions demonstrated red before repair; final suite green on both architectures. Remaining Settings paths require desktop validation. |
| 4. Visible frames, spacing, and desktop geometry | DWM/region/manual crop composition, collapsed spacing UI/INI, ordinary/HiDPI coordinates, monitor union confinement | Implemented. Independent GDI coverage oracle plus native framed ordinary/Ctrl resize cases are green in the final matrix. Real mixed-DPI transitions and home-monitor checks remain pending. |
| Later backlog phases | Audio removal; README; folder organization; naming; Clean Code refactoring | Not started; keep behavioral, structural, and naming changes separate. |
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

Status: initial repairs implemented and independently reviewed. The failed-subclass regression was reproduced before repair (12 passed, 1 failed in the Win32 Unit suite). Actual separate-process termination and in-flight callback/DLL-unload safety remain unverified; simulated ownership tests do not establish those guarantees.

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

The canonical list is [`.ai/backlog.md`](../../backlog.md), with stable item IDs and separate requested/proposed work. Next priorities are B-001 (actual desktop/process-lifetime verification) and B-002 (remaining Settings coverage). Audio removal, README, folder organization, naming, and Clean Code work remain separate follow-up milestones. A single launcher/tray is proposed and would still manage both helper architectures.

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

## Final automated verification and remaining work

| Platform | Debug Unit + Acceptance | Release Unit + Acceptance | Separate Release filters |
| --- | --- | --- | --- |
| Win32 | 206/206 | 206/206 | Unit 78/78; Acceptance 128/128 |
| x64 | 206/206 | 206/206 | Unit 78/78; Acceptance 128/128 |

All eight `ALLSNAPB.sln` builds passed: Debug, Release, Debug UNICODE, and Release UNICODE for both Win32 and x64. Verification used installed VS2022/MSVC v143/SDK 10.0.26100.0. Remaining build warnings are deprecated GetVersionEx calls in application/SDK compatibility code. `git diff --check` passed; generated output remains ignored.

The user also confirmed that Visual Studio Test Explorer discovered and ran the tests successfully; the IDE configuration/platform was not specified.

Commands: `.\tests\run-tests.ps1 -Platform <Win32|x64> -Configuration <Debug|Release>`, followed by independent VSTest FullyQualifiedName Unit/Acceptance filters. Results are under ignored `build/test-results/<platform>/<configuration>/`; no global hooks, unrelated windows, user INI changes, dependency installations, or commits were involved. Native tests use an active Windows desktop. The independent GDI union/difference oracle covers 237 placements, including the exact-gap regression.

Remaining verification and design work:

- Run actual x86/x64 target-application hook startup, repeated gestures, exit, and a long-duration trial. The intermittent-stop cause remains unconfirmed. Abrupt target-process termination may strand shared subclass accounting; in-flight callback and DLL-unload safety require a separate lifecycle increment.
- Check representative applications at 100% and on the current HiDPI display, then mixed-DPI monitor transitions and the older home monitor. UI compatibility scaling is approved; fractional caller coordinates can round physical spacing to a representable value.
- Exercise remaining actual Settings integrations, especially MDI, ignored/skinned classes, and tray behavior beyond the covered enabled-state opening interaction. Disabled aspect-ratio/auto-lineup features remain disabled. Opening-path tests substitute hidden actual Preferences creation for native property-sheet creation; full property-sheet interaction is not certified.
- Preserve explicit impossible-fit limits: oversized movement uses an accessible-origin fallback; ordinary resizing does not translate an outside fixed edge; centered resizing cannot always fit if its original center is outside the desktop.
- Review this checkpoint with the user before starting the separately listed audio, layout, naming, and refactoring backlog phases. The user owns commits.
