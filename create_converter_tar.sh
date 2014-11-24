#!/bin/bash

BASEDIR=$(dirname $0)

if [ $# -eq 0 ]; then
  TAR_FILE_NAME=converter-`date '+%Y%m%d-%H%M'`
  mkdir $BASEDIR/dep_location
  mkdir $BASEDIR/tar_location

  $BASEDIR/copy_converter_file_and_dependencies $BASEDIR/build/siemens_to_ismrmrd $BASEDIR/dep_location  
  cp $BASEDIR/LICENSE $BASEDIR/dep_location
  tar -zcf $BASEDIR/tar_location/$TAR_FILE_NAME.tar.gz --directory $BASEDIR/dep_location/ .
  rm -rf $BASEDIR/dep_location/
  exit 0
else
  echo -e "\nUsage:  $0\n"
  exit 1
fi
