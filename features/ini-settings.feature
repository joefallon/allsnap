Persisted non-sound Settings behavior

These scenarios document native Microsoft tests using temporary INI files.
There is no Gherkin runner or change to the user's INI file.

INI-001 - Invalid persisted thresholds preserve valid runtime values
Given existing valid thresholds and a malformed, empty, overflowing, or out-of-range
persisted threshold, when settings reload, the invalid value is ignored.
Valid thresholds span 2 through 100 inclusive; valid surrounding whitespace is accepted.
Full numeric tokens are required rather than accepting a valid-looking prefix.
Legacy 0x-prefixed hexadecimal values remain accepted; leading-zero decimals stay decimal.

INI-002 - Persisted booleans use canonical true and false
Given a valid nonzero integer stored for a boolean preference, when settings reload,
the engine receives TRUE rather than an arbitrary nonzero integer.
The real modifier/confinement path must still work with a released toggle key.
Zero continues to mean FALSE. Missing or invalid values preserve current settings.

INI-003 - Explicit empty class lists clear old lists
Given populated ignored/skinned class lists, when the INI contains an explicit empty
value, reloading clears that list. A missing key preserves its current value.
Audio-path loading remains outside this audit.

INI-004 - Dialog reload preserves live tray state
Given runtime enabled/hidden state differs from persisted values, when Settings
reloads its other saved preferences, it preserves both live states without transient
Shell icon changes. Startup still loads the saved enabled/hidden values normally.
