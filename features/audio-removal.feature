Application operation without sound functionality

These scenarios document Microsoft native acceptance/unit tests, not Gherkin.
Tests use owned windows and temporary INI files; no desktop hooks or audio are started.

AU-001 - Exactly three Settings pages remain
When the actual native modeless Settings sheet opens, it contains Preferences,
Snap to grid, and Advanced in that order. The retired Sounds dialog is not embedded.
Apply, OK, Cancel, validation and save-failure retry continue to work on retained pages.

AU-002 - Snapping state and lifecycle do not depend on audio
Repeated snap, continue, resize and unsnap transitions preserve the production
last-result history used for geometry. Reset clears it normally.
Hook startup/cleanup accepts no sound worker identifier, repeated native subclass
ownership remains safe, and the retired noisy-setting exports are absent.
Global hook installation/removal is substituted in these tests.

AU-003 - Legacy audio keys do not prevent normal operation
Given an older INI containing play_sounds and invalid/missing sound file paths,
loading ignores those retired values while preserving non-audio settings and
normal snapping/confinement. No worker, preview, or playback dependency is needed.

AU-004 - Current saves do not create or rewrite audio settings
A fresh save writes no retired audio keys and round-trips non-audio preferences.
Existing unknown/obsolete keys may remain untouched in an older file; saving current
preferences must not rewrite them or discard unrelated user values.

Build review also verifies that current EXE/DLL outputs do not import WinMM,
and no shipping project or installer references removed audio sources/assets.
Historical compiled help is an archive and is not a current audio feature.
