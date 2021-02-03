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

int main(int argc, char** argv) {
    if (argc != 9) {
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
        float pos_x = float_rand(0 + L / 100.0, L - L / 100.0);
        float pos_y = float_rand(0 + W / 100.0, W - W / 100.0);

        float vel_x = float_rand(-L / 100.0, L / 100.0);
        float vel_y = float_rand(-W / 100.0, W / 100.0);

        fprintf(f, "%f %f %f %f\n", pos_x, pos_y, vel_x, vel_y);
    }

    fclose(f);
    printf("File generated correctly\n");
}