find_package( PkgConfig )

pkg_search_module( HDC hdc REQUIRED )

add_library( hdc::hdc SHARED IMPORTED )

set_target_properties( hdc::hdc PROPERTIES
  INTERFACE_COMPILE_OPTIONS "${HDC_CFLAGS_OTHER}"
  INTERFACE_INCLUDE_DIRECTORIES "${HDC_INCLUDE_DIRS}"
  IMPORTED_LOCATION "${HDC_LIBRARY_DIRS}/libhdc.dylib"
  IMPORTED_SONAME_RELEASE "@rpath/libhdc.dylib"
)
