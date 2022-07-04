#!/bin/bash

rm -f PlugData-$1
rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins PlugData
cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt


if [[ "$OSTYPE" == "darwin"* ]]; then
  ./.github/scripts/MacOS/makeinstaller-mac.sh 0.6.0
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
 mv PlugData PlugData-$1
else
  mv PlugData PlugData-$1
fi
