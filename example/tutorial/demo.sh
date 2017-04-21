#!/bin/bash
./start.sh
sleep 5
node node/client.js --data_root ../data --num_requests 5
sleep 1
node node/client.js --data_root ../data --num_requests 5
sleep 1
node node/client.js --data_root ../data --num_requests 5
sleep 1
node node/client.js --data_root ../data --num_requests 5
sleep 1
node node/client.js --data_root ../data --num_requests 5
./stop.sh
