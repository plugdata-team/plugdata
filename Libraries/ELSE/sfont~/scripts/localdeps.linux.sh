#!/bin/sh
#
# creates local copies of all dependencies (dynamic libraries)
# and sets RUNPATH to $ORIGIN on each so they will find
# each other.
#
# usage: $0 <binary>



verbose=0
include_paths=
exclude_paths=



#default exclude/include paths
exclude_paths="*/libc.so.*:*/libarmmem.*.so.*:*/libdl.so.*:*/libglib-.*.so.*:*/libgomp.so.*:*/libgthread.*.so.*:*/libm.so.*:*/libpthread.*.so.*:*/libpthread.so.*:*/libstdc++.so.*:*/libgcc_s.so.*:*/libpcre.so.*:*/libz.so.*"
include_paths="/*"

# UTILITIES
if [ -e "${0%/*}/localdeps.utilities.source" ]; then
. "${0%/*}/localdeps.utilities.source"
else
    # the following section (from @BEGIN_UTILITIES@ to @END_UTILITIES@)
    # was copied from 'localdeps.utilities.source'.
    # changes you make to this section will be lost.
#@BEGIN_UTILITIES@
verbose=${verbose:-0}

error() {
   echo "$@" 1>&2
}

substitute() {
    # substitutes literal strings
    # usage: echo foo | substitute foo bar g
    sed "s/$(echo $1 | sed 's:[]\[^$.*/&]:\\&:g')/$(echo $2 | sed 's:[]\[^$.*/&]:\\&:g')/$3"
}

check_binaries() {
    local cmd
    for cmd in "$@"; do
        if ! which "${cmd}" > /dev/null; then
            error "Could not find '${cmd}'. Is it installed?"
            exit 127
        fi
    done
}


normalize_path() {
    # normalize a path specification, e.g. on Windows turn C:\Foo\Bar\ into /c/foo/bar/"
    # on most system this doesn't do anything, but override it to your needs...
    # e.g. on Windows use: ${CYGPATH} "$1" | tr "[A-Z]" "[a-z]"
    echo "$1"
}

list_dirs() {
    #
    local IN="$@"
    local iter
    while [ "$IN" ] ;do
        iter=${IN%%:*}
        echo "${iter}"
        [ "$IN" = "$iter" ] && IN='' || IN="${IN#*:}"
    done
}

check_in_path() {
    local needle=$1
    local p
    local patterns
    shift
    patterns="$@"
    while [ "${patterns}" ]; do
        p=${patterns%%:*}
        [ "$patterns" = "$p" ] && patterns='' || patterns="${patterns#*:}"

        case "${needle}" in
            ${p})
                echo "${needle}"
                break
                ;;
        esac
    done | grep . >/dev/null
}

check_includedep() {
    local path=$(normalize_path "$1")
    local p
    local result=0
    # exclude non-existing files
    if [ ! -e "${path}" ]; then
	return 0
    fi

    # skip paths that match one of the patterns in ${exclude_paths}
    if check_in_path "${path}" "${exclude_paths}"; then
	return 1
    fi
    # only include paths that match one of the patterns in ${include_paths}
    if check_in_path "${path}" "${include_paths}"; then
	echo "${path}"
	return 0
    fi
    # skip the rest
    return 1
}

usage() {
    cat >/dev/stderr <<EOF
usage: $0 [-I <includepath>] [-X <excludepath>] <binary> [<binary2> ...]
  recursively includes all dependencies of the given binaries

  -I <includepath>: adds one include path entry
  -X <excludepath>: adds one exclude path entry
  -v: raise verbosity
  -q: lower verbosity

EOF

    case "$0" in
        *win*)
            cat >/dev/stderr <<EOF
  dependencies are renamed from .dll to .w64 (resp .w32)

EOF
            ;;
    esac

  cat >/dev/stderr <<EOF
EXCLUDING/INCLUDING
-------------------
When traversing the runtime dependencies of a binary, dependencies are filtered
out based on their location (this is mainly to exclude system libraries that
can be very large and which are to be found on the target systems anyhow).

Only dependencies (and sub-dependencies) that live in a path that
do NOT match any of the EXCLUDEPATHs and match at least one of the INCLUDEPATHs
are considered for inclusion (in this order. a dependency that matches both
EXCLUDEPATHs and INCLUDEPATHs is dropped).

Matching is done with globbing patterns, so a pattern '/foo/bar*' matches the
dependencies '/foo/bar.dll', '/foo/bartender.dll' and '/foo/bar/pizza.dll',
whereas a pattern '/foo/bar/*' only matches '/foo/bar/pizza.dll'.

Only paths that are not excluded, will be considered for inclusion.
Thus if there are both an exclude pattern '/usr/*' and an include pattern
'/usr/lib/*', then a path '/usr/lib/libfoo.so' will be omitted (and the include
pattern is practically useless).

You can remove an element from the INCLUDEPATHs by excluding it (exactly),
and vice versa.

EOF
    exit 1
}


while getopts "hqvI:X:" arg; do
    case $arg in
	h)
	    usage
	    ;;
	I)
	    p=$(normalize_path "${OPTARG}")
	    if [ "x${p}" != "x" ]; then
		include_paths="${p}:${include_paths}"
	    fi
            exclude_paths=$(echo :${exclude_paths}: | substitute ":${p}:" ":" | sed -e 's|^:*||' -e 's|:*$||' -e 's|::*|:|g')
	    ;;
	X)
	    p=$(normalize_path "${OPTARG}")
	    if [ "x${p}" != "x" ]; then
		exclude_paths="${p}:${exclude_paths}"
	    fi
            include_paths=$(echo :${include_paths}: | substitute ":${p}:" ":" | sed -e 's|^:*||' -e 's|:*$||' -e 's|::*|:|g')
	    ;;
        q)
            verbose=$((verbose-1))
            ;;
        v)
            verbose=$((verbose+1))
            ;;
	*)
	    usage
	    ;;
    esac
done
shift $((OPTIND-1))
include_paths=${include_paths%:}
exclude_paths=${exclude_paths%:}

if [  ${verbose} -gt 0 ]; then
    error "EXCLUDEPATHs: ${exclude_paths}"
    error "INCLUDEPATHs: ${include_paths}"
fi
#@END_UTILITIES@
fi

# detect arch
arch=$(uname -m)
case $arch in
    x86_64)
        arch=amd64
        ;;
    i686)
        arch=i386
        ;;
    armv7l)
        arch=arm
esac

list_deps() {
    local libpath
    local inc
    ldd "$1" \
        | grep ' => ' \
        | while read _ _ libpath _; do
        inc=$(check_includedep "${libpath}")
	if [ "x${inc}" != "x" ]; then
	    echo "${inc}"
	fi
    done
}

install_deps () {
    # make a local copy of all linked libraries of given binary
    # and set RUNPATH to $ORIGIN (exclude "standard" libraries)
    # arg1: binary to check
    local outdir
    outdir="$(dirname "$1")/${arch}"
    local outfile
    if [ ! -d "${outdir}" ]; then
        outdir=.
    fi
    list_deps "$1" | while read libpath; do
        libname=$(basename "${libpath}")
        if [ ! -e "${libpath}" ]; then
            error "DEP: ${INSTALLDEPS_INDENT}  WARNING: could not make copy of '${libpath}'. Not found"
            continue
        fi
        outfile="${outdir}/$(basename ${libpath})"
        if [ -e  "${outfile}" ]; then
            error "DEP: ${INSTALLDEPS_INDENT}  ${libpath} SKIPPED"
        else
            error "DEP: ${INSTALLDEPS_INDENT}  ${libpath} -> ${outdir}/"
            cp "${libpath}" "${outfile}"
            patchelf --set-rpath \$ORIGIN "${outfile}"
        fi
    done
    patchelf --set-rpath \$ORIGIN/${arch} "${1}"
}



# Check dependencies
check_binaries grep ldd patchelf

for f in "$@"; do
    # Check if we can read from given file
    if ! ldd "${f}" > /dev/null 2>&1; then
        error "Skipping '${f}'. Is it a binary file?"
        continue
    fi
    depdir="$(dirname ${f})/${arch}"
    mkdir -p "${depdir}"
    install_deps "${f}"
done
