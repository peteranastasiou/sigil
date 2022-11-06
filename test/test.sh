#!/bin/sh

POND=../bin/pond

# Clear previous test results:
rm -rf out
mkdir out

RESULT=""

echo "< Result"
echo "> Expected"

for SCRIPT in scripts/*.pond
do
    NAME=`basename $SCRIPT .pond`
    EXPECTED=scripts/$NAME.expected
    OUT=out/$NAME.out
    ERR=out/$NAME.err
    echo "==== $SCRIPT ===="
    $POND $SCRIPT > $OUT 2> $ERR
    diff $EXPECTED $OUT -B
    if [ $? -ne 0 ]
    then
        RESULT="FAILURE"
    fi
done

echo $RESULT
if [ -n $RESULT ]
then
    exit 255
fi
