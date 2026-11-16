Settings behavior reference

These are documentation scenarios for Microsoft native acceptance tests. Tests use
the actual settings resources/controllers and production INI code with hidden,
test-owned dialogs and temporary INI files. System Shell notifications are
intercepted, so these tests do not establish actual system tray behavior.
ST-010 onward also create the actual native modeless PropertySheet;
its owned windows are hidden, and no global hooks or user INI files are used.

ST-001 — Initialize Preferences
Given configured thresholds, snap destinations, and a selected toggle key,
when the Preferences resource page initializes,
then its actual edit controls, checkboxes, and combo selection show those values.

ST-002 — Apply Preferences
Given edited Preferences controls,
when Apply is delivered to the actual page handler,
then engine values change and threshold/destination/toggle/visible-spacing values
and enabled/disabled state reload from the temporary INI.

ST-003 — Cancel pending edits
Given edits not yet applied in Preferences or Advanced,
when Cancel is delivered,
then engine state stays unchanged and the INI receives no pending values.

ST-004 — Apply Advanced
Given MDI, inverted toggle, inside-edge exclusion, icon hiding, confinement,
region exclusion, and top-crop selections,
when Apply is delivered,
then direct and inverse checkbox values map correctly and reload from the INI.

ST-005 — Apply grid
Given horizontal pixel spacing and a vertical single-line offset,
when Apply is delivered and settings reload,
then both modes, enable flags, and non-boolean integer values are preserved.

ST-006 — Invalid threshold or spacing
Given a Preferences threshold below its minimum, or spacing larger than the
supported 0..1000 pixel range,
when leaving the page invokes its validation,
then a validation message is requested and the page remains active without
changing the corresponding engine value.

ST-007 — Persistence failure
Given an INI destination that is a directory instead of a writable file,
when Apply attempts the actual save,
then a failure message is requested and the page is kept open instead of
reporting that settings were saved. Tests intercept only the message display.

ST-008 — Apply acknowledgement and retry on every page
Given any existing Preferences, Grid, or Advanced page,
when its actual INI save fails, its dialog procedure acknowledges the notification
and sets the property-sheet result to reject the Apply.
When the destination is corrected and Apply is retried, the procedure acknowledges
the notification, clears the previous failure result, and permits successful Apply.
Acknowledgement is the dialog procedure's return value; the notification outcome
is a separate DWLP_MSGRESULT value, not the return from SendMessage.
Contract reference: https://learn.microsoft.com/en-us/windows/win32/controls/psn-apply

ST-009 — Open Settings after toggling enabled state
Given the current runtime enabled state differs from the saved INI value,
when Settings opens, the runtime state and Preferences checkbox retain the current
enabled state. Canceling pending checkbox edits changes neither that runtime state
nor the saved INI value. Both enabled and disabled runtime states are covered.
Only property-sheet creation is replaced with a hidden actual Preferences page;
the production opening path, resource, controller, and INI reload are exercised.

ST-010 — Native Apply followed by Cancel
Editing Preferences enables the native Apply button. Apply updates the engine and
temporary INI, clears dirty state, and keeps Settings open. Cancel discards edits
made after that successful Apply.

ST-011 — Native OK across pages
Edits on Preferences, Grid, and Advanced are applied by native OK notifications.
The modeless sheet reports closure, and edited values reload from its temporary INI.

ST-012 — Native Cancel across pages
Cancel discards unapplied edits on all visited settings pages without changing
engine values or the saved settings.

ST-013 — Native Preferences validation and spinner endpoints
Invalid Preferences numbers prevent tab switching and OK. Correcting them permits
navigation. Both threshold spinners support the same inclusive 2..100 range as
manual threshold input. Spacing supports 0..1000 pixels.

ST-014 — Selected grid input validation
Each enabled grid axis requires a whole number in its selected mode: EVEN accepts
0..100 lines, PIXELS accepts 1..2147483647 pixels, and SINGLE accepts an offset of
0..2147483647 pixels. Empty, malformed, negative, and overflowing input is rejected.
Native tab changes and Apply retain invalid edits for correction without changing
the corresponding engine or saved grid settings.

ST-015 — Disable and retain a grid axis
Disabling horizontal or vertical grid snapping retains that axis's mode and value
for future re-enabling. Applying the disabled state preserves those settings.

ST-016 — Advanced top-crop input
Enabled top cropping requires a whole number from 0 to 2147483647. Invalid input
blocks native navigation and Apply without changing the crop setting. Zero turns
manual top cropping off. The crop checkbox enables/disables its edit control.

ST-017 — Complete Preferences numbers
Preferences rejects trailing nonnumeric characters and accepts both threshold and
spacing endpoints. These tests exercise actual native edit controls and validation.

ST-018 — Native save failure and retry
When the INI destination is unwritable, native Apply and OK report the failure and
keep Settings open. Correcting the destination permits retry, persistence, and
normal dirty-state clearing or closure.

ST-019 — All Preferences selections
Every destination checkbox and enabled state can be applied on and off. All four
toggle-key selections signal a change and apply through the actual native sheet.

ST-020 — All Advanced selections
Every live Advanced checkbox applies and restores its direct or inverted engine
value. The unfinished desktop-icon alignment option remains hidden.

ST-021 — Grid axis controls
Horizontal and vertical checkboxes enable and disable only their own radio buttons
and edit controls.

ST-022 — All grid modes and valid endpoints
Both axes support all three modes and their valid endpoints through native Apply.
Unselected edits do not invalidate the selected mode. System Shell notifications
remain intercepted; these scenarios establish controller/persistence behavior.

ST-023 — Settings without sound functionality
The actual native sheet contains exactly Preferences, Snap to grid, and Advanced,
in that order. Each tab opens the corresponding retained controls. The retired
Sounds dialog resource (169) is absent from the application resource image.
Existing native Apply, OK, Cancel, validation, save-failure/retry, and persistence
scenarios run against the three-page sheet without audio fixture adapters.
