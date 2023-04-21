#!/bin/bash

cat /dev/null > output/setup.csv

for m in CORDIC
do
  for o in SIN #COS TAN EXP LOG SQRT
  do
    for p in 6 8 10 12 14 16 18 20 22 24 26 28
    do
      METHOD=$m PRECISION=$p OPERATION=$o make method_setup
      wait
      ./bin/host/setup_performance_host
      wait
    done
  done
done

for m in LUT_LDEXPF LUT_LDEXPF_INTERPOLATE LUT_MULTI LUT_MULTI_INTERPOLATE
do
  for o in SIN #COS TAN EXP LOG SQRT
  do
    for s in 0
    do
      for p in 6 8 10 12 14 16
      do
        METHOD=$m PRECISION=$p OPERATION=$o STORE_IN_WRAM=$s make method_setup
        wait
        ./bin/host/setup_performance_host
        wait
      done
    done
    for s in 1
      do
        for p in 6 8 10 12
        do
          METHOD=$m PRECISION=$p OPERATION=$o STORE_IN_WRAM=$s make method_setup
          wait
          ./bin/host/setup_performance_host
          wait
        done
      done
  done
done

for m in CORDIC_LUT
do
  for o in SIN #COS TAN EXP
  do
    for s in 1 0
    do
      for p in 10 12 14 16 18 20 22 24 26 28
      do
        METHOD=$m PRECISION=$p OPERATION=$o STORE_IN_WRAM=$s make method_setup
        wait
        ./bin/host/setup_performance_host
        wait
      done
    done
  done
done

for m in LUT_DIRECT LUT_DIRECT_LDEXPF
do
  for o in SIN #TANH
  do
    for s in 0
    do
      for p in 10 12 14 16 18 20 22
      do
        METHOD=$m PRECISION=$p OPERATION=$o STORE_IN_WRAM=$s make method_setup
        wait
        ./bin/host/setup_performance_host
        wait
      done
    done
    for s in 1
    do
      for p in 8 10 12
      do
        METHOD=$m PRECISION=$p OPERATION=$o STORE_IN_WRAM=$s make method_setup
        wait
        ./bin/host/setup_performance_host
        wait
      done
    done
  done
done
