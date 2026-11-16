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
placement solver. Deterministic component coverage does not replace interactive
mixed-process lifetime, mixed-DPI transitions, or long-running desktop validation.
