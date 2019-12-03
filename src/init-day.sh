#!/bin/bash

DAY=$1

if [ -z $DAY ]; then
    echo "missind argument: day"
    exit 1
fi

DIR="day$DAY"

if [ -d $DIR ]; then
    echo "day $DAY already exists"
    exit 1
fi

mkdir $DIR

echo "/day$DAY" > $DIR/.gitignore

cat > $DIR/wscript_build <<XXX
ctx.program(target='day$DAY', source='day$DAY.c',
            use=['libcommon'])
XXX
