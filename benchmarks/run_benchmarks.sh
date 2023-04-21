#!/bin/bash

cat /dev/null > output/runs.csv

# Run the small test samples first
 for m in LUT_LDEXPF_INTERPOLATE LUT_MULTI_INTERPOLATE POLYNOMIAL
 do
   NR_TASKLETS=12 METHOD=$m make blackscholes_float
   wait
   ./bin/blackscholes_float_host blackscholes/in.txt blackscholes/out.txt
   wait
   NR_TASKLETS=12 METHOD=$m make softmax_float
   wait
   ./bin/softmax_float_host softmax/in.txt softmax/out.txt
   wait
   NR_TASKLETS=12 METHOD=$m make sigmoid_float
   wait
   ./bin/sigmoid_float_host sigmoid/in.txt sigmoid/out.txt
   wait
 done
 NR_TASKLETS=12 METHOD=LUT_FIXED_INTERPOLATE make blackscholes_fixed
 wait
 ./bin/blackscholes_fixed_host blackscholes/in.txt blackscholes/out.txt

wait
make softmax_cpu
./bin/softmax_cpu softmax/in.txt softmax/out.txt
wait
NTHREADS=32 make softmax_multi
./bin/softmax_multi softmax/in.txt softmax/out.txt
wait
make sigmoid_cpu
./bin/sigmoid_cpu sigmoid/in.txt sigmoid/out.txt
wait
NTHREADS=32 make sigmoid_multi
./bin/sigmoid_multi sigmoid/in.txt sigmoid/out.txt
wait

cd blackscholes/parsec
make 
./blackscholes 1 ../in.txt ../out.txt
wait
make version=openmp 
./blackscholes 32 ../in.txt ../out.txt
wait
cd ../..

# Run the large tests 10 times
for ((i=1; i<=10; i++))
do
  for m in LUT_LDEXPF_INTERPOLATE LUT_MULTI_INTERPOLATE POLYNOMIAL
  do
   NR_TASKLETS=12 METHOD=$m make blackscholes_float
   wait
   ./bin/blackscholes_float_host blackscholes/in_30m.txt blackscholes/out.txt
   wait
    NR_TASKLETS=12 METHOD=$m PRECISION=10 make softmax_float
    wait
    ./bin/softmax_float_host softmax/in_90m.txt softmax/out.txt
    wait
    NR_TASKLETS=12 METHOD=$m PRECISION=10 make sigmoid_float
    wait
    ./bin/sigmoid_float_host sigmoid/in_90m.txt sigmoid/out.txt
    wait
  done
 NR_TASKLETS=12 METHOD=LUT_FIXED_INTERPOLATE make blackscholes_fixed
 wait
 ./bin/blackscholes_fixed_host blackscholes/in_30m.txt blackscholes/out.txt
done

for ((i=1; i<=10; i++))
do
  for m in CORDIC_LUT 
  do
    NR_TASKLETS=12 METHOD=$m PRECISION=22 make softmax_float
    wait
    ./bin/softmax_float_host softmax/in_90m.txt softmax/out.txt
    wait
    NR_TASKLETS=12 METHOD=$m PRECISION=22 make sigmoid_float
    wait
    ./bin/sigmoid_float_host sigmoid/in_90m.txt sigmoid/out.txt
    wait
  done
done

wait
make softmax_cpu
./bin/softmax_cpu softmax/in_90m.txt softmax/out.txt
wait
NTHREADS=32 make softmax_multi
./bin/softmax_multi softmax/in_90m.txt softmax/out.txt
wait
make sigmoid_cpu
./bin/sigmoid_cpu sigmoid/in_90m.txt sigmoid/out.txt
wait
NTHREADS=32 make sigmoid_multi
./bin/sigmoid_multi sigmoid/in_90m.txt sigmoid/out.txt
wait

cd blackscholes/parsec
make 
./blackscholes 1 ../in_30m.txt ../out.txt
wait
make version=openmp 
./blackscholes 32 ../in_30m.txt ../out.txt
wait
cd ../..
