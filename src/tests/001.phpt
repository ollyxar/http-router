--TEST--
Check if httprouter is loaded
--SKIPIF--
<?php
if (!extension_loaded('httprouter')) {
	echo 'skip';
}
?>
--FILE--
<?php
echo 'The extension "httprouter" is available';
?>
--EXPECT--
The extension "httprouter" is available
