---
layout: default
title: Python
parent: Wrappers
---

# Pyuda python wrapper
{:.no_toc}



## Contents
{: .no_toc}
1. TOC
{:toc}

# Installation

pyuda can be pip-installed from the `python_installer` directory which will be installed once the main uda client build has been completed successfully. This directory will contain a `setup.py` file and the source code for the pyuda module and underlying cpyuda cython module. 

The build process will compile some cython code, so it's important to make sure you use the same compiler as was used to build the uda client libraries. 

There are some python packages that the build will depend on, namely: `cython`, `numpy`, and `six`

```sh
cd <uda-install-location>/python_installer
pip install cython numpy six
pip install .
```

# Examples

```py
import pyuda

# set the server address details. 
# This will be 'localhost' and 56565 if the server is hosted locally.
pyuda.Client.server = '<SERVER_ADDRESS>'
pyuda.Client.port = <PORT_NUMBER>

client = pyuda.Client()
result_object = client.get("help::help()", "")
print(result_object)

# if the server has an associated back-end plugin for the correct data type 
# you can read individual signals out of data files
signal_object = client.get("netcdf/signal/path", "archive/location/datafile.nc")
signal_object.plot()

# You can also call any plugin functions for the plugins that have been
# installed with the server
result_object = client.get("PLUGIN_NAME::FUNCTION_NAME(kwarg=value)", "")
```