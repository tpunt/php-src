Notes:
1.
Currently, the actual sending of the message is done synchronously - only the
processing of the message is done asynchronously. This works whilst sending it
locally to a known actor, but sending it to a remote actor will incur latency
overhead.

2.
The abstract Actor class must have its constructor and destructor called when
it is extended by another class. These should ideally be invoked automatically.

3.
The scheduler needs to be joined at the end of the script being executed
manually (via ActorSystem::block). This should be done automatically. It cannot
be done in request or module shutdown of ZE, however, since variables that may
be used by userland code (in Actor::receive) are already freed.
