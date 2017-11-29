#!/bin/bash

export JAEGER_AGENT_HOST=$JAEGER_PORT_6831_UDP_ADDR
export JAEGER_AGENT_PORT=$JAEGER_PORT_6831_UDP_PORT
/app/hotrod all &
envsubst '\$JAEGER_AGENT_HOST \$JAEGER_AGENT_PORT' < /app/nginx.conf > /etc/nginx/nginx.conf
nginx
while /bin/true; do
  sleep 50
done
