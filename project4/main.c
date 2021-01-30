#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
//#include <mpi.h>
//#include "individual.h"
#include "individual_list.h"
#include "error_codes.h"

#define INFECTION_THRESHOLD 60 * 10         // 10 minutes
#define INFECTION_TIME 60 * 60 * 24 * 10    // 10 days
#define IMMUNE_TIME 60 * 60 * 24 * 30 * 3   // 3 months

void initializeIndividuals(individual *individuals, node_ind **infected_head, int N, int I, int W, int L) {
    for (int i = 0; i < N; i++) {
        if (I > 0) {
            individuals[i].status = infected;
            node_ind* el = buildIndividualListNode(&individuals[i]);
            headInsertIndividualList(infected_head, el);
            I--;
        }
        individuals[i].id = i;

        individuals[i].pos.x = (rand() % (W * 1000)) / 1000.0;
        individuals[i].pos.y = (rand() % (L * 1000)) / 1000.0;

        // TODO: should be provided by input in next versions, just for testing with random values
        individuals[i].vel.x = (rand() % (W * 1000)) / 10000.0;
        individuals[i].vel.y = (rand() % (L * 1000)) / 10000.0;

        printIndividualState(individuals[i]);
    }
}

bool isMovementOutOfBounds(individual *el, int W, int L) {
    return (el->pos.x + el->vel.x > W) || (el->pos.y + el->vel.y > L);
}

void invertIndividualVelocity(individual *el) {
    el->vel.x = -el->vel.x;
    el->vel.y = -el->vel.y;
}

void updateIndividualPosition(individual *el) {
    el->pos.x += el->vel.x;
    el->pos.y += el->vel.y;
}

void moveIndividuals(individual *individuals, int N, int W, int L) {
    for (int i = 0; i < N; i++) {
        if (isMovementOutOfBounds(&individuals[i], W, L)) {
            invertIndividualVelocity(&individuals[i]);
        }
        updateIndividualPosition(&individuals[i]);
    }
}

void performSimulationStep(individual *individuals, node_ind **infected_list, int N, int W, int L, int t) {
    moveIndividuals(individuals, N, W, L);

    printf("----------- Simulation step --------------\n");
    for (int i = 0; i < N; i++) {
        printIndividualState(individuals[i]);
    }
    printIndividualList(*infected_list);
    printf("-------------------------------------------------------------------\n\n");
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
        exit(INVALID_ARG);
    }
    if (W < 0 || L < 0 || w < 0 || l < 0 || d < 0) {
        printf("Dimensions can't be negative!\n");
        exit(INVALID_ARG);
    }
    if (t <= 0) {
        printf("Time step can't be negative or null!\n");
        exit(INVALID_ARG);
    }

    individual *individuals;
    individuals = (individual*) malloc(N * sizeof(individual));
    node_ind *infected_list = NULL;

    printf("------ Initial state --------\n");
    initializeIndividuals(individuals, &infected_list, N, I, W, L);
    printIndividualList(infected_list);
    printf("-------------------------------------------------------------------\n\n");

    // TODO: make an assumption on how much steps to do
    int simulation_steps = 10;
    for (int i = 0; i < simulation_steps; i++) {
        performSimulationStep(individuals, &infected_list, N, W, L, t);
    }

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