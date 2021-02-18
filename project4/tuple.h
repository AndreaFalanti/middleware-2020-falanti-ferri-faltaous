#include <mpi.h>

typedef struct tuple {
    float x;
    float y;
} tuple;

double computeDistance(tuple pos1, tuple pos2);
void commitTupleTypeMPI(MPI_Datatype *mpi_type);