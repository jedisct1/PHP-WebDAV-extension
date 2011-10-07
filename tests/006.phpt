--TEST--
Check for webdav_unlink() workability
--SKIPIF--
<?php if (!extension_loaded("dav")) print "skip"; ?>
--FILE--
<?php

webdav_connect('http://carlos.prodin.orbus.fr:82/photos/tmp',
               'khalid', 'dav1337');
$ret = webdav_put('/dav-test', 'youpi');
var_dump($ret);
$ret = webdav_unlink('/dav-test');
var_dump($ret);
$ret = @webdav_get('/dav-test');
var_dump($ret);
webdav_close();

?>
--EXPECT--
bool(true)
bool(true)
bool(false)
