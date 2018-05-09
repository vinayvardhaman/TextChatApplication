# TextChatApplication

The chat application follows a typical client-server model, whereby we will have one server instance and two or more client instances.

The clients, when launched, log in to the server, identify themselves, and obtain the list of other clients that are connected to the server. Clients can either send a unicast message to any one of the other clients or a broadcast a message to all the other clients.

Clients maintain an active connection only with the server and not with any other clients. Consequently, all messages exchanged between the clients flow through the server. Clients never exchange messages directly with each other.

The server exists to facilitate the exchange of messages between the clients. The server can exchange control messages with the clients. Among other things, it maintains a list of all clients that are connected to it, and their related information (IP address, port number, etc.). Further, the server stores/buffers any messages destined to clients that are not logged-in at the time of the receipt of the message at the server from the sender, to be delivered at a later time when the client logs in to the server. You do NOT need to buffer messages for EXITed clients or from BLOCKed clients

Implemented Network and SHELL Dual Functionality
When launched (either as server or client), your application should work like a UNIX shell accepting specific commands (described below), in addition to performing network operations required for the chat application to work. select() system call is used to provide a user interface and perform network functions at the same time (simultaneously).
