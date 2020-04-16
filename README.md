# HTTPRouter

## Intro

This is simple PHP7 extension (written on pure C). You can build and install it on your system. It has tiny component that allows you to use [Lumen-like routes](https://lumen.laravel.com/docs/5.8/routing).

Also you may find completely identical [analog](https://ollyxar.com/blog/lumen-like-routes) written on raw PHP.

> This extension has been written for education purposes. It can be used to demonstrate basics / tutorial "how to build your own PHP extension"

## How to use

1) Extend class

```php
class Router extends HTTPRouter {
    public function get($pattern, $action) {
        return $this->method('GET', $pattern, $action);
    }
    public function post($pattern, $action) {
        return $this->method('POST', $pattern, $action);
    }
}
```

2) Create new instance, set necessary routes 

```php
$router = new Route($_SERVER['REQUEST_URI'], $_SERVER['REQUEST_METHOD']);


$router->group('forum', function () use ($router) {
    $router->get('{name}', 'ForumController@getMe');
    $router->group('news', function () use ($router) {
        $router->get('{name}', 'NewsController@getMe');
    });
    $router->post('edit-me', function () {
        return 'No!';
    });
});
```

3) Call action and get response

```php
$response = $router->action();

echo $response;
```


## How to build

To compile | build php extension for Linux you can use this docker image.

> _building PHP from source can take a long time. Don't panic!_

To build and run system use following command:

```bash
docker build -t php_ext:latest .
```

To build and test PHP extension use these instructions:

* To start container and log in:

```bash
docker run --rm --name php_ext -v d:/projects/php_extensions/router/src:/src -it php_ext sh
```

* Prepare extension build:

```bash
/usr/local/php7/bin/phpize && \
./configure --with-php-config=/usr/local/php7/bin/php-config
```

* Build the extension, test it and install:

```bash
make && make clean && make test && make install
```

> Remember: to recompile extension when you changed some code you need to perform only last operation. You don't need to reconfigure extension if you didn't change dependencies.

### Known issues

* Sometimes when you trying to compile project you've got:

```bash
make: stat: GNUmakefile: I/O error
```

Caused by: error on volumes

Solution: try again in few seconds. 

## Performance tests

We've used scripts that run both approaches with 999 instances of Class 100 times. 

Tests was made on Aser Aspire E15 (AMD A10) within docker container


|               | Regular PHP Class | PHP Extension (C implementation) |
|---------------|-------------------|----------------------------------|
|Time in seconds| 1.787871599197389 | 1.473563671112060                |
|Comparison    | 1.21 times slower | 1.21 times faster                |
