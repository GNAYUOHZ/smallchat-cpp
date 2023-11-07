# smallchat-cpp

This is a C++ implement of the original [smallchat](https://github.com/antirez/smallchat), utilizing epoll to manage TCP connections.


## Getting Started

### Build

To compile the project, run the following command:

```sh
make
```

### Running Server

Start the server with the following command:

```sh
./smallchat
```

The server will start and listen for incoming TCP connections on port 7711.

### Running Clients

You can launch multiple terminals and use Telnet:


```sh
telnet localhost 7711
```

Any message from one client will be broadcasted to all other connected clients.
