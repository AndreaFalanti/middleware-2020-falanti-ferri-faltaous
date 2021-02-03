#include <math.h>
#include <stddef.h>
#include "tuple.h"

double computeDistance(tuple pos1, tuple pos2) {
    return sqrt(pow(pos1.x - pos2.x, 2) + pow(pos1.y - pos2.y, 2));
}

void commitTupleTypeMPI(MPI_Datatype *mpi_type) {
    int struct_len = 2;    // 10 variables
    int block_lens[2] = {1, 1};
    MPI_Datatype types[2] = {MPI_FLOAT, MPI_FLOAT}; // MPI type for each variable

    // We need to compute the displacement to be really portable
    // (different compilers might align structures differently)
    MPI_Aint displacements[struct_len];

    displacements[0] = offsetof(tuple, x);
    displacements[1] = offsetof(tuple, y);

    MPI_Type_create_struct(struct_len, block_lens, displacements, types, mpi_type);
    MPI_Type_commit(mpi_type);
}