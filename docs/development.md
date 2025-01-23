---
layout: default
title: UDA Development
nav_order: 5
---

# UDA Development
{:.no_toc}

## Contents
{:.no_toc}
1. TOC 
{:toc}

## Setting up a development environment

To begin developing you will need to fork the uda repository from github, build the project, and run a local development server for testing before making your changes.

### Creating a development build
Follow the instructions in [server installation](/UDA/server_installation) for building an uda server from source for your platform. You can optionally change the cmake config option to generate debug symbols if required: 

```sh
cmake --build build  --config Debug --target intall
```

### Run the development server
There is a simple way to temporarily run the uda super-server daemon under xinted for testing or development only that doesn't require the same elevated privileges as the system-level install described in the [server installation](/UDA/server_installation) section; this method is currently only available for xinetd and is described in the next section. 

Note that if you need to run your server under `launchd` (for MacOS) or `systemd`, you will need to follow the instructions on the [server installation](/UDA/server_installation) page to run your development server.

### Running a development server under xinetd
Once uda has been built there will be a script called `rc.uda` in the `etc` subdirectory of your uda install which can be used as shown below to start a xinetd process listening on the port specified in `etc/xinetd.conf`. Xinetd here is used as a super-server daemon which will launch new uda server processes for each new incoming client connection. 

The `rc.uda` script takes one of 3 possible arguments: `start` to start the xinetd process, `stop` to kill a running xinetd process (from the pid written to a file called `xinetd.<hostname>.pid`), and `status` which will print whether a valid pid file is found or not, or if one is found but the process no longer exists. 

```sh
./rc.uda start
./rc.uda status
```
The text `server running` will be output to the console if this executes successfully. If the `start` command failed for any reason the status will output `xinetd.<hostname>.pid not found, server not running` instead. Logging from the xinetd process (including messages from successful or failed startup attempts) will be written to a file called `mylog.<hostname>`. Note that the port number specified in `xinetd.conf` must be unique for each running uda server installation, so may need to be changed from the default 56565 value if that is already in use by another server on the same host. 

Note that this method of running the xinetd super-server daemon is not appropriate for running a production server and should be used for development purposes only. See the instructions in the [server installation](/UDA/server_installation) guide instead for the recommended deployment methods. 

### Running a development server under launchd (MacOS)
See the launchd section of the instructions for [building and running a server](/UDA/server_installation#launchd). 

### Running a fat client (client and server in one process)
TODO: fat client instructions

## Running tests

The first two tests here will verify that a server can receive connections, the final section describes how to run the automated test suite. 

### Server socket connection is open and receiving connections

You can use the ncat commandline tool to attempt to connect to the server socket. Here the `-v` option sets the output level to verbose, `-z` is used to report the connection status only, and and the `-4` option specifies the use of IPv4 only.

```sh
nc -zv -4 localhost 56565
```
On success the output will include a line like `Ncat: Connected to localhost:56565`. On failure a message will be printed to the console saying `Ncat: Connection refused`. 

### Client-server communication working as expected 
You can use the UDA command-line interface tool, `uda_cli`, to test a simple request to the server which will return a simple text string. The `uda_cli` binary will be located in the `bin` subdirectory of your uda install location. 

```sh
export PATH=$PATH:<uda install location>/bin
uda_cli -h localhost -p 56565 "help::help()"
```

This should print the following text to the console on success.

```sh
request: help::help()

Help    List of HELP plugin functions:

services()      Returns a list of available services with descriptions
ping()          Return the Local Server Time in seconds and microseonds
servertime()    Return the Local Server Time in seconds and microseonds
```

### Running the UDA test suite

The binary for the automated tests will be located in your cmake build directory. Note that your server must be running (e.g. by running the `rc.uda start` command described above) for these tests to run. 

```sh
cd <uda source dir>/build/test/plugins
export UDA_HOST=localhost
export UDA_PORT=56565
./plugin_test_testplugin
```

## Writing new tests
TODO: testing strategy and CTest integration

## Debugging
This section will mainly focus on how to debug an uda server because of the complications of the client-server communication. For debugging either the client or the fat-client there aren't any additional considerations to be aware of in the same way, simply compile with debug symbols and use your usual choice of debugging tool (`gdb`, `lldb`, etc.).

### Server debugging procedure
The uda server process is launched by the super-server daemon (i.e. `xinetd` or `systemd`) only when a client request is received, and only runs while that client connection is alive. The server debugging process must be launched after a request has been receieved (and the server process has actually started), but before the request has been completed by the server.

The general server debugging procedure therefore involves using 2 separate debugging processes (ie `gdb` or `lldb` sessions): one on the client side which will pause the execution of the clientside routine at the correct time, and a separate debugging session on the server which can only be started when the client-side session hits its first breakpoint. 

### Pausing the client-side execution using gdb

The simplest client to use for debugging will be the `uda_cli` launched from the same host where the server is installed. The example below shows the syntax to use, the `"help::help()"` request is a placehodlder and can be replaced with whatever you are interested in testing. The line-number of the break point is selected to pause the client-side execution just after the request has been sent from the client to the server.

```sh
gdb --args uda_cli -h localhost -p 56565 "help::help()"
break udaClient.cpp:743
run
```

### Breaking into the server process
First you need to find the PID corresponding to the specific test-server process you're interested in. It's useful to note that the default name of all uda server processes will be `uda_server` (the name of the binary in the `bin` directory of your install location). If you have more than one server running, or more than one client connection open at that time to your test server, you will have to manually locate the correct PID.

```sh
# list all uda_server process ID numbers, all that's needed if there's only 1
pidof uda_server

# list all uda_server processes with start-times and binary locations to distinguish different processes
ps -fe | grep uda_server

# begin debugger using the server process you've selected
gdb -p <PID>

# set break points where required
break help_plugin.cpp:66
cont
```

### Unlocking the client-side process and debugging the server
Use the `cont` command on the client-side debugging process when you're ready for the server-side execution to progress.

From this point you can continue to use `gdb` on the server process as you would usually. 

## Code style conventions

TODO: clang-format and clang-tidy instructions

## Package Breakdown

TODO: UML diagrams

### Server

### Clientserver

### Client

### Plugins

### Structures

### Security

### Serialisation
