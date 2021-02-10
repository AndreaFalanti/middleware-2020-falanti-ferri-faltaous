#include <mpi.h>

typedef struct country_report {
    long susceptible;
    long infected;
    long immune;
} country_report;

void printCountryReports(country_report *cr, int X, int Y, FILE *out_file, long total_simulation_time, int simulated_days);
void commitCountryTypeMPI(MPI_Datatype *mpi_type);
void sumStructTs(void *in, void *inout, int *len, MPI_Datatype *type);