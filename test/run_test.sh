#!/bin/bash

export LD_LIBRARY_PATH=$HOME/iter/uda/bin/plugins:$LD_LIBRARY_PATH
#./test
gdb $HOME/iter/uda/test
