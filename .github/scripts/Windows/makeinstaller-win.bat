@echo off

REM - batch file to build Visual Studio project and zip the resulting binaries (or make installer)
REM - updating version numbers requires python and python path added to %PATH% env variable 
REM - building installer requires innosetup 6 in "%ProgramFiles(x86)%\Inno Setup 6\iscc"

echo ------------------------------------------------------------------
REM echo Updating version numbers ...

REM call python update_installer-win.py %DEMO%

REM --echo ------------------------------------------------------------------
REM --echo Code sign AAX binary...
REM --info at pace central, login via iLok license manager https://www.paceap.com/pace-central.html
REM --wraptool sign --verbose --account XXXXX --wcguid XXXXX --keyfile XXXXX.p12 --keypassword XXXXX --in .\build-win\aax\bin\PlugData.aaxplugin\Contents\Win32\PlugData.aaxplugin --out .\build-win\aax\bin\PlugData.aaxplugin\Contents\Win32\PlugData.aaxplugin
REM --wraptool sign --verbose --account XXXXX --wcguid XXXXX --keyfile XXXXX.p12 --keypassword XXXXX --in .\build-win\aax\bin\PlugData.aaxplugin\Contents\x64\PlugData.aaxplugin --out .\build-win\aax\bin\PlugData.aaxplugin\Contents\x64\PlugData.aaxplugin

REM - Make Installer (InnoSetup)

echo ------------------------------------------------------------------
echo Making Installer ...

if exist "%ProgramFiles(x86)%" (goto 64-Bit-is) else (goto 32-Bit-is)

:32-Bit-is
"%ProgramFiles%\Inno Setup 6\iscc" /Q ".\installer\PlugData.iss"
goto END-is

:64-Bit-is
"%ProgramFiles(x86)%\Inno Setup 6\iscc" /Q ".github\scripts\Windows\PlugData.iss"
goto END-is

:END-is

REM - Codesign Installer for Windows 8+
REM -"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\signtool.exe" sign /f "XXXXX.p12" /p XXXXX /d "PlugData Installer" ".\installer\PlugData Installer.exe"

REM -if %1 == 1 (
REM -copy ".\installer\PlugData Installer.exe" ".\installer\PlugData Demo Installer.exe"
REM -del ".\installer\PlugData Installer.exe"
REM -)

REM echo Making Zip File of Installer ...

REM FOR /F "tokens=* USEBACKQ" %%F IN (`call python scripts\makezip-win.py %DEMO% %ZIP%`) DO (
REM SET ZIP_NAME=%%F
REM )

echo ------------------------------------------------------------------
echo Printing log file to console...

type build-win.log
goto SUCCESS

:USAGE
echo Usage: %0 [demo/full] [zip/installer]
exit /B 1

:SUCCESS
echo %ZIP_NAME%

exit /B 0