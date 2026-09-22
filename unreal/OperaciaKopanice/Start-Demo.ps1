param(
    [string]$EngineRoot = 'D:\Hry\Epic\UE_5.8',
    [switch]$Build,
    [switch]$SmokeTest,
    [switch]$CabinReview,
    [ValidateRange(960,3840)][int]$Width = 1280,
    [ValidateRange(540,2160)][int]$Height = 720
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
$Arguments = @($Project, '/Engine/Maps/Entry', '-game', '-Windowed', "-ResX=$Width", "-ResY=$Height", '-NoSplash')
if ($SmokeTest) {
    $SmokeLog = Join-Path $PSScriptRoot ('Saved/Logs/DemoSmoke-' + [guid]::NewGuid().ToString('N') + '.log')
    $Arguments += @('-OKDemoSmoke', '-RenderOffscreen', '-Unattended', '-NoSound', '-ForceRes', "-AbsLog=$SmokeLog")
}
if ($CabinReview) { $Arguments += '-OKCabinReview' }
$WindowStyle = if ($SmokeTest) { 'Hidden' } else { 'Normal' }
$QuotedArguments = $Arguments | ForEach-Object { '"' + $_ + '"' }
$Process = Start-Process -FilePath $Editor -ArgumentList $QuotedArguments -WindowStyle $WindowStyle -PassThru
# Wait for the editor process, not persistent child services such as the DDC server.
$null = $Process.Handle
$Process.WaitForExit()
if ($Process.ExitCode -ne 0) { throw "Demo exited with code $($Process.ExitCode)" }
if ($SmokeTest) {
    $LogText = Get-Content -LiteralPath $SmokeLog -Raw
    if ($LogText -notmatch 'OK_DEMO_SMOKE: PASS' -or $LogText -match 'Failed to compile Material') {
        throw "Rendered smoke test failed. See $SmokeLog"
    }
    Write-Host "Rendered smoke test passed: $SmokeLog"
}
