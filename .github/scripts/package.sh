#!/bin/bash

rm -f PlugData-$1
cp -r Plugins PlugData

mkdir Plugins/LV2/PlugData.lv2

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
./Plugins/LV2/lv2_file_generator Plugins/LV2/PlugData_LV2.so PlugData
mv Plugins/LV2/PlugData_LV2.so Plugins/LV2/PlugData.lv2/PlugData.so

elif [[ "$OSTYPE" == "darwin"* ]]; then
./Plugins/LV2/lv2_file_generator Plugins/LV2/PlugData_LV2.dylib PlugData
mv Plugins/LV2/PlugData_LV2.dylib Plugins/LV2/PlugData.lv2/PlugData.dylib
fi

mv manifest.ttl Plugins/LV2/PlugData.lv2/manifest.ttl
mv presets.ttl Plugins/LV2/PlugData.lv2/presets.ttl
mv PlugData.ttl Plugins/LV2/PlugData.lv2/PlugData.ttl

rm  Plugins/LV2/lv2_file_generator

cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt


if [[ "$OSTYPE" == "darwin"* ]]; then
  zip -r -q PlugData PlugData-$1.zip
else
  mv PlugData PlugData-$1
fi
