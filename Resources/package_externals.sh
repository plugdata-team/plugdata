
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
rm ./Documentation/Makefile.am

mkdir Deken

cd ..
zip -r Library.zip Library
rm -r -f Library
