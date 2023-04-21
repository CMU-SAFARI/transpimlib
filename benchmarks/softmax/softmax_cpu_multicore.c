#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

#ifndef NTHREADS
#define NTHREADS 16
#endif

#define NUM_RUNS 10 // 100

int numError = 0;
int nThreads;
float *input;
float *output;
double sum = 0.0;
pthread_mutex_t sum_mutex = PTHREAD_MUTEX_INITIALIZER;


struct arg_exp_struct {
  float *array_in_start;
  float *array_out_start;
  int size;
};

void* run_exp(void* args) {
  struct arg_exp_struct arg = *(struct arg_exp_struct *) args;
  double sum_internal;
  float temp;

  for(int i = 0; i < arg.size; ++i){
    temp = exp(arg.array_in_start[i]);
    arg.array_out_start[i] = temp;
    sum_internal += temp;
  }

  pthread_mutex_lock( &sum_mutex);
  sum += sum_internal;
  pthread_mutex_unlock( &sum_mutex);

  return 0;
}

struct arg_mult_struct {
  float *array_out_start;
  double mult;
  int size;
};

void* run_mult(void* args) {
  struct arg_mult_struct arg = *(struct arg_mult_struct *) args;

  for (int i = 0; i < arg.size; ++i) {
    arg.array_out_start[i] *= arg.mult;
  }

  return 0;
}


int main (int argc, char **argv)
{
    FILE *file;
    int i;
    int loopnum;
    int numOptions;
    int nr_dpus;
    int rv;

    if (argc != 3) {
        printf("Usage:\n\t%s <inputFile> <outputFile>\n", argv[0]);
        exit(1);
    }
    char *inputFile = argv[1];
    char *outputFile = argv[2];

    //Read input data from file
    file = fopen(inputFile, "r");

    if(file == NULL) {
        printf("ERROR: Unable to open file `%s'.\n", inputFile);
        exit(1);
    }
    rv = fscanf(file, "%i", &numOptions);
    if(rv != 1) {
        printf("ERROR: Unable to read from file `%s'.\n", inputFile);
        fclose(file);
        exit(1);
    }

    // alloc spaces for the option data
    input = (float*)malloc(numOptions*sizeof(float));
    output = (float*)malloc(numOptions*sizeof(float));
    for ( loopnum = 0; loopnum < numOptions; ++ loopnum )
    {
        rv = fscanf(file, "%f", &input[loopnum]);
        if(rv != 1) {
            printf("ERROR: Unable to read from file `%s'.\n", inputFile);
            fclose(file);
            exit(1);
        }
    }
    rv = fclose(file);
    if(rv != 0) {
        printf("ERROR: Unable to close file `%s'.\n", inputFile);
        exit(1);
    }

    // Start measuring time
    struct timeval begin, end;
    gettimeofday(&begin, 0);
    for (int j=0; j<NUM_RUNS; j++) {

    int batch_size = (numOptions - 1 )/ NTHREADS + 1;
    pthread_t thread_id[NTHREADS];

    struct arg_exp_struct thread_args[NTHREADS];
    for(int i=0; i < NTHREADS; i++) {
      thread_args[i].array_in_start = &input[i * batch_size];
      thread_args[i].array_out_start = &output[i * batch_size];
      thread_args[i].size = batch_size < numOptions - i * batch_size ? batch_size : numOptions - i * batch_size;
    }

    for(int i=0; i < NTHREADS; i++) {
      pthread_create(&thread_id[i], NULL, run_exp, (void *)&thread_args[i]);
    }

    for(int i=0; i < NTHREADS; i++) {
      pthread_join( thread_id[i], NULL);
    }

    double inverted_sum = 1.0 / sum;

    struct arg_mult_struct thread_args_m[NTHREADS];
    for(int i=0; i < NTHREADS; i++) {
      thread_args_m[i].array_out_start = &output[i * batch_size];
      thread_args_m[i].size = batch_size < numOptions - i * batch_size ? batch_size : numOptions - i * batch_size;
      thread_args_m[i].mult = inverted_sum;
    }

    for(int i=0; i < NTHREADS; i++) {
      pthread_create(&thread_id[i], NULL, run_mult, (void *)&thread_args_m);
    }

	}
    // Stop measuring time and calculate the elapsed time
    gettimeofday(&end, 0);
    long seconds = end.tv_sec - begin.tv_sec;
    long microseconds = end.tv_usec - begin.tv_usec;
    double elapsed = seconds + microseconds*1e-6;

    printf("--- Softmax on CPU ---\n");
    printf("Total Time:                   %.2e secs.\n", elapsed/NUM_RUNS);
    printf("Size of data:                 %d\n", numOptions);

    // Save Output to File
    FILE *out_file = fopen("output/runs.csv", "a"); // write only
    fprintf(out_file, "softmax_multi, %d, %f\n", numOptions, elapsed/NUM_RUNS); // write to file

    //Write prices to output file
    file = fopen(outputFile, "w");
    if(file == NULL) {
        printf("ERROR: Unable to open file `%s'.\n", outputFile);
        exit(1);
    }
    rv = fprintf(file, "%i\n", numOptions);
    if(rv < 0) {
        printf("ERROR: Unable to write to file `%s'.\n", outputFile);
        fclose(file);
        exit(1);
    }
    for(i=0; i<numOptions; i++) {
        rv = fprintf(file, "%.18f\n", output[i]);
        if(rv < 0) {
            printf("ERROR: Unable to write to file `%s'.\n", outputFile);
            fclose(file);
            exit(1);
        }
    }
    rv = fclose(file);
    if(rv != 0) {
        printf("ERROR: Unable to close file `%s'.\n", outputFile);
        exit(1);
    }

    return 0;
}
