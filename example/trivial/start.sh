#!/bin/bash

go build go/server.go
./server &
echo $! >> backend_pids
mkdir -p nginx/logs
nginx -p nginx/ -c nginx.conf
