#!/bin/sh

FILE=$1
INFO_FILE=${FILE}.txt
DATE=$(date -u +"%d-%m-%y %H:%M UTC")

cat > $INFO_FILE <<END_FILE
$DATE
$GIT_HASH
END_FILE

curl -T ./${FILE} ftp://glyphpress.com/${FILE} --user ${FTP_USERNAME}:${FTP_PASSWORD}
curl -T ./${INFO_FILE} ftp://glyphpress.com/${INFO_FILE} --user ${FTP_USERNAME}:${FTP_PASSWORD}

exit 0
