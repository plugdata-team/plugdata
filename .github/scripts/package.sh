#!/bin/bash

rm -f PlugData-$1
rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins PlugData
cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt


if [[ "$OSTYPE" == "darwin"* ]]; then
  ./.github/scripts/MacOS/makeinstaller-mac.sh ${GITHUB_REF##*/}
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
 mv PlugData PlugData-$1
else
  ./.github/scripts/Windows/makeinstaller-win.sh ${GITHUB_REF##*/}
fi
