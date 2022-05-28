#!/bin/sh

## puts dependencies besides the binary
# LATER: put dependencies into a separate folder

## usage: $0 <binary> [<binary2>...]

#default exclude/include paths
exclude_paths="/usr/lib/*:/System/Library/Frameworks/*"
include_paths="/*"
recursion=false

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


while getopts "hqrvI:X:" arg; do
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
        r)
            recursion=true
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

basename () {
    local x=${1##*/}
    if [ "x$x" = "x" ]; then
        echo $1
    else
        echo $x
    fi
}
dirname () {
    local x=${1%/*}
    if [ "x$x" = "x" ]; then
        echo .
    else
        echo $x
    fi
}

list_deps() {
    local path
    local inc
    ${OTOOL} "$1" \
        | grep -v ":$" \
	| grep compatibility \
	| awk '{print $1}' \
	| egrep '^/' \
        | while read path; do
        inc=$(check_includedep "${path}")
	if [ "x${inc}" != "x" ]; then
	    echo "${inc}"
	fi
    done
}

install_deps () {
    local outdir="$2"
    local infile
    local depfile
    local dep
    error "DEP: ${INSTALLDEPS_INDENT}'$1' '$2'"

    if [ "x${outdir}" = "x" ]; then
        outdir=$(dirname "$1")
    fi
    if [ ! -d "${outdir}" ]; then
        outdir=.
    fi
    if ! $recursion; then
        outdir="${outdir}/${arch}"
        mkdir -p "${outdir}"
    fi
    list_deps "$1" | while read dep; do
        infile=$(basename "$1")
        depfile=$(basename "${dep}")
        if $recursion; then
            loaderpath="@loader_path/${depfile}"
        else
            loaderpath="@loader_path/${arch}/${depfile}"
        fi
        # make sure the binary looks for the dependency in the local path
        install_name_tool -change "${dep}" "${loaderpath}" "$1"

        if [ -e "${outdir}/${depfile}" ]; then
            error "DEP: ${INSTALLDEPS_INDENT}  ${dep} SKIPPED"
        else
            error "DEP: ${INSTALLDEPS_INDENT}  ${dep} -> ${outdir}"
            cp "${dep}" "${outdir}"
            chmod u+w "${outdir}/${depfile}"

            # make sure the dependency announces itself with the local path
            install_name_tool -id "${loaderpath}" "${outdir}/${depfile}"
            # recursively call ourselves, to resolve higher-order dependencies
            INSTALLDEPS_INDENT="${INSTALLDEPS_INDENT}    " $0 -r "${outdir}/${depfile}"
        fi
    done
}

if [ "x${OTOOL}" = "x" ]; then
    check_binaries otool
    OTOOL="otool -L"
fi

for f in "$@"; do
    if [ -e "${f}" ]; then
        error
        install_deps "${f}"
    fi
done

# Code signing
# On Monterey, binaries are automatically codesigned. Modifying them with this script renders the signature
# invalid. When Pd loads an external with an invalid signature, it exits immediately. Thus, we need to make sure
# that we codesign them again _after_ the localdeps process

# This needs to be the absolutely last step. We don't do it while we're still inside a recursion.
if ! $recursion; then
    echo -n "Code signing in progress... "
    outdir="$(dirname "$1")/${arch}"
    codesign --remove-signature "${ARGS[@]}" ${outdir}/*.dylib
    codesign -s -  "${ARGS[@]}" ${outdir}/*.dylib
    echo "Done"
fi
