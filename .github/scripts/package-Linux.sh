#!/bin/bash

rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins PlugData
cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt

# Create unique names for each distro
mv PlugData PlugData-$1

