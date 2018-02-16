#!/bin/bash
set -e

if [ -z $MOCKTRACER_LIBRARY ]
then
  >&2 echo "MOCKTRACER_LIBRARY must be set!"
  exit -1
fi

if [ -z $NGINX_OPENTRACING_MODULE ]
then
  >&2 echo "NGINX_OPENTRACING_MODULE must be set!"
  exit -1
fi

TEMP_DIR=`mktemp -d`
TEST_DIR=`dirname "$0"`
cd "$TEST_DIR"
cp -r . $TEMP_DIR
mkdir $TEMP_DIR/logs
echo $TEMP_DIR

export MOCKTRACER_CONFIG=$TEMP_DIR/tracer-configuration.json
export MOCKTRACER_OUTPUT_FILE=$TEMP_DIR/spans.json

envsubst <$TEMP_DIR/nginx.conf.in > $TEMP_DIR/nginx.conf
envsubst <$TEMP_DIR/tracer-configuration.json.in > $TEMP_DIR/tracer-configuration.json

echo "Starting date server"
python3 $TEMP_DIR/date.py &
DATE_SERVER_PID=$!
echo "Starting nginx"
nginx -p $TEMP_DIR -c $TEMP_DIR/nginx.conf
sleep 2
echo "Sending test data to nginx"
curl localhost:8080/
kill $DATE_SERVER_PID
nginx -p $TEMP_DIR -c $TEMP_DIR/nginx.conf -s stop
echo "Verifying results"
sleep 2
SPAN_COUNT=`jq 'length' < $MOCKTRACER_OUTPUT_FILE`
rm -rf $TEMP_DIR

EXPECTED_SPAN_COUNT=2
if [ $SPAN_COUNT != $EXPECTED_SPAN_COUNT ]
then
  >&2 echo "expected: $SPAN_COUNT == $EXPECTED_SPAN_COUNT"
  exit -1
fi
