@echo off
"%~dp0build\release\SimDataPlayer.exe" "C:/Users/23714/Downloads/scan_20260717_141034.csv" "C:/Users/23714/Downloads/scan_20260717_133933.csv" > "%~dp0soundscan_out.txt" 2>&1
echo EXITCODE=%ERRORLEVEL%
