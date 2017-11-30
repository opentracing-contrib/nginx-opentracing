/* eslint-env browser */
const {
  BatchRecorder,
  jsonEncoder: {JSON_V2}
} = require('zipkin');
const {HttpLogger} = require('zipkin-transport-http');

// Send spans to Zipkin asynchronously over HTTP
if (!process.env.ZIPKIN_HOST || !process.env.ZIPKIN_PORT) {
  console.log("ZIPKIN_HOST and ZIPKIN_PORT must be set!");
  process.exit(1);
}
const zipkinBaseUrl = 'http://' + process.env.ZIPKIN_HOST + ':' + 
                       process.env.ZIPKIN_PORT
const recorder = new BatchRecorder({
  logger: new HttpLogger({
    endpoint: `${zipkinBaseUrl}/api/v2/spans`,
    jsonEncoder: JSON_V2
  })
});

module.exports.recorder = recorder;
