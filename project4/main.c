#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <mpi.h>
#include "individual.h"

#define INFECTION_THRESHOLD 60 * 10         //10 minutes
#define INFECTION_TIME 60 * 60 * 24 * 10    //10 days
#define IMMUNE_TIME 60 * 60 * 24 * 30 * 3   // 3 months

void initializeIndividuals(individual *individuals, /*individual **infected_head,*/ int N, int I, int W, int L) {
    for (int i = 0; i < N; i++) {
        if (I > 0) {
            individuals[i].status = infected;
            I--;
        }
        individuals[i].pos.x = (rand() % (W * 1000)) / 1000.0;
        individuals[i].pos.y = (rand() % (L * 1000)) / 1000.0;

        printIndividualState(individuals[i]);
    }
}

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    // number of individuals
    int N = 15;
    // number of individuals that are initially infected 
    int I = 5;
    // width and length of the rectangular area where individuals move (in meters) 
    int W = 10, L = 10;
    // width and length of each country (in meters)
    int w = 3, l = 3;
    /* maximum spreading distance (in meters): a susceptible individual
    that remains closer than d to at least one infected individual becomes infected */
    float d = 2;
    /* time step (in seconds): the simulation recomputes the position and status (susceptible, 
     infected, immune) of each individual with a temporal granularity of t (simulated) seconds */
    int t = 5;

    if (I > N) {
        printf("Infected individuals can't be higher that total individuals!\n");
        exit(-1);
    }
    if (W < 0 || L < 0 || w < 0 || l < 0 || d < 0) {
        printf("Dimensions can't be negative!\n");
        exit(-1);
    }
    if (t <= 0) {
        printf("Time step can't be negative or null!\n");
        exit(-1);
    }

    individual *individuals;
    individuals = (individual*) malloc(N * sizeof(individual));

    initializeIndividuals(individuals, N, I, W, L);

/* TODO: define the MPI part to parallelize the simulation when the rest is ready

    // Init the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // Print off a hello world message
    printf("Hello world from processor %s (rank %d out of %d)\n", processor_name, world_rank, world_size);

    // Finalize the MPI environment
    MPI_Finalize(); */
}