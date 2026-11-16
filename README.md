# allSnap

allSnap is a native Windows tray utility for snapping windows to other windows,
desktop work areas, center guides, grids, and their previous positions. It also
provides desktop confinement, configurable spacing, centered resizing, and MDI
window support. The application is being modernized through tested, incremental
repairs. Sound functionality has been removed.

## Build

The current build uses Visual Studio 2022 with **Desktop development with C++**,
MSVC v143 x86/x64 tools, and a Windows 10/11 SDK. The verified development
environment is 64-bit Windows 11, VS 2022 17.14, MSVC 14.44, and SDK 10.0.26100.0.
Projects select the latest installed Windows SDK in the `10.0` family.

Open [allsnap.sln](allsnap.sln), set **allsnap** as the startup project, and
choose **Win32** or **x64**. Alternatively, run these commands from the repository
root in Developer PowerShell for Visual Studio 2022:

```powershell
msbuild .\allsnap.sln /p:Configuration=Release /p:Platform=Win32
msbuild .\allsnap.sln /p:Configuration=Release /p:Platform=x64
```

The solution was previously named `ALLSNAPB.sln`; the application project was
`snapit.vcxproj`. Open the renamed solution after updating an existing checkout.
The app project is now `allsnap.vcxproj`; executable and DLL names are unchanged.

The configurations are `Debug`, `Release`, `Debug UNICODE`, and `Release UNICODE`.
All four now build Unicode; the older configuration names are retained.

| Platform | Release executable | Matching DLL |
| --- | --- | --- |
| Win32 | `Win32\Release\allsnap.exe` | `Win32\Release\snap_lib.dll` |
| x64 | `x64\Release\allsnap64.exe` | `x64\Release\snap_lib64.dll` |

Other configurations use the same `<Platform>\<Configuration>\` layout. Keep each
rebuilt EXE and its matching DLL together, including when copying a build elsewhere.
The solution also builds native test projects and their isolated-process helpers.

## Run

On 64-bit Windows, **run both versions** to cover x86 and x64 applications. The
current injected-hook design requires matching architectures; there is no single
launcher managing them yet. Each version has its own tray icon and runtime settings.

For example, after the Release builds above, start both from the repository root:

```powershell
$allSnapWorkingDirectory = (Get-Location).Path
Start-Process -FilePath .\Win32\Release\allsnap.exe -WorkingDirectory $allSnapWorkingDirectory
Start-Process -FilePath .\x64\Release\allsnap64.exe -WorkingDirectory $allSnapWorkingDirectory
```

- Right-click a tray icon for **Settings...**, **Enable Window Snapping**, or **Exit**.
- Move or resize windows normally to use snapping.
- By default, hold **Alt** to bypass snapping and confinement temporarily. Preferences selects the
  toggle key; Advanced can invert its behavior.
- By default, hold **Ctrl** while resizing to resize around the original center.
- To enable confinement, check **Constrain windows to the screen** in Advanced.
  Confinement is off in the checked-in INI profile.
- If the tray icon is hidden, run the same version again to unhide it.

Configure both running variants as needed: their live settings are not synchronized.
Tray enable/disable changes are live only until Settings Apply/OK saves them.
Exit can remain blocked while a window has active subclass ownership; finish the
move/resize operation and retry if prompted.

## Settings and configuration

| Page | Main controls |
| --- | --- |
| Preferences | Enabled state, snap destinations, toggle key, activation distances (2–100), and window spacing (0–1000 physical pixels). |
| Snap to grid | Horizontal/vertical guides, evenly spaced lines, pixel intervals, or one offset line. Grids work independently of Desktop edge snapping. |
| Advanced | Confinement, MDI snapping, inside-edge behavior, inverted toggle, tray visibility, and compatibility cropping options. |

Spacing is applied **once** between adjacent visible window edges: a setting of
8 gives an 8-pixel gap, rather than adding a margin from each window. The same
setting applies at exposed desktop work-area edges. Zero means flush edges.
Activation distance is separate; center/grid alignment guides remain unpadded.

The app reads and writes [allsnap.ini](allsnap.ini) in its **process working
directory**, which can differ from the executable directory. Visual Studio uses
the repository root. For shortcuts, set **Start in** to a writable configuration
directory. Both variants read the same file when given the same working directory,
but maintain separate live state. The checked-in profile can override DLL defaults.

Apply/OK saves preferences. Cancel discards unapplied edits; it does not undo an
earlier Apply. Saves are sequential and have no atomic rollback guarantee if a
write fails. Older sound-related INI keys are ignored without discarding other
preferences. Class lists and center/equal key values also have INI-only entries;
see [current-state.md](.ai/current-state.md) for their boundaries. Equal-size/aspect
handling and auto-lineup remain disabled.

## Tests

Unit and acceptance tests use the **Microsoft Native C++ Unit Test Framework**
installed with Visual Studio. After building the solution, they are available in
Test Explorer. The repository runner discovers the installed VS tools and requires
no package downloads:

```powershell
.\tests\run-tests.ps1
.\tests\run-tests.ps1 -Configuration Release
.\tests\run-tests.ps1 -Platform x64 -Suite Unit
.\tests\run-tests.ps1 -Platform Win32 -Suite Acceptance
```

The default runs Debug tests for both architectures. Test outputs and TRX results are
under the ignored `build/` directory. Native window tests require an active Windows
desktop. Tests use owned windows, temporary INI files, and isolated helper processes;
they do not install global desktop hooks or alter unrelated windows/settings.
Run one native test host per architecture at a time because engine state is shared
within a test DLL.

See [tests/README.md](tests/README.md) for the test boundaries. Files in
[features/](features/) document intended behavior for acceptance tests; they are
plain specifications with no Gherkin runner. Latest recorded results are in the
[implementation plan](.ai/features/20261116-modernization/implementation-plan.md).

## Known limitations

- Automated coverage does not establish actual global-hook/Explorer delivery,
  context-help rendering, keyboard navigation, mixed-DPI transitions, or extended
  desktop reliability. The reported intermittent-stop cause remains unconfirmed.
- The engine DLL stays loaded in affected processes until they exit. Exiting
  allSnap alone may not unlock a DLL for replacement; close affected applications
  or build to a different output path. The UI uses Windows compatibility scaling,
  and fractional coordinate conversion can round physical spacing.
- Oversized windows cannot always fit inside the desktop. The movement fallback
  preserves their size and keeps their origin accessible.
- Installer scripts retain historical output layouts and unsupported dependencies.
  The checked-in `legacy/archive/snapTo.chm` is an old archive; source help has been updated but
  that compiled file has not been regenerated.

## Repository guide

Production code is separated by responsibility. The solution and project files
remain at the root; build commands, output paths, debugger working directory, and
the checked-in INI location are unchanged. Use the `.vcxproj` compile lists to
identify active code.

| Folder | Contents |
| --- | --- |
| `src/app/` | Tray application, Settings, persistence, and UI adapters. |
| `src/engine/` | Hook/subclass lifecycle, snapping, confinement, and geometry. |
| `src/common/` | Shared headers, rectangle helpers, and diagnostics. |
| `resources/` | Resource scripts/headers, manifests, icons, cursors, and bitmaps. |
| `help/` | HTML/context help sources, styles, images, and help projects. |
| `packaging/` | Historical Inno Setup scripts/language files; packaging remains unvalidated. |
| `legacy/` | Inactive source, old project definitions, and archived artifacts. |
| `tests/`, `features/`, `.ai/` | Automated tests, behavior specifications, and project records. |

The archived projects in `legacy/projects/` are historical references. Build with
the root solution. Compiling `help/snapTo.hhp` still targets root `allSnap.chm`,
matching the app's existing help lookup; the archived CHM has not been rebuilt.

| Area | Starting points |
| --- | --- |
| App startup, tray, and Settings | [snap_App.c](src/app/snap_App.c), [snap_taskbar.c](src/app/snap_taskbar.c), [snap_settings.c](src/app/snap_settings.c) |
| Hook/subclass ownership and settings API | [snap_lib.c](src/engine/snap_lib.c), [SNAP_LIB.H](src/engine/SNAP_LIB.H) |
| Move/resize, candidates, confinement | [Snapper.c](src/engine/Snapper.c), [snap_WinRects.c](src/engine/snap_WinRects.c), [snap_KeepToScreen.c](src/engine/snap_KeepToScreen.c) |
| Resources and persisted preferences | [snapit.rc](resources/snapit.rc), [snap_ini.c](src/app/snap_ini.c) |
| Developer briefing and next milestones | [current-state.md](.ai/current-state.md), [implementation plan](.ai/features/20261116-modernization/implementation-plan.md), [backlog](.ai/backlog.md) |

See [LICENSE](LICENSE) for the MIT license. Historical installer material also
includes [EULA.TXT](EULA.TXT).
