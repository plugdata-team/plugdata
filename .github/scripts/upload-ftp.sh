#!/bin/sh

FILE=$1
INFO_FILE=${FILE}.txt
DATE=$(date -u +"%d-%m-%y %H:%M UTC")
LATEST_HASH=$(git ls-remote https://github.com/plugdata-team/plugdata.git HEAD)

cat > $INFO_FILE <<END_FILE
$DATE
$GIT_HASH
END_FILE

cat > latest.txt <<END_FILE
$LATEST_HASH
END_FILE

curl -T ./${FILE} ftp://glyphpress.com/${FILE} --user ${FTP_USERNAME}:${FTP_PASSWORD}
curl -T ./${INFO_FILE} ftp://glyphpress.com/${INFO_FILE} --user ${FTP_USERNAME}:${FTP_PASSWORD}
curl -T ./latest.txt ftp://glyphpress.com/latest.txt --user ${FTP_USERNAME}:${FTP_PASSWORD}

exit 0
