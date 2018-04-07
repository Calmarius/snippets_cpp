#!/bin/bash

if [ $# -lt 1 ]
then
    echo "Need file as an argument."
    exit 1
fi

OPTIONS="-Wall -Wextra -Werror -Wfatal-errors -pedantic -std=c++11 -fno-rtti -fno-exceptions"

g++ -I`pwd` $2 -g $OPTIONS -DUNIT_TEST $1 -o testprogram

