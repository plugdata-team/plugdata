#!/bin/sh

## puts dependencies besides the binary
# LATER: put dependencies into a separate folder

## usage: $0 <binary> [<binary2>...]


###########################################
# WARNING
#
# this uses an ugly hack to allow side-by-side installation of 32bit and 64bit
# dependencies:
# embedded dependencies are renamed from "libfoo.dll" to "libfoo.w32" resp.
# "libfoo.w64", and the files are modified (using 'sed') to reflect this
# renaming.
# this is somewhat brittle and likely to break!

#default exclude/include paths
exclude_paths=""
include_paths="*mingw*:*/msys/*"

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

CYGPATH=$(which cygpath 2>/dev/null)
if [ -z "${CYGPATH}" ]; then
    CYGPATH=echo
fi

normalize_path() {
    # convert to unix-format (C:\foo\bar\ --> /c/foo/bar/)
    # and lower-case everything (because on microsoft-fs, paths are case-insensitive)
    ${CYGPATH} "$1" | tr "[A-Z]" "[a-z]"
}

list_deps() {
    local path
    local path0
    local inc
    "${NTLDD}" "$1" \
	| grep ' => ' \
	| sed -e 's|\\|/|g' -e 's|.* => ||' -e 's| (0.*||' \
	| while read path; do
	path0=$(echo $path |sed -e 's|/|\\|g')
	inc=$(check_includedep "${path0}")
	if [ "x${inc}" != "x" ]; then
	    echo "${path}"
	fi
    done
}

file2arch() {
    if file "$1" | grep -w "PE32+" >/dev/null; then
        echo "w64"
        return
    fi
    if file "$1" | grep -w "PE32" >/dev/null; then
        echo "w32"
        return
    fi
}

install_deps () {
    local outdir="$2"
    local idepfile
    local odepfile
    local archext
    local dep
    error "DEP: ${INSTALLDEPS_INDENT}'$1' '$2'"

    if [ "x${outdir}" = "x" ]; then
        outdir=${1%/*}
    fi
    if [ ! -d "${outdir}" ]; then
        outdir=.
    fi

    list_deps "$1" | while read dep; do
        idepfile=$(basename "${dep}")
        odepfile=${idepfile}
        archext=$(file2arch "${dep}")
        if [ "x${archext}" != "x" ]; then
            odepfile=$(echo ${idepfile} | sed -e "s|\.dll|.${archext}|")
        fi
        if [ "x${idepfile}" = "x${odepfile}" ]; then
	    archext=""
        fi
        if [ -e "${outdir}/${odepfile}" ]; then
            error "DEP: ${INSTALLDEPS_INDENT}  ${dep} SKIPPED"
        else
            error "DEP: ${INSTALLDEPS_INDENT}  ${dep} -> ${outdir}/${odepfile}"
            cp "${dep}" "${outdir}/${odepfile}"
            chmod a-x "${outdir}/${odepfile}"
        fi

        if [ "x${archext}" != "x" ]; then
            sed -b \
                -e "s|${idepfile}|${odepfile}|g" \
                -i \
                "${outdir}/${odepfile}" "${dep}" "$1"
        fi
        #recursively resolve dependencies
        INSTALLDEPS_INDENT="${INSTALLDEPS_INDENT}    " install_deps "${outdir}/${odepfile}"
    done
}

if [ "x${NTLDD}" = "x" ]; then
    check_binaries ntldd
    NTLDD="ntldd"
fi

for f in "$@"; do
    if [ -e "${f}" ]; then
        error
        install_deps "${f}"
    fi
done