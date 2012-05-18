#!/bin/bash

export LD_LIBRARY_PATH=.:${LD_LIBRARY_PATH}
./test $1 $2

