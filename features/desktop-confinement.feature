Desktop confinement behavior reference

These scenarios are documentation for native acceptance tests, not executable Gherkin.
The tests call production candidate matching, confinement, and move/resize code with
supplied work areas; they do not install desktop hooks or move unrelated windows.

DC-001 — Outside edges
Given a 1920 by 1040 work area and confinement enabled, with proximity snapping off,
when a 200 by 200 window is moved beyond the left, right, or top outside edge,
then it returns to the work-area boundary without changing width or height.

DC-002 — Taskbar reservation
Given the desktop work area ends at y=1040 above a reserved taskbar region,
when a window is moved below that boundary,
then its bottom edge ends at y=1040 and its size is preserved.

DC-003 — Negative virtual coordinates
Given a work area from (-1920,-200) to (0,880),
when a window is moved beyond its left edge,
then its left edge is limited to -1920, not incorrectly limited to zero.

DC-004 — Multiple monitors
Given adjacent or vertically staggered work areas,
when a window straddles their connected overlapping area,
then the internal seam does not constrain it as an outside desktop boundary.

DC-005 — Oversized window
Given a window wider than the work area,
when its origin is moved just beyond the left boundary,
then confinement preserves its dimensions and leaves its origin accessible.
Other oversized-window policies remain subject to explicit examples before changes.

DC-006 — Resize confinement
Given confinement enabled,
when the right or bottom edge is resized past the work-area boundary,
then the dragged edge is limited and the opposite edge stays fixed.

DC-007 — Confinement disabled
Given confinement and proximity snapping disabled,
when a window moves beyond a work-area edge,
then the proposed rectangle remains unchanged.

DC-008 — Consecutive gestures
Given confinement enabled,
when outside-edge and fully-inside gestures alternate repeatedly,
then each gesture produces the same result without carrying stale snap geometry.

DC-009 — Spacing does not create barriers between connected monitors
Given two monitor work areas sharing a complete edge and spacing of 8 pixels,
when a window approaches, straddles, occupies, or resizes across that internal seam,
then its proposed geometry is retained while it stays inside the desktop union.
This also applies at negative coordinates and to a narrow window inside the seam.

DC-010 — Staggered monitor edges are handled by segment
Given staggered work areas that share only part of an edge and spacing of 8 pixels,
when a window moves or resizes across their connected overlap,
then the seam remains open.
When the same window reaches an exposed portion or concave corner,
then the exposed desktop border retains its 8 pixel margin.
A neighbor covering one segment must not remove padding from the entire side.

DC-011 — A fully off-desktop corner move recovers safely
Given a valid desktop work area and a window moved completely beyond two outside edges,
when its complete proposed rectangle is outside the desktop union,
then the window is translated to the nearest valid position without changing its size.
The nearest position minimizes total horizontal plus vertical movement; equally near
valid placements are both acceptable, and a fitting rectangle may span connected work areas.
The configured margin is retained wherever the window fits.
For a window too large to fit, this fallback keeps its top-left origin reachable
and preserves its dimensions; this does not define every oversized-window policy.

DC-012 — Missing desktop information is not fabricated
Given confinement is enabled but no work areas are available,
when a window moves or resizes,
then confinement leaves its proposed geometry unchanged instead of using an unknown rectangle.

DC-013 — Complete rectangle coverage and nearest valid placement
Given staggered or disconnected work areas,
when a proposed rectangle has corners inside the desktop but crosses an unavailable gap or hole,
then confinement requires its whole rectangle plus margin to fit inside the work-area union.
The nearest valid translation wins over a distant monitor that merely shares one projection.
For A=(0,0,1000,800), B=(1000,200,2000,1000), S=8, and input=(810,850,1010,950),
the nearest result is (810,692,1010,792): its expanded bounds (802,684,1018,800)
fit A+B, and moving up 158 pixels is closer than moving right 198 pixels into B alone.

DC-014 — Resize respects the complete union and fixed opposite edges
Given fixed opposite resize edges and a requested right, bottom, or corner resize,
when the resulting rectangle would bridge an unavailable gap or leave a corner outside,
then the dragged edges are clamped to the nearest complete valid rectangle.
Opposite edges remain fixed. If their positions make any complete fit impossible,
confinement does not invent a translation of those fixed edges.

DC-015 — Confinement validates the proposed proximity snap
Given confinement and other-window snapping are both enabled,
when proximity snapping would pull an initially valid window beyond the desktop,
then confinement corrects the complete snapped rectangle to a valid desktop position.
The same priority applies to the dragged edge of a resize without moving its opposite edge.

DC-016 — Centered resizing preserves its center while respecting confinement
Given Ctrl center resizing and confinement are enabled,
when a left, right, top, bottom, or corner resize would extend beyond the desktop,
then both affected sides resize symmetrically around the original window center
to the nearest size whose visible rectangle fits the complete desktop and margin.
The nearer of either opposing desktop border limits expansion, including asymmetric
work areas and a proximity snap whose reflected opposite side would leave the desktop.
This also applies when a right/bottom gesture matches the left/top side instead:
confinement validates the side selected by matching and its final centered reflection.
Zero and 8 pixel margins apply to native framed windows as well as borderless windows;
signed frame insets must not move the original outer-window center.
Ordinary resizing continues to hold its opposite edges fixed.
When an anchored outer resize border extends beyond the desktop but its visible
edge is exactly at the configured margin, the visible fixed edge remains valid
and the dragged visible edge is still clamped to the opposite desktop border.

DCO-001/002 — Independent coverage oracle
Native acceptance tests also compare 236 fitting-window placements against a separate
GDI-region union/difference oracle, including adjacent, staggered, disconnected,
four-monitor hole layouts, zero/8-pixel margins, and translated negative coordinates.
This reference does not call the production exposed-edge or placement solver.

DCO-003 — A gap bounded exactly by monitor edges is unavailable
Given disconnected work areas ending at x=800 and starting at x=1000,
when a window exactly occupies the gap from x=800 to x=1000,
then coincident boundary corners do not make its uncovered interior valid.
Confinement moves the complete window into a work area, preserving its size.
The independent GDI-region oracle also verifies this placement.
