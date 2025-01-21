#!/bin/sh

git config --global --add safe.directory /__w/plugdata/plugdata

FILE=$1
INFO_FILE=${FILE}.txt
GIT_HASH=$(git log -1 --format=%H)
DATE=$(date -u +"%d-%m-%y %H:%M UTC")

cat > $INFO_FILE <<END_FILE
$DATE
$GIT_HASH
END_FILE

exit 0
