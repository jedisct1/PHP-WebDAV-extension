--TEST--
Check for webdav_get() workability
--SKIPIF--
<?php if (!extension_loaded("dav")) print "skip"; ?>
--FILE--
<?php

$ret = @webdav_get('/version.php');
var_dump($ret);
$dav = webdav_connect('http://svn.orbus.fr/svn/skyrockv3/trunk',
                      'j', 'proutdemammouth');
if ($dav === FALSE) {
    die();
}
$ret = webdav_get('/version.php');
if ($ret === FALSE) {
    die();
}
if (strstr($ret, 'phpinfo();') !== FALSE) {
    echo "OK\n";
}
$ret = webdav_close();
var_dump($ret);
$ret = @webdav_get('/version.php');
var_dump($ret);
@webdav_close();

?>
--EXPECT--
bool(false)
OK
bool(true)
bool(false)
