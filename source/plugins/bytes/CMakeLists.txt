find_package( Boost COMPONENTS filesystem REQUIRED )
find_package( OpenSSL REQUIRED )
find_package( fmt REQUIRED )

include( udaPlugins )

uda_plugin(
  NAME BYTES
  VERSION 1.0.0
  ENTRY_FUNC bytesPlugin
  DESCRIPTION "data reader to access files as a block of bytes without interpretation"
  EXAMPLE "BYTES::read()"
  LIBNAME bytes_plugin
  SOURCES bytesPlugin.cpp readBytesNonOptimally.cpp md5Sum.cpp
  EXTRA_LINK_LIBS Boost::filesystem OpenSSL::SSL OpenSSL::Crypto fmt::fmt
)