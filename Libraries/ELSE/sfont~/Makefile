
cflags = -Ishared -DHAVE_STRUCT_TIMESPEC

lib.name = ELSE

file := ../shared/elsefile.c 
sfont~.class.sources := sfont~.c $(file)

ldlibs = -lfluidsynth

datafiles = sfont~-help.pd
datadirs = sf

# Makefile based on pd-lib-builder by Katja Vetter, see: https://github.com/pure-data/pd-lib-builder

PDLIBBUILDER_DIR=../pd-lib-builder/
include $(firstword $(wildcard $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder Makefile.pdlibbuilder))

localdep_linux: install
	scripts/localdeps.linux.sh "${installpath}/sfont~.${extension}"

localdep_windows: install
	scripts/localdeps.win.sh "${installpath}/sfont~.${extension}"

localdep_macos: install
	scripts/localdeps.macos.sh "${installpath}/sfont~.${extension}"
