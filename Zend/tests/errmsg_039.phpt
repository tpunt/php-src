--TEST--
errmsg: cannot redeclare property
--FILE--
<?php

class test {
	public $var;
	public $var;
}

echo "Done\n";
?>
--EXPECTF--	
Fatal error: Cannot redeclare test::$var in %s on line %d
