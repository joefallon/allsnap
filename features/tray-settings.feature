Tray settings behavior

The tests call real Preferences/Advanced controllers and snap_taskbar.c. Only
Shell_NotifyIcon is substituted; tests do not add or remove an actual desktop icon.
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
