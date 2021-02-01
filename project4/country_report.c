#include <stdio.h>
#include "country_report.h"

void printCountryReports(country_report **cr, int X, int Y, FILE *out_file) {
    fprintf(out_file, "\nCountry Report:\n");
    for (int i = 0; i < Y; i++) {
        for (int j = 0; j < X; j++) {
            fprintf(out_file, "Country [%d, %d] -> susceptible: %ld, infected: %ld, immune: %ld. TOTAL: %ld\n",
                i, j, cr[i][j].susceptible, cr[i][j].infected, cr[i][j].immune, (cr[i][j].susceptible + cr[i][j].infected + cr[i][j].immune));
        }
    }
}