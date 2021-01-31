#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
//#include <mpi.h>
//#include "individual.h"
#include "individual_list.h"
#include "country_report.h"
#include "error_codes.h"

#define INFECTION_THRESHOLD (60 * 10)         // 10 minutes (600)
#define INFECTION_TIME (60 * 60 * 24 * 10)    // 10 days (864.000)
#define IMMUNE_TIME (60 * 60 * 24 * 30 * 3)   // 3 months (7.776.000)
#define DAY (60 * 60 * 24)                    // 86.400 seconds

void initializeIndividuals(individual *individuals, node_ind **infected_head, int N, int I, int W, int L) {
    for (int i = 0; i < N; i++) {
        if (I > 0) {
            individuals[i].status = infected;
            node_ind* el = buildIndividualListNode(&individuals[i]);
            headInsertIndividualList(infected_head, el);
            I--;
        }
        individuals[i].id = i;

        individuals[i].pos.x = (rand() % (W * 1000)) / 1000.0;
        individuals[i].pos.y = (rand() % (L * 1000)) / 1000.0;

        // TODO: should be provided by input in next versions, just for testing with random values
        individuals[i].vel.x = (rand() % (W * 1000)) / 100000.0;
        individuals[i].vel.y = (rand() % (L * 1000)) / 100000.0;

        printIndividualState(individuals[i]);
    }
}

bool isMovementOutOfBounds(individual *el, int W, int L) {
    float new_x = el->pos.x + el->vel.x;
    float new_y = el->pos.y + el->vel.y;

    return (new_x > W || new_x < 0) || (new_y > L || new_y < 0);
}

void invertIndividualVelocity(individual *el) {
    el->vel.x = -el->vel.x;
    el->vel.y = -el->vel.y;
}

void updateIndividualPosition(individual *el) {
    el->pos.x += el->vel.x;
    el->pos.y += el->vel.y;
}

// move all individuals, based on their actual speed
void moveIndividuals(individual *individuals, int N, int W, int L) {
    for (int i = 0; i < N; i++) {
        if (isMovementOutOfBounds(&individuals[i], W, L)) {
            // TODO: could still be out of bound by inverting, find a better method
            invertIndividualVelocity(&individuals[i]);
        }
        updateIndividualPosition(&individuals[i]);
    }
}

// update status timer of individual, potentially triggering a status change when conditions are met
void updateIndividualStatus(individual *el, node_ind **infected_list, int t, float d) {
    bool exposure = false;
    node_ind *head_temp = *infected_list;
    int exposure_time = t;

    if (el->status == infected) {
        el->statusCumulatedTime += t;
        if (el->statusCumulatedTime >= INFECTION_TIME) {
            // extra time is converted into time of immunity
            el->statusCumulatedTime = el->statusCumulatedTime % INFECTION_TIME;
            el->status = immune;

            removeNodeWithId(infected_list, el->id);
        }
    }
    else if (el->status == immune) {
        el->statusCumulatedTime += t;
        if (el->statusCumulatedTime >= IMMUNE_TIME) {
            // extra time is converted into possible time of exposure
            exposure_time = el->statusCumulatedTime % IMMUNE_TIME;
            el->statusCumulatedTime = 0;
            el->status = susceptible;
        }
    }

    // both cases in which was already susceptible or it become susceptible from immune status in this simulation step 
    if (el->status == susceptible) {
        while (head_temp != NULL && !exposure) {
            if (computeDistance(el->pos, head_temp->ind->pos) <= d) {
                exposure = true;
                el->statusCumulatedTime += exposure_time;
                if (el->statusCumulatedTime >= INFECTION_THRESHOLD) {
                    el->statusCumulatedTime = el->statusCumulatedTime % INFECTION_THRESHOLD;
                    el->status = infected;

                    node_ind *new_el = buildIndividualListNode(el);
                    headInsertIndividualList(infected_list, new_el);
                }
            }

            head_temp = head_temp->next;
        }

        // if not in prossimity of any infected individual, reset the counter
        if (!exposure) {
            el->statusCumulatedTime = 0;
        }   
    }
}

void computeCountriesStatus(country_report **cr_matrix, int X, int Y, individual *individuals, int N, int w, int l) {
    for (int i = 0; i < Y; i++) {
        for (int j = 0; j < X; j++) {
            // reset counters
            cr_matrix[i][j].susceptible = 0;
            cr_matrix[i][j].infected = 0;
            cr_matrix[i][j].susceptible = 0;
        }
    }

    for (int i = 0; i < N; i++) {
        int xc = floor(individuals[i].pos.x / l);
        int yc = floor(individuals[i].pos.y / w);

        enum STATUS status = individuals[i].status;
        if (status == susceptible) {
            cr_matrix[yc][xc].susceptible += 1;
        }
        else if (status == infected) {
            cr_matrix[yc][xc].infected += 1;
        }
        else {
            cr_matrix[yc][xc].immune += 1;
        }
    }
}

// compute status updates for each individual, move them and print various debug information
void performSimulationStep(individual *individuals, node_ind **infected_list, int N, int W, int L, int t, int d) {
    for (int i = 0; i < N; i++) {
        updateIndividualStatus(&individuals[i], infected_list, t, d);
    }
    moveIndividuals(individuals, N, W, L); 
}

// TODO: refactor inputs as global variables or a struct to avoid huge amount of parameters in declaration
void printSimulationStatus(individual *individuals, node_ind **infected_list, int N,
                            country_report **cr, int xc, int yc, int w, int l, long total_simulation_time) {
    printf("----------- Simulation step (time: %ld) --------------\n", total_simulation_time);
    // for (int i = 0; i < N; i++) {
    //     printIndividualState(individuals[i]);
    // }

    // print list of infected individuals
    // printIndividualList(*infected_list);

    computeCountriesStatus(cr, xc, yc, individuals, N, w, l);
    printCountryReports(cr, xc, yc);
    printf("-------------------------------------------------------------------\n\n");
}

int main(int argc, char** argv) {
    // srand((unsigned int)time(NULL));
    // fixed seed to have easier time estimation
    srand(123);

    // number of individuals
    int N = 1000;
    // number of individuals that are initially infected 
    int I = 30;
    // width and length of the rectangular area where individuals move (in meters) 
    int W = 100, L = 100;
    // width and length of each country (in meters)
    int w = 25, l = 25;
    /* maximum spreading distance (in meters): a susceptible individual
    that remains closer than d to at least one infected individual becomes infected */
    float d = 1.5;
    /* time step (in seconds): the simulation recomputes the position and status (susceptible, 
     infected, immune) of each individual with a temporal granularity of t (simulated) seconds */
    int t = 50;

    if (I > N) {
        printf("Infected individuals can't be higher that total individuals!\n");
        exit(INVALID_ARG);
    }
    if (W < 0 || L < 0 || w < 0 || l < 0 || d < 0) {
        printf("Dimensions can't be negative!\n");
        exit(INVALID_ARG);
    }
    if (t <= 0) {
        printf("Time step can't be negative or null!\n");
        exit(INVALID_ARG);
    }
    if (w > W || l > L) {
        printf("Invalid country dimensions!");
        exit(INVALID_ARG);
    }

    // compute the number of countries per axis 
    int x_countries = ceil(L / (float)l);
    int y_countries = ceil(W / (float)w);

    // allocate country matrix
    country_report **country_matrix;
    country_matrix = (country_report**)malloc(y_countries * sizeof(country_report*));

    for (int i = 0; i < y_countries; i++) {
        country_matrix[i] = (country_report*)malloc(x_countries * sizeof(country_report));
    }

    // allocate individuals vector
    individual *individuals;
    individuals = (individual*) malloc(N * sizeof(individual));
    node_ind *infected_list = NULL;

    printf("------ Initial state --------\n");
    initializeIndividuals(individuals, &infected_list, N, I, W, L);
    printIndividualList(infected_list);
    printf("-------------------------------------------------------------------\n\n");

    // TODO: make an assumption on how much steps to do
    long simulation_time = 60 * 60 * 24 * 10;   // 10 days
    int simulation_steps = ceil(simulation_time / t);
    long total_simulation_time = 0;
    int simulated_days = 0;

    clock_t begin = clock();

    for (int i = 0; i < simulation_steps; i++) {
        performSimulationStep(individuals, &infected_list, N, W, L, t, d);
        total_simulation_time += t;

        // TODO: it should print only at the end of the day in the final app, but depending on t this condition will not hold, fix later
        if (floor(total_simulation_time / DAY) > simulated_days) {
            simulated_days += 1;
            printSimulationStatus(individuals, &infected_list, N,
                country_matrix, x_countries, y_countries, w, l, total_simulation_time);
        }
    }

    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Simulation time: %g seconds\n", time_spent);

    return 0;

/* TODO: define the MPI part to parallelize the simulation when the rest is ready

    // Init the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // Print off a hello world message
    printf("Hello world from processor %s (rank %d out of %d)\n", processor_name, world_rank, world_size);

    // Finalize the MPI environment
    MPI_Finalize(); */
}