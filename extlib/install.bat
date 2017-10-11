mkdir lib
mkdir include

copy build\portablexdr-4.9.1\Debug\xdr.lib lib
copy build\portablexdr-4.9.1\Debug\xdr.pdb lib

mkdir include\rpc
copy portablexdr-4.9.1\rpc include\rpc\