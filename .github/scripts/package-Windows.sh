X64BitMode=""

if [[ $1 == "x64" ]]; then
  X64BitMode="x64"
fi

VERSION=0.9.1

rm -f ./plugdata.wxs
cat > ./plugdata.wxs <<-EOL
<?xml version="1.0"?>
<?define ProductVersion = "$VERSION" ?>
<?define ProductUpgradeCode = "54a05500-6d7a-479d-b884-ba844ccaf4ce" ?>
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
<Wix
	xmlns="http://schemas.microsoft.com/wix/2006/wi">
	<Product Id="*" UpgradeCode="\$(var.ProductUpgradeCode)"
            Name="plugdata" Version="\$(var.ProductVersion)" Manufacturer="Timothy Schoen" Language="1033">
		<Package InstallerVersion="200" Compressed="yes" Comments="Windows Installer Package"/>
		<Media Id="1" Cabinet="product.cab" EmbedCab="yes"/>
		<Icon Id="ProductIcon" SourceFile="Resources\Icons\icon.ico"/>
		<Property Id="ARPPRODUCTICON" Value="ProductIcon"/>
		<WixVariable Id="WixUILicenseRtf" Value="Resources\Installer\LICENSE.rtf" />
		<Property Id="ARPHELPLINK" Value="http://www.github.com/plugdata-team/plugdata"/>
		<Property Id="ARPURLINFOABOUT" Value="http://www.github.com/plugdata-team/plugdata"/>
		<Property Id="ARPNOREPAIR" Value="1"/>
<Directory Id="TARGETDIR" Name="SourceDir">
			<!-- Copy Standalone to Program Files -->
			<Directory Id="\$(var.PlatformProgramFilesFolder)">
				<Directory Id="INSTALLDIR" Name="plugdata">
					<Component Id="STANDALONE_FILES" Guid="0a2563f0-5f49-4ae8-acda-143a019f73a2" Win64="\$(var.Win64)">
						<RemoveFile Id="STANDALONE_EXE" Name="plugdata.exe" On="both"/>
						<RemoveFile Id="PD_DLL" Name="Pd.dll" On="both"/>
						<File Id="STANDALONE_EXE" Source="Plugins\Standalone\plugdata.exe"/>
						<File Id="PD_DLL" Source="Plugins\Standalone\Pd.dll"/>
						<ReserveCost Id="STANDALONE_COST" RunFromSource="43200000" RunLocal="43200000"></ReserveCost>
					</Component>
				</Directory>
			</Directory>
			<!-- Create start menu shortcut for standalone -->
			<Directory Id="ProgramMenuFolder">
				<Directory Id="ProgramMenuSubfolder" Name="plugdata">
					<Component Id="STANDALONE_SHORTCUTS" Guid="8f2ac8c2-b3bc-4d3c-8097-b62b5eed28ae">
						<Shortcut Id="ApplicationShortcut1" Name="plugdata" Description="plugdata"
                            Target="[INSTALLDIR]plugdata.exe" WorkingDirectory="INSTALLDIR"/>
						<RegistryValue Root="HKCU" Key="Software\plugdata\plugdata"
                            Name="installed" Type="integer" Value="1" KeyPath="yes"/>
						<RemoveFolder Id="ProgramMenuSubfolder" On="uninstall"/>
					</Component>
				</Directory>
			</Directory>
			<Directory Id="\$(var.PlatformCommonFilesFolder)">
				<!-- Copy VST3 to Common Files\VST3 -->
				<Directory Id="VST3_INSTALL_DIR" Name="VST3">
					<Directory Id="VST3_PLUGIN_DIR" Name="plugdata.vst3">
						<Directory Id="VST3_CONTENTS" Name="Contents">
							<Directory Id="VST3_ARCH" Name="\$(var.VstArch)">
								<Component Id="VST3_BIN" Guid="d227e6fe-9fca-4908-a60c-f25d260f642e" Win64="\$(var.Win64)">
									<RemoveFile Id="VST3_PLUGIN" Name="plugdata.vst3" On="both"/>
									<File Id="VST3_PLUGIN" Source="Plugins\VST3\plugdata.vst3\Contents\\\$(var.VstArch)\plugdata.vst3"/>
								</Component>
							</Directory>
						</Directory>
						<Component Id="VST3_EXTRA" Guid="7962383a-5b95-4f04-b644-f13f7896e4df" Win64="\$(var.Win64)">
							<RemoveFile Id="VST3_DESKTOP" Name="desktop.ini" On="both"/>
							<RemoveFile Id="VST3_ICON" Name="Plugin.ico" On="both"/>
							<File Id="VST3_DESKTOP" Source="Plugins\VST3\plugdata.vst3\desktop.ini"/>
							<File Id="VST3_ICON" Source="Plugins\VST3\plugdata.vst3\Plugin.ico"/>
							<File Id="VST3_MODULEINFO" Source="Plugins\VST3\plugdata.vst3\Contents\Resources\moduleinfo.json"/>
							<ReserveCost Id="VST3_COST" RunFromSource="89900000" RunLocal="89900000"></ReserveCost>
						</Component>
					</Directory>
					<Directory Id="VST3_FX_PLUGIN_DIR" Name="plugdata-fx.vst3">
						<Directory Id="VST3_FX_CONTENTS" Name="Contents">
							<Directory Id="VST3_FX_ARCH" Name="\$(var.VstArch)">
								<Component Id="VST3_FX_BIN" Guid="9ce8241b-af1f-48b1-b61e-db74c4564d64" Win64="\$(var.Win64)">
									<RemoveFile Id="VST3_FX_PLUGIN" Name="plugdata-fx.vst3" On="both"/>
									<File Id="VST3_FX_PLUGIN" Source="Plugins\VST3\plugdata-fx.vst3\Contents\\\$(var.VstArch)\plugdata-fx.vst3"/>
								</Component>
							</Directory>
						</Directory>
						<Component Id="VST3_FX_EXTRA" Guid="e195aac1-1211-4b6d-a86e-9a57448955a2" Win64="\$(var.Win64)">
							<RemoveFile Id="VST3_FX_DESKTOP" Name="desktop.ini" On="both"/>
							<RemoveFile Id="VST3_FX_ICON" Name="Plugin.ico" On="both"/>
							<File Id="VST3_FX_DESKTOP" Source="Plugins\VST3\plugdata-fx.vst3\desktop.ini"/>
							<File Id="VST3_FX_ICON" Source="Plugins\VST3\plugdata-fx.vst3\Plugin.ico"/>
							<File Id="VST3_FX_MODULEINFO" Source="Plugins\VST3\plugdata-fx.vst3\Contents\Resources\moduleinfo.json"/>
						</Component>
					</Directory>
				</Directory>
				<!-- Copy LV2 to Common Files\LV2 -->
				<Directory Id="LV2_INSTALL_DIR" Name="LV2">
					<Directory Id="LV2_PLUGIN_DIR" Name="plugdata.lv2">
						<Component Id="LV2_FILES" Guid="07a69bc3-61f7-4907-8086-561f4e4150eb" Win64="\$(var.Win64)">
							<RemoveFile Id="LV2_PLUGIN" Name="plugdata.dll" On="both"/>
							<RemoveFile Id="LV2_MANIFEST" Name="manifest.ttl" On="both"/>
							<RemoveFile Id="LV2_DSP" Name="dsp.ttl" On="both"/>
							<RemoveFile Id="LV2_UI" Name="ui.ttl" On="both"/>
							<File Id="LV2_PLUGIN" Source="Plugins\LV2\plugdata.lv2\plugdata.dll"/>
							<File Id="LV2_MANIFEST" Source="Plugins\LV2\plugdata.lv2\manifest.ttl"/>
							<File Id="LV2_DSP" Source="Plugins\LV2\plugdata.lv2\dsp.ttl"/>
							<File Id="LV2_UI" Source="Plugins\LV2\plugdata.lv2\ui.ttl"/>
							<ReserveCost Id="LV2_COST" RunFromSource="82700000" RunLocal="82700000"></ReserveCost>
						</Component>
					</Directory>
					<Directory Id="LV2_FX_PLUGIN_DIR" Name="plugdata-fx.lv2">
						<Component Id="LV2_FX_FILES" Guid="b676dc16-52f8-46ef-82c2-fbc7268b12d0" Win64="\$(var.Win64)">
							<RemoveFile Id="LV2_FX_PLUGIN" Name="plugdata-fx.dll" On="both"/>
							<RemoveFile Id="LV2_FX_MANIFEST" Name="manifest.ttl" On="both"/>
							<RemoveFile Id="LV2_FX_DSP" Name="dsp.ttl" On="both"/>
							<RemoveFile Id="LV2_FX_UI" Name="ui.ttl" On="both"/>
							<File Id="LV2_FX_PLUGIN" Source="Plugins\LV2\plugdata-fx.lv2\plugdata-fx.dll"/>
							<File Id="LV2_FX_MANIFEST" Source="Plugins\LV2\plugdata-fx.lv2\manifest.ttl"/>
							<File Id="LV2_FX_DSP" Source="Plugins\LV2\plugdata-fx.lv2\dsp.ttl"/>
							<File Id="LV2_FX_UI" Source="Plugins\LV2\plugdata-fx.lv2\ui.ttl"/>
						</Component>
					</Directory>
				</Directory>
				<Directory Id="CLAP_INSTALL_DIR" Name="CLAP">
					<Component Id="CLAP_FILES" Guid="deb58e55-8e6d-435d-8cdc-790970132f53" Win64="\$(var.Win64)">
						<RemoveFile Id="CLAP_PLUGIN" Name="plugdata.clap" On="both"/>
						<File Id="CLAP_PLUGIN" Source="Plugins\CLAP\plugdata.clap"/>
					</Component>
					<Component Id="CLAP_FX_FILES" Guid="90ff8eae-2cfe-4070-9c73-b62e4d219a36" Win64="\$(var.Win64)">
						<RemoveFile Id="CLAP_FX_PLUGIN" Name="plugdata-fx.clap" On="both"/>
						<File Id="CLAP_FX_PLUGIN" Source="Plugins\CLAP\plugdata-fx.clap"/>
					</Component>
				</Directory>
			</Directory>
		</Directory>
		<Property Id="WIXUI_INSTALLDIR" Value="INSTALLDIR" />
		<UI>
			<UIRef Id="WixUI_FeatureTree" />
		</UI>
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
    <Feature Id="CLAP" Level="1" Title="CLAP Plugin">
			<ComponentRef Id="CLAP_FILES"/>
			<ComponentRef Id="CLAP_FX_FILES"/>
		</Feature>
		<!-- define powershell script as base64 that will remove registry entries for old plugdata versions -->
		<Property Id="reg_clean">powershell.exe -ExecutionPolicy Bypass -NoProfile -WindowStyle Hidden -e JABkAGkAcwBwAGwAYQB5AE4AYQBtAGUAIAA9ACAAIgBwAGwAdQBnAGQAYQB0AGEAIgAKACQAcAB1AGIAbABpAHMAaABlAHIAIAA9ACAAIgBUAGkAbQBvAHQAaAB5ACAAUwBjAGgAbwBlAG4AIgAKACQAcgBlAGcAaQBzAHQAcgB5AFAAYQB0AGgAIAA9ACAAIgBIAEsATABNADoAXABTAE8ARgBUAFcAQQBSAEUAXABNAGkAYwByAG8AcwBvAGYAdABcAFcAaQBuAGQAbwB3AHMAXABDAHUAcgByAGUAbgB0AFYAZQByAHMAaQBvAG4AXABVAG4AaQBuAHMAdABhAGwAbAAiAAoAJABzAHUAYgBLAGUAeQBzACAAPQAgAEcAZQB0AC0AQwBoAGkAbABkAEkAdABlAG0AIAAtAFAAYQB0AGgAIAAkAHIAZQBnAGkAcwB0AHIAeQBQAGEAdABoAAoACgBmAG8AcgBlAGEAYwBoACAAKAAkAHMAdQBiAEsAZQB5ACAAaQBuACAAJABzAHUAYgBLAGUAeQBzACkAIAB7AAoAIAAgACAAIAAkAGMAdQByAHIAZQBuAHQASwBlAHkAIAA9ACAARwBlAHQALQBJAHQAZQBtAFAAcgBvAHAAZQByAHQAeQAgAC0AUABhAHQAaAAgACQAcwB1AGIASwBlAHkALgBQAFMAUABhAHQAaAAKACAAIAAgACAAaQBmACAAKAAkAGMAdQByAHIAZQBuAHQASwBlAHkALgBEAGkAcwBwAGwAYQB5AE4AYQBtAGUAIAAtAGUAcQAgACQAZABpAHMAcABsAGEAeQBOAGEAbQBlACAALQBhAG4AZAAgACQAYwB1AHIAcgBlAG4AdABLAGUAeQAuAFAAdQBiAGwAaQBzAGgAZQByACAALQBlAHEAIAAkAHAAdQBiAGwAaQBzAGgAZQByACkAIAB7AAoAIAAgACAAIAAgACAAIAAgAFIAZQBtAG8AdgBlAC0ASQB0AGUAbQAgAC0AUABhAHQAaAAgACQAcwB1AGIASwBlAHkALgBQAFMAUABhAHQAaAAgAC0AUgBlAGMAdQByAHMAZQAgAC0ARgBvAHIAYwBlAAoAIAAgACAAIAAgACAAIAAgAFcAcgBpAHQAZQAtAEgAbwBzAHQAIAAiAFIAZQBnAGkAcwB0AHIAeQAgAGUAbgB0AHIAeQAgAHIAZQBtAG8AdgBlAGQAOgAgACQAKAAkAHMAdQBiAEsAZQB5AC4AUABTAFAAYQB0AGgAKQAiAAoAIAAgACAAIAB9AAoAfQA=
    	</Property>
		<CustomAction Id="CleanRegistry" Execute="deferred" Directory="TARGETDIR" ExeCommand='[reg_clean]' Return="ignore" Impersonate="no"/>
		<InstallExecuteSequence>
      		<Custom Action="CleanRegistry" After="InstallInitialize"></Custom>
			<RemoveExistingProducts After="InstallValidate"/>
    	</InstallExecuteSequence>
	</Product>
</Wix>
EOL


if [[ $X64BitMode == "x64" ]]; then
"C:/Program Files (x86)/WiX Toolset v3.14/bin/candle" -arch x64 plugdata.wxs
"C:/Program Files (x86)/WiX Toolset v3.14/bin/light" plugdata.wixobj -out plugdata-Installer.msi -ext WixUIExtension
cp ".\plugdata-Installer.msi" ".\plugdata-Win64.msi"
else
"C:/Program Files (x86)/WiX Toolset v3.14/bin/candle" plugdata.wxs
"C:/Program Files (x86)/WiX Toolset v3.14/bin/light" plugdata.wixobj -out plugdata-Installer.msi -ext WixUIExtension
cp ".\plugdata-Installer.msi" ".\plugdata-Win32.msi"
fi

# - Codesign Installer for Windows 8+
# -"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\signtool.exe" sign /f "XXXXX.p12" /p XXXXX /d "plugdata Installer" ".\installer\plugdata Installer.exe"

# -if %1 == 1 (
# -copy ".\installer\plugdata Installer.exe" ".\installer\plugdata Demo Installer.exe"
# -del ".\installer\plugdata Installer.exe"
# -)
