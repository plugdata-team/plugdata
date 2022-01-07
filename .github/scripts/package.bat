@echo off
del PlugData-%1.zip /Q
xcopy Plugins PlugData /E /I

md Plugins/LV2/PlugData.lv2
./Plugins/LV2/lv2_file_generator Plugins/LV2/PlugData_LV2.dylib PlugData
copy Plugins/LV2/PlugData_LV2.dylib Plugins/LV2/PlugData.lv2/PlugData.dylib
move manifest.ttl Plugins/LV2/PlugData.lv2/manifest.ttl
move presets.ttl Plugins/LV2/PlugData.lv2/presets.ttl
move PlugData.ttl Plugins/LV2/PlugData.lv2/PlugData.ttl

md Plugins/LV2/Extra
move Plugins/LV2/lv2_file_generator Plugins/LV2/Extra/lv2_file_generator
move Plugins/LV2/PlugData_LV2.dylib Plugins/LV2/Extra/PlugData.dylib
copy README.md PlugData\README.md
copy ChangeLog.md PlugData\ChangeLog.md
copy LICENSE PlugData\LICENSE.txt
7z a PlugData-%1.zip PlugData
@echo on
