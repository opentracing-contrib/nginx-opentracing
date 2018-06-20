<?php

require_once 'vendor/autoload.php';

use Jaeger\Config;
use OpenTracing\GlobalTracer;
use OpenTracing\Formats;

if (!function_exists('getallheaders')) {
    function getallheaders() {
    $headers = [];
    foreach ($_SERVER as $name => $value) {
        if (substr($name, 0, 5) == 'HTTP_') {
            $headers[str_replace(' ', '-', strtolower(str_replace('_', ' ', substr($name, 5))))] = $value;
        }
    }
    return $headers;
    }
}

$config = new Config(
    [
        'sampler' => [
            'type' => 'const', 
            'param' => true,
        ],
        'local_agent' => [
          'reporting_host' => 'jaeger',
        ],
        'logging' => true,
    ],
    'date-app'
);
$config->initializeTracer();

$tracer = GlobalTracer::get();

$spanContext = GlobalTracer::get()->extract(
    Formats\HTTP_HEADERS,
    getallheaders()
);
$scope = $tracer->startActiveSpan('getDate', ['child_of' => $spanContext]);

$currentDateTime = date('Y-m-d H:i:s');
echo $currentDateTime;

$scope->close();

$tracer->flush();
?>
