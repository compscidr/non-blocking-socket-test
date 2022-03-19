#  non-blocking-socket-test
Whipped up a quick demo to help debug some non-blocking sockets we're working on.

For now, the non-blocking socket is only on the client side, but it would be easy to extend the same pattern to the
server side.

SELECT is used after the socket is put into non-blocking mode. We use SELECT to determine when the connection has been
made, and then we use it again to determine when the socket is ready to read from.

To keep it simple, the server only supports a single connection, but it could be extended to support multiple
concurrent connections by adding the read / write fd sets.

The server starts up, listens for a recv, and then sends a response.
The client connects, sends a message and the waits for a response. It then terminates. This could be extended to do this
back and forth in a loop if desired.

Another further enhancement could be to not require a strict send / recv ordering, but to have either side be able to
send / recv in any order. This would be useful if one side wanted to periodically generate events that do or not dot
require a response for.