mkdir lib 2>/dev/null
mkdir include 2>/dev/null

# Check if the platform is Windows
if [ "$OS" = "Windows_NT" ]
then
	cp portablexdr-4.9.1/.libs/libportablexdr.a lib
	cp portablexdr-4.9.1/.libs/libportablexdr.la lib
	cp portablexdr-4.9.1/.libs/libportablexdr.dll.a lib
	cp portablexdr-4.9.1/.libs/libportablexdr-0.dll lib
else
	cp portablexdr-4.9.1/.libs/libportablexdr.a lib
	cp portablexdr-4.9.1/.libs/libportablexdr.la lib
	cp portablexdr-4.9.1/.libs/libportablexdr.so lib
fi

mkdir include/rpc 2>/dev/null

# Check if the platform is Windows
if [ "$OS" = "Windows_NT" ]
then
	cp -r portablexdr-4.9.1/rpc include/rpc/
	cp -r portablexdr-4.9.1/config.h include/rpc/rpc
else
	cp -r portablexdr-4.9.1/rpc include/rpc/
	cp -r portablexdr-4.9.1/config.h include/rpc/rpc
fi
