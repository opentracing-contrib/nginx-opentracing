#!/bin/bash
DATA_ROOT=/app/data
IMG_ROOT=$DATA_ROOT/images

node node/setup.js --data_root $DATA_ROOT

for i in {1..3}; do
  let port="3000+$i"
  node node/server.js --port $port --data_root $DATA_ROOT --access_token $LIGHTSTEP_ACCESS_TOKEN&
done

while /bin/true; do
  sleep 50
done
