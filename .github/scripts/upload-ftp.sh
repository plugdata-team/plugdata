#!/bin/sh

if [ -z "$FTP_USERNAME" ]; then
    echo "No user name, skipping ftp upload"
    exit 0
fi

git config --global --add safe.directory /__w/plugdata/plugdata

FILE=$1
INFO_FILE=${FILE}.txt
TIMESTAMP_FILE=${FILE}_timestamp.txt

DATE=$(date -u +"%d-%m-%y %H:%M UTC")
COMMIT_TIMESTAMP=$(git show -s --format=%ct HEAD)

LATEST_HASH=$(git rev-parse HEAD)

cat > $INFO_FILE <<END_FILE
$DATE
$GIT_HASH
END_FILE

cat > $TIMESTAMP_FILE <<END_FILE
$COMMIT_TIMESTAMP
END_FILE

cat > latest.txt <<END_FILE
$LATEST_HASH
END_FILE

# Get the last timestamp
TIMESTAMP_URL=https://glyphpress.com/plugdata/$TIMESTAMP_FILE
STATUS_CODE=$(curl --write-out %{http_code} --silent --output /dev/null ${TIMESTAMP_URL})

if [ $STATUS_CODE -lt "400" ]; then
    echo "Timestamp found"
    curl ${TIMESTAMP_URL} --output ./last_timestamp.txt
    LAST_TIMESTAMP=$(<last_timestamp.txt)
else
    echo "No timestamp found"
    LAST_TIMESTAMP="0"
fi

echo "Commit Timestamp: $COMMIT_TIMESTAMP"
echo "Last Timestamp: $LAST_TIMESTAMP"

if [ -z "$LAST_TIMESTAMP" ]; then
   LAST_TIMESTAMP="0"
fi

# Make sure that a later commit didn't finish earlier than this one
if [ "$COMMIT_TIMESTAMP" -gt "$LAST_TIMESTAMP" ]; then
# Upload files and additional information
curl -T ./${FILE} ftp://glyphpress.com/${FILE} --user ${FTP_USERNAME}:${FTP_PASSWORD}
curl -T ./${INFO_FILE} ftp://glyphpress.com/${INFO_FILE} --user ${FTP_USERNAME}:${FTP_PASSWORD}
curl -T ./${TIMESTAMP_FILE} ftp://glyphpress.com/${TIMESTAMP_FILE} --user ${FTP_USERNAME}:${FTP_PASSWORD}
fi

# Update the latest version
curl -T ./latest.txt ftp://glyphpress.com/latest.txt --user ${FTP_USERNAME}:${FTP_PASSWORD}

exit 0
