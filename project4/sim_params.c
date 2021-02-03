#include <stddef.h>
#include "sim_params.h"

void commitSimParamsTypeMPI(MPI_Datatype *mpi_type) {
    int struct_len = 10;    // 10 variables
    int block_lens[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    MPI_Datatype types[10] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT,
        MPI_INT, MPI_INT, MPI_INT, MPI_FLOAT}; // MPI type for each variable

    // We need to compute the displacement to be really portable
    // (different compilers might align structures differently)
    MPI_Aint displacements[struct_len];

    displacements[0] = offsetof(sim_params, N);
    displacements[1] = offsetof(sim_params, I);
    displacements[2] = offsetof(sim_params, W);
    displacements[3] = offsetof(sim_params, L);
    displacements[4] = offsetof(sim_params, w);
    displacements[5] = offsetof(sim_params, l);
    displacements[6] = offsetof(sim_params, t);
    displacements[7] = offsetof(sim_params, xc);
    displacements[8] = offsetof(sim_params, yc);
    displacements[9] = offsetof(sim_params, d);

    MPI_Type_create_struct(struct_len, block_lens, displacements, types, mpi_type);
    MPI_Type_commit(mpi_type);
}