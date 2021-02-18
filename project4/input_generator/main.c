#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

// Generates a valid randomized input.txt file for the simulator 

float float_rand(float min, float max) {
    float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
    return min + scale * ( max - min );      /* [min, max] */
}

// ./a.out 2500 75 250 250 50 50 2.1 100 7.0
int main(int argc, char** argv) {
    if (argc != 10) {
        printf("Invalid arguments");
        exit(-1);
    }

    srand((unsigned int)time(NULL));

    int N = strtol(argv[1], NULL, 10);
    int I = strtol(argv[2], NULL, 10);
    int W = strtol(argv[3], NULL, 10);
    int L = strtol(argv[4], NULL, 10);
    int w = strtol(argv[5], NULL, 10);
    int l = strtol(argv[6], NULL, 10);
    float d = strtod(argv[7], NULL);
    int t = strtol(argv[8], NULL, 10);
    float vel_max = strtod(argv[9], NULL);

    
    FILE *f = fopen("input.txt", "w");
    if (f == NULL)
    {
        printf("Error while opening file!\n");
        exit(-2);
    }

    fprintf(f, "%d %d\n", N, I);
    fprintf(f, "%d %d %d %d\n", W, L, w, l);
    fprintf(f, "%f %d\n\n", d, t);

    for (int i = 0; i < N; i++) {
        float pos_x = float_rand(0 + vel_max, L - vel_max);
        float pos_y = float_rand(0 + vel_max, W - vel_max);

        float vel_x = float_rand(-vel_max, vel_max);
        float vel_y = float_rand(-vel_max, vel_max);

        fprintf(f, "%f %f %f %f\n", pos_x, pos_y, vel_x, vel_y);
    }

    fclose(f);
    printf("File generated correctly\n");
}