#!/bin/bash

rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins PlugData
cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt

if [[ "$OSTYPE" == "darwin"* ]]; then
  ./.github/script/MacOS/makeinstaller-mac.sh ${{github.ref_name}}
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
 mv PlugData PlugData-$1
else
   ./.github/script/Windows/makeinstaller-win.sh ${{github.ref_name}} $1
fi

