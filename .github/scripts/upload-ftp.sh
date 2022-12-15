#!/bin/sh

HOST='glyphpress.com'
USER=${FTP_USERNAME}
PASSWD=${FTP_PASSWORD}
FILE=$1

ftp -inv $HOST <<END_SCRIPT
quote USER $USER
quote PASS $PASSWD
passive
binary
put $FILE
quit
END_SCRIPT
exit 0
