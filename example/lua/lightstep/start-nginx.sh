#!/bin/bash

envsubst '\$LIGHTSTEP_ACCESS_TOKEN' < /lightstep-config.json.in > /etc/lightstep-config.json
/usr/local/openresty/nginx/sbin/nginx -g "daemon off;"
