<?php

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

$headers = getallheaders();

foreach ($headers as $name => $value) {
    echo "$name: $value\n";
}

assert(isset($headers["x-ot-span-context"]));

?>
