X64BitMode=""

if [[ $1 == "x64" ]]; then
  X64BitMode="x64"
fi

VERSION=${GITHUB_REF#refs/*/}

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
<?else ?>
  <?define Win64 = "no" ?>
  <?define PlatformProgramFilesFolder = "ProgramFilesFolder" ?>
  <?define PlatformCommonFilesFolder = "CommonFilesFolder" ?>
  <?define WixPlatform = "x64" ?>
<?endif ?>

<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi">
   <Product Id="*" UpgradeCode="\$(var.ProductUpgradeCode)" 
            Name="PlugData" Version="\$(var.ProductVersion)" Manufacturer="Timothy Schoen" Language="1033">
      <Package InstallerVersion="200" Compressed="yes" Comments="Windows Installer Package"/>
      <Media Id="1" Cabinet="product.cab" EmbedCab="yes"/>

      <Property Id="ARPPRODUCTICON" Value="ProductIcon"/>
      <Property Id="ARPHELPLINK" Value="http://www.github.com/timothyschoen/PlugData"/>
      <Property Id="ARPURLINFOABOUT" Value="http://www.github.com/timothyschoen/PlugData"/>
      <Property Id="ARPNOREPAIR" Value="1"/>
      <Condition Message="A newer version of this software is already installed.">NOT NEWERVERSIONDETECTED</Condition>
         <Directory Id="TARGETDIR" Name="SourceDir">

         <!-- Copy Standalone to Program Files -->
         <Directory Id="PlatformProgramFilesFolder">
            <Directory Id="INSTALLDIR" Name="PlugData">
               <Component Id="STANDALONE_FILES" Guid="0a2563f0-5f49-4ae8-acda-143a019f73a2">
                  <File Id="MainExecutable" Source="Plugins\Standalone\PlugData.exe"/>
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

         
         <Directory Id="PlatformCommonFilesFolder">

            <!-- Copy VST3 to Common Files\VST3 -->
            <Directory Id="VST3_INSTALL_DIR" Name="VST3">
               <Component Id="VST3_FILES" Guid="0a2563f0-5f49-4ae8-acda-143a019f73a2">
                  <File Id="VST3_PLUGIN" Source="Plugins\VST3\PlugData.vst3"/>
               </Component>
            </Directory>

            <!-- Copy LV2 to Common Files\LV2 -->
            <Directory Id="LV2_INSTALL_DIR" Name="LV2">
               <Component Id="LV2_FILES" Guid="0a2563f0-5f49-4ae8-acda-143a019f73a2">
                  <File Id="LV2_PLUGIN" Source="Plugins\LV2\PlugData.lv2"/>
               </Component>
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
         <ComponentRef Id="VST3_FILES"/>
      </Feature>
      <Feature Id="LV2" Level="1" Title="LV2 Plugin">
         <ComponentRef Id="LV2_FILES"/>
      </Feature>
   </Product>

   
</Wix>
EOL

"C:/Program Files (x86)/WiX Toolset v3.11/bin/candle" PlugData.wxs
"C:/Program Files (x86)/WiX Toolset v3.11/bin/light" PlugData.wixobj -out PlugData-Installer.msi -ext WixUIExtension

if [[ $1 == "x64" ]]; then
cp ".\PlugData-Installer.msi" ".\PlugData-Win64.msi"
else
cp ".\PlugData-Installer.msi" ".\PlugData-Win32.msi"
fi

# - Codesign Installer for Windows 8+
# -"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\signtool.exe" sign /f "XXXXX.p12" /p XXXXX /d "PlugData Installer" ".\installer\PlugData Installer.exe"

# -if %1 == 1 (
# -copy ".\installer\PlugData Installer.exe" ".\installer\PlugData Demo Installer.exe"
# -del ".\installer\PlugData Installer.exe"
# -)
