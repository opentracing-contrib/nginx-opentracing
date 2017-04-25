#!/bin/sh
./start.sh
sleep 5
node node/client.js --firstname Pangy --lastname B --profile ../data/pangolin.jpg
sleep 1
./stop.sh
