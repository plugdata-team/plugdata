
rm Abstractions.zip

mkdir Abstractions
cp ../Libraries/ELSE/camomile/*.pd ./Abstractions
cp playhead.pd ./Abstractions

cp -R ../Libraries/pure-data/doc ./Documentation
cp -R ./pddoc ./Documentation/pddoc
cp -R ../Libraries/ELSE/Help-files/ ./Documentation/9.else
cp -R ../Libraries/cyclone/documentation/help_files ./Documentation/10.cyclone
rm ./Documentation/Makefile.am

zip -r Abstractions.zip Abstractions Documentation

rm -r -f Abstractions
rm -r -f Documentation
