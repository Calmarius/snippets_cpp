#!/bin/bash

CPPFILES=`find . -name "*.cpp"`

set -e

for F in $CPPFILES
do
    echo
    echo "*** Running test in $F ***"
    echo
    ./buildfile.sh $F
    valgrind --leak-check=full --show-reachable=yes --error-exitcode=1 ./testprogram
done

echo "All done!"
