---
layout: default
title: Matlab
parent: Wrappers
---

# UDA Matlab Wrapper
{:.no_toc}

Support for matlab is provided through the matlab-python interface and uses the pyuda python wrapper under the hood. 

## Contents
{: .no_toc}
1. TOC
{:toc}


# Installation

This requires a working installation of the pyuda python wrapper to already be available (see [pyuda wrapper docs](/UDA/wrappers/python/) ). It's also important to ensure you are using compatible versions on matlab and python as listed [here](https://uk.mathworks.com/support/requirements/python-compatibility.html). 

To use the matpyuda matlab-pyuda interface wrapper, just ensure that the `+matpyuda` matlab module is in your MATLABPATH environment variable, and that the `matpyuda` python module is in your PYTHONPATH. 

```sh
export MATLABPATH=<matpyuda-location>:$MATLABPATH
export PYTHONPATH=<matpyuda-location>:$PYTHONPATH
```

# Known limitations and to-do:

The following functionality is not yet available. Note that all python functionality can be used directly through the matlab-python interface as described in more detail [below](#matlab-wrapper-vs-direct-python).

Unimplemented return types:
All return objects have been implemented, it's worth noting, however, that performance is poor for the current method to convert structured data and geometry data into nested matlab structs. This may be worth looking more into. 

One caveat worth mentioning is that this matlab/pyuda interface has so far only been tested with matlab 2023a.

## MASTU specific items:

unimplemented mast client methods:
- [ ] mast client metadata queries: list_shots, list_archive_directories, etc.

unimplemented geom client methods:
- [ ] geom client metadata queries: listGeomSignals, listGeomGroups, signal map
- [ ] any system-specific geometry routines 


# Examples

## Configuring client and server options

```matlab
client = matpyuda.Client()

% print connection details
client.port
client.server

% set new connection details
client.port = 12345;
client.server = 'new.server.path';

% query and set client flag options
client.set_property('get_meta', py.False)
client.get_property('get_meta')

% expected output:
% ans =
%  logical
%   0


```

## Basic data request syntax

``` matlab

% the matlab client wrapper should parse all pyuda return objects
% to matlab-native types
signal_data = client.get("signal_name", data_source);
structured_data = client.get("group_name", data_source);
string_data = client.get("help::help()", "");
video_data = client.get("image_file", data_source);

% note comma-separated string for batch-lists as matlab hasn't implemented
% python array strings
signal_list = client.get_batch("signal1,signal2,signal3", "source1,source2,source3");

```

## Running mast regression tests

Note that currently mast-specific functions are commented out. These will be moved into the mastcodes repo later along with the mast-specific python-wrapper code (just stored here for convenience during intitial development).

``` matlab
results = runtests("TestMatpyudaMast.m")
```

## Mast client functions

Note that currently mast-specific functions are commented out in the matpyuda python module by default; these need to be enabled for the MastClient matlab class to work. All mast-specific code will be moved into the mastcodes repo later along with the mast-specific python-wrapper code (just stored here for convenience during intitial development).

Runnable examples for the following features can also be found in the `TestMatpyudaMast.m` test script. 

``` matlab

client = MastClient()
signal_list = client.list_signals(alias=<file_alias>, shot=<experiment_number>);

structured_geometry_data = client.geometry("/magnetics/fluxloops", "<experiment_number>");
node = matpyuda.get_node_from_path(structured_geometry_data.data, "path/of/interest");

% get a single frame of a video file, or all frames
frame_data = client.get_images("<file_alias>", "<experiment_number>", last_frame=0);
video_data = client.get_images("<file_alias>", "<experiment_number>");

```

# matlab wrapper vs. direct python

When using the matlab wrapper class for matpyuda, each underlying pyuda method needs to be explicitly wrapped in a matlab method. This may mean only a subset of the pyuda functionality is available in the matlab wrapper. The underlying pyuda client can still be accessed directly, though, and all it's functionality is available through this interface. 

This has additional advantages where any additional pyuda sub-clients may be available for device-specific functionality, as is the case for MAST fusion data. 

Some of the functions available in the matpyuda matlab module will be able to convert the returned pyuda (python) objects into matlab types. These have been implemented for signal data, structured-data, and image data objects so far, as well as simple string data.  

```matlab
client = py.matpyuda.Client()
pyuda_signal_object = client.get("signal_name", data_source)
matlab_signal_object = matpyuda.get_signal(pyuda_signal_object)

pyuda_structured_data = client.get("group_name", data_source)
matlab_structued_data = matpyuda.get_structured_data(pyuda_structured_data)

python_obj = client.get_batch(signal_list, source_list)
```

