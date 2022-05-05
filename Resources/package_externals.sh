
rm Library.zip

mkdir Library
cd Library

mkdir Abstractions
mkdir Abstractions/else
cp ../../Libraries/pure-data/extra/*.pd ./Abstractions
cp ../../Libraries/ELSE/camomile/*.pd ./Abstractions/else
cp ../playhead.pd ./Abstractions

cp -R ../../Libraries/pure-data/doc ./Documentation
cp -R ../pddp ./Documentation/pddp
cp -R ../../Libraries/ELSE/Help-files/ ./Documentation/9.else
cp -R ../../Libraries/cyclone/documentation/help_files ./Documentation/10.cyclone

# Remove else prefix in helpfiles
find ./Documentation/9.else/ -name '*.pd' -print0 | xargs -0 sed -i "" "s/else\///g"
find ./Documentation/10.cyclone/ -name '*.pd' -print0 | xargs -0 sed -i "" "s/cyclone\///g"

rm ./Documentation/Makefile.am

mkdir Deken

cd ..
zip -r Library.zip Library
zip -d Library.zip $1 "__MACOSX/*" "*/.DS_Store";

rm -r -f Library
