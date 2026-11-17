Tray settings behavior

The tests call real Preferences/Advanced controllers and snap_taskbar.c.
Shell_NotifyIcon is substituted; resource-failure cases also control resource
loading/destruction. Tests do not add or remove an actual desktop icon.
Host menu command dispatch and actual Explorer delivery require separate validation.

TR-001 - Preferences updates the enabled-state tray indication
When Preferences Apply changes enabled state, the real tray controller requests
the corresponding icon and resource tooltip from the Shell notification boundary.

TR-002 - Hide/unhide and Explorer recovery
When Advanced Apply hides the icon, the tray controller requests deletion and
does not restore it on TaskbarCreated handling. Unchecking Hide restores the icon
and enabled-state indication. A later Explorer restart restores a visible icon.

TR-003 - Opening Settings preserves recovered visibility
Given the saved icon is hidden and runtime recovery has made it visible, opening
Settings must not hide it again, even transiently. Canceling leaves runtime recovery
intact and does not overwrite the saved preference. See also INI-004.

TR-004 - Retry failed Shell registration
Given a Shell add request fails, a later Explorer restart retries the add and then
updates the icon/tooltip. A failed add must not proceed as a successful modification.

TR-005 - Initialized Shell notification packets
Add, modify, and delete requests contain initialized packet fields and only the
flags needed for that request. Fields not in use contain zero values.

TR-006 - Bounded tooltip and valid resources
Localized tooltip text uses the notification field's capacity in characters,
including its terminator. Longer text truncates safely to the available field.
Failure to load the tooltip or icon reports failure without sending a malformed
modify request, and any icon successfully loaded is released after use.

TR-007 - Failed initial refresh cleanup and retry
If add succeeds but the first icon/tooltip refresh fails, request deletion of the
empty icon and report the original failure. A later recovery can retry add/refresh.
Cleanup must not replace the error captured from the failing operation.
If the Shell also rejects deletion, cleanup is best-effort rather than a guarantee
that Explorer removed the icon.
Requested hide/unhide preference remains separate from Shell delivery success.
