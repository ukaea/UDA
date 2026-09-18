# External header-only dependencies: nlohmann/json and jwt-cpp.
#
# Both are resolved the way UDA resolves its other eighteen dependencies — with
# find_package — and fall back to a pinned FetchContent download only when the system does
# not provide them. Neither is vendored into the repository.
#
# Why the fallback exists at all: nlohmann/json is packaged essentially everywhere
# (nlohmann-json3-dev, json-devel, nlohmann-json, vcpkg), but jwt-cpp is not in EPEL, and
# the manylinux wheel image builds on EPEL 8. A hard find_package requirement would break
# that image until someone packages it.
#
# Controls:
#   UDA_FETCH_DEPENDENCIES=OFF   never download; require system packages. Use this for
#                                air-gapped builds and for distribution packaging, where
#                                downloading during configure is not acceptable.
#   FETCHCONTENT_SOURCE_DIR_<NAME>  point a fetch at an already-present source tree, which
#                                is the standard way to satisfy this offline.

include( FetchContent )

option( UDA_FETCH_DEPENDENCIES
        "Download pinned copies of header-only dependencies that the system does not provide" ON )

# Versions are pinned rather than floating: a JWT verifier changing underneath the build
# without anyone choosing to is not a thing we want.
set( UDA_NLOHMANN_JSON_VERSION "3.11.3" CACHE STRING "nlohmann/json version to fetch if not found" )
# v0.7.2 or later: earlier tags lack jwt::helper::create_public_key_from_rsa_components
# and create_public_key_from_ec_components, which the JWKS key-loading path uses.
set( UDA_JWT_CPP_VERSION       "v0.7.2" CACHE STRING "jwt-cpp tag to fetch if not found" )

# ---------------------------------------------------------------------------------------
# nlohmann/json — needed by the client as well as the server, so it is always required.

function( uda_require_nlohmann_json )
  if( TARGET nlohmann_json::nlohmann_json )
    return()
  endif()

  find_package( nlohmann_json 3.9 QUIET )
  if( nlohmann_json_FOUND )
    message( STATUS "nlohmann/json: using system package ${nlohmann_json_VERSION}" )
    return()
  endif()

  if( NOT UDA_FETCH_DEPENDENCIES )
    message( FATAL_ERROR
      "nlohmann/json was not found and UDA_FETCH_DEPENDENCIES is OFF.\n"
      "Install it (nlohmann-json3-dev / json-devel / nlohmann-json), or set\n"
      "FETCHCONTENT_SOURCE_DIR_NLOHMANN_JSON to a local copy." )
  endif()

  message( STATUS "nlohmann/json: not found, fetching v${UDA_NLOHMANN_JSON_VERSION}" )
  FetchContent_Declare( nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v${UDA_NLOHMANN_JSON_VERSION}/json.tar.xz
  )
  FetchContent_MakeAvailable( nlohmann_json )
endfunction()

# ---------------------------------------------------------------------------------------
# jwt-cpp — OIDC only, so only required when that is enabled.
#
# JWT_DISABLE_PICOJSON is set globally by the top-level CMakeLists: UDA uses the nlohmann
# traits, and the setting changes jwt-cpp's own types, so every translation unit that sees
# jwt.h has to agree on it.

function( uda_require_jwt_cpp )
  if( TARGET jwt-cpp::jwt-cpp )
    return()
  endif()

  # 0.7.2 is the floor: earlier releases lack the JWKS key-construction helpers.
  find_package( jwt-cpp 0.7.2 QUIET )
  if( jwt-cpp_FOUND )
    message( STATUS "jwt-cpp: using system package ${jwt-cpp_VERSION}" )
    return()
  endif()

  if( NOT UDA_FETCH_DEPENDENCIES )
    message( FATAL_ERROR
      "jwt-cpp was not found and UDA_FETCH_DEPENDENCIES is OFF.\n"
      "Install it, set FETCHCONTENT_SOURCE_DIR_JWT-CPP to a local copy, or build without\n"
      "-DENABLE_OIDC_AUTH." )
  endif()

  message( STATUS "jwt-cpp: not found, fetching ${UDA_JWT_CPP_VERSION}" )
  set( JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )
  set( JWT_DISABLE_PICOJSON ON CACHE BOOL "" FORCE )
  set( JWT_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE )
  FetchContent_Declare( jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG        ${UDA_JWT_CPP_VERSION}
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable( jwt-cpp )
endfunction()
