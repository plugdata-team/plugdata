#! /bin/bash

pushd ..

mkdir cvs
cd cvs

git cvsimport -v -u -a -m -k -d :pserver:anoncvs@sourceware.org:/cvs/pthreads-win32 pthreads

popd


