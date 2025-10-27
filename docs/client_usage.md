---
layout: default
title: Using the UDA client
nav_order: 5
---

# Using the UDA client 
{:.no_toc}

This section will describe the main features to be aware of in the UDA client library.

## Contents
{:.no_toc}
1. TOC 
{:toc}

## Connection details

Two fields define the server that a client will connect to: the server address (as an IP or DNS name) and a port number. These details can either be set as environment variables in the shell before launching your client application, or can be set through API functions (the exact syntax will depend on the wrapper you use). 

syntax using environment variables:
```sh
export UDA_HOST=<server_address>
export UDA_PORT=<port_number>
```

example syntax using the python wrapper:
```py
pyuda.Client.server = "<server_address>"
pyuda.Client.port = <port_number>
```

### Authentication

UDA currently supports authenticaed server access using SSL certificates. The options and certificate locations must be set using environment variables before you connect to an authenticated server. See the Authentication page for more details.

## TODO: The get and put APIs

### TODO: calling plugin functions

### TODO: subsetting syntax

## Configuration flags and settings

There are a number of options that can be set to configure the client properties. 

### Timeout

When you establish a connection with a UDA server the same connection stays alive for either the duration of the client process (ie multiple data requests are all served by the same server) or when the server timeout duration has passed. 

There are currently 2 modes for how this timeout behavior works and a configuration option exists to toggle between the two. The first (and default) setting is where the timeout time describes the total lifetime of the server. Once this has passed the server process will be killed and a new one will start to server further requests from the client. The second setting is where the timeout time only accumulates between data requests and is reset after each one is fulfilled, so the server is only killed due to inactivity after a specified idle time. 

The value of the timeout duration is also customisable and by default is set to 10 minutes.

example syntax using the python wrapper:
```py

# query the current status of the IDLE_TIMEOUT flag (default is false for total-lifetime behaviour)
client.get_property(pyuda.Properties.IDLE_TIMEOUT)

# change the timeour mode to idle time instead of total lifetime
client.set_property(pyuda.Properties.IDLE_TIMEOUT, True)

# change the timeout duration from 600s (default) to 30 seconds
client.set_property(pyuda.Properties.TIMEOUT, 30)

```

### Type-casting of returned data

A number of flags exist to force a conversion of returned data to a specific data type. These optionas all default to false which means the original data type from the data source will be returned unless otherwise specified. 

```py

# cast data to double precision floating point numbers
client.set_property(pyuda.Properties.GET_DATA_DOUBLE, True)

# cast dimension data to double precision floating point numbers
client.set_property(pyuda.Properties.GET_DIM_DOUBLE, True)

# cast time data to double precision floating point numbers
client.set_property(pyuda.Properties.GET_TIME_DOUBLE, True)
```

### Ignoring dimension data

UDA usually returns array data in a rich structure containing the data itself along with labels, units, and dimension data. Dimensions could be the time points the data was recorded at for a 1D array, or perhaps time and 2 position vectors for a 3D array. 

An option exists to request just the data itself and not any dimension values. The default behaviour is dimension data is always returned to the client where it exists. 

```py
client.set_property(pyuda.Properties.GET_NO_DIM_DATA, True)

```
