#!/bin/bash

envsubst '\$LIGHTSTEP_ACCESS_TOKEN' < /tmp/lightstep-config.json.in > /etc/lightstep-config.json
nginx-debug -g "daemon off;"
