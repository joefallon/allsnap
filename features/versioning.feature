Release version identity

VER-001 - Readable release and Windows tuple agree
The version is 2.0.0 as text and 2.0.0.0 in Windows numeric metadata.
The obvious version header beside startup contains the release representations;
VersionUnitTests checks text/tuple consistency.

VER-002 - Running-instance token stays separate
Startup treats the release version as a string. Its instance-message token is
numeric, differs from the legacy token, and is not a pointer or parsed UI text.
VersionUnitTests checks the new token and separation from the legacy release.

VER-003 - Compiled version resources describe a stable release
Actual compiled VERSIONINFO reports matching file/product text and numeric values.
Release builds have no Debug, Prerelease, or SpecialBuild flags; Debug builds set
only the Debug flag. VersionAcceptanceTests queries the native compiled resource.

VER-004 - Both About templates show the new version
Native hidden dialogs created from both About resources contain the release label
and no old Beta wording. VersionAcceptanceTests inspects their actual child text.

Release verification also inspects both actual architecture-specific EXE/DLL pairs,
embedded manifests and x64 icon payloads; this does not exercise real desktop hooks.
