Window snapping behavior reference

These scenarios document native acceptance tests. No feature-file parser is used.
Unless a scenario says otherwise, configured window spacing is zero.
Rectangles describe geometric window bounds; live visible-frame/DPI behavior requires
separate integration coverage before the reported visual-gap issue is considered fixed.

WS-001 — Screen edge
Given screen snapping with a threshold of 10 pixels,
when a window's left edge is 5 pixels inside the work-area left edge,
then that edge aligns to the work-area edge while the window retains its size.

WS-002 — Touching neighboring window
Given a target window starting at x=500 and a threshold of 10 pixels,
when a moving window's right edge reaches x=495 while their vertical ranges overlap,
then its right edge aligns to x=500; the threshold does not add permanent spacing.

WS-003 — Outside activation distance
Given the same target window and a threshold of 10 pixels,
when the moving window's right edge is x=489,
then it does not snap because it is more than 10 pixels from the desired edge.

WS-004 — Resizing
Given the same target window,
when the dragged right edge reaches x=495,
then it aligns to x=500 while the opposite edge remains fixed.

WS-005 — No destinations
Given all snap destinations and confinement disabled,
when a proposed window rectangle is close to another window,
then it remains unchanged.

WS-006 — Old position as the only destination
Given only Old position is selected as a snapping destination,
when a moving or resizing window returns within the configured threshold of its initial rectangle,
then it snaps back to that initial position or dragged edge without requiring other-window snapping.

WS-007 — Old position combined with other windows
Given Old position and Other windows are both selected,
when a drag begins,
then the initial rectangle is collected exactly once as a destination.

WS-008 — Other windows excludes the dragged window
Given Other windows is selected and Old position is not selected,
when a drag begins,
then the dragged window's initial rectangle is not added as an old-position destination.

WS-009 — Inclusive activation boundary
Given an activation distance of 10 pixels and zero spacing,
when the right edge reaches exactly 10 pixels from a neighboring left edge,
then snapping activates and aligns the edges.

WS-010 — Old position with inside-edge exclusion
Given Old position is selected and ordinary inside-edge snapping is disabled,
when a window or dragged resize edge returns within the activation threshold,
then the saved initial same-side edge remains an active destination.

WS-011 — Old position is not an adjacent window
Given Old position is selected with a positive spacing,
when a window moves near the opposite side of its old rectangle,
then it does not snap beside a phantom neighboring window at that rectangle;
Old position aligns corresponding initial edges only, without spacing padding.
