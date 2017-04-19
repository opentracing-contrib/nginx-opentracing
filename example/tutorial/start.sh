#!/bin/bash
if [ -z $LIGHTSTEP_ACCESS_TOKEN ]
then
  echo "LIGHTSTEP_ACCESS_TOKEN must be set"
  exit -1
fi

DATA_ROOT=$PWD/zoo-data
IMG_ROOT=$DATA_ROOT/images

mkdir -p $IMG_ROOT

echo "lightstep_access_token $LIGHTSTEP_ACCESS_TOKEN;" > nginx/lightstep_access_token_params
echo "root $IMG_ROOT;" > nginx/image_params

node node/setup.js --data_root $DATA_ROOT

rm  nginx/upstream_params
for i in {1..3}; do
  let port="3000+$i"
  node node/server.js --port $port --data_root $DATA_ROOT --access_token $LIGHTSTEP_ACCESS_TOKEN&
  echo $! >> $DATA_ROOT/backend_pids
  echo "server localhost:$port;" >> nginx/upstream_params
done

mkdir -p nginx/logs
nginx -p nginx/ -c nginx.conf
