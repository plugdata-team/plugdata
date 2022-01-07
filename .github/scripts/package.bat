@echo off
del PlugData-%1.zip \Q
xcopy Plugins PlugData \E \I

md PlugData\LV2\PlugData.lv2
PlugData\LV2\lv2_file_generator.exe PlugData\LV2\PlugData_LV2.dylib PlugData
copy PlugData\LV2\PlugData_LV2.dylib PlugData\LV2\PlugData.lv2\PlugData.dylib
move manifest.ttl PlugData\LV2\PlugData.lv2\manifest.ttl
move presets.ttl PlugData\LV2\PlugData.lv2\presets.ttl
move PlugData.ttl PlugData\LV2\PlugData.lv2\PlugData.ttl

md PlugData\LV2\Extra
move PlugData\LV2\lv2_file_generator.exe PlugData\LV2\Extra\lv2_file_generator.exe
move PlugData\LV2\PlugData_LV2.dylib PlugData\LV2\Extra\PlugData.dylib
copy README.md PlugData\README.md
copy ChangeLog.md PlugData\ChangeLog.md
copy LICENSE PlugData\LICENSE.txt
7z a PlugData-%1.zip PlugData
@echo on
