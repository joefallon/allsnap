[CmdletBinding()]
param(
    [ValidateSet('All', 'Win32', 'x64')]
    [string]$Platform = 'All',
    [ValidateSet('Debug', 'Release', 'Debug UNICODE', 'Release UNICODE')]
    [string]$Configuration = 'Debug',
    [ValidateSet('All', 'Unit', 'Acceptance')]
    [string]$Suite = 'All'
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path -LiteralPath $vswherePath)) {
    throw 'Visual Studio Installer/vswhere was not found. Install the Desktop development with C++ workload.'
}
$visualStudioPath = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $visualStudioPath) {
    throw 'A Visual Studio installation with the C++ x86/x64 toolset was not found.'
}
$msbuildPath = Join-Path $visualStudioPath 'MSBuild\Current\Bin\MSBuild.exe'
$vstestPath = Join-Path $visualStudioPath 'Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe'
if (-not (Test-Path -LiteralPath $vstestPath)) {
    throw 'The Visual Studio VSTest runner was not found.'
}

$platforms = if ($Platform -eq 'All') { @('Win32', 'x64') } else { @($Platform) }
foreach ($targetPlatform in $platforms) {
    Write-Host "Building $Configuration|$targetPlatform tests"
    & $msbuildPath (Join-Path $PSScriptRoot 'allsnap_tests.vcxproj') "/p:Configuration=$Configuration" "/p:Platform=$targetPlatform" /verbosity:minimal /nologo
    if ($LASTEXITCODE -ne 0) { throw "Test build failed for $targetPlatform." }

    $testDll = Join-Path $repoRoot "build\tests\$targetPlatform\$Configuration\allsnap_tests.dll"
    $resultsDirectory = Join-Path $repoRoot "build\test-results\$targetPlatform\$Configuration"
    $runnerPlatform = if ($targetPlatform -eq 'Win32') { 'x86' } else { 'x64' }
    $runnerArguments = @($testDll, "/Platform:$runnerPlatform", "/ResultsDirectory:$resultsDirectory", "/Logger:trx;LogFileName=$Suite.trx")
    if ($Suite -ne 'All') {
        # Class names identify suites, using VSTest's built-in property rather than
        # relying on adapter-specific mapping of native custom attributes.
        $runnerArguments += "/TestCaseFilter:FullyQualifiedName~$Suite"
    }
    & $vstestPath @runnerArguments
    if ($LASTEXITCODE -ne 0) { throw "$Suite tests failed for $targetPlatform." }

    [xml]$result = Get-Content -LiteralPath (Join-Path $resultsDirectory "$Suite.trx")
    $counters = $result.SelectSingleNode('//*[local-name()="Counters"]')
    if (-not $counters -or [int]$counters.total -eq 0) {
        throw "No $Suite tests were executed for $targetPlatform. Check native test discovery."
    }
    Write-Host "$targetPlatform $Suite tests: $($counters.passed)/$($counters.total) passed."
}
