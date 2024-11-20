#!/bin/sh

git config --global --add safe.directory /__w/plugdata/plugdata

FILE=$1
INFO_FILE=${FILE}.txt
TIMESTAMP_FILE=${FILE}_timestamp.txt

DATE=$(date -u +"%d-%m-%y %H:%M UTC")
COMMIT_TIMESTAMP=$(git show -s --format=%ct HEAD)

exit 0
