Input and toggle behavior reference

These scenarios document native Microsoft acceptance tests. The tests provide
key-state values through a test-only reader, never press keys or change global
keyboard state, and do not install global hooks or move unrelated windows.

IN-001 — Default snapping toggle
Given the normal toggle mode and a window near a desktop edge,
when the configured key is released, snapping applies;
when held, snapping is bypassed;
when recently pressed but now released, snapping applies again.

IN-002 — Inverted snapping toggle
Given inverted toggle mode,
when the configured key is held, snapping applies;
when released, including a recent-press-only state, snapping is bypassed.

IN-003 — Center-resize modifier
Given the configured center-resize key,
when held, center resizing is active;
when released, including a recent-press-only state, it is inactive.
This does not enable the unfinished equal-size/aspect-ratio modifier.

IN-004 — Disabled snapping
Given Enable Snapping is disabled,
when the production hook dispatcher receives a move-entry message for a test-owned window,
then it does not attempt to install a subclass.

IN-005 — Inside-edge selection
Given a nearby target window with overlapping vertical range,
when inside-edge snapping is enabled, same-side edges can align;
when inside-edge exclusion is selected, those candidates do not alter the rectangle.
