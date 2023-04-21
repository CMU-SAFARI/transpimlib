# TransPimLib: A Library for Efficient Transcendental Functions on Processing-in-Memory Systems
[Processing-in-memory (PIM)](https://arxiv.org/pdf/2012.03112.pdf) promises to alleviate the data movement bottleneck in modern computing systems. However, current real-world PIM systems have the inherent disadvantage that their hardware is more constrained than in conventional processors (CPU, GPU), due to the difficulty and cost of building processing elements near or inside the memory. As a result, general-purpose PIM architectures support fairly limited instruction sets and struggle to execute complex operations such as transcendental functions and other hard-to-calculate operations (e.g., square root). These operations are particularly important for some modern workloads, e.g., activation functions in machine learning applications.

To provide support for transcendental (and other hard-to-calculate) functions in general-purpose PIM systems, TransPimLib is a library that provides CORDIC-based and LUT-based methods for trigonometric functions, hyperbolic functions, exponentiation, logarithm, square root, etc. The first implementation of TransPimLib is for the [UPMEM](https://www.upmem.com/) PIM architecture.

## Citation
Please cite the following papers if you find this repository useful.

ISPASS2023 paper version:

Maurus Item, Juan Gómez-Luna, Yuxin Guo, Geraldo F. Oliveira, Mohammad Sadrosadati, and Onur Mutlu, "[TransPimLib: Efficient Transcendental Functions for Processing-in-Memory Systems](https://ispass.org/ispass2023/program.php)". 2023 IEEE International Symposium on Performance Analysis of Systems and Software (ISPASS), 2023.

Bibtex entry for citation:
```
@inproceedings{item2023transpimlibispass,
  title={{TransPimLib: Efficient Transcendental Functions for Processing-in-Memory Systems}}, 
  author={Maurus Item and Juan Gómez-Luna and Yuxin Guo and Geraldo F. Oliveira and Mohammad Sadrosadati and Onur Mutlu},
  year={2023},
  booktitle = {ISPASS}
}
```

arXiv paper version:

Maurus Item, Juan Gómez-Luna, Yuxin Guo, Geraldo F. Oliveira, Mohammad Sadrosadati, and Onur Mutlu, "[TransPimLib: A Library for Efficient Transcendental Functions on Processing-in-Memory Systems](https://arxiv.org/pdf/2304.01951.pdf)". arXiv:2304.01951 [cs.MS], 2023.

Bibtex entries for citation:
```
@misc{item2023transpimlib,
  title={{TransPimLib: A Library for Efficient Transcendental Functions on Processing-in-Memory Systems}}, 
  author={Maurus Item and Juan Gómez-Luna and Yuxin Guo and Geraldo F. Oliveira and Mohammad Sadrosadati and Onur Mutlu},
  year={2023},
  howpublished={arXiv:2304.01951 [cs.MS]}
}
```

## Repository Structure
We point out next the repository structure and some important folders and files. 
The repository also includes `run_*.sh` scripts to run the experiments in the paper.

```
.
+-- LICENSE
+-- README.md
+-- run_strong_full.py
+-- run_strong_rank.py
+-- run_weak.py
+-- benchmarks/
|   +-- blackscholes/
|	|	+-- parsec/
|   +-- sigmoid/
|   +-- softmax/
|   +-- makefile
|	+-- polynomial.c
|	+-- run_benchmarks.sh
+-- dpu/
+-- host/
+-- microbenchmarks/
|   +-- dpu/
|   +-- host/
|   +-- makefile
|	+-- run_extension_performance_sin.sh
|	+-- run_extension_performance.sh
|	+-- run_method_performance_sin.sh
|	+-- run_method_performance.sh
|	+-- run_method_setup_sin.sh
|	+-- run_method_setup.sh
+-- validation/
|   +-- dpu/
|   +-- host/
|   +-- makefile
```

## Usage
Here is a minimal example on how you can use TransPimLib in your code. 
On the host side
```c
#include "lut_ldexpf_host.c" // <<--- Add this

#define DPU_BINARY "dpu"

int main(void) {
    
    // Get a DPU and load our kernel on it
    DPU_ASSERT(dpu_alloc(1, NULL, &set));
    DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));
    
    // Get some number and move it over to the DPU
    int number = 0.5;
    DPU_ASSERT(dpu_broadcast_to(set, "number", 0, &number, sizeof(int), DPU_XFER_DEFAULT));
    
    // Fill the tables on the DPU
    broadcast_tables(set); // <<--- Add this
    
    // Run the DPU Kernel
    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));
    
    // Retrieve the result
    DPU_FOREACH(set, dpu) {
        DPU_ASSERT(dpu_copy_from(dpu, "number", 0, &number, sizeof(int)));
        printf("Result of sin() on dpu: %f", number);
       
    }
    
    return 0;
}
```
and on the host side
```c
#include "lut_ldexpf_interpolate.c" // <<--- Add this

__host uint32_t number;

int main(){
    number = sinf(number); // <<--- Now you can use transcendntal functions!
    return 0;
}
```

## TransPimLib's Methods
TransPimLib contains different implementation methods with different memory requirements, host setup time, accuracy, and performance. 

### Recomended Methods
- `cordic.c` Has the smallest table sizes
- `cordic_lut.c` Has medium table sizes (size independent of precision, but bigger tables = less cycles)
- `lut_ldexpf_interpolate.c` Has large table sizes but runs significantly faster
- `lut_direct_ldexpf.c` same as `lut_ldexpf_interpolate.c` but with non-linear spacing that might be beneficial for machine learning activation functions

### Prototype Methods
- `lut_multi_interpolate.c`same as `lut_ldexpf_interpolate.c` but slower
- `lut_direct.c` same as `lut_direct_ldexpf.c` but without any table values for small numbers

### Non-interpolated Methods
- `lut_ldexpf.c` Can be run with `lut_ldexpf_interpolate.c` (same on host side) and is faster but less accurate
- `lut_multi.c` Can be run with `lut_multi_interpolate.c` (same on host side) and is faster but less accurate

Not all methods have all functions, because some functions rely on some parts of the implementation. 
Functions in brackets () have a **limited range**

| Method                     | sinf | cosf | tanf | sinhf | coshf | tanhf | expf | logf | sqrtf | gelu |
|----------------------------|------|------|------|-------|-------|-------|------|------|-------|------|
| `cordic.c`                 | x    | x    | x    | (x)   | (x)   | (x)   | x    | x    | x     |      |
| `cordic_lut.c`             | x    | x    | x    | (x)   | (x)   | (x)   | x    |      |       |      |
| `lut_ldexpf_interpolate.c` | x    | x    | x    |       |       |       | x    | x    | x     |      |
| `lut_direct_ldexpf.c`      |      |      |      |       |       | x     |      |      |       | x    |
| `lut_multi_interpolate.c`  | x    | x    | x    |       |       |       | x    | x    | x     |      |
| `lut_direct.c`             | x    | x    | x    |       |       |       | x    | x    | x     |      |
| `lut_ldexpf.c`             | x    | x    | x    |       |       |       | x    | x    | x     |      |
| `lut_multi.c`              |      |      |      |       |       | x     |      |      |       | x    |


Check the paper for explanations and use cases.

## Customization

### Precision
The implementations default to the best possible precision where there is no diminishing performance returns yet.
The precision can be changed by defining `#define PRECISION xyz` before the include. All methods now use the defined precision.
Alternatively, if extra precision is only needed for one or two functions, the precision can be changed with the define in each implementation section. 
This change needs to be done on both the host and the dpu codes! 
E.g., in `lut_ldexpf_interpolate.c` change `PRECISION` to some value:
```c
...
#define SIN_COS_TAN_PRECISION 16 // This needs to match on CPU and DPU side!
...
```
and the same change of value needs to be applied in `lut_ldexpf_host.c`.

Approximate table sizes for LUT-based implementations (i.e., all methods except `cordic.c`):

| Precision | Table Size | Memory Requirement <br/>(with 6 Tables, <br/>as in `lut_ldexpf_interpolate.c`) |
|-----------|------------|-----------------------------------------------------------------------|
| 6         | 256 bytes  | 1.5 KiB                                                               |
| 8         | 1 KiB      | 6 KiB                                                                 |
| 10        | 4 KiB      | 24 KiB (recomended size)                                              |
| 12        | 16 KiB     | 96 KiB                                                                |
| 14        | 64 KiB     | 384 KiB                                                               |
| 16        | 256 KiB    | 1.5 MiB                                                               |
| 18        | 1 MiB      | 6 MiB                                                                 |
| 20        | 4 MiB      | 24 MiB                                                                |
| 22        | 16 MiB     | 96 MiB                                                                |

### MRAM / WRAM
We suggest to save LUT tables in MRAM, as the performance gain from storing them in WRAM is pretty small.
To change this, there is a define per table on the dpu side.
E.g., in `lut_ldexpf_interpolate.c` set
```c
...
#define SIN_COS_TAN_STORE_IN_WRAM 1
...
```

## Adding a New Function
New functions can be integrated into TransPimLib with the following three major steps:
1. **On the host side, add new code to calculate the lookup tables, and then transfer them over.** 

    In all methods, there is a function called `fill_table` or similar, which creates a lookup table. This function just needs to be given the right inputs. Then, the generated table needs to be transferred to the PIM side.
    
    All methods are split into sections that do everything needed for one particular function. For example, in `lut_ldexpf_host.c`
    there is a section for exponentiation that looks as follows:
    
    ```c
        /***********************************************************
        *   EXP
        */
        #define EXP_PRECISION PRECISION // This needs to match on CPU and DPU side! // <-- Defining 
        float exp_table[1 << EXP_PRECISION]; // <-- Generating the Array on the Host side
        int exp_granularity_exponent; // <-- Defining additional variable(s) that are needed to define the spacing of the LUT
    
        fill_table(0, log(2), exp, 1 << EXP_PRECISION, exp_table, &_unused_zero_address, &exp_granularity_exponent); // <-- Filling the table on the host side
        DPU_ASSERT(dpu_broadcast_to(set, "exp_table", 0, &exp_table, sizeof(exp_table), DPU_XFER_DEFAULT)); // <-- Transfering the table to the DPU
        DPU_ASSERT(dpu_broadcast_to(set, "exp_granularity_exponent", 0, &exp_granularity_exponent, sizeof(exp_granularity_exponent), DPU_XFER_DEFAULT)); // <-- Transfering additional variables to the DPU
    ```
    
    We suggest that you just copy such a section, and then go through it, rename all the variables and add the new limits and base function for the new lookup table.

2. **On the DPU side, add code to get the input from the lookup table.**

    Helper functions, which must be declared at the start of the file, can be created to obtain the desired address from the variables handed over from the host side.
    For some methods, the input is a float value, where the integer part is the address, and the float part can be used for interpolation.
    For other methods, there are two distinct functions, one providing the integer part, and one providing the fractional part (called `xxx_diff`).
    (This is due to the fact that depending on the method, one or the other might be cheaper to calculate)

    There snippets that completely manage one function. For example, for `lut_ldexpf_interpolate.c` exponentiation looks like this:

    ```c
    /***********************************************************
    *   EXP
    */

    // We define the same parameters on the host side
    #define EXP_PRECISION PRECISION // This needs to match on CPU and DPU side!
    
    // We create some directives to store in WRAM or MRAM
    #ifndef EXP_STORE_IN_WRAM
    #define EXP_STORE_IN_WRAM 0
    #endif
    
    // Get the additional variables from the host
    __host int exp_granularity_exponent; 
    
    // Get the main array from the host
    #if EXP_STORE_IN_WRAM > 0 
    __host float exp_table[1 << EXP_PRECISION];
    #else
    __mram_noinit float exp_table[1 << EXP_PRECISION];
    #endif
    
    // Now we define our function
    float expf (float x) { 
   
    // Allocate an additional variable for particular range extension this function needs
    int extra_data; 
   
    // Get our table address (here it is in the combined integer and fraction form)
    float offset_float = float_to_roughaddress_ldexpf(exp_range_extension_in(x, &extra_data), exp_granularity_exponent); 

    // Separate the address (integer) part from the interpolation part
    int offset_addr_down = (int) offset_float; 
    
    // Get our result for the address
    float base = exp_table[offset_addr_down]; 
    
    // Interpolate using the next address and the fractional part
    return exp_range_extension_out(base + (exp_table[offset_addr_down + 1] - base) * (offset_float - (float) offset_addr_down), &extra_data); 
   ```
   
3. **Function calls that are ouside of the range of the new lookup table.**
    This may require:
    - Generating some code that checks for symmetry (e.g., `_quadrants.c`)
    - Having some range extension formula (e.g., `_range_extensions.c`)
    - Returning a fixed value
    
    There is no unique process for this, as it may vary widely between different functions!


## Getting Help

If you have any suggestions for improvement, please contact el1goluj at gmail dot com and maurus.item at gmail dot com
If you find any bugs or have further questions or requests, please post an issue at the [issue page](https://github.com/CMU-SAFARI/transpimlib/issues).


## Acknowledgments 

We thank the anonymous reviewers of ISPASS 2023 for feedback. We acknowledge the generous gifts provided by our industrial partners, including ASML, Facebook, Google, Huawei, Intel, Microsoft, and VMware. We acknowledge support from the Semiconductor Research Corporation and the ETH Future Computing Laboratory.
