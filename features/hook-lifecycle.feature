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
Shared accounting after forced process termination remains an explicitly unverified limitation;
the bookkeeping unit tests do not constitute a real cross-process termination test.
