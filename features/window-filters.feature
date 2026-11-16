WF-001 — Ignored classes exclude complete destination class names
Given a semicolon-separated ignored_classes setting,
when top-level or MDI sibling windows are collected as snap destinations,
then an exact class-name token excludes that destination, including native names
longer than the old 19-character retrieval buffer.
A class sharing only a listed prefix remains eligible; empty tokens do not match.
Clearing the setting restores eligibility.
This setting excludes destinations, while the window's own gestures remain eligible.

WF-002 — Skinned classes bypass only manual top cropping
Given manual top cropping is enabled,
when a window's complete class name appears in skinned_classes,
then its manual top crop is suppressed, including long native class names.
Clearing the class list enables its manual crop again.
Automatic visible-frame and clipping-region bounds retain their separate behavior.

WF-003 — MDI candidates follow normal destination rules
Given MDI snapping is enabled and an active MDI child has sibling windows,
when candidates are refreshed,
then ignored, hidden, and minimized siblings are excluded, the active child is excluded,
and included siblings use the same visible-frame/manual-crop bounds as other windows.
The MDI client's work area is expressed in desktop coordinates.

WF-004 — The MDI option controls admission
Given a native MDI child starts a size/move gesture,
when Snap MDI windows is disabled, then no subclass registration is admitted.
When it is enabled, the same child can be admitted and cleaned up normally.
Acceptance tests call the production hook entry with owned native windows;
they do not install a system-wide hook or alter another application's windows.

WF-005 — Manual cropping cannot empty or invert a visible window
Given a positive-height visible window and a configured manual top crop,
when the configured crop exceeds that window's available visible height,
then its effective crop retains at least one visible pixel row for moving and sizing.
The stored nonnegative setting remains unchanged so taller windows can use it.

WF-006 — Invalid class-list lengths do not replace valid settings
Given an existing ignored or skinned class list,
when an invalid negative list length is supplied,
then the existing setting is preserved. An explicitly empty valid list clears it.
