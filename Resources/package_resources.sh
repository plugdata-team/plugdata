
rm Filesystem.zip

mkdir plugdata_version
cd plugdata_version

mkdir Abstractions
mkdir Abstractions/else
cp ../../Libraries/pure-data/extra/*.pd ./Abstractions
cp ../../Libraries/ELSE/Abstractions/*.pd ./Abstractions/else
cp ../playhead.pd ./Abstractions
cp ../param.pd ./Abstractions

rm ./Abstractions/else/All_objects.pd

cp -R ../../Libraries/pure-data/doc ./Documentation
cp -R ../pddp ./Documentation/pddp
cp -R ../../Libraries/ELSE/Help-files/ ./Documentation/9.else
cp ../../Libraries/ELSE/sfont~/sfont~-help.pd ./Documentation/9.else
cp ../param-help.pd ./Documentation/5.reference
cp ../playhead-help.pd ./Documentation/5.reference
cp -R ../../Libraries/cyclone/documentation/help_files ./Documentation/10.cyclone
cp -R ../../Libraries/ELSE/Live-Electronics-Tutorial/ ./Documentation/11.live-electronics-tutorial

# Remove else prefix in helpfiles
sed -i -- 's/else\///g' ./Abstractions/else/*.pd
sed -i -- 's/else\///g' ./Documentation/9.else/*.pd
sed -i -- 's/cyclone\///g' ./Documentation/10.cyclone/*.pd

rm ./Documentation/Makefile.am

cd ..
zip -r Filesystem.zip plugdata_version
zip -d Filesystem.zip $1 "__MACOSX/*" "*/.DS_Store";

rm -r -f plugdata_version
