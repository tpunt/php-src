Notes:
1.
Currently, the actual sending of the message is done synchronously - only the
processing of the message is done asynchronously. This works whilst sending it
locally to a known actor, but sending it to a remote actor will incur latency
overhead.

2.
