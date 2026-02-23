#!/bin/bash

rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins plugdata
cp README.md plugdata/README.md
cp LICENSE plugdata/Resources/Installer/LICENSE.txt

CANONICAL=$(find plugdata -name "plugdata-resources.bin" | head -1)
if [ -n "$CANONICAL" ]; then
    find plugdata -name "plugdata-resources.bin" ! -path "$CANONICAL" -exec ln -f "$CANONICAL" {} \;
fi
# Create tar.xz with unique name for each distro
tar -cvf - plugdata | xz -9 > $1

.github/scripts/generate-upload-info.sh $1
