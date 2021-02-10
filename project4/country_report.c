#include <stdio.h>
#include <stddef.h>
#include "country_report.h"

void printCountryReports(country_report *cr, int X, int Y, FILE *out_file, long total_simulation_time, int simulated_days) {
    fprintf(out_file, "----------- Simulation step (day: %d, total time: %ld) --------------\n",
        simulated_days, total_simulation_time);

    fprintf(out_file, "\nCountry Report:\n");
    for (int i = 0; i < X * Y; i++) {
        fprintf(out_file, "Country [%d, %d] -> susceptible: %ld, infected: %ld, immune: %ld. TOTAL: %ld\n",
                i / X, i % X, cr[i].susceptible, cr[i].infected, cr[i].immune, (cr[i].susceptible + cr[i].infected + cr[i].immune));
    }

    fprintf(out_file, "-------------------------------------------------------------------\n\n");
}

void commitCountryTypeMPI(MPI_Datatype *mpi_type) {
    int struct_len = 3;    // 3 variables
    int block_lens[3] = {1, 1, 1};
    MPI_Datatype types[3] = {MPI_LONG, MPI_LONG, MPI_LONG}; // MPI type for each variable

    // We need to compute the displacement to be really portable
    // (different compilers might align structures differently)
    MPI_Aint displacements[struct_len];

    displacements[0] = offsetof(country_report, susceptible);
    displacements[1] = offsetof(country_report, infected);
    displacements[2] = offsetof(country_report, immune);

    MPI_Type_create_struct(struct_len, block_lens, displacements, types, mpi_type);
    MPI_Type_commit(mpi_type);
}

void sumStructTs(void *in, void *inout, int *len, MPI_Datatype *type) {
    /* ignore type, just trust that it's our struct type */
    country_report *invals = in;
    country_report *inoutvals = inout;

    for (int i=0; i < *len; i++) {
        inoutvals[i].susceptible += invals[i].susceptible;
        inoutvals[i].infected += invals[i].infected;
        inoutvals[i].immune += invals[i].immune;
    }

    return;
}

