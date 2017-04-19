# Get started

## Requirements

* Linux environment
* G++ 4.2.x
* GNU Make 3.8 or above
* Protobuf compiler (`protoc` must be available)
* Protobuf gRPC C++ plugin

## Build

Change directory to `databroker`

```sh
$ make
```

## Try it!
Run the server, which will listen on port 50051:

```sh
$ ./databorker_server
```

Run the client (in a different terminal):

Send data to server:
```sh
$ ./databorker_client 50051 PUT /local/path/to/file
```

Retrieve data from server:
```sh
$ ./databorker_client 50051 GET <ID> /path/to/store/file
```

# Future work

For simplicity and due to time limitation, tests are not included at this moment.
