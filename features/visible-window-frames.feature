Feature: Snap and constrain visible window frames
  Window manager resize borders are not intentional user spacing.
  These scenarios use test-owned offscreen framed windows and native DWM queries.

  VF-001: Visible desktop margin
    Given a framed window with an invisible resize border
    And configured desktop spacing of 8 pixels
    When the window is constrained at an exposed desktop work-area edge
    Then its visible frame has that margin, with no additional resize-border gap
    And its dimensions are preserved

  VF-002: Consistent move and resize bounds
    Given the same framed window
    When its moving and resizing bounds are calculated
    Then both use the same visible right and bottom borders
    And conversions back to outer window coordinates preserve the signed border offsets

  VF-003: DPI and unavailable compositor
    Given caller coordinates differ from physical desktop pixels
    When visible insets and configured physical spacing are converted
    Then they use the caller's coordinate scale
    And the caller's DPI context is restored on success and failure
    And unavailable visible-frame bounds retain the safe legacy fallback

  VF-004: Collapsed spacing between two native visible frames while moving
    Given two test-owned offscreen framed windows with native DWM bounds
    When the moving window snaps beside the other through crop, matching, and uncrop
    Then configured spacings of 0, 8, and 24 physical pixels each produce exactly one visible gap
    And the moving window keeps its dimensions

  VF-005: Collapsed spacing between two native visible frames while resizing
    Given the same two native framed windows
    When the active window's right edge snaps through crop, matching, and uncrop
    Then the resulting visible gap is exactly the configured 0, 8, or 24 physical pixels
    And the anchored left edge is preserved

  VF-006: Manual top crop is an absolute minimum
    Given a visible-frame top inset of 8 pixels and a manual top crop of 3 pixels
    When moving and sizing bounds are calculated and converted back
    Then the effective top inset is 8 pixels rather than 11
    And a manual minimum greater than the visible-frame inset replaces that minimum instead of adding to it

  VF-007: Visible frame and explicit window region intersect
    Given explicit region insets and visible-frame insets
    When moving and sizing crop bounds are calculated
    Then each edge uses the more restrictive inset without adding the two insets
    And uncropping restores that same signed offset
