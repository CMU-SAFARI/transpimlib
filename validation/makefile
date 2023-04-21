HOST_FLAGS := $ --std=c99 `dpu-pkg-config --cflags --libs dpu` -lm

.PHONY: ldexpf

ldexpf:
	dpu-upmem-dpurte-clang dpu/ldexpf_validation.c -o ../bin/dpu/ldexpf_validation
	clang host/ldexpf_validation_host.c -o ../bin/hostldexpf_validation_host ${HOST_FLAGS}