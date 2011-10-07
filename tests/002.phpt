--TEST--
Check for webdav_connect()/webdav_close() workability
--SKIPIF--
<?php if (!extension_loaded("dav")) print "skip"; ?>
--FILE--
<?php

$dav = webdav_connect('http://svn.orbus.fr/svn/skyrockv3',
                      'j', 'proutdemammouth');
var_dump($dav);                      
webdav_close($dav);                      

?>
--EXPECT--
resource(4) of type (DAV Session Buffer)
