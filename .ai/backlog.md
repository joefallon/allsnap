# allSnap backlog

Canonical backlog for follow-up work. Execution details and verified results belong in [the modernization implementation plan](features/20261116-modernization/implementation-plan.md); current architecture is in [current-state.md](current-state.md). The user handles commits. Keep behavioral repairs, structural changes, and naming changes in separate reviewable increments.

## Next priorities

| ID | Item | Status and completion boundary |
| --- | --- | --- |
| B-001 | Validate desktop behavior and finish lifecycle reliability | Automated lifetime increment implemented: 12 isolated-process cases cover termination, independent owners, shutdown reservations, host restart, and callback/module retention, and remain green in the current full matrix. Shared scalar replaced with identity-aware owner records; callback code pinned until affected processes exit. Still pending: actual global-hook delivery in representative x86/x64 applications, repeated gestures, startup/exit, and a long-duration trial. The intermittent-stop cause remains unconfirmed. Check 100%, HiDPI, mixed-DPI transitions, negative/staggered monitors, and the older home monitor; record hardware checks separately from automated tests. |
| B-002 | Complete Settings behavior coverage | Automated audit/repairs implemented: 55 added methods, final suite 277/277 in Debug/Release on both architectures. Functional non-sound controls, actual property-sheet interactions, INI validation, owned MDI/class behavior and the real tray controller are covered. Still pending: actual Explorer/host-menu integration, context-help rendering, keyboard navigation and manual desktop validation. Preserve UI compatibility scaling; unfinished aspect-ratio and auto-lineup features stay disabled. See Phase 6 coverage inventory and feature contracts. |

## Requested follow-up milestones

| ID | Item | Status and completion boundary |
| --- | --- | --- |
| B-003 | Remove all sound functionality | Implemented and independently reviewed in Phase 7: audio UI/preview, worker/messages/API/settings, exclusive assets/dependencies and packaging/source-help references removed. Legacy INI keys are ignored while other preferences remain intact. Final suite 286/286 in Debug/Release on both architectures; eight solution builds and Release import/export checks pass. The stale compiled CHM is retained as a historical archive rather than silently regenerated. |
| B-004 | Create root README.md | Complete in Phase 8. Documents purpose, verified environment, build/test commands, usage, settings, dual-architecture operation, source navigation, and known limitations. Commands and claims reviewed against source; documentation-only, with no builds or tests rerun. |
| B-005 | Introduce a source folder structure | Complete and independently reviewed in Phase 9. App/engine/common code, resources, help, packaging, and legacy material have separate folders; existing tests/features stay in place. Project/filter/include, installer-input, help-output, and documentation references updated together. Production C/header/resource contents and runtime/output paths preserved. All eight solution builds and reference checks pass; no tests executed for this structural increment. |
| B-006 | Review ALLSNAPB naming | Complete and independently reviewed in Phase 10: root solution is `allsnap.sln`, app project/filter files are `allsnap.vcxproj` / `allsnap.vcxproj.filters`, and app RootNamespace matches its existing `allsnap` ProjectName. Active references updated; stale solution-item help paths corrected. Runtime names, GUIDs, configurations, and outputs preserved. All eight solution builds and reference checks pass; no tests executed. The historical reason for the trailing B remains unknown. |
| B-007 | Uncle Bob Clean Code refactoring | In progress: first focused increment complete and independently reviewed in Phase 11. INI save serialization separated from ordered traversal; goto/status bookkeeping removed while preserving save semantics. All eight solution builds pass; existing regression tests inspected but not executed. Further naming, responsibility, state ownership, duplication, and dead-code improvements remain separate bounded milestones. Preserve the C/Win32 executable/DLL architecture. |

## Proposed improvement

| ID | Item | Status and completion boundary |
| --- | --- | --- |
| B-008 | One launcher/tray controlling x86/x64 helpers | Proposed; defer until lifecycle behavior is verified. Provide one user-facing control point that starts/stops both helpers and keeps configuration consistent. Both helper architectures remain necessary for the current injected-hook/subclass design; this would simplify management rather than eliminate a helper. |
