#!/bin/bash

rm -rf Plugins/Release
rm -f Plugins/Tests
cp -r Plugins plugdata
cp README.md plugdata/README.md
cp LICENSE plugdata/LICENSE.txt

# Create unique names for each distro
mv plugdata plugdata-$1

