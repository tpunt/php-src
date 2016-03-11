--TEST--
Deprecate var member modifier
--FILE--
<?php

class A
{
	var $public;
}

--EXPECTF--
Deprecated: The "var" member modifier has been deprecated in favour of "public" in %s on line %d