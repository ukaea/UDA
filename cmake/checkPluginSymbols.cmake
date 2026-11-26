# Link guard: plugin support and plugin libraries must not embed the server, or their own copies of the server's
# logger and error-stack state (each copy is a separate instance on macOS and Windows, so errors and diagnostics
# recorded through it never reach the server).
#
# Usage: cmake -DNM=<nm> -DLIBS="<lib1>;<lib2>;..." -P checkPluginSymbols.cmake

set( FORBIDDEN
  udaLog udaSetLogLevel udaGetLogLevel udaSetLogFile udaCloseLogging
  addIdamError concatUdaError closeUdaError initUdaErrorStack udaNumErrors udaMoveErrorsSince
  udaServerPlugin udaGetData udaProvenancePlugin getServerEnvironment initPluginList freePluginList
  udaCreatePluginInterface udaFreePluginInterface
)

if( APPLE )
  set( NM_ARGS -gU )
  set( PREFIX "_" )
else()
  set( NM_ARGS -D --defined-only )
  set( PREFIX "" )
endif()

set( FAILED FALSE )
foreach( LIB ${LIBS} )
  execute_process( COMMAND ${NM} ${NM_ARGS} ${LIB} OUTPUT_VARIABLE SYMBOLS RESULT_VARIABLE RC )
  if( NOT RC EQUAL 0 )
    message( SEND_ERROR "Failed to read symbols from ${LIB}" )
    set( FAILED TRUE )
    continue()
  endif()
  foreach( SYM ${FORBIDDEN} )
    if( SYMBOLS MATCHES "[ \t]${PREFIX}${SYM}\n" )
      message( SEND_ERROR "${LIB} defines ${SYM}: it must not embed the server, logger or error-stack objects" )
      set( FAILED TRUE )
    endif()
  endforeach()
endforeach()

if( FAILED )
  message( FATAL_ERROR "Plugin link guard failed" )
endif()
message( STATUS "Plugin link guard passed for: ${LIBS}" )
