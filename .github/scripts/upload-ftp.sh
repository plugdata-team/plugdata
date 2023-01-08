#!/bin/sh

HOST=glyphpress.com
USER=${FTP_USERNAME}
PASSWD=${FTP_PASSWORD}
FILE=$1
INFO_FILE=${FILE}.txt
DATE=$(date)

UNAME=$(uname)
if [[ "$UNAME" == CYGWIN* || "$UNAME" == MINGW*  || "$UNAME" == MSYS* ]] ; then

cat > INFO_FILE <<END_FILE
$DATE
$GIT_HASH
END_FILE

cat > ./plugdata.ftp <<END_SCRIPT
open $HOST
$USER
$PASSWD
binary
delete $FILE
delete $INFO_FILE
quit
END_SCRIPT

ftp -s:plugdata.ftp

# Wait 2 minutes to ensure the file is gone
sleep 40

rm plugdata.ftp

cat > ./plugdata.ftp <<END_SCRIPT
open $HOST
$USER
$PASSWD
binary
put $FILE
put $INFO_FILE
quit
END_SCRIPT

ftp -s:plugdata.ftp
rm plugdata.ftp

else

ftp -p -inv $HOST <<END_SCRIPT
quote USER $USER
quote PASS $PASSWD
quote PASV
binary
delete $FILE
delete $INFO_FILE
quit
END_SCRIPT

# Wait 2 minutes to ensure the file is gone
sleep 40

ftp -p -inv $HOST <<END_SCRIPT
quote USER $USER
quote PASS $PASSWD
quote PASV
binary
put $FILE
put $INFO_FILE
quit
END_SCRIPT

fi

exit 0
