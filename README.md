# Valgrind Bug

Reproduction:

`./buildconf`
`./configure --disable-all --enable-phactor --enable-maintainer-zts --enable-debug`
`make`

```php
<?php

$actorSystem = new ActorSystem();

class Test extends Actor
{
    public function receive($sender, $message){}
}

new class (new Test) extends Actor {
    function __construct(Test $test)
    {
        var_dump($this, $test);
        $this->send($test, 1);
    }

    function receive($sender, $message){}
};

$actorSystem->block();
```

`valgrind sapi/cli/php ext/phactor/script.php`

Result:
LLDB should crash, causing Terminal to crash, forcing the computer to restart
in order to make Terminal usable again.
