#!/bin/bash

BASEDIR=$(dirname $0)

if [ $# -eq 1 ]; then
  TAR_FILE_NAME=converter-`date '+%Y%m%d-%H%M'`
  mkdir $BASEDIR/dep_location
  mkdir $BASEDIR/tar_location

  CONVERTER_BINARY=${1}

  $BASEDIR/copy_converter_file_and_dependencies $CONVERTER_BINARY $BASEDIR/dep_location  
  cp $BASEDIR/LICENSE $BASEDIR/dep_location
  tar -zcf $BASEDIR/tar_location/$TAR_FILE_NAME.tar.gz --directory $BASEDIR/dep_location/ .
  rm -rf $BASEDIR/dep_location/
  exit 0
else
  echo -e "\nUsage:  $0 <converter binary>\n"
  exit 1
fi
