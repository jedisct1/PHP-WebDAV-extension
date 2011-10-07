--TEST--
Check for webdav_copy() workability
--SKIPIF--
<?php if (!extension_loaded("dav")) print "skip"; ?>
--FILE--
<?php

webdav_connect('http://carlos.prodin.orbus.fr:82/photos/tmp',
               'khalid', 'dav1337');
$ret = webdav_put('/dav-test', 'youpi');
var_dump($ret);
$ret = webdav_copy('/dav-test', '/dav-test2', TRUE, TRUE);
var_dump($ret);
$ret = webdav_get('/dav-test2');
var_dump($ret);
webdav_close();

?>
--EXPECT--
bool(true)
bool(true)
string(5) "youpi"
