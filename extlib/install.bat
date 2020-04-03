mkdir lib
mkdir include

del /Q lib\*
copy build\portablexdr-4.9.1\Release\xdr.lib lib

rmdir /S /Q include\rpc
mkdir include\rpc
mkdir include\rpc\rpc

copy portablexdr-4.9.1\rpc include\rpc\rpc
copy build\config.h include\rpc