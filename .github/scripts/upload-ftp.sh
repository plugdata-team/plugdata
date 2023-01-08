#!/bin/sh

HOST=glyphpress.com
FILE=$1
INFO_FILE=${FILE}.txt
DATE=$(date)

UNAME=$(uname)

curl -T ${FILE} ftp://glyphpress.com/plugdata/${FILE} --user ${FTP_USERNAME}:${FTP_PASSWORD}
curl -T ${INFO_FILE} ftp://glyphpress.com/plugdata/${INFO_FILE} --user ${FTP_USERNAME}:${FTP_PASSWORD}

exit 0
