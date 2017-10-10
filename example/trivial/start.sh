#!/bin/bash

./server &
envsubst '\$ZIPKIN_PORT_9411_TCP_ADDR \$ZIPKIN_PORT_9411_TCP_PORT' < /app/nginx.conf > /etc/nginx/nginx.conf
nginx
while /bin/true; do
  sleep 50
done
