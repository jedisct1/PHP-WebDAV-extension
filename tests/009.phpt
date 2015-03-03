--TEST--
Check for throwing exceptions on webdav
--SKIPIF--
<?php if (!extension_loaded("dav")) print "skip"; ?>
--FILE--
<?php

try {
    webdav_connect('https://webdav.yandex.ru',
                   'webdav.extension', 'some4Webdav');
    $ret = webdav_put('/dav-test', 'youpi');
    var_dump($ret);
    $ret = webdav_copy('/dav-test', '/dav-test2', TRUE, TRUE);
    var_dump($ret);
    $ret = webdav_get('/dav-test2');
} catch (\RunTimeException $e) {
    echo 'RunTimeException';
}
webdav_close();

?>
--EXPECT--
RunTimeException
