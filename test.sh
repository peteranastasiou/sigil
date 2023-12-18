#!/bin/sh

SIGIL=./bin/sigil

# Clear previous test results:
rm -rf out
mkdir out
mkdir test/out

RESULT="OK"

for SCRIPT in test/scripts/*.sigil
do
    NAME=`basename $SCRIPT .sigil`
    EXPECTED=test/scripts/$NAME.expected
    OUT=test/out/$NAME.out
    ERR=test/out/$NAME.err
    echo "==== $SCRIPT ===="
    $SIGIL $SCRIPT > $OUT 2> $ERR
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
