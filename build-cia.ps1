# Builds an installable Meteo3DS.cia from the compiled .elf.
# Prereq: run the normal devkitPro build first (produces Meteo3DS.elf):
#   make
# Then:  powershell -ExecutionPolicy Bypass -File build-cia.ps1
# Run from the project root (the script cd's to its own location).
#
# This recipe mirrors a known-working homebrew CIA (aliceinpalth/3dfetch). The
# THREE things that made an installed CIA boot (without them it crashed at launch
# with ErrDisp "The SD card was removed") are:
#   1) -exefslogo + "Logo: Nintendo" in the RSF  (embedded boot logo)
#   2) a RomFS partition (even minimal — see romfs/placeholder.txt + RSF RootPath)
#   3) the icon built with `bannertool makesmdh` (NOT the Makefile's smdhtool .smdh)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
$bt = "tools\bannertool\windows-x86_64\bannertool.exe"
$mr = "tools\makerom184\makerom.exe"   # makerom 0.18.4

# 1) Banner (256x128 image + audio -> banner.bin)
& $bt makebanner -i "cia\banner.png" -a "cia\audio.wav" -o "cia\banner.bin"
if ($LASTEXITCODE -ne 0) { throw "bannertool makebanner failed" }

# 2) Icon/SMDH via bannertool (important: smdhtool's .smdh did NOT work here)
& $bt makesmdh -s "Meteo3DS" -l "Meteo3DS" -p "xPsycho999" -i "icon.png" -o "cia\icon.bin"
if ($LASTEXITCODE -ne 0) { throw "bannertool makesmdh failed" }

# 3) Pack the CIA (run from project dir so RSF "RomFs RootPath: romfs" resolves)
& $mr -f cia -DAPP_ENCRYPTED=false -rsf "cia\build.rsf" -target t -exefslogo `
      -elf "Meteo3DS.elf" -icon "cia\icon.bin" -banner "cia\banner.bin" -o "Meteo3DS.cia"
if ($LASTEXITCODE -ne 0) { throw "makerom failed" }

Write-Output ("CIA built: {0} bytes" -f (Get-Item "Meteo3DS.cia").Length)
