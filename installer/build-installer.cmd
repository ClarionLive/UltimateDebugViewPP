@echo off
rem Builds Ultimate DebugView++ (Release), signs it, and builds the signed installer.
rem
rem   installer\build-installer.cmd          signed build: plug in the code-signing USB token first;
rem                                           Windows asks for its PIN when signing starts
rem   installer\build-installer.cmd nosign   unsigned test build
rem
rem Output: installer\Output\UltimateDebugViewPP-Setup-<version>.exe
setlocal EnableExtensions

set "ROOT=%~dp0.."
for %%i in ("%ROOT%") do set "ROOT=%%~fi"

rem Certificate on the token (Sectigo EV, Kennewick Computer Company; valid until 2026-11-27 23:59:59 UTC).
rem Every signature is timestamped, so it stays valid after the certificate expires.
set "CERT_SHA1=85C3D22C215029A9F59EFF775720446F3B12FE3A"
set "TIMESTAMP_URL=http://timestamp.sectigo.com"
set "SIGN_DESC=Ultimate DebugView++"
set "SIGN_URL=https://github.com/ClarionLive/UltimateDebugViewPP"

set "NOSIGN="
if /i "%~1"=="nosign" set "NOSIGN=1"

rem (No paths inside if-blocks below: "Program Files (x86)" would end the block at its ")".)
set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" goto :no_iscc

rem The newest x64 signtool from the Windows SDK (version folders sort in ascending order).
set "SIGNTOOL="
for /f "delims=" %%i in ('dir /b /s /o:n "%ProgramFiles(x86)%\Windows Kits\10\bin\signtool.exe" 2^>nul ^| findstr /i /c:"\x64\signtool.exe"') do set "SIGNTOOL=%%i"
if not defined NOSIGN if not defined SIGNTOOL goto :no_signtool

echo.
echo === Building Release (UltimateDebugviewpp, fsqlf)
if not exist "%ROOT%\build\CMakeCache.txt" (
    cmake -S "%ROOT%" -B "%ROOT%\build" -G "Visual Studio 17 2022" || exit /b 1
)
cmake --build "%ROOT%\build" --config Release --target UltimateDebugviewpp fsqlf -- -m:4 -v:m || exit /b 1

set "EXE=%ROOT%\bin\Release\UltimateDebugviewpp.exe"
set "DLL=%ROOT%\bin\Release\fsqlf.dll"
for %%f in ("%EXE%" "%DLL%") do if not exist "%%~f" (
    echo Missing build output: %%~f
    exit /b 1
)

if defined NOSIGN (
    echo.
    echo === Building the installer, UNSIGNED
    "%ISCC%" /Q "%ROOT%\installer\UltimateDebugViewPP.iss" || exit /b 1
    goto :done
)

echo.
echo === Signing the program files (enter the token PIN if asked)
"%SIGNTOOL%" sign /sha1 %CERT_SHA1% /fd sha256 /tr %TIMESTAMP_URL% /td sha256 /d "%SIGN_DESC%" /du %SIGN_URL% "%EXE%" "%DLL%" || exit /b 1

echo.
echo === Building and signing the installer and uninstaller
"%ISCC%" /Q /DSign "/Ssigntool=$q%SIGNTOOL%$q sign /sha1 %CERT_SHA1% /fd sha256 /tr %TIMESTAMP_URL% /td sha256 /d $q%SIGN_DESC% Setup$q /du %SIGN_URL% $f" "%ROOT%\installer\UltimateDebugViewPP.iss" || exit /b 1

echo.
echo === Verifying signatures
set "SETUP="
for /f "delims=" %%i in ('dir /b /o:d "%ROOT%\installer\Output\UltimateDebugViewPP-Setup-*.exe"') do set "SETUP=%ROOT%\installer\Output\%%i"
"%SIGNTOOL%" verify /pa /q "%EXE%" || exit /b 1
"%SIGNTOOL%" verify /pa /q "%DLL%" || exit /b 1
"%SIGNTOOL%" verify /pa /q "%SETUP%" || exit /b 1
echo All signatures verify.

:done
echo.
echo Installer:
dir /b /o:d "%ROOT%\installer\Output\UltimateDebugViewPP-Setup-*.exe"
endlocal
exit /b 0

:no_iscc
echo Inno Setup 6 not found: %ISCC%
exit /b 1

:no_signtool
echo signtool.exe not found in the Windows 10/11 SDK.
exit /b 1
