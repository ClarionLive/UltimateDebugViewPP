@echo off
REM Compile Ribbon XML to binary resource
REM Requires Windows SDK with UICC.exe

set UICC="C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x86\uicc.exe"
if not exist %UICC% (
    set UICC="C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x86\uicc.exe"
)
if not exist %UICC% (
    set UICC="C:\Program Files (x86)\Windows Kits\10\bin\10.0.18362.0\x86\uicc.exe"
)
if not exist %UICC% (
    set UICC="C:\Program Files (x86)\Windows Kits\10\bin\x86\uicc.exe"
)
if not exist %UICC% (
    echo ERROR: UICC.exe not found. Please install Windows SDK or update path in this script.
    exit /b 1
)

echo Compiling ribbon XML...
%UICC% DebugViewRibbon_Simple.xml DebugViewRibbon.bml /header:DebugViewRibbon.h /res:DebugViewRibbon.rc

if errorlevel 1 (
    echo ERROR: Failed to compile ribbon
    exit /b 1
)

echo Ribbon compiled successfully
exit /b 0