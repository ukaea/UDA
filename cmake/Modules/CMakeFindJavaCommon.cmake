# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

# Macro to convert detected path to MinGW compliant path
macro(MINGW_CONVERT_PATH path)
  # Windows MSYS/MinGW: Convert path to 1) suppress space 2) adapt to Linux
  if (MINGW AND NOT "${${path}}" STREQUAL "" AND IS_DIRECTORY "${${path}}")
    message(STATUS "Before: ${path} = ${${path}}")
    execute_process(COMMAND cygpath.exe -ms "${${path}}" OUTPUT_VARIABLE ${path})
    execute_process(COMMAND cygpath.exe -u "${${path}}" OUTPUT_VARIABLE ${path})
    string(STRIP ${${path}} ${path})
    message(STATUS "After: ${path} = ${${path}}")
  endif()
endmacro()


# Do not include this module directly from code outside CMake!
set(_JAVA_HOME "")
if(JAVA_HOME AND IS_DIRECTORY "${JAVA_HOME}")
  set(_JAVA_HOME "${JAVA_HOME}")
  set(_JAVA_HOME_EXPLICIT 1)
else()
  set(_ENV_JAVA_HOME "")
  if(DEFINED ENV{JAVA_HOME})
    file(TO_CMAKE_PATH "$ENV{JAVA_HOME}" _ENV_JAVA_HOME)
  endif()
  if(_ENV_JAVA_HOME AND IS_DIRECTORY "${_ENV_JAVA_HOME}")
    set(_JAVA_HOME "${_ENV_JAVA_HOME}")
    set(_JAVA_HOME_EXPLICIT 1)
  else()
    set(_CMD_JAVA_HOME "")
    if(APPLE AND EXISTS /usr/libexec/java_home)
      execute_process(COMMAND /usr/libexec/java_home
        OUTPUT_VARIABLE _CMD_JAVA_HOME OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
    if(_CMD_JAVA_HOME AND IS_DIRECTORY "${_CMD_JAVA_HOME}")
      set(_JAVA_HOME "${_CMD_JAVA_HOME}")
      set(_JAVA_HOME_EXPLICIT 0)
    endif()
    unset(_CMD_JAVA_HOME)
  endif()
  unset(_ENV_JAVA_HOME)
endif()

MINGW_CONVERT_PATH(_JAVA_HOME)