#!/bin/bash

DATA_ROOT=$PWD/zoo-data

nginx -p nginx/ -c nginx.conf -s stop

while read pid; do
  kill $pid
done <$DATA_ROOT/backend_pids
rm $DATA_ROOT/backend_pids
