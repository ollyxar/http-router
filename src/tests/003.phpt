--TEST--
HTTPRouter Memory reuse test
--SKIPIF--
<?php
if (!extension_loaded('httprouter')) {
	echo 'skip';
}
?>
--FILE--
<?php

class Route extends HTTPRouter {
    public function get($pattern, $action) {
        return $this->method('GET', $pattern, $action);
    }
    public function post($pattern, $action) {
        return $this->method('POST', $pattern, $action);
    }
}

class MyTest {
	public function hola($a, $b) {
		return "a = $a, b = $b\n";
	}
}

for ($i = 1; $i <= 3; $i++) {
	$router = new Route("/custom-uri/integer/$i/sub/99?param=val", 'GET');

	$router->group('custom-uri', function() use ($router) {
		$router->group('integer', function() use ($router) {
			$router->get('{a}/sub/{b}', 'MyTest@hola');
		});
	});

	echo $router->action();
}
?>
--EXPECT--
a = 1, b = 99
a = 2, b = 99
a = 3, b = 99