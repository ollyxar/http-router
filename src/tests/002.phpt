--TEST--
HTTPRouter Basic test
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

$router = new Route('/custom-uri/sub/99?param=val', 'POST');

$router->group('custom-uri', function() use ($router) {
	echo 1;
	$router->group('sub', function() use ($router) {
		echo 2;
		$router->post('{id}', function($id) {
			return $id;
		});
	});
});

echo $router->action();
?>
--EXPECT--
1299