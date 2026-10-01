param(
    [string]$EngineRoot = 'D:\Hry\Epic\UE_5.8',
    [switch]$Build,
    [switch]$SmokeTest,
    [switch]$CabinReview,
    [switch]$LegacyGridDemo,
    [ValidateRange(480,3840)][int]$Width = 1280,
    [ValidateRange(480,2160)][int]$Height = 720
)
$ErrorActionPreference = 'Stop'
$Project = Join-Path $PSScriptRoot 'OperaciaKopanice.uproject'
$Editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
if (!(Test-Path -LiteralPath $Editor)) { throw "Unreal Editor not found: $Editor" }
$Module = Join-Path $PSScriptRoot 'Binaries/Win64/UnrealEditor-OperaciaKopanice.dll'
if ($Build -or !(Test-Path -LiteralPath $Module)) {
    # Short build paths avoid Windows compiler intermediate-path limits.
    $Stage = Join-Path $env:LOCALAPPDATA ('OperaciaKopanice/DemoBuild-' + [guid]::NewGuid().ToString('N').Substring(0,8))
    New-Item -ItemType Directory -Path $Stage -Force | Out-Null
    Copy-Item -LiteralPath $Project -Destination $Stage
    foreach ($Folder in @('Source', 'Config')) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot $Folder) -Destination $Stage -Recurse
    }
    & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') OperaciaKopaniceEditor Win64 Development "-Project=$Stage/OperaciaKopanice.uproject" -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) { throw "Build failed. Build files retained in $Stage" }
    Copy-Item -LiteralPath (Join-Path $Stage 'Binaries') -Destination $PSScriptRoot -Recurse -Force
    Write-Host "Build files retained in $Stage"
}
$Map = if ($LegacyGridDemo) { '/Engine/Maps/Entry?game=/Script/OperaciaKopanice.OKDemoGameMode' } else { '/Engine/Maps/Entry' }
$Arguments = @($Project, $Map, '-game', '-Windowed', "-ResX=$Width", "-ResY=$Height", '-NoSplash')
if ($SmokeTest) {
    $SmokeLog = Join-Path $PSScriptRoot ('Saved/Logs/DemoSmoke-' + [guid]::NewGuid().ToString('N') + '.log')
    $SmokeFlag = if ($LegacyGridDemo) { '-OKDemoSmoke' } else { '-OKRTSmoke' }
    $Arguments += @($SmokeFlag, '-RenderOffscreen', '-Unattended', '-NoSound', '-ForceRes', "-AbsLog=$SmokeLog")
}
if ($CabinReview) { $Arguments += '-OKCabinReview' }
$WindowStyle = if ($SmokeTest) { 'Hidden' } else { 'Normal' }
$QuotedArguments = $Arguments | ForEach-Object { '"' + $_ + '"' }
$Process = Start-Process -FilePath $Editor -ArgumentList $QuotedArguments -WindowStyle $WindowStyle -PassThru
# Wait for the editor process, not persistent child services such as the DDC server.
$null = $Process.Handle
if ($SmokeTest) {
    if (!$Process.WaitForExit(240000)) { Stop-Process -Id $Process.Id; throw "Smoke timeout: $SmokeLog" }
} else { $Process.WaitForExit() }
if ($Process.ExitCode -ne 0) { throw "Demo exited with code $($Process.ExitCode)" }
if ($SmokeTest) {
    $LogText = Get-Content -LiteralPath $SmokeLog -Raw
    $Pass = if ($LegacyGridDemo) { 'OK_DEMO_SMOKE: PASS' } else { 'OK_RT_SMOKE: PASS' }
    if ($LogText -notmatch $Pass -or $LogText -match 'Failed to compile Material') {
        throw "Rendered smoke test failed. See $SmokeLog"
    }
    Write-Host "Rendered smoke test passed: $SmokeLog"
}
