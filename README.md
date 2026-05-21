# Lan server tool for Linux

## Current structure

### main

- start server on specified port

### server

- create server socket file descriptor
- accept client connections and create a socket for each client
- handle client request
- close connection
- repeat accept->handle->close loop for any new client

### socket
 - create server socket (server_fd)
 - setup sin_addr and bind address to socket
 - listen for connections (can call accept() on this socket)

### client
- receive request
- parse contents
- pass forward to router which then calls the processing function

### http
- parse Http request: method, path, version, content length and content body

### file
- read file as binary and allocate into dynamic buffer
- detect static file format/extension

### response
- send response to client (text or deliver static file)
