# Link guard for the plugin libraries.
#
# 1. LIBS (libuda_plugins and every built-in plugin) must not define the client/server logger,
#    error-stack or server entry points: each defining library would hold its own copy of that state (a separate
#    instance on macOS and Windows), so errors and diagnostics recorded through it never reach the server.
# 2. STATELESS_LIBS (libuda_plugins and plugins that do not opt in with LINK_CLIENT) must not even
#    reference the library-global logger or error stack, and must not depend on libuda_client: they report only
#    through the plugin interface or an explicit UDA_DIAGNOSTICS context.
#
# Usage: cmake -DNM=<nm> -DDEPS_TOOL=<otool|readelf> -DLIBS="<lib1>;..." -DSTATELESS_LIBS="<lib1>;..."
#              -P checkPluginSymbols.cmake

set( FORBIDDEN_DEFINED
  udaLog udaSetLogLevel udaGetLogLevel udaSetLogFile udaCloseLogging
  addIdamError concatUdaError closeUdaError initUdaErrorStack udaNumErrors udaGlobalDiagnostics
  udaServerPlugin udaGetData udaProvenancePlugin getServerEnvironment initPluginList freePluginList
  udaCreatePluginInterface udaFreePluginInterface
)

set( FORBIDDEN_REFERENCED
  udaLog udaSetLogLevel udaGetLogLevel udaSetLogFile udaCloseLogging
  addIdamError concatUdaError closeUdaError initUdaErrorStack udaNumErrors udaGlobalDiagnostics printIdamErrorStack
)

if( APPLE )
  set( NM_DEFINED_ARGS -gU )
  set( NM_UNDEFINED_ARGS -u )
  set( DEPS_ARGS -L )
  set( PREFIX "_" )
else()
  set( NM_DEFINED_ARGS -D --defined-only )
  set( NM_UNDEFINED_ARGS -D --undefined-only )
  set( DEPS_ARGS -d )
  set( PREFIX "" )
endif()

set( FAILED FALSE )

foreach( LIB ${LIBS} )
  execute_process( COMMAND ${NM} ${NM_DEFINED_ARGS} ${LIB} OUTPUT_VARIABLE SYMBOLS RESULT_VARIABLE RC )
  if( NOT RC EQUAL 0 )
    message( SEND_ERROR "Failed to read symbols from ${LIB}" )
    set( FAILED TRUE )
    continue()
  endif()
  foreach( SYM ${FORBIDDEN_DEFINED} )
    if( SYMBOLS MATCHES "[ \t]${PREFIX}${SYM}\n" )
      message( SEND_ERROR "${LIB} defines ${SYM}: it must not embed the client/server logger or error-stack objects" )
      set( FAILED TRUE )
    endif()
  endforeach()
endforeach()

foreach( LIB ${STATELESS_LIBS} )
  execute_process( COMMAND ${NM} ${NM_UNDEFINED_ARGS} ${LIB} OUTPUT_VARIABLE SYMBOLS RESULT_VARIABLE RC )
  if( NOT RC EQUAL 0 )
    message( SEND_ERROR "Failed to read symbols from ${LIB}" )
    set( FAILED TRUE )
    continue()
  endif()
  # nm -u on macOS prints bare names; GNU nm prints "U name"
  set( SYMBOLS "\n${SYMBOLS}" )
  foreach( SYM ${FORBIDDEN_REFERENCED} )
    if( SYMBOLS MATCHES "[ \t\n]${PREFIX}${SYM}(@[^\n]*)?\n" )
      message( SEND_ERROR "${LIB} references ${SYM}: report through the plugin interface or a UDA_DIAGNOSTICS context" )
      set( FAILED TRUE )
    endif()
  endforeach()

  execute_process( COMMAND ${DEPS_TOOL} ${DEPS_ARGS} ${LIB} OUTPUT_VARIABLE DEPS RESULT_VARIABLE RC )
  if( NOT RC EQUAL 0 )
    message( SEND_ERROR "Failed to read the dependencies of ${LIB}" )
    set( FAILED TRUE )
  elseif( DEPS MATCHES "libuda_client" )
    message( SEND_ERROR "${LIB} depends on libuda_client: only plugins built with LINK_CLIENT may" )
    set( FAILED TRUE )
  endif()
endforeach()

if( FAILED )
  message( FATAL_ERROR "Plugin link guard failed" )
endif()
message( STATUS "Plugin link guard passed for: ${LIBS}" )
