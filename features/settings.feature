Settings behavior reference

These are documentation scenarios for Microsoft native acceptance tests. Tests use
the actual settings resources/controllers and production INI code with hidden,
test-owned dialogs and temporary INI files. Audio and tray APIs are boundary
adapters, so these tests do not establish actual audio or tray behavior.

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
Given any existing Preferences, Sounds, Grid, or Advanced page,
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
