#!/bin/bash

DATA_ROOT=$PWD/multipart-data

while read pid; do
  kill $pid
done <$DATA_ROOT/backend_pids
rm $DATA_ROOT/backend_pids

nginx -p nginx/ -c nginx.conf -s stop
