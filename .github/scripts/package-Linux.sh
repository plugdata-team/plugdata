#!/bin/bash

rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins plugdata
cp README.md plugdata/README.md
cp LICENSE plugdata/LICENSE.txt

CANONICAL=$(find plugdata -name "libBinaryData.so" | head -1)
if [ -n "$CANONICAL" ]; then
    find plugdata -name "libBinaryData.so" ! -path "$CANONICAL" -exec ln -f "$CANONICAL" {} \;
fi
# Create tar.xz with unique name for each distro
tar -cvf - plugdata | xz -9 > $1

.github/scripts/generate-upload-info.sh $1
