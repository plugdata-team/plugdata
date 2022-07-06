#!/bin/bash

rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins PlugData
cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt

if [[ "$OSTYPE" == "darwin"* ]]; then
  #sudo chmod -R 777 ./Plugins/
  #codesign -f -v -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/VST3/*.vst3 --timestamp
  #codesign -f -v -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/Standalone/*.app --timestamp
  #codesign -f -v -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/AU/*.component --timestamp
  #codesign -f -v -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/LV2/PlugData.lv2/libPlugData.so --timestamp
  #codesign -f -v -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/LV2/PlugDataFx.lv2/libPlugDataFx.so --timestamp
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
 mv PlugData PlugData-$1
fi