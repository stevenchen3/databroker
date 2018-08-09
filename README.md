# About

This is an example program implemented using C++ to demonstrate the how to use gRPC and protocol
buffer to exchange data between processes.

# Get started

## Requirements

* Linux environment
* G++ 4.2.x
* GNU Make 3.8 or above
* Protobuf compiler (`protoc` must be available)
* Protobuf gRPC C++ plugin

## Build


```sh
$ cd databroker
$ make
```

## Try it!
Run the server, which will listen on port 50051:

```sh
$ ./databorker_server
```

Run the client (in a different terminal):

Note that input validation is not supported at this moment.

Send data to server:
```sh
$ ./databorker_client 50051 PUT /local/path/to/file
```

Retrieve data from server:
```sh
$ ./databorker_client 50051 GET <ID> /path/to/store/file
```
