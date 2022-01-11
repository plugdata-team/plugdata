#!/bin/bash

rm -f PlugData-$1
cp -r Plugins PlugData

mkdir PlugData/LV2/PlugData.lv2

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
./Plugins/LV2/lv2_file_generator Plugins/LV2/PlugData_LV2.so PlugData
mv PlugData/LV2/PlugData_LV2.so Plugins/LV2/PlugData.lv2/PlugData.so

elif [[ "$OSTYPE" == "darwin"* ]]; then
./Plugins/LV2/lv2_file_generator PlugData/LV2/PlugData_LV2.dylib PlugData
mv PlugData/LV2/PlugData_LV2.dylib Plugins/LV2/PlugData.lv2/PlugData.dylib
fi

mv manifest.ttl PlugData/LV2/PlugData.lv2/manifest.ttl
mv presets.ttl PlugData/LV2/PlugData.lv2/presets.ttl
mv PlugData.ttl PlugData/LV2/PlugData.lv2/PlugData.ttl

rm PlugData/LV2/lv2_file_generator

cp README.md PlugData/README.md
cp LICENSE PlugData/LICENSE.txt


if [[ "$OSTYPE" == "darwin"* ]]; then
  zip -r -q PlugData-$1.zip PlugData
else
  mv PlugData PlugData-$1
fi
