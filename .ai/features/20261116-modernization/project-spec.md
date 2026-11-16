# allSnap modernization

Status: approved for incremental implementation, beginning with the Microsoft test foundation and confinement coverage. Track execution in `implementation-plan.md` and actual repository behavior in `.ai/current-state.md`.

## Goal and constraints

Make the existing Windows utility reliable through incremental, tested repairs. Preserve its C implementation, Win32 executable/DLL architecture, working Visual Studio 2022 builds, and established setting names unless a specific change requires otherwise. A rewrite is outside this effort.

Desktop confinement is important, but the user explicitly considers its implementation untrusted too. Tests must enforce intended correct behavior, including visible borders, rather than freeze legacy output. Use the union of monitor work areas; internal seams allow transitions, exposed edges respect configured spacing, and unavailable gaps are not valid window-placement regions. Preserve window size during movement. Oversized windows cannot always fit and require an accessible fallback rather than invented full-fit guarantees.

The user's reported gaps and intermittent stops are investigation targets. Source defects do not yet establish their complete causes. Distinguish process exit, a live process with a failed hook, and a live hook with incorrect snapping.

## Acceptance correctness

Acceptance expectations come from the behavior contract, not from copying the current output. A defect must fail its test. Ambiguous semantics must be made explicit before treating a test as a contract; passing component tests do not certify untested UI, hardware, or lifetime behavior. Activation distance includes its boundary (distance <= configured threshold); this is an intentional decision replacing the inherited exclusive comparison.

## Confirmed requirements

- Add both unit and acceptance tests using the **same testing framework** for Win32 and x64.
- This is a personal application with one user. Prioritize that workflow without public-release or multi-user infrastructure.
- Support conventional 100% and HiDPI displays, including negative/staggered monitor layouts. Keep the legacy UI under Windows compatibility scaling; explicit DPI conversions belong in engine geometry. Mixed-DPI transitions and the older home monitor require separate hardware validation.
- Root `features/*.feature` files are human-readable behavior documentation used when writing acceptance tests. No Gherkin runner, parser, test generation, or executable feature-file format.
- Keep `.ai/current-state.md` and `implementation-plan.md` current as work progresses. Clearly separate proposed work, implementation, executed validation, and remaining uncertainty.
- Fix demonstrated defects in small increments, with a regression that fails before the repair and passes afterward where practical.
- Keep both application architectures working. Replacing the two-process user experience is a later design choice, not a prerequisite for repairs.
- Add removal of all sound-related functionality to the backlog; do not restore sound notifications as a bug fix.
- Add one configurable pixel-spacing value (0–1000, default 0). Apply it once between adjacent visible window edges (collapsed spacing) and at desktop work-area edges. Zero means flush edges. Activation thresholds remain separate; center/grid guides are alignment targets without padding.

## Pre-repair findings that justified initial work

| Finding | Source evidence | Proposed response |
| --- | --- | --- |
| Hook installation failure is ignored at startup. | `snap_App.c` calls `SnapHookAll` without checking its result; `snap_lib.c` reports failure. | Test the failure path; report failure and unwind completed initialization. |
| Failed subclass installation leaves the window/active flag recorded as installed. | `Subclass` in `snap_lib.c` assigns state before `SetWindowSubclass`, without rollback on failure. | Commit active state only on success or roll back on failure; verify exit remains possible. |
| Subclass ownership combines a shared flag with a process-local HWND, and detach clears the shared flag. | Shared section and `UnSubclass`/`DllMain` in `snap_lib.c`. | Reproduce cross-process teardown; make bookkeeping reflect actual ownership without redesigning the hook mechanism. |
| Grid lines and monitor work areas share `g_num_screens`; enough lines make the next monitor hit the monitor limit. | `AddScreen`, `AddGrid`, and `EnumMonitorsProc` in `snap_WinRects.c`. | Test multiple monitors with grids; separate physical-work-area accounting from guide candidates. Preserve confinement behavior. |
| Grid input reaches arithmetic and UI indexing without validation. | `setGridSnap` in `snap_lib.c`, `AddGrid` in `snap_WinRects.c`, and `Grid_InitDim` in `snap_settings.c`. | Test invalid persisted types/values and bounds. Prevent invalid indexes, division by zero, and unbounded generation while preserving valid settings. |
| Center-candidate arrays have capacity 10, but append guards allow 110. | Arrays and `AddHCenter*`/`AddVCenter*` in `snap_WinRects.c`. | Align bounds with actual storage and test capacity boundaries. This mismatch has not been reproduced as a runtime overflow. |
| Window geometry uses legacy bounds/cropping. | `snap_Crop.c`, `snap_WinRects.c`, `Snapper.c`. | Investigate visible frame bounds and DPI before changing gap behavior. Invisible resize borders are a hypothesis, not a confirmed diagnosis. |

These describe the original inspection, rather than the repaired source. The first checkpoint implements the hook failure/ownership, guide accounting, validation/capacity, and visible-frame repairs, plus Settings and modifier-key regressions. Native owned-window tests now demonstrate consistent DWM-visible gaps and ordinary/centered resize margins. They do not establish the complete cause of the user's reported gaps or intermittent stops. Executed results and outstanding checks belong in the living implementation plan.

## Proposed test architecture

Use the **Microsoft Native C++ Unit Test Framework** already installed with this machine's Visual Studio C++ workload. Both x86/x64 libraries, headers, and `vstest.console.exe` were found. This approved choice avoids adding a package manager or test dependency.

- Keep production `.c` sources compiled as C. Use C++ test files with C linkage declarations to call the same production code.
- Add one native test project to `ALLSNAPB.sln`, with unit and acceptance suites grouped separately and the same assertions, runner, and reporting.
- Unit tests exercise rectangle math, thresholds, settings validation, candidate collection, and lifecycle state transitions.
- Acceptance tests exercise documented user behavior through the actual production move/resize, configuration, and lifecycle paths, combining components rather than replacing the snapping algorithm with a test implementation.
- Introduce only the small seams needed to supply monitor layouts, rectangles, cursor/key state, temporary INI paths, and Win32 failures. Do not build a general mocking framework or require a wholesale engine extraction.
- Default test runs must not install global desktop hooks, modify the user's INI, move unrelated windows, or interfere with a running allSnap instance. Use test-owned windows and temporary data, restore state between cases, and serialize cases sharing mutable globals.
- Controlled Win32 integration checks and eventual interactive desktop validation complement deterministic acceptance tests. Component acceptance tests alone do not establish real desktop, DPI, mixed-process, or long-duration reliability.

Framework reference: [Microsoft's native C++ testing documentation](https://learn.microsoft.com/en-us/visualstudio/test/how-to-use-microsoft-test-framework-for-cpp?view=visualstudio).

## Behavior and acceptance priorities

1. **Confinement baseline:** moving/resizing at each work-area edge, taskbar reservations, negative monitor coordinates, adjacent/staggered monitors, oversized windows, and snap settings enabled/disabled. Record existing behavior and resolve ambiguous cases before changing it.
2. **Lifecycle:** hook/subclass success and failure, repeat enter/exit, cleanup with no active subclass, unrelated process detach, and shutdown after partial startup. A failed installation must not look healthy or leave exit blocked.
3. **Candidates/settings:** valid grids retain all monitor work areas; malformed settings cannot index outside controls or cause invalid arithmetic; capacity limits are deterministic and safe.
4. **Basic snapping:** move and resize against window/work-area edges; threshold boundaries; no candidate; ignored windows; toggle behavior; repeatability across consecutive gestures.
5. **Visible gaps:** establish the desired visible gap and coordinate system, then cover different frame types, scaling, and monitor layouts while retaining confinement tests.

Each documented acceptance scenario should have a stable identifier recorded in its corresponding test. The documents are the behavior reference; tests contain the executable setup and assertions.

## Backlog and exclusions

- **Completed in Phase 7: remove all app sound functionality.** Sound preferences/previews, playback/worker/messages, noisy API/settings, exclusive resources/assets, installer references and WinMM linkage are removed. Shared dialog/help/thread utilities with other uses remain. Obsolete INI keys are ignored without discarding other preferences. Native UI/tracking/controlled lifecycle/INI regression tests and actual binary inspection verify the increment; main app global-hook startup remains a separate manual check. Historical compiled help is retained explicitly as an archive; source help is updated. See the implementation plan for executed results and limits.
- Consider one launcher/tray UI coordinating x86/x64 helpers after lifecycle ownership is reliable. Both helper architectures remain necessary for the current injected-hook approach.
- Requested follow-up work: create a root `README.md`, organize source/resource folders, review the unexplained `ALLSNAPB` solution name, and perform an Uncle Bob Clean Code refactoring pass behind the regression suite. These are backlog items, not a reason to mix a large structural change into the current behavior repairs.
- Installer modernization, UI redesign, alternate snapping engines, broad legacy-code cleanup, and automatic restart/watchdog behavior are outside the initial repair pass.

## Completion standard

Initial repairs are complete only when both test suites pass for x86/x64, affected application builds pass, each repaired defect has meaningful coverage, and desktop-confinement checks show no regression. Record what was actually executed. Report manual/interactive coverage and unresolved stability or DPI questions separately from automated results.
