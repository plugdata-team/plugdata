@echo off
del PlugData-%1.zip /Q
xcopy Plugins PlugData /S /H /E /I


cd PlugData\LV2
md PlugData.lv2
start lv2_file_generator.exe PlugData_LV2.dll PlugData

move PlugData_LV2.dll PlugData.lv2\PlugData.dll
xcopy manifest.ttl PlugData.lv2\manifest.ttl
xcopy presets.ttl PlugData.lv2\presets.ttl
xcopy PlugData.ttl PlugData.lv2\PlugData.ttl

del lv2_file_generator.exe /F

cd ..
cd ..

move PlugData\VST3\PlugData.vst3\Contents\x86-win\PlugData.vst3 PlugData\VST3\PlugData_temp.vst3
move PlugData\VST3\PlugData.vst3\Contents\x86_64-win\PlugData.vst3 PlugData\VST3\PlugData_temp.vst3
rd PlugData\VST3\PlugData.vst3\ /S /Q
move PlugData\VST3\PlugData_temp.vst3 PlugData\VST3\PlugData.vst3

move PlugData\VST3\PlugDataFx.vst3\Contents\x86-win\PlugDataFx.vst3 PlugData\VST3\PlugDataFx_temp.vst3
move PlugData\VST3\PlugDataFx.vst3\Contents\x86_64-win\PlugDataFx.vst3 PlugData\VST3\PlugDataFx_temp.vst3
rd PlugData\VST3\PlugDataFx.vst3\ /S /Q
move PlugData\VST3\PlugDataFx_temp.vst3 PlugData\VST3\PlugDataFx.vst3



copy README.md PlugData\README.md
copy ChangeLog.md PlugData\ChangeLog.md
copy LICENSE PlugData\LICENSE.txt
move PlugData PlugData-%1
@echo on
