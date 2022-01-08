@echo off
del PlugData-%1.zip /Q
xcopy Plugins PlugData /S /H /E /I

md PlugData\LV2\PlugData.lv2
start PlugData\LV2\lv2_file_generator.exe PlugData\LV2\PlugData_LV2.dll PlugData
copy PlugData\LV2\PlugData_LV2.dll PlugData\LV2\PlugData.lv2\PlugData.dll
move manifest.ttl PlugData\LV2\PlugData.lv2\manifest.ttl
move presets.ttl PlugData\LV2\PlugData.lv2\presets.ttl
move PlugData.ttl PlugData\LV2\PlugData.lv2\PlugData.ttl

del PlugData\LV2\lv2_file_generator.exe /F

copy README.md PlugData\README.md
copy ChangeLog.md PlugData\ChangeLog.md
copy LICENSE PlugData\LICENSE.txt
7z a PlugData-%1.zip PlugData
@echo on
