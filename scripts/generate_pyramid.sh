#!/bin/bash

INPUT=$1
BASEFOLDER=$(dirname $1)
BASEINPUT=$(basename $1 _strip.tif)
TEMP_SUFFIX="_tiled_"
EXT=".tif"
TEMP_FOLDER="temp"
SEP=" "
FINAL_SUFFIX="_pyramid"
BIGMARS_DIR="/Users/fredericdelhoume/Documents/GitHub/BigMars"

NUMLEVELS=${2:-20}

mkdir -p $TEMP_FOLDER

LEVELS=$TEMP_FOLDER/$BASEINPUT$TEMP_SUFFIX"0"$EXT
echo "Levels: " $LEVELS 

$BIGMARS_DIR/bin/strip2tiled.jpg $INPUT $LEVELS

for i in `seq 0 $NUMLEVELS` 
do
     echo $i
     OLDFILE=$TEMP_FOLDER/$BASEINPUT$TEMP_SUFFIX$i$EXT
     NEWFILE=$TEMP_FOLDER/$BASEINPUT$TEMP_SUFFIX$((i + 1))$EXT
     LEVELS=$LEVELS$SEP$NEWFILE
     $BIGMARS_DIR/bin/halftiff_stb $OLDFILE $NEWFILE
done

$BIGMARS_DIR/bin/tiffmerge.first `echo $LEVELS` $BASEINPUT$FINAL_SUFFIX$EXT
