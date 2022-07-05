[Setup]
AppName=PlugData
AppContact=timschoen123@gmail.com
AppCopyright= Distributed under GPLv3 license
AppPublisher=OCTAGONAUDIO
AppPublisherURL=https://github.com/timothyschoen/PlugData
AppSupportURL=https://github.com/timothyschoen/PlugData
AppVersion=0.6.0
VersionInfoVersion=0.6.0
DefaultDirName={commonpf}\PlugData
DefaultGroupName=PlugData
Compression=lzma2
SolidCompression=yes
OutputDir=.\
ArchitecturesInstallIn64BitMode=x64
OutputBaseFilename=PlugData Installer
LicenseFile=..\..\..\LICENSE
SetupLogging=yes
ShowComponentSizes=no
; WizardImageFile=installer_bg-win.bmp
; WizardSmallImageFile=installer_icon-win.bmp

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Messages]
WelcomeLabel1=Welcome to the PlugData installer
SetupWindowTitle=PlugData installer
SelectDirLabel3=The standalone application and supporting files will be installed in the following folder.
SelectDirBrowseLabel=To continue, click Next. If you would like to select a different folder (not recommended), click Browse.

[Components]
Name: "app"; Description: "Standalone application (.exe)"; Types: full custom;
Name: "lv2"; Description: "LV2 Plugin (.lv2)"; Types: full custom;
Name: "vst3"; Description: "VST3 Plugin (.vst3)"; Types: full custom;

; Name: "manual"; Description: "User guide"; Types: full custom; Flags: fixed

; [Dirs] 
; Name: "{commoncf}\VST3\"; Attribs: readonly; Components:vst3; 
; Name: "{commoncf}\LV2\"; Attribs: readonly; Components:lv2; 
; Name: "{commonpf}\PlugData\"; Attribs: readonly; Components:lv2; 

[Files]
Source: "..\..\..\Plugins\VST3\**"; Excludes: "*.pdb,*.exp,*.lib,*.ilk,*.ico,*.ini"; DestDir: "{commoncf}\VST3\"; Components:vst3; Flags: ignoreversion recursesubdirs;
Source: "..\..\..\Plugins\LV2\**"; Excludes: "*.exe,*.pdb,*.exp,*.lib,*.ilk,*.ico,*.ini"; DestDir: "{commoncf}\LV2\"; Components:lv2; Flags: ignoreversion recursesubdirs;
Source: "..\..\..\Plugins\Standalone\**"; Excludes: "*.pdb,*.exp,*.lib,*.ilk,*.ico,*.ini"; DestDir: "{commonpf}\PlugData\"; Components:app; Flags: ignoreversion recursesubdirs;
; Source: "..\build-win\PlugData.vst3\Desktop.ini"; DestDir: "{cf32}\VST3\PlugData.vst3\"; Components:vst3; Flags: overwritereadonly ignoreversion; Attribs: hidden system;
; Source: "..\build-win\PlugData.vst3\PlugIn.ico"; DestDir: "{cf32}\VST3\PlugData.vst3\"; Components:vst3; Flags: overwritereadonly ignoreversion; Attribs: hidden system;


; Source: "..\manual\PlugData manual.pdf"; DestDir: "{app}"

[Icons]
Name: "{group}\PlugData"; Filename: "{app}\PlugData.exe"
; Name: "{group}\User guide"; Filename: "{app}\PlugData manual.pdf"
; Name: "{group}\Changelog"; Filename: "{app}\changelog.txt"
; Name: "{group}\Uninstall PlugData"; Filename: "{app}\unins000.exe"

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