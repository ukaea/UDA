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

NOTE: matlab currently does not support conversion of string arrays into python lists. this makes it harder to implement the get_batch functionality. Could maybe have a string-splitter on the python side with a specified delimiter. 

Unimplemented return types:
- [x] Video return objects when getting image or video data
- [x] Lists of signal objects from the get_batch API
- [x] the optional "meta" attribute for the Signal class
- [x] errors on the signal object

uninplemented methods:
- [x] get_batch API
- [x] setting client flags

The final caveat is that this has so far only been tested with matlab 2023a.

# Examples

```m
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

% the matlab client wrapper should parse all pyuda return objects
% to matlab-native types
signal_data = client.get("signal_name", data_source);
structured_data = client.get("group_name", data_source);
string_data = client.get("help::help()", "");
video_data = client.get("image_file", data_source);

% other wrapped functions
% note comma-separated string for batch-lists as matlab hasn't implemented
% python array strings
signal_list = client.get_batch("signal1,signal2,signal3", "source1,source2,source3");

% initialise the client object with a device-specific pyuda client
% instead of the default one
mast_client = py.mast.matpyuda.Client();
client = matpyuda.Client(mast_client);
```

# matlab wrapper vs. direct python

When using the matlab wrapper class for matpyuda, each underlying pyuda method needs to be explicitly wrapped in a matlab method. This may mean only a subset of the pyuda functionality is available in the matlab wrapper. The underlying pyuda client can still be accessed directly, though, and all it's functionality is available through this interface. 

This has additional advantages where any additional pyuda sub-clients may be available for device-specific functionality, as is the case for MAST fusion data. 

Some of the functions available in the matpyuda matlab module will be able to convert the returned pyuda (python) objects into matlab types. These have been implemented for signal data, structured-data, and image data objects so far, as well as simple string data.  

```m
client = py.matpyuda.Client()
pyuda_signal_object = client.get("signal_name", data_source)
matlab_signal_object = matpyuda.get_signal(pyuda_signal_object)

pyuda_structured_data = client.get("group_name", data_source)
matlab_structued_data = matpyuda.get_structured_data(pyuda_structured_data)

python_obj = client.get_batch(signal_list, source_list)
```

