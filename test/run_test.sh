#!/bin/bash

cd ..

make -C build install

cd $PWD/etc
rm *.log
./rc.uda stop
./rc.uda start


cd $PWD/../test
./test
#export LD_LIBRARY_PATH=$HOME/iter/uda/bin/plugins:$LD_LIBRARY_PATH
#./test
#gdb $HOME/iter/uda/test
