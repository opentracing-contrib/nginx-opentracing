#!/bin/bash

./server -collector_host $ZIPKIN_PORT_9411_TCP_ADDR -collector_port $ZIPKIN_PORT_9411_TCP_PORT &
envsubst '\$ZIPKIN_PORT_9411_TCP_ADDR \$ZIPKIN_PORT_9411_TCP_PORT' < /app/nginx.conf > /etc/nginx/nginx.conf
nginx
while /bin/true; do
  sleep 50
done
