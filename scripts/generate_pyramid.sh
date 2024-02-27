#!/bin/bash

INPUT=$1
BASEFOLDER=$(dirname $1)
BASEINPUT=$(basename $1 _tiled.tif)
EXT=".tif"
TEMP_FOLDER="temp"
TEMP_SUFFIX="_level_"
SEP=" "
FINAL_SUFFIX="_pyramid"
BIGMARS_DIR="/Users/fredericdelhoume/Documents/GitHub/BigMars"
FIRST=1

NUMLEVELS=${2:-20}

mkdir -p $TEMP_FOLDER

LEVELS=$INPUT
echo "Levels: " $LEVELS 

# create first level in temp
$BIGMARS_DIR/bin/halftiff_stb $INPUT TEMP_FOLDER/$BASEINPUT$TEMP_SUFFIX$FIST$EXT

for i in `seq 0 $NUMLEVELS` 
do
     echo $i $LEVELS
     OLDFILE=$TEMP_FOLDER/$BASEINPUT$TEMP_SUFFIX$i$EXT
     NEWFILE=$TEMP_FOLDER/$BASEINPUT$TEMP_SUFFIX$((i + 1))$EXT
     echo $OLDFILE $NEWFILE
     $BIGMARS_DIR/bin/halftiff_stb $OLDFILE $NEWFILE && LEVELS=$LEVELS$SEP$OLDFILE
done

echo $LEVELS
$BIGMARS_DIR/bin/tiffmerge.first `echo $LEVELS` $BASEINPUT$FINAL_SUFFIX$EXT
