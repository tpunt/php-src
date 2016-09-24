To Do's
 - actor ctor function
 - global mutex for updating globals

Notes:
1. (working on it)
The abstract Actor class must have its constructor and destructor called when
it is extended by another class. These should ideally be invoked automatically.

2.
The scheduler needs to be joined at the end of the script being executed
manually (via ActorSystem::block). This should be done automatically. It cannot
be done in request or module shutdown of ZE, however, since variables that may
be used by userland code (in Actor::receive) are already freed.

3.
Should be --with-phactor in config.m4 (for pthreads).

4.
ZMM macros aren't used for memory allocation because PHP's allocator is not
thread safe, and so one thread's heap cannot be used across multiple threads.
So whilst a memory limit can still be set for PHP, the actor system will be
exempt from this limit (thus making the set limit somewhat useless).

API:
```php
class Actor
{
    public abstract function receive($message);
    public final function receiveBlock(); // name as just receive?
    public final function remove();
    public final function send($to, $message);
    public final function sentAfter($to, $message, $time);
}
```
Message execution:
1. Use the Actor::receive method

2. Provide a callback instead
This would have to be done by either passing the callable through
`Actor::__construct`, or via a setter.

Will a task ever need to be executed that didn't belong to an Actor-extended class?


General todo:
1.
Remove the mutex on the whole of the event loop, otherwise only one thread will
be able to execute an actor at once. At the very least the call to receive
should not be in the mutex lock.
2.
Add the method `isRemoteActor()` onto the actor object.



store thread_offset as a global (rather than using i) - necessary anymore if we
are not going to use pthread conditions?

store worker_threads as a global to enable other threads to send signals to each other

store current_message_value as a zend global since it is thread-specific only


Actors may be blocking if they either:
A) invoke their receiveBlock() method
B) are executing a particularly CPU intensive task (perhaps count frame slots?)
C) are executing an IO-bound task that does not utilise the CPU

In these instances, the worker thread needs to save the state of the currently
executing actor, add it back onto the task queue, and then restore its state
before continuing with execution.

For A), an additional member to the actor struct (`new_message`) will need to be
added to act as a flag for when a check should be performed to see if the actor
is manually blocking. This flag will be set when either receiveBlock() is called,
or when a new message is sent to a manually blocking actor. Flags:
 -1 = not manually blocking = process the actor
  0 = manually blocking with no new messages (don't attempt to process the actor)
  1 = manually blocking with at least 1 new message (attempt to process the actor)

`blocking`
0 - not blocking (process)
1 - blocking via preemptive interrupt (process)
2 - blocking via manual interrupt without new messages (don't process)
3 - blocking via manual interrupt with new messages (process)

For B) and C), we should check if the actor has a current state, and if so,
restore it before executing it. An additional member to the actor struct
(`state`) will need to be added, where a check to see if it has a previous state
(`state != NULL`) will need to be performed.

Store actors in a hashtable with actors' UUID as primary key instead of singly-
linked list as there is now?

---

When Actor::receiveBlock is called, freeze the current call stack for the
invocation context. Set a special flag for the blocking actor so that when it is
processed, a notification can be used to resume the frozen call stack.

Perhaps add the frozen call stack to a singly-linked list with an associated ID,
where this ID is stored in the actor (assigned upon receiveBlock call)? That way,
upon processing that actor, the global singly-linked list can be traversed to
find the frozen call stack, which can then in turn be added to the task queue.
