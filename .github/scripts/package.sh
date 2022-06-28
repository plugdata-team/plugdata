#!/bin/bash

rm -f PlugData-$1
rm -f Plugins/Tests
cp -r Plugins PlugData
cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt


if [[ "$OSTYPE" == "darwin"* ]]; then
  zip -r -q PlugData-$1.zip PlugData
else
  mv PlugData PlugData-$1
fi
