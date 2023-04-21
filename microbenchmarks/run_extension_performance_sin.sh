#!/bin/bash

cat /dev/null > output/extension.csv

for t in SIN #EXP LOG SQRT
do
    OPERATION=$t make extension_performance
    wait
    ./bin/host/range_extension_performance_host
    wait
done
