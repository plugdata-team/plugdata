# Makefile for the pthreads test suite.
# If all of the .pass files can be created, the test suite has passed.
#
# --------------------------------------------------------------------------
#
#      pthreads-win32 - POSIX Threads Library for Win32
#      Copyright(C) 1998 John E. Bossom
#      Copyright(C) 1999-2021 pthreads-win32 / pthreads4w contributors
# 
#      The current list of contributors is contained
#      in the file CONTRIBUTORS included with the source
#      code distribution. The list can also be seen at the
#      following World Wide Web location:
#      http://sources.redhat.com/pthreads-win32/contributors.html
# 
#      This library is free software; you can redistribute it and/or
#      modify it under the terms of the GNU Lesser General Public
#      License as published by the Free Software Foundation; either
#      version 2 of the License, or (at your option) any later version.
# 
#      This library is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#      Lesser General Public License for more details.
# 
#      You should have received a copy of the GNU Lesser General Public
#      License along with this library in the file COPYING.LIB;
#      if not, write to the Free Software Foundation, Inc.,
#      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
#

DLL_VER	= 2$(EXTRAVERSION)

CP			= cp -f
MV			= mv -f
RM			= rm -f
CAT			= cat
MKDIR		= mkdir
ECHO		= echo
TOUCH		= $(ECHO) Passed >
TESTFILE	= test -f
TESTDIR		= test -d
AND			= &&

# For cross compiling use e.g.
# # make CROSS=i386-mingw32msvc- clean GC
CROSS   =

# For cross testing use e.g.
# # make RUN=wine CROSS=i386-mingw32msvc- clean GC
RUN     =

AR      = $(CROSS)ar
DLLTOOL = $(CROSS)dlltool
CC      = $(CROSS)gcc
CXX     = $(CROSS)g++
RANLIB  = $(CROSS)ranlib

#
# Mingw
#
XLIBS	=
XXCFLAGS	= 
XXLIBS	=
OPT	= -O3
DOPT	= -g -O0
CFLAGS	= ${OPT} $(ARCH) -UNDEBUG -Wall $(XXCFLAGS)
LFLAGS	= $(ARCH) $(XXLFLAGS)
#
# Uncomment this next to link the GCC/C++ runtime libraries statically
# (Be sure to read about these options and their associated caveats
# at http://gcc.gnu.org/onlinedocs/gcc/Link-Options.html)
#
# NOTE 1: Doing this appears to break GCE:pthread_cleanup_*(), which
# relies on C++ class destructors being called when leaving scope.
#
# NOTE 2: If you do this DO NOT distribute your pthreads DLLs with
# the official filenaming, i.e. pthreadVC2.dll, etc. Instead, change DLL_VER
# above to "2slgcc" for example, to build "pthreadGC2slgcc.dll", etc.
#
#LFLAGS	+= -static-libgcc -static-libstdc++
BUILD_DIR	= ..
INCLUDES	= -I.

TEST	= GC

# Default lib version
GCX	= GC$(DLL_VER)

# Files we need to run the tests
# - paths are relative to pthreads build dir.
HDR	= pthread.h semaphore.h sched.h
LIB	= libpthread$(GCX).a
DLL	= pthread$(GCX).dll
# The next path is relative to $BUILD_DIR
QAPC	=  # ../QueueUserAPCEx/User/quserex.dll

include common.mk

.INTERMEDIATE: $(ALL_KNOWN_TESTS:%=%.exe) $(BENCHTESTS:%=%.exe)
.SECONDARY: $(ALL_KNOWN_TESTS:%=%.exe) $(ALL_KNOWN_TESTS:%=%.pass) $(BENCHTESTS:%=%.exe) $(BENCHTESTS:%=%.bench)
.PRECIOUS: $(ALL_KNOWN_TESTS:%=%.exe) $(ALL_KNOWN_TESTS:%=%.pass) $(BENCHTESTS:%=%.exe) $(BENCHTESTS:%=%.bench)

ASM				= $(ALL_KNOWN_TESTS:%=%.s)
TESTS			= $(ALL_KNOWN_TESTS)
BENCHRESULTS	= $(BENCHTESTS:%=%.bench)

#
# To build and run "foo.exe" and "bar.exe" only use, e.g.:
# make clean GC NO_DEPS=1 TESTS="foo bar"
#
# To build and run "foo.exe" and "bar.exe" and run all prerequisite tests
# use, e.g.:
# make clean GC TESTS="foo bar"
#
# Set TESTS to one or more tests.
#
ifndef NO_DEPS
include runorder.mk
endif

help:
	@ $(ECHO) "Run one of the following command lines:"
	@ $(ECHO) "$(MAKE) clean GC                 (to test using GC dll with C (no EH) applications)"
	@ $(ECHO) "$(MAKE) clean GCX                (to test using GC dll with C++ (EH) applications)"
	@ $(ECHO) "$(MAKE) clean GCE                (to test using GCE dll with C++ (EH) applications)"
	@ $(ECHO) "$(MAKE) clean GC-bench           (to benchtest using GNU C dll with C cleanup code)"
	@ $(ECHO) "$(MAKE) clean GC-debug           (to test using GC dll with C (no EH) applications)"
	@ $(ECHO) "$(MAKE) clean GC-static          (to test using GC static lib with C (no EH) applications)"
	@ $(ECHO) "$(MAKE) clean GC-static-debug    (to test using GC static lib with C (no EH) applications)"
	@ $(ECHO) "$(MAKE) clean GCE-static         (to test using GC static lib with C (no EH) applications)"
	@ $(ECHO) "$(MAKE) clean GCE-static-debug   (to test using GC static lib with C (no EH) applications)"
	@ $(ECHO) "$(MAKE) clean GCE-debug          (to test using GCE dll with C++ (EH) applications)"
	@ $(ECHO) "$(MAKE) clean GCX-static         (to test using GC static lib with C++ applications)"
	@ $(ECHO) "$(MAKE) clean GCX-static-debug   (to test using GC static lib with C++ applications)"
	@ $(ECHO) "$(MAKE) clean GCX-debug          (to test using GCE dll with C++ (EH) applications)"
	@ $(ECHO) "$(MAKE) clean GC TESTS="foo bar" (to build individual tests \"foo.c and bar.c\" with C and run using GC dll)"

GC:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)" CC=$(CC) XXCFLAGS="-D__CLEANUP_C" allpassed

GC-asm:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)" CC=$(CC) XXCFLAGS="-D__CLEANUP_C" all-asm

GC-bench:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)" CC=$(CC) XXCFLAGS="-D__CLEANUP_C" XXLIBS="benchlib.o" all-bench

GC-bench-debug:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)d" CC=$(CC) XXCFLAGS="-D__CLEANUP_C" XXLIBS="benchlib.o" OPT="${DOPT}" all-bench

GC-debug:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)d" CC=$(CC) XXCFLAGS="-D__CLEANUP_C" OPT="${DOPT}" allpassed

GC-static GC-small-static:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)" CC=$(CC) XXCFLAGS="-D__CLEANUP_C -DPTW32_STATIC_LIB" DLL="" allpassed

GC-static-debug GC-small-static-debug:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)d" CC=$(CC) XXCFLAGS="-D__CLEANUP_C -DPTW32_STATIC_LIB" OPT="$(DOPT)" DLL="" allpassed

GCE:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GCE$(DLL_VER)" CC=$(CXX) XXCFLAGS="-mthreads -D__CLEANUP_CXX" allpassed

GCE-debug:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GCE$(DLL_VER)d" CC=$(CXX) XXCFLAGS="-D__CLEANUP_CXX" OPT="${DOPT}" allpassed

GCE-static GCE-small-static:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GCE$(DLL_VER)" CC=$(CXX) XXCFLAGS="-D__CLEANUP_CXX -DPTW32_STATIC_LIB" DLL="" allpassed

GCE-static-debug GCE-small-static-debug:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GCE$(DLL_VER)d" CC=$(CXX) XXCFLAGS="-D__CLEANUP_CXX -DPTW32_STATIC_LIB" OPT="$(DOPT)" DLL="" allpassed

GCX:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)" CC=$(CXX) XXCFLAGS="-mthreads -D__CLEANUP_C" allpassed

GCX-debug:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)d" CC=$(CXX) XXCFLAGS="-D__CLEANUP_C" OPT="${DOPT}" allpassed

GCX-static GCX-small-static:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)" CC=$(CXX) XXCFLAGS="-D__CLEANUP_C -DPTW32_STATIC_LIB" DLL="" allpassed

GCX-static-debug GCX-small-static-debug:
	@ $(MAKE) --no-builtin-rules TEST=$@ GCX="GC$(DLL_VER)d" CC=$(CXX) XXCFLAGS="-D__CLEANUP_C -DPTW32_STATIC_LIB" OPT="$(DOPT)" DLL="" allpassed

all-asm: $(ASM)
	@ $(ECHO) "ALL TESTS PASSED! Congratulations!"

allpassed: $(HDR) $(LIB) $(DLL) $(QAPC) $(TESTS:%=%.pass)
	@ $(ECHO) "ALL TESTS PASSED! Congratulations!"

all-bench: $(HDR) $(LIB) $(DLL) $(QAPC) $(XXLIBS) $(BENCHRESULTS)
	@ $(ECHO) "ALL BENCH TESTS COMPLETED."

cancel9.exe: XLIBS = -lws2_32

%.pass: %.exe
	@ $(ECHO) Running $(TEST) test \"$*\"
	@ $(RUN) ./$*
	@ $(ECHO) Passed
	@ $(TOUCH) $@

%.bench: %.exe
	@ $(ECHO) Running $(TEST) test \"$*\"
	@ $(RUN) ./$*
	@ $(ECHO) Done
	@ $(TOUCH) $@

%.exe: %.c
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $< $(INCLUDES) -L. -lpthread$(GCX) $(XLIBS) $(XXLIBS)

%.pre: %.c $(HDR)
	$(CC) -E $(CFLAGS) -o $@ $< $(INCLUDES)

%.s: %.c $(HDR)
	@ $(ECHO) Compiling $@
	$(CC) -S $(CFLAGS) -o $@ $< $(INCLUDES)

$(HDR) $(LIB) $(DLL) $(QAPC):
	@ $(ECHO) Copying $(BUILD_DIR)/$@
	@ $(TESTFILE) $(BUILD_DIR)/$@ $(AND) $(CP) $(BUILD_DIR)/$@ .

benchlib.o: benchlib.c
	@ $(ECHO) Compiling $@
	$(CC) -c $(CFLAGS) $< $(INCLUDES)

clean:
	- $(RM) *.dll
	- $(RM) *.lib
	- $(RM) pthread.h
	- $(RM) semaphore.h
	- $(RM) sched.h
	- $(RM) *.a
	- $(RM) *.e
	- $(RM) *.i
	- $(RM) *.o
	- $(RM) *.s
	- $(RM) *.so
	- $(RM) *.obj
	- $(RM) *.pdb
	- $(RM) *.exe
	- $(RM) *.manifest
	- $(RM) *.pass
	- $(RM) *.bench
	- $(RM) *.log
	
