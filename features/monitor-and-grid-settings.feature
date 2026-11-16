Feature: Monitor work areas and optional grid guides
  This file documents behavior for native acceptance tests. It is not executed or parsed.

  Scenario MG01: Grid guides do not hide a later monitor
    Given two adjacent 1000 by 1000 monitor work areas
    And the first monitor has 20 horizontal grid guides
    And desktop confinement is enabled
    When a 200-wide window is moved 100 pixels beyond the second monitor's right edge
    Then its right edge is kept at the second monitor's right work-area edge
    And its width is preserved

  Scenario MG02: Guides are not desktop work areas
    Given one monitor work area with a vertical guide at x=500
    When monitor and guide candidates are collected
    Then confinement sees exactly one real work area
    And the guide remains separate from that work-area collection

  Scenario MG03: Valid grid guides still snap windows
    Given a monitor work area with a vertical guide at x=500
    And snapping has a threshold of 10 pixels
    When a 200-wide window is moved with its left edge at x=493
    Then its left edge snaps to x=500
    And its width is preserved

  Scenario MG04: Malformed grid settings do not disable confinement
    Given an invalid grid mode and a negative pixel spacing
    And two adjacent monitor work areas
    And desktop confinement is enabled
    When a window is moved beyond the second monitor's right edge
    Then invalid guides are disabled safely
    And the window remains constrained to the real work areas

  Capacity and validation examples for unit coverage:
    At most 10 real work areas and 100 generated guide candidates are stored.
    Dense grids reserve storage for later real work areas.
    Center-guide arrays are bounded by their actual storage capacity.
    Negative values and overflowing even-grid counts are disabled.
    Zero retains the existing no-guides behavior.
    Valid integer spacing such as 25 pixels remains unchanged.
    Single guides beyond a work area are skipped before coordinate addition.
    Evenly spaced guides distribute integer rounding across the full dimension:
    100 interior lines in a 1000-pixel dimension begin at 10 and end at 990,
    rather than accumulating a rounded-down step and ending at 900.
    When the requested count exceeds distinct interior pixel positions, duplicate
    or boundary lines are skipped instead of consuming capacity at offset zero.
