#!/bin/bash

rm -f PlugData-$1
rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins PlugData
cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt


if [[ "$OSTYPE" == "darwin"* ]]; then
  hdiutil create ./tmp.dmg -ov -volname "PlugData-MacOS-Universal" -fs HFS+ -srcfolder "./PlugData/"
  hdiutil convert ./tmp.dmg -format UDZO -o PlugData-MacOS-Universal.dmg

  rm -f ./tmp.dmg
else
  mv PlugData PlugData-$1
fi
