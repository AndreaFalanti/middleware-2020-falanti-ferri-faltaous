#include <stdio.h>
#include <stddef.h>
#include "individual.h"

const char* statusNames[] = { "susceptible", "infected", "immune" };

void printIndividualState(individual i) {
    printf("[ID: %d] status: %s, position: (%f, %f), speed: (%f, %f), cumulated time: %ld\n",
        i.id, statusNames[i.status], i.pos.x, i.pos.y, i.vel.x, i.vel.y, i.status_cumulated_time);
}

void commitIndividualTypeMPI(MPI_Datatype *mpi_type, MPI_Datatype MPI_TUPLE) {
    int struct_len = 5;    // 5 variables
    int block_lens[5] = {1, 1, 1, 1, 1};
    MPI_Datatype types[5] = {MPI_INT, MPI_TUPLE, MPI_TUPLE, MPI_INT, MPI_LONG}; // MPI type for each variable

    // We need to compute the displacement to be really portable
    // (different compilers might align structures differently)
    MPI_Aint displacements[struct_len];

    displacements[0] = offsetof(individual, id);
    displacements[1] = offsetof(individual, vel);
    displacements[2] = offsetof(individual, pos);
    displacements[3] = offsetof(individual, status);
    displacements[4] = offsetof(individual, status_cumulated_time);

    MPI_Type_create_struct(struct_len, block_lens, displacements, types, mpi_type);
    MPI_Type_commit(mpi_type);
}