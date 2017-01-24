#!/bin/bash

export LD_LIBRARY_PATH=$HOME/iter/idam/bin/plugins:$LD_LIBRARY_PATH

gdb $HOME/iter/idam/test
