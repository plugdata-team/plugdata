@echo off
del PlugData-%1.zip /Q
xcopy Plugins PlugData /S /H /E /I

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
