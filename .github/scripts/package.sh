#!/bin/bash

rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins PlugData
cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt

if [[ "$OSTYPE" == "darwin"* ]]; then
  sudo chmod -R 777 ./Plugins/

# Sign app with hardened runtime and audio entitlement
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" --options runtime --entitlements ./Resources/Entitlements.plist ./PlugData/Standalone/*.app

# Sign plugins
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./PlugData/VST3/*.vst3
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./PlugData/AU/*.component
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./PlugData/LV2/PlugData.lv2/libPlugData.so
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./PlugData/LV2/PlugDataFx.lv2/libPlugDataFx.so

elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
 mv PlugData PlugData-$1
fi

