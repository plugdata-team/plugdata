X64BitMode=""

if [[ $1 == "x64" ]]; then
  X64BitMode="x64"
fi

VERSION=0.9

rm -f ./PlugData.wxs 
cat > ./PlugData.wxs <<-EOL
<?xml version="1.0"?>
<?define ProductVersion = "$VERSION"?>
<?define ProductUpgradeCode = "097876c6-da37-4cf4-932e-58cd65ce272f"?>
<?if "$X64BitMode" = "x64" ?>
  <?define Win64 = "yes" ?>
  <?define PlatformProgramFilesFolder = "ProgramFiles64Folder" ?>
  <?define PlatformCommonFilesFolder = "CommonFiles64Folder" ?>
  <?define WixPlatform = "x64" ?>
  <?define VstArch = "x86_64-win" ?>
<?else ?>
  <?define Win64 = "no" ?>
  <?define PlatformProgramFilesFolder = "ProgramFilesFolder" ?>
  <?define PlatformCommonFilesFolder = "CommonFilesFolder" ?>
  <?define VstArch = "x86-win" ?>
<?endif ?>

<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
   <Product Id="*" UpgradeCode="\$(var.ProductUpgradeCode)" 
            Name="PlugData" Version="\$(var.ProductVersion)" Manufacturer="Timothy Schoen" Language="1033">
      <Package InstallerVersion="200" Compressed="yes" Comments="Windows Installer Package"/>
      <Media Id="1" Cabinet="product.cab" EmbedCab="yes"/>

      <Property Id="LV2_SOURCE_DIR" Value="Plugins/LV2/PlugData.lv2" />
      <Property Id="VST3_SOURCE_DIR" Value="Plugins/VST3/PlugData.vst3" />
      <Property Id="ARPPRODUCTICON" Value="ProductIcon"/>
      <Property Id="ARPHELPLINK" Value="http://www.github.com/timothyschoen/PlugData"/>
      <Property Id="ARPURLINFOABOUT" Value="http://www.github.com/timothyschoen/PlugData"/>
      <Property Id="ARPNOREPAIR" Value="0"/>
         <Directory Id="TARGETDIR" Name="SourceDir">

         <!-- Copy Standalone to Program Files -->
         <Directory Id="\$(var.PlatformProgramFilesFolder)">
            <Directory Id="INSTALLDIR" Name="PlugData">
               <Component Id="STANDALONE_FILES" Guid="0a2563f0-5f49-4ae8-acda-143a019f73a2" Win64="\$(var.Win64)">
                  <File Id="MainExecutable" Source="Plugins\Standalone\PlugData.exe"/>
                  <File Id="PdDLL" Source="Plugins\Standalone\Pd.dll"/>
               </Component>
            </Directory>
         </Directory>

         <!-- Create start menu shortcut for standalone -->
         <Directory Id="ProgramMenuFolder">
            <Directory Id="ProgramMenuSubfolder" Name="PlugData">
               <Component Id="STANDALONE_SHORTCUTS" Guid="8f2ac8c2-b3bc-4d3c-8097-b62b5eed28ae">
                  <Shortcut Id="ApplicationShortcut1" Name="PlugData" Description="PlugData" 
                            Target="[INSTALLDIR]PlugData.exe" WorkingDirectory="INSTALLDIR"/>
                  <RegistryValue Root="HKCU" Key="Software\PlugData\PlugData" 
                            Name="installed" Type="integer" Value="1" KeyPath="yes"/>
                  <RemoveFolder Id="ProgramMenuSubfolder" On="uninstall"/>
               </Component>
            </Directory>
         </Directory>

         <Directory Id="\$(var.PlatformCommonFilesFolder)">

            <!-- Copy VST3 to Common Files\VST3 -->
            <Directory Id="VST3_INSTALL_DIR" Name="VST3">
               <Directory Id="VST3_PLUGIN_DIR" Name="PlugData.vst3">
                <Directory Id="VST3_CONTENTS" Name="Contents">
                      <Directory Id="VST3_ARCH" Name="\$(var.VstArch)">
                          <Component Id="VST3_BIN" Guid="d227e6fe-9fca-4908-a60c-f25d260f642e" Win64="\$(var.Win64)">
                          <File Id="VST3_PLUGIN" Source="Plugins\VST3\PlugData.vst3\Contents\x86_64-win\PlugData.vst3"/>
                          </Component>
                      </Directory>
                  </Directory>
                  <Component Id="VST3_EXTRA" Guid="7962383a-5b95-4f04-b644-f13f7896e4df" Win64="\$(var.Win64)">
                  <File Id="VST3_DESKTOP" Source="Plugins\VST3\PlugData.vst3\desktop.ini"/>
                  <File Id="VST3_ICON" Source="Plugins\VST3\PlugData.vst3\Plugin.ico"/>
                  </Component>
                </Directory>

               <Directory Id="VST3_FX_PLUGIN_DIR" Name="PlugDataFx.vst3">
                <Directory Id="VST3_FX_CONTENTS" Name="Contents">
                      <Directory Id="VST3_FX_ARCH" Name="\$(var.VstArch)">
                          <Component Id="VST3_FX_BIN" Guid="9ce8241b-af1f-48b1-b61e-db74c4564d64" Win64="\$(var.Win64)">
                          <File Id="VST3_FX_PLUGIN" Source="Plugins\VST3\PlugDataFx.vst3\Contents\x86_64-win\PlugDataFx.vst3"/>
                          </Component>
                      </Directory>
                  </Directory>
                  <Component Id="VST3_FX_EXTRA" Guid="e195aac1-1211-4b6d-a86e-9a57448955a2" Win64="\$(var.Win64)">
                  <File Id="VST3_FX_DESKTOP" Source="Plugins\VST3\PlugDataFx.vst3\desktop.ini"/>
                  <File Id="VST3_FX_ICON" Source="Plugins\VST3\PlugDataFx.vst3\Plugin.ico"/>
                  </Component>
                </Directory>

            </Directory>

            <!-- Copy LV2 to Common Files\LV2 -->
            <Directory Id="LV2_INSTALL_DIR" Name="LV2">
                <Directory Id="LV2_PLUGIN_DIR" Name="PlugData.lv2">
                <Component Id="LV2_FILES" Guid="07a69bc3-61f7-4907-8086-561f4e4150eb" Win64="\$(var.Win64)">
                      <File Id="LV2_PLUGIN" Source="Plugins\LV2\PlugData.lv2\PlugData.dll"/>
                      <File Id="LV2_MANIFEST" Source="Plugins\LV2\PlugData.lv2\manifest.ttl"/>
                      <File Id="LV2_DSP" Source="Plugins\LV2\PlugData.lv2\dsp.ttl"/>
                      <File Id="LV2_UI" Source="Plugins\LV2\PlugData.lv2\ui.ttl"/>
                </Component>
              </Directory>
                <Directory Id="LV2_FX_PLUGIN_DIR" Name="PlugDataFx.lv2">
                <Component Id="LV2_FX_FILES" Guid="b676dc16-52f8-46ef-82c2-fbc7268b12d0" Win64="\$(var.Win64)">
                    <File Id="LV2_FX_PLUGIN" Source="Plugins\LV2\PlugDataFx.lv2\PlugDataFx.dll"/>
                    <File Id="LV2_FX_MANIFEST" Source="Plugins\LV2\PlugDataFx.lv2\manifest.ttl"/>
                    <File Id="LV2_FX_DSP" Source="Plugins\LV2\PlugDataFx.lv2\dsp.ttl"/>
                    <File Id="LV2_FX_UI" Source="Plugins\LV2\PlugDataFx.lv2\ui.ttl"/>
                </Component>
               </Directory>
            </Directory>
         </Directory>
            
         </Directory>

      <Property Id="WIXUI_INSTALLDIR" Value="INSTALLDIR" />

      <UI>
         <UIRef Id="WixUI_InstallDir" />
         <Publish Dialog="WelcomeDlg"
               Control="Next"
               Event="NewDialog"
               Value="FeaturesDlg"
               Order="2">1</Publish>
          
          <Publish Dialog="FeaturesDlg"
               Control="Next"
               Event="NewDialog"
               Value="InstallDirDlg"
               Order="2">1</Publish>
         <Publish Dialog="FeaturesDlg" Control="Back" Event="NewDialog" Value="WelcomeDlg">1</Publish>
         <Publish Dialog="InstallDirDlg"
               Control="Back"
               Event="NewDialog"
               Value="FeaturesDlg"
               Order="2">1</Publish>
      </UI>

      <InstallExecuteSequence>
         <RemoveExistingProducts After="InstallValidate"/>
      </InstallExecuteSequence>

      <Feature Id="DefaultFeature" Level="1" Title="Standalone App">
         <ComponentRef Id="STANDALONE_FILES"/>
         <ComponentRef Id="STANDALONE_SHORTCUTS"/>
      </Feature>
      <Feature Id="VST3" Level="1" Title="VST3 Plugin">
         <ComponentRef Id="VST3_BIN"/>
         <ComponentRef Id="VST3_EXTRA"/>
         <ComponentRef Id="VST3_FX_BIN"/>
         <ComponentRef Id="VST3_FX_EXTRA"/>
      </Feature>
      <Feature Id="LV2" Level="1" Title="LV2 Plugin">
         <ComponentRef Id="LV2_FILES"/>
         <ComponentRef Id="LV2_FX_FILES"/>
      </Feature>
   </Product>

   
</Wix>
EOL



if [[ $1 == "x64" ]]; then
"C:/Program Files (x86)/WiX Toolset v3.11/bin/candle" -arch x64 PlugData.wxs
"C:/Program Files (x86)/WiX Toolset v3.11/bin/light" PlugData.wixobj -out PlugData-Installer.msi -ext WixUIExtension
cp ".\PlugData-Installer.msi" ".\PlugData-Win64.msi"
else
"C:/Program Files (x86)/WiX Toolset v3.11/bin/candle" PlugData.wxs
"C:/Program Files (x86)/WiX Toolset v3.11/bin/light" PlugData.wixobj -out PlugData-Installer.msi -ext WixUIExtension
cp ".\PlugData-Installer.msi" ".\PlugData-Win32.msi"
fi

# - Codesign Installer for Windows 8+
# -"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\signtool.exe" sign /f "XXXXX.p12" /p XXXXX /d "PlugData Installer" ".\installer\PlugData Installer.exe"

# -if %1 == 1 (
# -copy ".\installer\PlugData Installer.exe" ".\installer\PlugData Demo Installer.exe"
# -del ".\installer\PlugData Installer.exe"
# -)
