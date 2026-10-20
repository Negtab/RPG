# Собирает Release-версию игры и упаковывает её в установщик Inno Setup.
# Запуск: powershell -ExecutionPolicy Bypass -File installer\build_installer.ps1 [-Version 1.0.0]
param(
    [string]$Version = "1.0.0",
    [string]$CLionDir = "D:\CLion 2025.3.2"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root "cmake-build-release"

# Тулчейн из CLion: MinGW + CMake
$env:PATH = "$CLionDir\bin\mingw\bin;$env:PATH"
$cmake = "$CLionDir\bin\cmake\win\x64\bin\cmake.exe"
if (-not (Test-Path $cmake)) { throw "Не найден CMake: $cmake (укажите -CLionDir)" }

& $cmake -S $root -B $buildDir -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "CMake configure завершился с ошибкой" }
& $cmake --build $buildDir -j 8
if ($LASTEXITCODE -ne 0) { throw "Сборка завершилась с ошибкой" }

# DLL SDL копируем рядом с exe (в debug их кладёт CLion, здесь — сами)
foreach ($lib in "SDL2", "SDL2_image", "SDL2_mixer", "SDL2_ttf") {
    Copy-Item "$root\$lib\x86_64-w64-mingw32\bin\*.dll" $buildDir -Force
}

# ISCC: winget ставит per-user в LocalAppData, обычный установщик — в Program Files
$iscc = @(
    "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe",
    "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
    "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $iscc) { throw "Inno Setup 6 не найден: winget install JRSoftware.InnoSetup" }

& $iscc "/DAppVersion=$Version" "/DBuildDir=$buildDir" (Join-Path $PSScriptRoot "RPG.iss")
if ($LASTEXITCODE -ne 0) { throw "ISCC завершился с ошибкой" }

Write-Host "Готово: $PSScriptRoot\Output\RPG-Setup-$Version.exe"
