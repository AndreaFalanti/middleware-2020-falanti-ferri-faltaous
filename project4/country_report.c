#include <stdio.h>
#include "country_report.h"

void printCountryReports(country_report **cr, int X, int Y) {
    printf("\nCountry Report:\n");
    for (int i = 0; i < Y; i++) {
        for (int j = 0; j < X; j++) {
            printf("Country [%d, %d] -> susceptible: %ld, infected: %ld, immune: %ld. TOTAL: %ld\n",
                i, j, cr[i][j].susceptible, cr[i][j].infected, cr[i][j].immune, (cr[i][j].susceptible + cr[i][j].infected + cr[i][j].immune));
        }
    }
}