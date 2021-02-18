#include <mpi.h>
#include "tuple.h"

// contagion status of an individual
enum STATUS { susceptible, infected, immune };

typedef struct {
    // id of the individual, used in particular to find correspondent element in infected list
    int id;
    // moving speed
    tuple vel;
	// position
    tuple pos;
    enum STATUS status;
    // time (in seconds) that individual spent in its actual status, reset on status change
    long status_cumulated_time;
} individual;

void printIndividualState(individual i);
void commitIndividualTypeMPI(MPI_Datatype *mpi_type, MPI_Datatype MPI_TUPLE);