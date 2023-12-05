########################################################################################################################
# Dependencies
find_package( LibXml2 QUIET )
find_package( Boost COMPONENTS filesystem REQUIRED )

if( NOT LIBXML2_FOUND )
  message( WARNING "Libxml2 not found - skipping template plugin" )
  return()
endif()

include( plugins )

uda_plugin(
  NAME BYTES
  ENTRY_FUNC bytesPlugin
  DESCRIPTION "data reader to access files as a block of bytes without interpretation"
  EXAMPLE "BYTES::read()"
  LIBNAME bytes_plugin
  SOURCES bytesPlugin.cpp readBytesNonOptimally.cpp md5Sum.cpp
  EXTRA_LINK_LIBS
    LibXml2::LibXml2
    Boost::filesystem
)
