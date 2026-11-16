# allSnap modernization implementation plan

Status: draft for discussion; no runtime implementation or test harness yet. This is a living execution document. Update the relevant step, verification result, and outstanding issue after each increment; update `.ai/current-state.md` when actual architecture or behavior changes. Requirements are in `project-spec.md`.

## Current position

- Completed: source inspection of snapping, confinement, settings/candidates, hook/subclass lifecycle, startup, and sound handling.
- Completed: verified local availability of Microsoft's native C++ testing headers, x86/x64 libraries, and VSTest runner. Proposed as the common unit/acceptance framework; no dependencies installed.
- Confirmed with user: root `features/*.feature` files are documentation only; no Gherkin tooling.
- Confirmed with user: all sound functionality is to be removed in a future backlog increment.
- Prior verified baseline: all eight existing solution configuration/platform combinations built successfully during build modernization. Builds and tests have not been rerun for this planning pass; there is no regression suite yet.

## Proposed execution sequence

### 1. Establish the test foundation and protect confinement

Status: not started.

- Add a native test project in the existing solution, with `tests/unit/`, `tests/acceptance/`, and only necessary shared fixtures. Use the same Microsoft C++ framework and VSTest runner for both suites.
- Compile reused production C modules as C; use C++ only for the test harness. Link real production behavior and avoid including application/DLL entry-point objects in the test harness.
- Add the smallest test seams needed for geometry/settings and deterministic Win32 inputs. Use temporary files and test-owned window handles; prevent default tests from installing global hooks.
- Write `features/desktop-confinement.feature` and `features/window-snapping.feature` as plain behavior documentation with stable scenario IDs, then map initial acceptance tests to those IDs.
- Characterize confinement and add focused geometry unit tests. Include work-area edges, taskbar reservations, negative coordinates, multi-monitor transitions, oversized windows, move versus resize, no candidates, toggles, and threshold boundaries. Do not enshrine an observed defect as intended behavior.
- Document reproducible build/run commands and filtering for Unit versus Acceptance, with explicit x86/x64 execution. Provide one entry point for both suites without downloading tools implicitly.

Verification: discover and run meaningful unit and acceptance tests on both architectures; ensure tests use production code and leave user state untouched. Build the application projects after introducing seams. Record actual commands/results here.

### 2. Repair lifecycle failure handling and establish diagnostics

Status: not started; depends on step 1.

- Document `features/hook-lifecycle.feature` scenarios and add regressions for ignored hook startup failure and failed subclass registration.
- Add coverage for shared versus process-local ownership, unrelated process detach, repeated cleanup, and shutdown following partial initialization. Use controlled failure injection for deterministic failure paths; add isolated process checks where ownership requires them.
- Fix state transitions and startup unwinding locally in `snap_lib.c`/`snap_App.c`; preserve the existing hook API and DLL boundaries. Do not solve a shared-ownership defect by simply making every shared flag process-local.
- Add small, actionable diagnostics for startup/hook/subclass failures and clean versus abnormal shutdown. Capture errors at the failing API. Avoid synchronous disk logging or blocking IPC in hot hook callbacks. Do not introduce a watchdog or claim the reported intermittent stop is diagnosed.

Verification: demonstrate each targeted regression before/after its repair, run both suites for x86/x64, and check normal startup/shutdown in a controlled environment. Report crash reproduction separately from successful state-transition tests.

### 3. Repair monitor/grid accounting and validate settings

Status: not started; depends on step 1 and confinement coverage.

- Write `features/monitor-and-grid-settings.feature` scenarios for multiple monitors with many grid lines and malformed persisted settings.
- Separate real work-area enumeration/accounting from artificial snap-guide candidates. Ensure enabling a grid cannot hide later monitors or turn a guide line into a confinement work area.
- Validate grid modes and numeric values before indexing UI arrays or dividing; preserve valid legacy keys and preferences. Set deterministic limits that match actual array capacities. Add unit tests for those boundaries.
- Keep changes to `snap_WinRects.c`, grid/settings boundaries, and their immediate callers. Reuse the existing rectangle result structures wherever suitable.

Verification: acceptance scenarios retain all work areas with grids enabled; invalid input is handled safely; baseline confinement and edge-snapping tests pass for x86/x64.

### 4. Diagnose visual gaps and remaining snapping failures

Status: not started; depends on baseline tests and corrected candidate handling.

- Reproduce a representative gap and distinguish activation threshold from the final visible spacing. Agree on the desired spacing before altering geometry.
- Inspect DWM visible frame bounds, existing crop adjustments, and DPI coordinate conversions together. Do not change only the threshold or toggle manifest DPI awareness without matching the calculation path.
- Extend documented snapping scenarios and acceptance fixtures for window/window and window/work-area edges, resizing, negative coordinates, scaling, and repeated gestures. Repair only failures demonstrated by those examples.
- Explicitly check candidate ordering and centering paths with tests before changing them; suspicious source patterns alone do not establish the correct repair.

Verification: unit/acceptance results on both architectures plus a recorded interactive matrix for representative windows/scaling. Keep confinement results separate and visible.

## Backlog

| Item | Status and boundary |
| --- | --- |
| Remove all sound-related functionality | Requested; not started. After test foundation, remove audio UI, preview/playback/thread code, event messages and callers, persistence entries, sound-only assets/resources, installer references, and exclusively audio dependencies. Preserve non-audio preferences in existing INI files. Add an acceptance document and regression coverage for startup/shutdown/snapping without audio. Do not restore the currently disabled notification playback. |
| Single launcher/tray controlling x86/x64 helpers | Proposed improvement; defer until hook lifecycle and ownership are tested. Preserve both helper architectures. |

## Per-increment record

For each implemented increment, record its completed scope, changed behavior, relevant feature/scenario IDs, executed unit/acceptance/build results by architecture, review outcome, and remaining limitations. Update statuses as work occurs; do not treat planned checks as passing checks. No implementation increment has been completed yet.
