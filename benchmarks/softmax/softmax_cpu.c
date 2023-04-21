#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

#define ROWS_PER_TASKLET 4

#define NUM_RUNS 10 // 100

int numError = 0;
int nThreads;
float *input;
float *output;


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

    double sum = 0.0;
    float temp;
    float max;
    int imax;
    for(int i = 0; i < numOptions; ++i){
        temp = exp(input[i]);
        output[i] = temp;
        sum += temp;
    }

    double inverted_sum = 1.0 / sum;

    for(int i = 0; i < numOptions ; ++i){
        output[i] *= inverted_sum;
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
    fprintf(out_file, "softmax_cpu, %d, %f\n", numOptions, elapsed/NUM_RUNS); // write to file

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