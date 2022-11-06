#!/bin/sh

POND=./bin/pond

# Clear previous test results:
rm -rf out
mkdir out

RESULT="OK"

for SCRIPT in test/scripts/*.pond
do
    NAME=`basename $SCRIPT .pond`
    EXPECTED=test/scripts/$NAME.expected
    OUT=test/out/$NAME.out
    ERR=test/out/$NAME.err
    echo "==== $SCRIPT ===="
    $POND $SCRIPT > $OUT 2> $ERR
    diff $EXPECTED $OUT -B
    if [ $? -ne 0 ]
    then
        RESULT="FAILURE"
        echo "--- Expected ---"
        cat $EXPECTED
        echo ""
        echo "--- Result ---"
        cat $OUT
        echo ""
        echo "--- Error ---"
        cat $ERR
        echo ""
    fi
done

echo $RESULT
if [ $RESULT != "OK" ]
then
    exit 255
fi
