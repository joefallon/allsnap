# allSnap repository state

## Purpose and stack

allSnap is a native Windows tray utility that changes how other applications' windows move and resize. It snaps to other windows and monitor work areas, with optional center/grid guides, equal-size and center-resize modifiers, MDI handling, cropping adjustments, and class-based exclusions (`snap_App.c`, `SNAP_LIB.H`, `Snapper.c`, `snap_WinRects.c`). The UI provides Preferences, Sounds, Grid, and Advanced property pages (`snap_settings.c`).

The implementation is procedural C using Win32, common controls, resources, and Windows hooks. There is no web application, database, or package-manager dependency tree. The current solution is `ALLSNAPB.sln`, containing the `allsnap` executable project (`snapit.vcxproj`) and `snap_lib` DLL project (`snap_lib.vcxproj`). Both target Visual Studio 2022 (`v143`) and the latest installed Windows 10/11 SDK (`10.0`). `LICENSE` is MIT.

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
| Snap transition tracking and sound message thread | `snap_TrackResults.c`, `snap_sounds.c`, `snap_mywm_msg.h` |
| Debug UI and contextual help | `snap_debug.c`, `snap_tips.c` |

`snapIt.c` contains an older application entry point but is **not** compiled by `snapit.vcxproj`; the active entry is in `snap_App.c`. Likewise, `snap_WinRects2.c`, `sides_old.c`, and `snap_CropTop.c` are outside the current compile lists. `.dsp`/`.dsw` and `.vcproj` files preserve older build definitions; `.bak` files are historical material. The DLL's active resource file is `snap_lib1.rc`.

## Execution and architectural boundaries

1. `_tWinMain` in `snap_App.c` checks character-set support and named mutexes, registers a window class, and creates a hidden main window. Mutex/window names differ for 32-bit and 64-bit builds. Starting another instance of the same variant can unhide the existing tray icon.
2. Startup creates the sound message thread, calls `SnapHookAll`, then loads INI settings and adds the tray icon. The application runs a Win32 message loop and hosts modeless settings/about/debug windows. Shutdown requests sound-thread exit and removes the hook.
3. `SnapHookAll` in `snap_lib.c` installs a global `WH_CALLWNDPROC` hook. On eligible `WM_ENTERSIZEMOVE` messages, it subclasses the target window with `SetWindowSubclass`. The subclass handles `WM_MOVING` and `WM_SIZING`, invoking `Snapper.c` to adjust the proposed `RECT` before normal message processing continues. Hook/subclass teardown and `SnapCanUnHook` govern safe exit.
4. `snap_WinRects.c` snapshots eligible window rectangles, monitor work areas, and configured guide candidates at move/resize entry. It filters visibility, minimization, ignored classes, and other window conditions. `snap_testers.c` evaluates threshold matches; `Snapper.c` combines these with cropping, modifier keys, and move/resize tracking.

The executable imports the DLL API in `SNAP_LIB.H`; settings setters/getters are the UI-to-engine boundary. Selected DLL globals, including settings, hook handle, and application HWND, occupy writable shared PE sections named `Shared32` or `Shared64` (`snap_lib.c`). These are shared across loaded instances of the same DLL variant; subclass bookkeeping also includes process-local state. Preserve this distinction when changing state ownership. Sound transitions use `PostThreadMessage` to the application's sound thread (`snap_TrackResults.c`, `snap_mywm_msg.h`).

## Configuration, persistence, and local integrations

- `snap_ini.c` maps `[settings]` entries to DLL/UI setters and getters. It resolves and caches `allsnap.ini` relative to **the process working directory**, not the executable directory or user profile. Launch location therefore affects settings selection and write permissions.
- Startup and opening the settings sheet load the INI; property-page Apply/OK handlers save it (`snap_App.c`, `snap_settings.c`). General shutdown saves are commented out. Missing entries leave existing/default values in place. Preserve existing key spellings such as `win_tresh` and `screen_tresh`.
- `snap_registry.c` still compiles and implements persistence under `HKCU\Software\IvanHeckman\allSnap\Settings`, but the application's startup/shutdown registry calls are commented out. Registry settings are not the current startup source.
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

All eight solution configuration/platform combinations compile and link successfully with Visual Studio 2022 17.14, MSVC 14.44, and Windows SDK 10.0.26100.0. Release binary inspection verified the EXE/DLL machine types, matching DLL imports, and embedded application manifest for both architectures. The remaining compiler warnings concern deprecated `GetVersionEx` use in `snap_App.c` and SDK `Multimon.h` compatibility stubs. Build compatibility fixes align enum names with their declarations, add explicit integer types, and use pointer-sized debug/help APIs and dialog callback results; snapping policy was not changed.

No automated test project, test runner, CI workflow, or repository build wrapper was found. `snap_testers.c` and `snap_InTests.c` implement production geometry checks, not a regression suite. Debug configurations expose a diagnostic dialog through `snap_debug.c`. No tests or application launches have been performed; modern Windows behavior, including confinement, visual gaps, and long-running stability, remains unverified.

## Modernization work in discussion

Draft requirements and the living implementation plan are in `.ai/features/20261116-modernization/project-spec.md` and `implementation-plan.md` in that directory. They propose one Microsoft Native C++ test framework for both unit and acceptance tests; its headers, x86/x64 libraries, and VSTest runner are installed locally, but no test project has been added. Root `features/*.feature` files are documentation used to write acceptance tests, with no Gherkin runner or parser.

The user identifies desktop confinement as the reliable behavior to protect. Initial proposed work is confinement regression coverage, hook/subclass failure handling, monitor/grid accounting and input validation, then diagnosis of snapping gaps. Inspection confirmed ignored hook-install failure, failed-subclass state not rolled back, shared/process-local subclass ownership inconsistency, and grid guides consuming the monitor-enumeration counter (`snap_App.c`, `snap_lib.c`, `snap_WinRects.c`). These findings have not established the cause of intermittent stops. No runtime repair has been implemented in this planning pass.

Requested backlog: remove all sound-related functionality, including its UI, thread/messages, persistence, assets, and installer references. Existing sound code remains in place; restoring notification playback is not planned. Keep this briefing aligned with actual implementation and the implementation plan aligned with progress and executed validation.

## Consequential constraints and gaps

- `snapit.rc` embeds the checked-in, architecture-neutral `allSnap.exe.manifest`, enabling common controls v6 and running as the invoking user. The executable linker does not generate a second manifest. The DLL resource uses SDK headers rather than the old MFC-only `afxres.h`.
- Optional auto-lineup support remains incomplete and is disabled in all current configurations. `snap_lineup.c` is not compiled by the executable project, and its `auto_lineup/autolineup.h` dependency is absent. Root `.lib`/`.exp` artifacts do not supply the missing source/header setup.
- Inno Setup scripts (`allSnap.iss`, `allSnapDebug.iss`, `snapTo.iss`) retain older output names/layouts and packaging dependencies, including auto-lineup/help artifacts. They are not established packaging instructions for the current Unicode outputs.
- Hooked code runs in other applications, and exit can be blocked while subclassing is active (`snap_lib.c`, `snap_App.c`). The older `knownproblems.html` documents custom move/size incompatibilities and DLL unloading issues; these are historical documented limitations, not newly reproduced failures.
- The current hook/subclass design uses separate x86 and x64 executables/DLLs for the two application architectures. No launcher coordinates them; each variant has its own tray window, settings state, and hook lifecycle (`snap_App.c`, `snap_lib.c`).
- Candidate storage is bounded by fixed arrays in `snap_WinRects.c`; changes to enumeration or guide generation must respect those limits. The source also retains Win9x/XP-era platform branches (`StdAfx.h`, `snap_App.c`). Current mixed-bitness coverage, DPI behavior, and compatibility with modern window types have not been established by runtime validation.
