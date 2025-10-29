# This package is deprecated

Please use ['pyuda'](https://pypi.org/project/pyuda/) instead. This package is now for compatibility only and may be removed in the future. Installing this 'uda' package will now only install a shim which references the underlying 'pyuda' package. 

Install the new package directly using:
```
pip install pyuda
```

Previous release versions (2.7.6-2.8.1) are still hosted here. The first release under the new pyuda name is 2.8.2. 

## Pyuda package description

See ['pyuda'](https://pypi.org/project/pyuda/) for most up to date descriptions. Below is the package information as of release 2.8.1.

pyuda is the python interface to the uda client library. It is used for remote access to scientific and experimental data from a number of international labs hosting uda data servers. 

- **Website:** https://ukaea.github.io/UDA/
- **Documentation:** https://ukaea.github.io/UDA/
- **Source Code:** https://github.com/ukaea/UDA
- **Issue Tracker:** https://github.com/ukaea/UDA/issues


## Quick-start

```py
import pyuda

pyuda.Client.server = "<server_address>"
pyuda.Client.port = <port_number>
client = pyuda.Client()
signal_object = client.get(signal, source)

```
