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
