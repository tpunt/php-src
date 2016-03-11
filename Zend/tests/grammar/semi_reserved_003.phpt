--TEST--
Test semi-reserved words as class properties
--FILE--
<?php

class Obj
{
    public $empty = 'empty';
    public $callable = 'callable';
    public $class = 'class';
    public $trait = 'trait';
    public $extends = 'extends';
    public $implements = 'implements';
    public $static = 'static';
    public $abstract = 'abstract';
    public $final = 'final';
    public $public = 'public';
    public $protected = 'protected';
    public $private = 'private';
    public $const = 'const';
    public $enddeclare = 'enddeclare';
    public $endfor = 'endfor';
    public $endforeach = 'endforeach';
    public $endif = 'endif';
    public $endwhile = 'endwhile';
    public $and = 'and';
    public $global = 'global';
    public $goto = 'goto';
    public $instanceof = 'instanceof';
    public $insteadof = 'insteadof';
    public $interface = 'interface';
    public $namespace = 'namespace';
    public $new = 'new';
    public $or = 'or';
    public $xor = 'xor';
    public $try = 'try';
    public $use = 'use';
    public $var = 'var';
    public $exit = 'exit';
    public $list = 'list';
    public $clone = 'clone';
    public $include = 'include';
    public $include_once = 'include_once';
    public $throw = 'throw';
    public $array = 'array';
    public $print = 'print';
    public $echo = 'echo';
    public $require = 'require';
    public $require_once = 'require_once';
    public $return = 'return';
    public $else = 'else';
    public $elseif = 'elseif';
    public $default = 'default';
    public $break = 'break';
    public $continue = 'continue';
    public $switch = 'switch';
    public $yield = 'yield';
    public $function = 'function';
    public $if = 'if';
    public $endswitch = 'endswitch';
    public $finally = 'finally';
    public $for = 'for';
    public $foreach = 'foreach';
    public $declare = 'declare';
    public $case = 'case';
    public $do = 'do';
    public $while = 'while';
    public $as = 'as';
    public $catch = 'catch';
    public $die = 'die';
    public $self = 'self';
    public $parent = 'parent';
    public $isset = 'isset';
    public $unset = 'unset';
    public $__CLASS__ = '__CLASS__';
    public $__TRAIT__ = '__TRAIT__';
    public $__FUNCTION__ = '__FUNCTION__';
    public $__METHOD__ = '__METHOD__';
    public $__LINE__ = '__LINE__';
    public $__FILE__ = '__FILE__';
    public $__DIR__ = '__DIR__';
    public $__NAMESPACE__ = '__NAMESPACE__';
    public $__halt_compiler = '__halt_compiler';
}

$obj = new Obj;

echo $obj->empty, PHP_EOL;
echo $obj->callable, PHP_EOL;
echo $obj->class, PHP_EOL;
echo $obj->trait, PHP_EOL;
echo $obj->extends, PHP_EOL;
echo $obj->implements, PHP_EOL;
echo $obj->static, PHP_EOL;
echo $obj->abstract, PHP_EOL;
echo $obj->final, PHP_EOL;
echo $obj->public, PHP_EOL;
echo $obj->protected, PHP_EOL;
echo $obj->private, PHP_EOL;
echo $obj->const, PHP_EOL;
echo $obj->enddeclare, PHP_EOL;
echo $obj->endfor, PHP_EOL;
echo $obj->endforeach, PHP_EOL;
echo $obj->endif, PHP_EOL;
echo $obj->endwhile, PHP_EOL;
echo $obj->and, PHP_EOL;
echo $obj->global, PHP_EOL;
echo $obj->goto, PHP_EOL;
echo $obj->instanceof, PHP_EOL;
echo $obj->insteadof, PHP_EOL;
echo $obj->interface, PHP_EOL;
echo $obj->namespace, PHP_EOL;
echo $obj->new, PHP_EOL;
echo $obj->or, PHP_EOL;
echo $obj->xor, PHP_EOL;
echo $obj->try, PHP_EOL;
echo $obj->use, PHP_EOL;
echo $obj->var, PHP_EOL;
echo $obj->exit, PHP_EOL;
echo $obj->list, PHP_EOL;
echo $obj->clone, PHP_EOL;
echo $obj->include, PHP_EOL;
echo $obj->include_once, PHP_EOL;
echo $obj->throw, PHP_EOL;
echo $obj->array, PHP_EOL;
echo $obj->print, PHP_EOL;
echo $obj->echo, PHP_EOL;
echo $obj->require, PHP_EOL;
echo $obj->require_once, PHP_EOL;
echo $obj->return, PHP_EOL;
echo $obj->else, PHP_EOL;
echo $obj->elseif, PHP_EOL;
echo $obj->default, PHP_EOL;
echo $obj->break, PHP_EOL;
echo $obj->continue, PHP_EOL;
echo $obj->switch, PHP_EOL;
echo $obj->yield, PHP_EOL;
echo $obj->function, PHP_EOL;
echo $obj->if, PHP_EOL;
echo $obj->endswitch, PHP_EOL;
echo $obj->finally, PHP_EOL;
echo $obj->for, PHP_EOL;
echo $obj->foreach, PHP_EOL;
echo $obj->declare, PHP_EOL;
echo $obj->case, PHP_EOL;
echo $obj->do, PHP_EOL;
echo $obj->while, PHP_EOL;
echo $obj->as, PHP_EOL;
echo $obj->catch, PHP_EOL;
echo $obj->die, PHP_EOL;
echo $obj->self, PHP_EOL;
echo $obj->parent, PHP_EOL;
echo $obj->isset, PHP_EOL;
echo $obj->unset, PHP_EOL;
echo $obj->__CLASS__, PHP_EOL;
echo $obj->__TRAIT__, PHP_EOL;
echo $obj->__FUNCTION__, PHP_EOL;
echo $obj->__METHOD__, PHP_EOL;
echo $obj->__LINE__, PHP_EOL;
echo $obj->__FILE__, PHP_EOL;
echo $obj->__DIR__, PHP_EOL;
echo $obj->__NAMESPACE__, PHP_EOL;
echo $obj->__halt_compiler, PHP_EOL;

echo "\nDone\n";

?>
--EXPECTF--
empty
callable
class
trait
extends
implements
static
abstract
final
public
protected
private
const
enddeclare
endfor
endforeach
endif
endwhile
and
global
goto
instanceof
insteadof
interface
namespace
new
or
xor
try
use
var
exit
list
clone
include
include_once
throw
array
print
echo
require
require_once
return
else
elseif
default
break
continue
switch
yield
function
if
endswitch
finally
for
foreach
declare
case
do
while
as
catch
die
self
parent
isset
unset
__CLASS__
__TRAIT__
__FUNCTION__
__METHOD__
__LINE__
__FILE__
__DIR__
__NAMESPACE__
__halt_compiler

Done
