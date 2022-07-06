#!/bin/bash

rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins PlugData
cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt

if [[ "$OSTYPE" == "darwin"* ]]; then
  sudo chmod -R 777 ./Plugins/

  gon -log-level=debug -log-json .github/scripts/MacOS/gon_regular.json
  gon -log-level=debug -log-json .github/scripts/MacOS/gon_fx.json
  gon -log-level=debug -log-json .github/scripts/MacOS/gon_midi.json

elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
 mv PlugData PlugData-$1
fi