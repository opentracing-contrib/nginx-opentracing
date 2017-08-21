#!/bin/bash

go build go/server.go
./server &
echo $! >> backend_pids
nginx -p nginx/ -c nginx.conf
