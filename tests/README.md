# Native tests

Unit and component acceptance tests use the Microsoft Native C++ Unit Test
Framework installed with Visual Studio 2022's Desktop development with C++ workload.
No packages are downloaded by the test build or runner.

From the repository root:

```powershell
.\tests\run-tests.ps1
.\tests\run-tests.ps1 -Platform x64 -Suite Unit
.\tests\run-tests.ps1 -Platform Win32 -Suite Acceptance
.\tests\run-tests.ps1 -Configuration Release
```

The default builds and runs Debug tests for Win32 and x64 sequentially. Test DLLs
and TRX results go under ignored `build/` directories. All tests are also available
in Visual Studio's Test Explorer after building `allsnap_tests` in `ALLSNAPB.sln`.

The project compiles production C sources as C, with C++ used only for the native
test harness. Acceptance tests combine actual matching, confinement, and
move/resize code with supplied monitor/window rectangles. Scenario identifiers
map to documentation in root `features/`; the files are not parsed at runtime.

Tests do not install global hooks, move unrelated windows, or change the user's
INI. Mutable engine state is reset between tests. Run a single native test host
per architecture at a time because production engine globals are shared within
that test DLL. Native dialog and visible-frame cases require an active Windows
desktop and use hidden or nonactivating, offscreen test-owned windows. The
confinement oracle uses GDI region union/difference independently of the production
placement solver. Automated coverage does not replace actual global-hook delivery,
mixed-DPI transitions, or long-running desktop validation.

Settings acceptance tests create the actual modeless property sheet with its
production resources and controllers, then hide the owned sheet. Apply/OK/Cancel,
tab validation, save failure/retry, and functional non-sound controls run through
native property-sheet notifications. Tray tests compile the real tray controller
and substitute only Shell_NotifyIcon; no icon is added to the user's taskbar.
Window-filter tests use owned native top-level/MDI windows and the production
collector with controlled work areas. Settings fixtures restore crop/key/class
state and use temporary INI files. Context-help rendering, keyboard navigation,
actual Explorer delivery, and audio are outside these automated checks.

Process-lifetime acceptance tests build `tests/process/allsnap_process_engine.vcxproj`
and `allsnap_process_helper.vcxproj` through test-project references. Each scenario
copies the dedicated engine DLL into a private directory under the ignored test
output and loads that same copy into separate owned helper processes. The DLL
reuses production C sources and the actual production DllMain; global hook
installation/removal is substituted, while native subclassing and loader behavior
remain real. Tests terminate only their own helpers and use owned cleanup jobs.
No currently running allSnap instance or production DLL is used by this harness.
The production retention policy pins the engine until its process exits. Component
tests substitute pinning so their test assembly remains unloadable; the dedicated
process engine uses the real Windows pin operation and exits with its owned helper.
