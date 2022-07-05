echo "------------------------------------------------------------------"
echo "Making Installer ..."

python ".github\scripts\Windows\update-version.py" $1

iscc.exe ".github\scripts\Windows\PlugData.iss"

if [[ $2 == "x64" ]]; then
cp ".github\scripts\Windows\PlugData Installer.exe" ".\PlugData-Win64.exe"
else
cp ".github\scripts\Windows\PlugData Installer.exe" ".\PlugData-Win32.exe"
fi

# - Codesign Installer for Windows 8+
# -"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\signtool.exe" sign /f "XXXXX.p12" /p XXXXX /d "PlugData Installer" ".\installer\PlugData Installer.exe"

# -if %1 == 1 (
# -copy ".\installer\PlugData Installer.exe" ".\installer\PlugData Demo Installer.exe"
# -del ".\installer\PlugData Installer.exe"
# -)