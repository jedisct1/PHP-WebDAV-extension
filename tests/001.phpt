--TEST--
Check for dav presence
--SKIPIF--
<?php if (!extension_loaded("dav")) print "skip"; ?>
--FILE--
<?php 
echo "dav extension is available";
?>
--EXPECT--
dav extension is available
