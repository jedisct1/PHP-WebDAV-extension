--TEST--
Check for webdav_get() workability
--SKIPIF--
<?php if (!extension_loaded("dav")) print "skip"; ?>
--FILE--
<?php

$dav = webdav_connect('http://svn.orbus.fr/svn/skyrockv3/trunk',
                      'j', 'proutdemammouth');
if ($dav === FALSE) {
    die();
}
$ret = webdav_get('/version.php', $dav);
if ($ret === FALSE) {
    die();
}
if (strstr($ret, 'phpinfo();') !== FALSE) {
    echo "OK\n";
}
webdav_close($dav);                      

?>
--EXPECT--
OK
