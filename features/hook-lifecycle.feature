Feature: Window-hook lifecycle
These scenarios document the behavior exercised by native Microsoft acceptance tests.
The file is documentation; it is not parsed or executed by a Gherkin runner.

HL001 - Hook setup failure reaches the startup caller
Given the operating system rejects hook installation
When the application requests its window hook
Then installation reports failure with the captured operating-system error
And no installed hook or active subclass is recorded
The application's startup error display and cleanup require separate boundary verification; this component test does not establish them.

HL002 - Consecutive move sessions release their native subclasses
Given a test-owned window and a successfully established hook session
When the window enters and exits several consecutive move sessions
Then each session owns one subclass while active
And every exit releases it so a later session and normal shutdown remain possible

HL003 - Destroying a window releases its native subclass
Given a test-owned window currently being tracked
When that window is destroyed
Then WM_NCDESTROY releases its ownership exactly once
And shutdown is no longer blocked by the destroyed window

HL004 - Shutdown cannot unload a live subclass
Given a window in an active move session
When shutdown is requested
Then shutdown reports busy and keeps the hook available
When the move session exits
Then shutdown can remove the hook successfully

HL005 - Failed subclass registration cannot strand shutdown
Given a hook session whose subclass-registration operation fails
When a move session is requested
Then the failed registration leaves no active ownership
And the hook session can still shut down normally

Additional unit coverage checks removal failure/retry, repeated and unrelated cleanup,
cross-thread cleanup rejection, unrelated-process bookkeeping, and in-flight registration.
Default test operations cannot install a system-wide hook. Acceptance tests substitute
hook installation and exercise real subclass callbacks only on hidden test-owned windows.

Isolated process-lifetime acceptance contracts (Phase 5)
Each scenario uses a private copy of the actual C engine DLL shared by test-owned
child processes, native hidden windows, bounded handshakes, and substituted global
hook install/remove operations. These contracts do not certify real desktop hook
delivery or a long-running user session.

HL006 - A forcibly terminated owner cannot strand shutdown
Given a target process has reserved or installed a native window subclass,
when that target is forcibly terminated without DLL cleanup,
then the observer recognizes the dead owner and can shut down once no live owners remain.
Unknown or inaccessible owners remain busy; uncertainty is not proof of death.

HL007 - Normal process exit releases only its own registration
Given separate processes share the engine DLL and one owns a subclass,
when that owner exits normally, shutdown becomes eligible.
When an unrelated process exits, the live owner's registration remains active.

HL008 - Multiple live owners remain independent
Given two target processes have active native subclasses,
when either exits or releases its subclass, the other still blocks shutdown.
Only after the final live owner releases or exits can shutdown succeed.

HL009 - An in-flight installation reserves shutdown ownership
Given installation is paused after reserving ownership and before native installation,
when the observer requests shutdown, it reports busy and retains its hook session.
Releasing the pause permits normal installation and cleanup.
Terminating the paused process permits recovery without requiring its cleanup code.

HL010 - Callback code remains loaded through ordinary reference release
Given a target has installed the engine's native subclass,
when its ordinary LoadLibrary reference is released while a callback remains installed
or is executing through the target window procedure,
then the engine module remains mapped and the callback can finish and release ownership.
The conservative retention policy keeps the DLL loaded until the target process exits;
replacing that DLL may therefore require closing affected applications.

HL011 - A new hook host preserves existing live owners
Given a hook host exits while a separate target still owns a native subclass,
when another host starts using the same engine instance,
then it retains the live owner's shutdown protection rather than clearing all state.

HL012 - Successful shutdown closes registration admission
Given the observer has successfully stopped its hook session,
when another process tries to install a new subclass,
then registration is refused without installing a native callback.
A later successfully started session can admit new registrations.

HL013 - Hook entry retains code even without subclass ownership
Given a target loads the engine and enters its actual hook procedure,
when the ordinary DLL reference is released without installing a subclass,
then the module remains mapped until process exit and no subclass owner is invented.
The isolated test invokes a negative hook code directly without installing a desktop hook.

Loader lifetime references:
https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandleexw
https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-unhookwindowshookex
