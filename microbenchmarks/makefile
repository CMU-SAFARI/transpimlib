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

# What operation should be benchmarked?
# Options:
# - SIN
# - COS
# - TAN
# - EXP
# - LOG
# - SQRT
# - TANH
OPERATION ?= SIN

# How many tests should be undertaken to in total?
ARRAY_SIZE ?= 16

# Should the LUT be saved in WRAM?
# -> Does not make sense for standard CORDIC, as the tables are always stored in WRAM
STORE_IN_WRAM ?= 0

HOST_FLAGS := $ --std=c99 `dpu-pkg-config --cflags --libs dpu` -lm

OPTIONS := $ -D PRECISION=${PRECISION} -D ${METHOD} -D ${EXTENSION} -D ARRAY_SIZE=${ARRAY_SIZE} -D STORE_IN_WRAM=${STORE_IN_WRAM}

.PHONY: performance extension setup all

all:
	make method_performance
	make extension_performance
	make method_setup

method_performance: dpu/transcendental_performance.c host/transcendental_performance_host.c
	dpu-upmem-dpurte-clang dpu/transcendental_performance.c -o bin/dpu/transcendental_performance ${OPTIONS} -D ${OPERATION}
	clang host/transcendental_performance_host.c -o bin/host/transcendental_performance_host ${HOST_FLAGS} ${OPTIONS} -D ${OPERATION}

extension_performance: dpu/range_extension_performance.c host/range_extension_performance_host.c
	dpu-upmem-dpurte-clang dpu/range_extension_performance.c -o bin/dpu/range_extension_performance ${OPTIONS} -D ${OPERATION}
	clang host/range_extension_performance_host.c -o bin/host/range_extension_performance_host ${HOST_FLAGS} ${OPTIONS} -D ${OPERATION}

method_setup: dpu/setup_performance.c host/setup_performance_host.c
	dpu-upmem-dpurte-clang dpu/setup_performance.c -o bin/dpu/setup_performance ${OPTIONS} -D ${OPERATION}
	clang host/setup_performance_host.c -o bin/host/setup_performance_host ${HOST_FLAGS} ${OPTIONS} -D ${OPERATION}

method_performance_fixed: dpu/transcendental_performance_fixed.c host/transcendental_performance_fixed_host.c
	dpu-upmem-dpurte-clang dpu/transcendental_performance_fixed.c -o bin/dpu/transcendental_performance_fixed ${OPTIONS} -D ${OPERATION}
	clang host/transcendental_performance_fixed_host.c -o bin/host/transcendental_performance_fixed_host ${HOST_FLAGS} ${OPTIONS} -D ${OPERATION}
