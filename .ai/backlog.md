# allSnap backlog

Canonical backlog for follow-up work. Execution details and verified results belong in [the modernization implementation plan](features/20261116-modernization/implementation-plan.md); current architecture is in [current-state.md](current-state.md). The user handles commits. Keep behavioral repairs, structural changes, and naming changes in separate reviewable increments.

## Next priorities

| ID | Item | Status and completion boundary |
| --- | --- | --- |
| B-001 | Validate desktop behavior and finish lifecycle reliability | Automated lifetime increment implemented: 12 isolated-process cases cover termination, independent owners, shutdown reservations, host restart, and callback/module retention; full suite 222/222 in Debug/Release on both architectures. Shared scalar replaced with identity-aware owner records; callback code pinned until affected processes exit. Still pending: actual global-hook delivery in representative x86/x64 applications, repeated gestures, startup/exit, and a long-duration trial. The intermittent-stop cause remains unconfirmed. Check 100%, HiDPI, mixed-DPI transitions, negative/staggered monitors, and the older home monitor; record hardware checks separately from automated tests. |
| B-002 | Complete Settings behavior coverage | Pending. Document remaining options in `features/` and add intended-behavior acceptance tests using the existing Microsoft framework, particularly MDI, ignored/skinned classes, and actual property-sheet/tray interactions. Preserve Windows compatibility scaling for the UI. Keep unfinished aspect-ratio and auto-lineup features disabled unless separately approved. |

## Requested follow-up milestones

| ID | Item | Status and completion boundary |
| --- | --- | --- |
| B-003 | Remove all sound functionality | Requested; not started. Remove audio UI and previews, playback/thread code, event messages and callers, persistence entries, sound-only assets/resources, installer references, and exclusively audio dependencies. Preserve non-audio preferences in existing INI files. Add acceptance documentation and regression coverage for normal operation without audio. Do not restore disabled notification playback. |
| B-004 | Create root README.md | Requested; not started. Document purpose, supported environment, build/test commands, usage, settings, dual-architecture operation, and known limitations. |
| B-005 | Introduce a source folder structure | Requested; not started. Separate application, engine, resources, help, and tests; update project, resource, test, and installer references together. Keep moves separate from behavior changes. |
| B-006 | Review ALLSNAPB naming | Requested; not started. The reason for the trailing B is unknown. Agree on consistent solution/project naming before renaming and update references together. |
| B-007 | Uncle Bob Clean Code refactoring | Requested; not started. Improve names, function responsibilities, state ownership, duplication, and dead code incrementally behind tests. Preserve the C/Win32 executable/DLL architecture. |

## Proposed improvement

| ID | Item | Status and completion boundary |
| --- | --- | --- |
| B-008 | One launcher/tray controlling x86/x64 helpers | Proposed; defer until lifecycle behavior is verified. Provide one user-facing control point that starts/stops both helpers and keeps configuration consistent. Both helper architectures remain necessary for the current injected-hook/subclass design; this would simplify management rather than eliminate a helper. |
