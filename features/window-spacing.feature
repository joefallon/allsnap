Feature: Consistent configurable window spacing
  One setting controls intentional spacing in pixels. It is independent of
  the snap activation distance and defaults to zero.

  SP-001: Collapsed spacing between windows
    Given a configured spacing of 8 pixels
    When a moving or resizing visible edge snaps beside another visible window
    Then the visible gap is 8 pixels, never 16
    And the same rule applies horizontally and vertically

  SP-002: Desktop margins and confinement
    Given the same spacing setting and a monitor work area
    When a visible edge snaps or is constrained to an outer work-area border
    Then the visible margin is that spacing
    And resizing leaves the opposite edge fixed

  SP-003: Activation distance remains independent
    Given a spacing of 8 and activation distance of 2 pixels
    When an edge is 4 pixels from its desired spaced position
    Then proximity snapping does not activate

  SP-004: Zero spacing
    Given spacing is zero
    When neighboring visible edges snap
    Then the visible edges touch without an unexplained resize-border gap

  SP-005: Persistence and guides
    Given a spacing setting is applied in Preferences
    When settings are saved and reloaded
    Then the same value is restored
    And center or grid guides remain unpadded alignment targets
