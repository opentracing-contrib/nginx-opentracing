#!/bin/bash

export ZIPKIN_HOST=$ZIPKIN_PORT_9411_TCP_ADDR
export ZIPKIN_PORT=$ZIPKIN_PORT_9411_TCP_PORT
envsubst '\$ZIPKIN_PORT_9411_TCP_ADDR \$ZIPKIN_PORT_9411_TCP_PORT' < /app/nginx.conf > /etc/nginx/nginx.conf

npm start &
nginx
while /bin/true; do
  sleep 50
done
