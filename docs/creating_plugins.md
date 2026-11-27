---
layout: default
title: Creating a UDA plugin
nav_order: 8
---

## UDA plugin

A UDA plugin is shared library that is loaded dynamically by the
UDA server to enable extra functionality. The plugin makes available
a number of functions which can be called by the UDA client by specifying
the plugin name and function along with any number of arguments.

For example, if we have a plugin which is called `TEST` we might have
functions:
- `TEST::help()`
- `TEST::readData(filename=foo)`
- `TEST::process(signal=foo,operation=bar,flag)`

These functions read the arguments, do some processing, and return a
UDA `DataBlock` containing the data to be passed to the client.

## Structure of a plugin

The internals of a plugin is up to the plugin author. It can written in any
language that can be compiled to a shared library (.so on Linux, .dylib on MacOS,
.dll on Windows) but must provide a C style entry function which is
called by the UDA server.

The entry function must look like:

`int entryFunction(IDAM_PLUGIN_INTERFACE*)`

Then function can have any name — the name is specified in the plugin configuration
file, see below. The `IDAM_PLUGIN_INTERFACE` is a structure defined in `uda/plugins.h`
and provides a mechanism for the plugin to interact with the UDA server — reading the
function arguments, returning the resultant data, etc. The returned `int` value specifies
the return state of the plugin call — zero for success or non-zero for an error state
(the actual error is returned via the `IDAM_PLUGIN_INTERFACE`).

## Accessing function name and arguments

The function being called is provided in the `IDAM_PLUGIN_INTERFACE` structure passed into
entry function as a C-string (`const char*`) via:

`plugin_interface->request->function`

The arguments that where passed to the plugin function is passed a list of name-value pairs
via:

`plugin_interface->request->nameValueList`

You can the provided helper functions to find the argument in the list and return the value:

`bool find<TYPE>Value(NAMEVALUELIST* nvl, TYPE* value, const char* name)`

where `<TYPE>` is the name of type being expected, i.e. `Int` or `Double`, `name` is the name of the argument
being read and `value` is where the found value is stored. If the argument is not found the function will return
`false`, otherwise it will return `true` and populate `data` with the found value.

Help macros are also available when the name of the variable being read into is the same as the
name of the argument being read, i.e. to read a function argument called `arg1` we can used:

```c
int arg1;
bool found = FIND_INT_VALUE(nvl, arg1);
```

If the function argument is required you can use another helper macro which will set an error message
and return from the function with a non-zero value, if the argument is not found:

```c
int arg1;
FIND_REQUIRED_INT_VALUE(nvl, arg1);
```

## Reporting errors

Errors must be recorded on the `IDAM_PLUGIN_INTERFACE` passed to the plugin: the server collects
`plugin_interface->error_stack` after the call and returns it to the client. Do not use `addIdamError()` /
`concatUdaError()` for this. They record into library-global state, and each UDA library (and on macOS and
Windows, each loaded copy of it) has its own, so the server may never see those errors.

```c
// Record an error, log it and return the code from the current function
UDA_PLUGIN_RAISE_ERROR_CODE(plugin_interface, 1234, "Unable to open file");
UDA_PLUGIN_RAISE_ERROR(plugin_interface, "Something failed");            // code 999
UDA_PLUGIN_RAISE_ERROR_F(plugin_interface, "Bad value: %d", value);      // formatted, code 999

// Record without returning
UDA_PLUGIN_ADD_ERROR(plugin_interface, code, "message");
UDA_PLUGIN_ADD_SYS_ERROR(plugin_interface, errno, "");                   // appends strerror(errno)
```

Records are kept in insertion order (at most `UDA_MAX_ERRORS`). The first record is treated as the root cause and
becomes the error code and message the client sees. If a plugin returns non-zero without recording anything, the
server still reports the failure with the returned code and a generic message.

The older `RAISE_PLUGIN_ERROR` / `FIND_REQUIRED_*` macros do not take the interface. To route them through it,
define the name of the interface variable before including the UDA headers. The compiler then reports every
function that uses them without an interface in scope:

```c
#define UDA_PLUGIN_INTERFACE_VAR plugin_interface
#include <plugins/udaPlugin.h>
```

A plugin that makes its own UDA client request should read the downstream failure through the client API (for
example `getUdaServerErrorStack()`) and append the relevant records to its interface with `udaAppendErrorStack()`.

## Using the shared helpers

Shared helpers that can fail or trace take a diagnostics context rather than recording into global state. Pass
`udaPluginDiagnostics(plugin_interface)`, which records their errors on the interface's error stack and sends their
traces to the server's log:

```c
UDA_DIAGNOSTICS diag = udaPluginDiagnostics(plugin_interface);
udaExpandEnvironmentVariables(&diag, path);
err = udaMakeRequestData(&diag, &request, *plugin_interface->pluginList, plugin_interface->environment);
udaPrintDataBlock(&diag, *plugin_interface->data_block);
```

The context points into `plugin_interface`, so only use it during the current call. The older forms of these
helpers (`expand_environment_variables()`, `makeRequestData()`, `name_value_pairs()`, `printDataBlock()`,
`printAction()`, `serverSubsetData()`, ...) report through the client or server library's own error stack. Those
libraries are not available to plugins by default (see [Linking](#linking)).

Helpers that cannot fail (data block initialisation, string utilities, user defined type registration, the malloc
log) are unchanged.

## Linking

Plugins built with the `uda_plugin()` CMake macro link only `libuda_plugins`, which includes the shared helpers
above. It has no error stack or logger of its own and does not link the UDA client or server, so plugins report only
through the plugin interface.

A plugin that uses the UDA client API itself (for example to make downstream requests) opts in with `LINK_CLIENT`:

```cmake
uda_plugin(
  LINK_CLIENT
  NAME MYPLUGIN
  ...
)
```

It must still report through its interface: errors recorded with `addIdamError()` go to the client library's
error stack, which the server never reads. The `plugin_link_guard` test checks that the built-in plugins follow
these rules.

## Calling other plugins

Use `udaCallPlugin(plugin_interface, "OTHER::function(args)")` (or `CALL_PLUGIN`). The child runs with an empty
error stack and the parent's logging, and its error records are appended to the parent's before any context the
parent adds. A child failure is returned before any subsetting in the request (e.g. `[0:10]`) is applied.

## Logging

Log through the interface so diagnostics go to the server's configured log files at the server's configured
level, and never to stdout/stderr (which may be the client connection under xinetd):

```c
UDA_PLUGIN_LOG(plugin_interface, UDA_LOG_DEBUG, "Reading %s\n", path);
```

The level check happens before formatting, so disabled debug logging costs little. Under a host that only
provides interface version 1 plugin logging is a no-op.

## Interface versions

`interfaceVersion` on the interface is the version the server is offering this plugin: the lower of the version the
plugin is registered for (configuration column 6, see below) and the highest version the server supports
(`UDA_PLUGIN_INTERFACE_VERSION`). Version 2 adds the host logging fields (`log_function`, `log_context`,
`log_level`). Only read those when `interfaceVersion >= 2`. A plugin should reject versions newer than it
understands:

```c
if (plugin_interface->interfaceVersion > THISPLUGIN_MAX_INTERFACE_VERSION) {
    UDA_PLUGIN_RAISE_ERROR(plugin_interface, "Plugin Interface Version Unknown to this plugin");
}
```

The server refuses to call a plugin whose registration asks for a newer version than it supports.

## Returning data from a plugin

To return

## Plugin configuration file

The UDA server has a plugin configuration file that specifies how to load and use
the plugins available to the server. The plugin configuration file is a comman separated
text file with each line defined as:

`NAME, TYPE, ENTRY_FUNC, LIBRARY, *, INTERFACE_VERSION, 1, 1, DESC`

Where:

- `NAME`: the name of the plugin — this is the name by which the client can call the call the plugin.
- `TYPE`: the type of the plugin — should be `function` for the plugins defined here.
- `ENTRY_FUNC`: the name of the plugin entry function.
- `LIBRARY`: the file name of the compile shared library, including extension.
- `INTERFACE_VERSION`: the highest plugin interface version the plugin binary supports (1 if empty). It must match
  the plugin's `THISPLUGIN_MAX_INTERFACE_VERSION`, so deploy configuration files and rebuilt plugins together. When
  building with the `uda_plugin()` CMake macro, set it with the `INTERFACE_VERSION` argument (default 1).
- `DESC`: a short description of the plugin which is used by UDA when listing the plugins available. 

Note:

The other elements (`*`, and the two `1`s after the interface version) are not applicable to the `function` type
plugins and should always be set to these values.

For example:

`HELP, function, helpPlugin, libhelp_plugin.dylib, *, 2, 1, 1, Service Discovery: list the details on all registered services, HELP::services()`

## Copying the template plugin

The easies way to create a new plugin is to copy the `templatePlugin` from the UDA repository.
Copy the template plugin directory `source/plugin/template` and rename as required. The directory contains
the following files:

- `CMakeLists.txt`: the CMake configuration file specifying how to build the plugin
- `templatePlugin.h`: the plugin header file
- `templatePlugin.cpp`: the plugin source file

Now update the plugin:

1. Rename the `templatePlugin.*` files to something that makes sense for your plugin and update
the `CMakeLists.txt` file as appropriate.
2. Change the entry function name in the source and header files from `templatePlugin(...)` to
something that corresponds to your plugin, and update `CMakeLists.txt` accordingly.
3. Implement the functionality required in the plugin.

The template plugin is written in C++ and uses a TemplatePlugin object to handle any of the function
requests. The plugin object is static as data can be cached between calls to avoid having to repeat work,
for example holding onto file handles to avoid re-opening files.

You can use the structure as provided or implement the functionality however you desire as long as each call
to the entry function handles the function requested and returns an error code accordingly.
