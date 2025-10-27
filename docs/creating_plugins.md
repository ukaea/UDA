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

## Returning data from a plugin

To return

## Plugin configuration file

The UDA server has a plugin configuration file that specifies how to load and use
the plugins available to the server. The plugin configuration file is a comman separated
text file with each line defined as:

`NAME, TYPE, ENTRY_FUNC, LIBRARY, *, 1, 1, 1, DESC`

Where:

- `NAME`: the name of the plugin — this is the name by which the client can call the call the plugin.
- `TYPE`: the type of the plugin — should be `function` for the plugins defined here.
- `ENTRY_FUNC`: the name of the plugin entry function.
- `LIBRARY`: the file name of the compile shared library, including extension.
- `DESC`: a short description of the plugin which is used by UDA when listing the plugins available. 

Note:

The other elements (`*, 1, 1, 1`) are not applicable to the `function` type plugins are should always be set
to these values.

For example:

`HELP, function, helpPlugin, libhelp_plugin.dylib, *, 1, 1, 1, Service Discovery: list the details on all registered services, HELP::services()`

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
