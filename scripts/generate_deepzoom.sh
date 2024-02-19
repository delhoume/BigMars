#!/bin/bash

HTML_EXT="html"
TIFF_EXT="tif"
PYRAMID_EXT="_pyramid"
TEMP_SUFFIX="_files"
DEEP_ZOOM_FILES_FOLDER="DeepZoom_files"
BIGMARS_DIR="/Users/fredericdelhoume/Documents/GitHub/BigMars"


INPUT=$1
BASEFOLDER=$(dirname $INPUt)
BASEINPUT=$(basename $INPUT $PYRAMID_EXT.$TIFF_EXT)
NAME=$BASEINPUT

echo $BASEINPUT
echo $NAME
seq 1 25 | xargs -I % mkdir -p $NAME$DEEP_ZOOM_FILES_FOLDER/%

$BIGMARS_DIR/bin/pyramid2deepzoom $INPUT $NAME

sed "s/SOURCE/$BASEINPUT/g" deepzoom.tpl.html > $BASEINPUT.$HTML_EXT