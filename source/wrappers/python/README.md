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
