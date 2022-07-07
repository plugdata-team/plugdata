X64BitMode=""

if [[ $1 == "x64" ]]; then
  X64BitMode="x64"
fi

VERSION=${GITHUB_REF#refs/*/}

cat > ./PlugData.iss <<-EOL
[Setup]
AppName=PlugData
AppContact=timschoen123@gmail.com
AppCopyright=Distributed under GPLv3 license
AppPublisher=OCTAGONAUDIO
AppPublisherURL=https://github.com/timothyschoen/PlugData
AppSupportURL=https://github.com/timothyschoen/PlugData
AppVerName=$VERSION
VersionInfoVersion=1.0.0
DefaultDirName={commonpf}\PlugData
DefaultGroupName=PlugData
Compression=lzma2
SolidCompression=yes
OutputDir=.\
ArchitecturesInstallIn64BitMode=$X64BitMode
OutputBaseFilename=PlugData Installer
LicenseFile=LICENSE
SetupLogging=yes
ShowComponentSizes=yes

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Messages]
WelcomeLabel1=Welcome to the PlugData installer
SetupWindowTitle=PlugData Installer
SelectDirLabel3=The standalone application and supporting files will be installed in the following folder.
SelectDirBrowseLabel=To continue, click Next. If you would like to select a different folder (not recommended), click Browse.

[Components]
Name: "app"; Description: "Standalone application (.exe)"; Types: full custom;
Name: "lv2"; Description: "LV2 Plugin (.lv2)"; Types: full custom;
Name: "vst3"; Description: "VST3 Plugin (.vst3)"; Types: full custom;

[Files]
Source: ".\Plugins\VST3\**"; Excludes: "*.pdb,*.exp,*.lib,*.ilk,*.ico,*.ini"; DestDir: "{commoncf}\VST3\"; Components:vst3; Flags: ignoreversion recursesubdirs;
Source: ".Plugins\LV2\**"; Excludes: "*.exe,*.pdb,*.exp,*.lib,*.ilk,*.ico,*.ini"; DestDir: "{commoncf}\LV2\"; Components:lv2; Flags: ignoreversion recursesubdirs;
Source: ".Plugins\Standalone\**"; Excludes: "*.pdb,*.exp,*.ilk,*.ico,*.ini"; DestDir: "{commonpf}\PlugData\"; Components:app; Flags: ignoreversion recursesubdirs;

[Icons]
Name: "{group}\PlugData"; Filename: "{app}\PlugData.exe"

[Code]
var
  OkToCopyLog : Boolean;

procedure InitializeWizard;
begin
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssDone then
    OkToCopyLog := True;
end;

procedure DeinitializeSetup();
begin
  if OkToCopyLog then
    FileCopy (ExpandConstant ('{log}'), ExpandConstant ('{app}\InstallationLogFile.log'), FALSE);
  RestartReplace (ExpandConstant ('{log}'), '');
end;

[UninstallDelete]
Type: files; Name: "{app}\InstallationLogFile.log"
EOL

iscc.exe PlugData.iss

if [[ $1 == "x64" ]]; then
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