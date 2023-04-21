# How large should the Lookup table be / how many iterations of CORDIC?
# Reasonable Parameters
# LUT: 6 - 20
# CORDIC 8 - 28
PRECISION ?= 10

# What method should be used to calculate transcendental functions?
# Options:
# - CORDIC CORDIC_LUT
# - LUT_LDEXPF_INTERPOLATE
# - LUT_LDEXPF
# - LUT_MULTI_INTERPOLATE
# - LUT_MULTI
# - LUT_DIRECT
# - LUT_DIRECT_LDEXPF
METHOD ?= LUT_LDEXPF_INTERPOLATE

# Should the range extension functions also be included in the benchmark?
# Options:
# - WRAP
# - NOWRAP
EXTENSION ?= WRAP

# Should the LUT be saved in WRAM?
# -> Does not make sense for standard CORDIC, as the tables are always stored in WRAM
STORE_IN_WRAM ?= 1

# How many tasklets should be used at maximum (If there isn't enough work, less might be used)
# Reasonable Range:
# 1 - 24
NR_TASKLETS ?= 16 # How many tasklets should be used?

NTHREADS ?= 16 # How many CPU threads should be used?

HOST_FLAGS := $ --std=c99 `dpu-pkg-config --cflags --libs dpu` -lm

OPTIONS := $ -D PRECISION=${PRECISION} -D ${METHOD} -D ${EXTENSION}  -D STORE_IN_WRAM=${STORE_IN_WRAM}

.PHONY: all blackscholes blackscholes_float blackscholes_poly blackscholes_fixed softmax softmax_float softmax_cpu softmax_multi sigmoid sigmoid_float sigmoid_cpu sigmoid_multi

all:
	make blackscholes
	make softmax
	make sigmoid

blackscholes:
	make blackscholes_float
	make blackscholes_fixed

blackscholes_float: blackscholes/blackscholes_float.c blackscholes/blackscholes_float_host.c
	dpu-upmem-dpurte-clang blackscholes/blackscholes_float.c -o bin/blackscholes_float -D NR_TASKLETS=${NR_TASKLETS} ${OPTIONS}
	clang blackscholes/blackscholes_float_host.c -o bin/blackscholes_float_host ${HOST_FLAGS} -D NR_TASKLETS=${NR_TASKLETS} ${OPTIONS}

blackscholes_fixed: blackscholes/blackscholes_fixed.c blackscholes/blackscholes_fixed_host.c
	dpu-upmem-dpurte-clang blackscholes/blackscholes_fixed.c -o bin/blackscholes_fixed -D NR_TASKLETS=${NR_TASKLETS} ${OPTIONS} -D FIXED_FRACTION_BITS=20
	clang blackscholes/blackscholes_fixed_host.c -o bin/blackscholes_fixed_host ${HOST_FLAGS} -D NR_TASKLETS=${NR_TASKLETS} ${OPTIONS} -D FIXED_FRACTION_BITS=20

softmax:
	make softmax_float
	make softmax_cpu
	make softmax_multi

softmax_float: softmax/softmax_float.c softmax/softmax_float_host.c
	dpu-upmem-dpurte-clang softmax/softmax_float.c -o bin/softmax_float -D NR_TASKLETS=${NR_TASKLETS} ${OPTIONS}
	clang softmax/softmax_float_host.c -o bin/softmax_float_host ${HOST_FLAGS} -D NR_TASKLETS=${NR_TASKLETS} ${OPTIONS}

softmax_cpu: softmax/softmax_cpu.c
	clang softmax/softmax_cpu.c -o bin/softmax_cpu -lm

softmax_multi: softmax/softmax_cpu_multicore.c
	clang softmax/softmax_cpu_multicore.c -o bin/softmax_multi -lm -pthread -D NTHREADS=${NTHREADS}

sigmoid:
	make sigmoid_float
	make sigmoid_cpu
	make sigmoid_multi

sigmoid_float: sigmoid/sigmoid_float.c sigmoid/sigmoid_float_host.c
	dpu-upmem-dpurte-clang sigmoid/sigmoid_float.c -o bin/sigmoid_float -D NR_TASKLETS=${NR_TASKLETS} ${OPTIONS}
	clang sigmoid/sigmoid_float_host.c -o bin/sigmoid_float_host ${HOST_FLAGS} -D NR_TASKLETS=${NR_TASKLETS} ${OPTIONS}

sigmoid_cpu: sigmoid/sigmoid_cpu.c
	clang sigmoid/sigmoid_cpu.c -o bin/sigmoid_cpu -lm

sigmoid_multi: sigmoid/sigmoid_cpu_multicore.c
	clang sigmoid/sigmoid_cpu_multicore.c -o bin/sigmoid_multi -lm -pthread -D NTHREADS=${NTHREADS}

clean:
	rm -r ./bin/*
