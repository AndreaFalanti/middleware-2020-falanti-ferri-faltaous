typedef struct country_report {
    long susceptible;
    long infected;
    long immune;
} country_report;

void printCountryReports(country_report **cr, int X, int Y, FILE *out_file);