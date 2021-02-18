#include <mpi.h>

typedef struct sim_params {
    // number of individuals
    int N;
    // number of individuals that are initially infected 
    int I;
    // width and length of the rectangular area where individuals move (in meters) 
    int W, L;
    // width and length of each country (in meters)
    int w, l;
    /* time step (in seconds): the simulation recomputes the position and status (susceptible, 
     infected, immune) of each individual with a temporal granularity of t (simulated) seconds */
    int t;
    // indirect parameters, number of countries subdivisions per axis
    int xc, yc;
    /* maximum spreading distance (in meters): a susceptible individual
    that remains closer than d to at least one infected individual becomes infected */
    float d;
} sim_params;

void commitSimParamsTypeMPI(MPI_Datatype *mpi_type);