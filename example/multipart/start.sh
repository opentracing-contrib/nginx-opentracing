#!/bin/bash
if [ -z $LIGHTSTEP_ACCESS_TOKEN ]
then
  echo "LIGHTSTEP_ACCESS_TOKEN must be set"
  exit -1
fi

DATA_ROOT=$PWD/multipart-data
UPLOAD_DIR=$DATA_ROOT/upload

mkdir -p $UPLOAD_DIR
for i in {0..9}; do
  mkdir -p $UPLOAD_DIR/$i
done


echo "lightstep_access_token $LIGHTSTEP_ACCESS_TOKEN;" > nginx/lightstep_access_token_params
echo "upload_store $UPLOAD_DIR 1;" > nginx/upload_path_params;
echo "root $IMG_ROOT;" > nginx/image_params

node node/server.js --access_token $LIGHTSTEP_ACCESS_TOKEN&
echo $! >> $DATA_ROOT/backend_pids

mkdir -p nginx/logs
nginx -p nginx/ -c nginx.conf
