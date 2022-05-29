@echo off
del PlugData-%1.zip /Q
xcopy Plugins PlugData /S /H /E /I

copy README.md PlugData\README.md
copy ChangeLog.md PlugData\ChangeLog.md
copy LICENSE PlugData\LICENSE.txt
move PlugData PlugData-%1
@echo on
