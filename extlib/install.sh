mkdir lib
mkdir include

cp build/portablexdr-4.9.1/Debug/xdr.lib lib
cp build/portablexdr-4.9.1/Debug/xdr.pdb lib

mkdir include/rpc
cp -r portablexdr-4.9.1/rpc include/rpc/
cp -r build/config.h include/rpc/
