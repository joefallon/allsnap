# allSnap repository state

## Purpose and stack

allSnap is a native Windows tray utility that changes how other applications' windows move and resize. It snaps to other windows and monitor work areas, with optional center/grid guides, a center-resize modifier, MDI handling, cropping adjustments, and class-based exclusions (`snap_App.c`, `SNAP_LIB.H`, `Snapper.c`, `snap_WinRects.c`). The UI provides Preferences, Sounds, Grid, and Advanced property pages (`snap_settings.c`).

The implementation is procedural C using Win32, common controls, resources, and Windows hooks. There is no web application, database, or package-manager dependency tree. The current solution is `ALLSNAPB.sln`, containing the `allsnap` executable (`snapit.vcxproj`), `snap_lib` DLL (`snap_lib.vcxproj`), native Microsoft tests (`tests/allsnap_tests.vcxproj`), and two test-only process-fixture projects under `tests/process/`. They target Visual Studio 2022 (`v143`) and the latest installed Windows 10/11 SDK (`10.0`). `LICENSE` is MIT.

## Repository map and active code

The repository is flat: application code, DLL code, resources, help, and older artifacts share the root. Use the `.vcxproj` compile lists to identify active code; filenames alone are misleading.

| Area | Starting points |
| --- | --- |
| Application entry, hidden main window, tray commands, shutdown | `snap_App.c`, `snap_App.h` |
| Tray icon and Explorer restart handling | `snap_taskbar.c` |
| Settings UI and dialog resources | `snap_settings.c`, `snapit.rc`, `resource.h` |
| Current persistence and UI-to-DLL grid adapters | `snap_ini.c`, `snap_grid.c` |
| Hook installation, subclass lifecycle, DLL settings/defaults | `snap_lib.c`, `SNAP_LIB.H`, `snap_lib_internal.h` |
| Move/resize orchestration and candidate collection | `Snapper.c`, `snap_WinRects.c` |
| Snap matching and geometry helpers | `snap_testers.c`, `snap_InTests.c`, `SIDES.C`, `snap_CenterSize.c`, `snap_EqualSize.c`, `snap_Crop.c`, `snap_KeepToScreen.c` |
| Visible frame/DPI conversions and physical spacing | `snap_frame.c`, `snap_Crop.c` |
| Held-key interpretation and failure diagnostics | `snap_keys.c`, `snap_diagnostics.c` |
| Isolated-process lifecycle harness and acceptance contracts | `tests/process/`, `tests/support/ProcessFixture.h`, `tests/acceptance/process_lifetime_acceptance.cpp`, `features/hook-lifecycle.feature` |
| Snap transition tracking and sound message thread | `snap_TrackResults.c`, `snap_sounds.c`, `snap_mywm_msg.h` |
| Debug UI and contextual help | `snap_debug.c`, `snap_tips.c` |

`snapIt.c` contains an older application entry point but is **not** compiled by `snapit.vcxproj`; the active entry is in `snap_App.c`. Likewise, `snap_WinRects2.c`, `sides_old.c`, and `snap_CropTop.c` are outside the current compile lists. `.dsp`/`.dsw` and `.vcproj` files preserve older build definitions; `.bak` files are historical material. The DLL's active resource file is `snap_lib1.rc`.

## Execution and architectural boundaries

1. `_tWinMain` in `snap_App.c` checks character-set support and named mutexes, registers a window class, and creates a hidden main window. Mutex/window names differ for 32-bit and 64-bit builds. Starting another instance of the same variant can unhide the existing tray icon.
2. Startup creates the sound message thread, checks `SnapHookAll`, then loads INI settings and adds the tray icon. Hook failure reports an error and unwinds initialization. The application runs a Win32 message loop and hosts modeless settings/about/debug windows; message-loop failure is logged. Explicit exit stays open if hook removal fails; successful shutdown requests sound-thread exit and removes the hook.
3. `SnapHookAll` in `snap_lib.c` installs a global `WH_CALLWNDPROC` hook. On eligible `WM_ENTERSIZEMOVE` messages, it subclasses the target window with `SetWindowSubclass`. The subclass handles `WM_MOVING` and `WM_SIZING`, invoking `Snapper.c` to adjust the proposed `RECT` before normal message processing continues. Hook/subclass teardown and `SnapCanUnHook` govern safe exit.
4. `snap_WinRects.c` snapshots eligible window rectangles, monitor work areas, and configured guide candidates at move/resize entry. Work-area accounting is separate from guide capacity. It filters visibility, minimization, ignored classes, and other window conditions. `snap_testers.c` evaluates inclusive threshold matches; `Snapper.c` combines these with visible-frame cropping, held modifier keys, and move/resize tracking. `snap_KeepToScreen.c` and `snap_InTests.c` validate the full proposed rectangle against the work-area union, preserving open internal seams and rejecting gaps or holes.

The executable imports the DLL API in `SNAP_LIB.H`; settings setters/getters are the UI-to-engine boundary. Settings, application HWND, stopping state, and 64 registration-owner records occupy writable shared PE sections named `Shared32` or `Shared64` (`snap_lib.c`). The installing host owns its process-local hook handle; each injected process owns its local subclass HWND/thread. Atomic generation-tagged claims reserve admission before native subclass installation; published records contain full process/thread creation identities. Shutdown discards only confirmed dead or replaced owners, retaining live or uncertain records. Exact-token cleanup cannot clear a reused slot. Host restart preserves live registrations. Process detach performs only local writes and atomic release, without probing, waiting, or User32 calls under loader lock.

Host, hook, and subclass entry pin the engine DLL with `GetModuleHandleEx(PIN | FROM_ADDRESS)` before admitting native callbacks. It remains mapped until that process exits, including after ordinary reference release or hook removal; replacing a loaded binary may require closing affected applications or building to a different path. The existing one-active-subclass-per-process geometry restriction is preserved. Sound transitions still use `PostThreadMessage` to the application's sound thread (`snap_TrackResults.c`, `snap_mywm_msg.h`).

## Configuration, persistence, and local integrations

- `snap_ini.c` maps `[settings]` entries to DLL/UI setters and getters. It resolves and caches `allsnap.ini` relative to **the process working directory**, not the executable directory or user profile. Launch location therefore affects settings selection and write permissions.
- Startup and opening the settings sheet load the INI; property-page Apply/OK handlers save it (`snap_App.c`, `snap_settings.c`). General shutdown saves are commented out. Missing entries leave existing/default values in place. Preserve existing key spellings such as `win_tresh` and `screen_tresh`.
- `snap_registry.c` still compiles and implements persistence under `HKCU\Software\IvanHeckman\allSnap\Settings`, but the application's startup/shutdown registry calls are commented out. Registry settings are not the current startup source.
- Preferences now include one spacing value (0–1000 physical pixels, default 0), persisted as `window_spacing`; adjacent windows use the value once, while center/grid guides remain unpadded. The enable checkbox also persists as `enabled`. Legacy INI files without these keys retain defaults. Moving/resizing uses DWM visible bounds intersected with explicit regions; manual top crop is an absolute minimum. Physical/logical conversion rounds to representable caller coordinates at fractional scales. The user approved retaining Windows compatibility DPI scaling for the UI; geometry conversions remain explicit in the engine.
- Hook/subclass failures post messages to the application thread, which logs captured error codes to `%LOCALAPPDATA%\allSnap\diagnostics32.log` or `diagnostics64.log`. Hook callbacks do not write logs synchronously. Startup hook failure is reported; failed explicit unhook keeps the application open.
- The checked-in `allsnap.ini` contains absolute sound paths from a previous developer's machine. DLL defaults are in `snap_lib.c`; do not treat this INI as portable defaults.
- Sound settings and previews exist, but normal playback in `SnapSounds_Play` is commented out; preview playback remains active (`snap_sounds.c`).
- Help sources are the root HTML files and `snapTo.hhp`, which targets `allSnap.chm`; the checked-in compiled help is named `snapTo.chm`. The application Help command opens `allSnap.chm`. Debug builds link the SDK's `htmlhelp.lib` for contextual popup help; Release configurations retain `NOHTMLHELP`, disabling those popups (`snap_tips.c`).

## Development and validation

Install Visual Studio 2022's **Desktop development with C++** workload, including the MSVC v143 x86/x64 tools and a Windows 10/11 SDK. No MFC installation is required. Open `ALLSNAPB.sln`, choose the `allsnap` executable as startup project, and select `Win32` or `x64`. Configurations are `Debug`, `Release`, `Debug UNICODE`, and `Release UNICODE`; all now build Unicode. The older names remain for compatibility, with the same behavior as their unsuffixed equivalents. This matches `CheckCharSet` in `snap_App.c`, which rejects non-Unicode builds on Unicode-capable Windows.

The corresponding MSBuild commands, from a developer shell with the configured toolchain available, are:

```powershell
msbuild .\ALLSNAPB.sln /p:Configuration=Release /p:Platform=Win32
msbuild .\ALLSNAPB.sln /p:Configuration=Release /p:Platform=x64
```

Outputs go to `<Platform>\<Configuration>\`: `allsnap.exe` and `snap_lib.dll` for Win32, `allsnap64.exe` and `snap_lib64.dll` for x64. Each project uses its own `obj\<ProjectName>\` intermediate directory beneath that output directory. The executable's project reference links the matching DLL import library, including in Debug; no Release build is required first. Visual Studio's debugger working directory is the repository root to select the checked-in INI. Both EXE and DLL are needed when copying a build elsewhere.

All eight solution builds passed after the Phase 5 lifetime repairs: Debug, Release, Debug UNICODE, and Release UNICODE on Win32 and x64, using Visual Studio 2022 17.14, MSVC 14.44, and Windows SDK 10.0.26100.0. Earlier Release binary inspection verified the EXE/DLL machine types, matching DLL imports, and embedded application manifest for both architectures. The remaining compiler warnings concern deprecated `GetVersionEx` use in `snap_App.c` and SDK `Multimon.h` compatibility stubs. Build compatibility fixes align enum names with their declarations, add explicit integer types, and use pointer-sized debug/help APIs and dialog callback results. Runtime repairs and their current verification are described below.

The native Microsoft C++ test project is `tests/allsnap_tests.vcxproj`; `tests/run-tests.ps1` builds/runs Unit and Acceptance suites on Win32/x64 without package downloads. Current verification: **222/222 passed in Debug and Release on each architecture** (82 unit, 140 acceptance, including 12 isolated-process methods); separate Release suite filters also passed on both architectures. The C engine is reused directly with controlled monitor/window inputs, actual native dialog controllers/resources and temporary INI files, and test-owned framed windows queried through DWM. An independent GDI-region oracle checks complete desktop coverage. The process harness builds dependencies automatically, gives each scenario a private engine-DLL copy shared by its child processes, and exercises actual DllMain, native subclass callbacks, process exit/termination, and loader retention. Global hook installation/removal is substituted. Owned jobs and bounded handshakes clean up helpers; user settings and unrelated windows remain untouched. Native window tests require an active Windows desktop. Debug configurations expose a diagnostic dialog through `snap_debug.c`. Actual global-hook delivery, mixed-DPI transitions, and long-running desktop behavior remain unverified.

## Modernization work in progress

Approved requirements and the living implementation plan are in `.ai/features/20261116-modernization/project-spec.md` and `implementation-plan.md` in that directory. The first repair checkpoint includes the test foundation, lifecycle failure handling, monitor/grid/input and Settings repairs, visible-frame calculations, configurable spacing, and complete work-area union confinement. Root `features/*.feature` files are documentation used to write acceptance tests, with no Gherkin runner or parser.

The user committed that checkpoint and authorized the isolated process-lifetime increment. Three genuine child-process regressions were reproduced before repair: forced target death stranded shutdown, and ordinary DLL-reference release unloaded installed or executing callback code. The repaired registry and retention policy pass the isolated-process cases, including death during reserved installation, independent live/dead owners, host restart, and cross-process shutdown admission. Independent source/harness review found no required corrections. Manual desktop validation has not been performed, and these results do not establish the complete cause of the reported intermittent stops.

The user explicitly considers every feature, including confinement, untrusted. Acceptance contracts describe intended behavior and must fail defects; legacy output is not the oracle. Regression failures demonstrated defects before the repairs, and independent review added cases missed by the first green suite. Confinement now checks full rectangle coverage, including holes, disconnected gaps and exact boundary coincidences; ordinary resizing preserves fixed outer edges, while Ctrl resizing preserves the original center and validates reflected opposite-side snaps. Settings Apply acknowledges save failures and successful retries correctly, and opening Settings preserves an unsaved tray enable/disable toggle. These changes have not established the complete cause of intermittent stops or certified every Settings option.

The canonical backlog is `.ai/backlog.md`. Next priorities are real desktop/process-lifetime validation and remaining Settings coverage. Requested later milestones cover all sound removal, root README, source-folder organization, ALLSNAPB naming, and incremental Clean Code refactoring. Existing sound code remains in place; restoring notification playback is not planned. One launcher/tray managing both architectures is proposed. The user handles commits. Keep this briefing and the implementation plan aligned with actual progress and executed validation.

## Consequential constraints and gaps

- `snapit.rc` embeds the checked-in, architecture-neutral `allSnap.exe.manifest`, enabling common controls v6 and running as the invoking user. The executable linker does not generate a second manifest. The DLL resource uses SDK headers rather than the old MFC-only `afxres.h`.
- Optional auto-lineup support remains incomplete and is disabled in all current configurations. `snap_lineup.c` is not compiled by the executable project, and its `auto_lineup/autolineup.h` dependency is absent. Root `.lib`/`.exp` artifacts do not supply the missing source/header setup.
- Inno Setup scripts (`allSnap.iss`, `allSnapDebug.iss`, `snapTo.iss`) retain older output names/layouts and packaging dependencies, including auto-lineup/help artifacts. They are not established packaging instructions for the current Unicode outputs.
- Hooked code runs in other applications, and exit can be blocked while subclassing is active (`snap_lib.c`, `snap_App.c`). The older `knownproblems.html` documents custom move/size incompatibilities and DLL unloading issues; these are historical documented limitations, not newly reproduced failures.
- The current hook/subclass design uses separate x86 and x64 executables/DLLs for the two application architectures. No launcher coordinates them; each variant has its own tray window, settings state, and hook lifecycle (`snap_App.c`, `snap_lib.c`).
- Equal-size/aspect-ratio handling remains disabled by the absent `SIZINGTOGGLE_ENABLED` build flag. Some legacy options are connected but still lack complete acceptance or interactive coverage; a populated Settings dialog is not proof every feature works.
- Candidate storage is bounded by fixed arrays in `snap_WinRects.c`: ten work areas and one hundred additional guides. Changes to enumeration or guide generation must respect those limits. The source also retains Win9x/XP-era platform branches (`StdAfx.h`, `snap_App.c`). Automated geometry and both-architecture coverage do not establish compatibility with every modern application window.
- Lifecycle ownership is bounded at 64 records and refuses new admission on exhaustion. Unknown owner access keeps shutdown busy. If an owner dies before identity publication and its thread ID is already reused, recovery remains conservative until death can be proved; published identities guard ID reuse with full creation times. Pinned DLLs cannot be unloaded for replacement while affected processes remain alive. Real global-hook/desktop validation remains necessary.
- An original center outside the desktop can make centered full-fit confinement impossible. Real mixed-DPI/100% hardware checks and a long-running desktop trial remain pending.
