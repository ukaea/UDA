CC=gcc
ROOT_DIR=../../../../
BUILD_DIR=$(ROOT_DIR)/build
SERVER_DIR=$(BUILD_DIR)/server
PLUGIN_DIR=$(SERVER_DIR)/plugins

#$(SRCS:.c=.o)

DEFINES=-DPLUGINTEST -DTESTCODE -DPLUGIN -DXDRSTDIO -DGENERALSTRUCTS -DFATCLIENT

CFLAGS=-fPIC -g -I../../../include -I/jet/share/include
CFLAGS+=-Werror=implicit-function-declaration
##CFLAGS+=-Werror=strict-prototypes
CFLAGS+=-fdiagnostics-show-option
LDFLAGS=-L. -L/jet/share/lib -L/usr/lib

INCLUDES = -I./

TEST=_test

.PHONY: all
# tidy clean strip install

all:  $(TEST)
test: $(TEST)

.c.o: 
	$(CC) -c -Wall $(CFLAGS)  $< -o $@   

$(TEST): test0.o
	$(CC)   test0.o -L$(ROOT_DIR)/build/lib -lidam -o $(TEST)  $(LDFLAGS) --no-as-needed -ldl 

test0.0: test0.c
	$(CC) $(CFLAGS) -c test0.c

tidy:
	rm -f Error.err Debug.dbg
	rm -rf core $(OBJS) *~ test0.o

clean: tidy
	rm -rf rm $(TEST)

strip:
	strip --strip-all $(TARGET_LIB)

depend: $(SRCS)	
	makedepend $(INCLUDES) $^





