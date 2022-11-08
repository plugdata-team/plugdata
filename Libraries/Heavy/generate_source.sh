rm -rf Source
python3 -m nuitka --generate-c-only ./hvcc/hvcc/__init__.py --follow-stdlib --standalone --output-dir=Source
mv -f Source/__init__.build/*.h Source
mv -f Source/__init__.build/*.c Source
rm -rf Source/__init__.build/
rm -f Source/__init__.dist