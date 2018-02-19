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

if [ -z $NGINX_OPENTRACING_TEST_DIR ]
then
  >&2 echo "NGINX_OPENTRACING_TEST_DIR must be set!"
  exit -1
fi

TEST_DIR=`dirname "$0"`
cd "$TEST_DIR"
export TMPDIR="$NGINX_OPENTRACING_TEST_DIR"
TEST_WORK_DIR=`mktemp -dt 01-XXXX`
chmod a+rwx "$TEST_WORK_DIR"
ls -ltr "$TEST_WORK_DIR"
cp -r . "$TEST_WORK_DIR"
mkdir "$TEST_WORK_DIR/logs"
echo "$TEST_WORK_DIR"

export MOCKTRACER_CONFIG="$TEST_WORK_DIR/tracer-configuration.json"
export MOCKTRACER_OUTPUT_FILE="$TEST_WORK_DIR/spans.json"

envsubst <"$TEST_WORK_DIR/nginx.conf.in" > "$TEST_WORK_DIR/nginx.conf"
envsubst <"$TEST_WORK_DIR/tracer-configuration.json.in" > "$MOCKTRACER_CONFIG"
chmod a+rx "$MOCKTRACER_CONFIG"
ls -ld "$NGINX_OPENTRACING_TEST_DIR"
ls -ld "$TEST_WORK_DIR"
ls -l "$MOCKTRACER_CONFIG"

echo "Starting date server"
python3 "$TEST_WORK_DIR/date.py" &
DATE_SERVER_PID=$!
echo "Starting nginx"
nginx -p "$TEST_WORK_DIR" -c "$TEST_WORK_DIR/nginx.conf"
sleep 1
echo "Sending test data to nginx"
curl localhost:8080/
kill $DATE_SERVER_PID
nginx -p "$TEST_WORK_DIR" -c "$TEST_WORK_DIR/nginx.conf" -s stop
echo "Verifying results"
sleep 2
SPAN_COUNT=`jq 'length' < "$MOCKTRACER_OUTPUT_FILE"`
rm -rf "$TEST_WORK_DIR"

EXPECTED_SPAN_COUNT=2
if [ $SPAN_COUNT != $EXPECTED_SPAN_COUNT ]
then
  >&2 echo "expected: $SPAN_COUNT == $EXPECTED_SPAN_COUNT"
  exit -1
fi
