#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <mpi.h>
//#include "individual.h"
#include "sim_params.h"
#include "individual_list.h"
#include "country_report.h"
#include "error_codes.h"

#define INFECTION_THRESHOLD (60 * 10)         // 10 minutes (600)
#define INFECTION_TIME (60 * 60 * 24 * 10)    // 10 days (864.000)
#define IMMUNE_TIME (60 * 60 * 24 * 30 * 3)   // 3 months (7.776.000)
#define DAY (60 * 60 * 24)                    // 86.400 seconds

void initializeIndividuals(FILE *fi, individual *individuals, node_ind **infected_head, sim_params params) {
    for (int i = 0; i < params.N; i++) {
        if (params.I > 0) {
            individuals[i].status = infected;
            node_ind* el = buildIndividualListNode(&individuals[i]);
            headInsertIndividualList(infected_head, el);
            params.I--;
        }
        individuals[i].id = i;

        // read pos and vel from file
        fscanf(fi, "%f %f %f %f", &individuals[i].pos.x, &individuals[i].pos.y, &individuals[i].vel.x, &individuals[i].vel.y);
        fgetc(fi);

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
        el->status_cumulated_time += t;
        if (el->status_cumulated_time >= INFECTION_TIME) {
            // extra time is converted into time of immunity
            el->status_cumulated_time = el->status_cumulated_time % INFECTION_TIME;
            el->status = immune;

            removeNodeWithId(infected_list, el->id);
        }
    }
    else if (el->status == immune) {
        el->status_cumulated_time += t;
        if (el->status_cumulated_time >= IMMUNE_TIME) {
            // extra time is converted into possible time of exposure
            exposure_time = el->status_cumulated_time % IMMUNE_TIME;
            el->status_cumulated_time = 0;
            el->status = susceptible;
        }
    }

    // both cases in which was already susceptible or it become susceptible from immune status in this simulation step 
    if (el->status == susceptible) {
        while (head_temp != NULL && !exposure) {
            if (computeDistance(el->pos, head_temp->ind->pos) <= d) {
                exposure = true;
                el->status_cumulated_time += exposure_time;
                if (el->status_cumulated_time >= INFECTION_THRESHOLD) {
                    el->status_cumulated_time = el->status_cumulated_time % INFECTION_THRESHOLD;
                    el->status = infected;

                    node_ind *new_el = buildIndividualListNode(el);
                    headInsertIndividualList(infected_list, new_el);
                }
            }

            head_temp = head_temp->next;
        }

        // if not in prossimity of any infected individual, reset the counter
        if (!exposure) {
            el->status_cumulated_time = 0;
        }   
    }
}

void computeCountriesStatus(country_report **cr_matrix, individual *individuals, sim_params params) {
    for (int i = 0; i < params.yc; i++) {
        for (int j = 0; j < params.xc; j++) {
            // reset counters
            cr_matrix[i][j].susceptible = 0;
            cr_matrix[i][j].infected = 0;
            cr_matrix[i][j].immune = 0;
        }
    }

    for (int i = 0; i < params.N; i++) {
        int xc = floor(individuals[i].pos.x / params.l);
        int yc = floor(individuals[i].pos.y / params.w);

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
void performSimulationStep(individual *individuals, node_ind **infected_list, sim_params params) {
    for (int i = 0; i < params.N; i++) {
        updateIndividualStatus(&individuals[i], infected_list, params.t, params.d);
    }
    moveIndividuals(individuals, params.N, params.W, params.L); 
}

// TODO: refactor inputs as global variables or a struct to avoid huge amount of parameters in declaration
void printSimulationStatus(individual *individuals, node_ind **infected_list, country_report **cr, sim_params params,
                            long total_simulation_time, int simulated_days, FILE *out_file) {
    fprintf(out_file, "----------- Simulation step (day: %d, total time: %ld) --------------\n",
        simulated_days, total_simulation_time);
    // for (int i = 0; i < N; i++) {
    //     printIndividualState(individuals[i]);
    // }

    // print list of infected individuals
    // printIndividualList(*infected_list);

    computeCountriesStatus(cr, individuals, params);
    printCountryReports(cr, params.xc, params.yc, out_file);
    fprintf(out_file, "-------------------------------------------------------------------\n\n");
}

void readInputParamsFromFile(FILE *fi, sim_params *params) {
    // read paramters from input, fgetc used to read the \n
    fscanf(fi, "%d %d", &params->N, &params->I);
    fgetc(fi);
    fscanf(fi, "%d %d %d %d", &params->W, &params->L, &params->w, &params->l);
    fgetc(fi);
    fscanf(fi, "%f %d", &params->d, &params->t);
    fgetc(fi);
    fgetc(fi);
}

void checkParametersConstraints(sim_params params) {
    if (params.I > params.N) {
        printf("Infected individuals can't be higher that total individuals!\n");
        exit(INVALID_ARG);
    }
    if (params.W < 0 || params.L < 0 || params.w < 0 || params.l < 0 || params.d < 0) {
        printf("Dimensions can't be negative!\n");
        exit(INVALID_ARG);
    }
    if (params.t <= 0) {
        printf("Time step can't be negative or null!\n");
        exit(INVALID_ARG);
    }
    if (params.w > params.W || params.l > params.L) {
        printf("Invalid country dimensions!");
        exit(INVALID_ARG);
    }
}

country_report** allocateCountryMatrix(sim_params *params) {
    // compute the number of countries per axis 
    params->xc = ceil(params->L / (float)params->l);
    params->yc = ceil(params->W / (float)params->w);

    // allocate country matrix
    country_report **country_matrix;
    country_matrix = (country_report**)malloc(params->yc * sizeof(country_report*));

    for (int i = 0; i < params->yc; i++) {
        country_matrix[i] = (country_report*)malloc(params->xc * sizeof(country_report));
    }

    return country_matrix;
}

int main(int argc, char** argv) {
    // srand(123);
    // sim_params params;

    // FILE *fi = fopen("input.txt", "r");
    // if (fi == NULL) {
    //     printf("Error while opening input file!\n");
    //     exit(FILE_ERROR);
    // }

    // readInputParamsFromFile(fi, &params);
    // checkParametersConstraints(params);

    // country_report **country_matrix = allocateCountryMatrix(&params);

    // // allocate individuals vector
    // individual *individuals;
    // individuals = (individual*) malloc(params.N * sizeof(individual));
    // node_ind *infected_list = NULL;

    // printf("------ Initial state --------\n");
    // initializeIndividuals(fi, individuals, &infected_list, params);
    // printIndividualList(infected_list);
    // printf("-------------------------------------------------------------------\n\n");

    // free(fi);

    // // TODO: make an assumption on how much steps to do
    // long simulation_time = 60 * 60 * 24 * 10;   // 10 days
    // int simulation_steps = ceil(simulation_time / params.t);
    // long total_simulation_time = 0;
    // int simulated_days = 0;

    // FILE *fo = fopen("output.txt", "w");
    // if (fo == NULL) {
    //     printf("Error while opening output file!\n");
    //     exit(FILE_ERROR);
    // }

    // clock_t begin = clock();

    // for (int i = 0; i < simulation_steps; i++) {
    //     performSimulationStep(individuals, &infected_list, params);
    //     total_simulation_time += params.t;

    //     // TODO: it should print only at the end of the day in the final app, but depending on t this condition will not hold, fix later
    //     if (floor(total_simulation_time / DAY) > simulated_days) {
    //         simulated_days += 1;
    //         printf("Days simulated: %d\n", simulated_days);
    //         printSimulationStatus(individuals, &infected_list, country_matrix, params,
    //             total_simulation_time, simulated_days, fo);
    //     }
    // }

    // clock_t end = clock();
    // double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    // printf("Simulation time: %g seconds\n", time_spent);

    // fclose(fo);
    // // TODO: should free vectors and lists?

    // return 0;

    // TODO: define the MPI part to parallelize the simulation when the rest is ready

    // Init the MPI environment
    MPI_Init(NULL, NULL);

    // Commit MPI datatype versions of structs used by the program
    MPI_Datatype MPI_SIM_PARAMS, MPI_TUPLE, MPI_INDIVIDUAL;
    commitSimParamsTypeMPI(&MPI_SIM_PARAMS);
    commitTupleTypeMPI(&MPI_TUPLE);
    commitIndividualTypeMPI(&MPI_INDIVIDUAL, MPI_TUPLE);

    // Get the number of processes
    int processor_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &processor_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the name of the processor
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int name_len;
    MPI_Get_processor_name(processor_name, &name_len);

    // Print off a hello world message
    printf("Processor %s online (rank %d out of %d)\n", processor_name, processor_rank, world_size);

    srand(123);
    sim_params params;
    individual *individuals;
    individual *process_individuals;
    node_ind *infected_list = NULL;

    int *sizes = NULL;

    // only root reads the input file
    if (processor_rank == 0) {
        FILE *fi = fopen("input.txt", "r");
        if (fi == NULL) {
            printf("Error while opening input file!\n");
            exit(FILE_ERROR);
        }

        readInputParamsFromFile(fi, &params);
        checkParametersConstraints(params);

        individuals = (individual*) malloc(params.N * sizeof(individual));
        printf("------ Initial state --------\n");
        initializeIndividuals(fi, individuals, &infected_list, params);
        printIndividualList(infected_list);
        printf("-------------------------------------------------------------------\n\n");

        // compute correct sizes for each partition (will be used in scatterv later)
        sizes = (int*)malloc(world_size * sizeof(int));
        int rest = params.N % world_size;
        for (int i = 0; i < world_size; i++) {
            sizes[i] = params.N / world_size;
            if (rest > 0) {
                sizes[i]++;
                rest--;
            }
            printf("%d ", sizes[i]);
        }

        fclose(fi);
    }

    MPI_Bcast(&params, 1, MPI_SIM_PARAMS, 0, MPI_COMM_WORLD);
    printf("\n");
    printf("Processor %d: %d %d %d %d %f\n", processor_rank, params.N, params.I, params.w, params.l, params.d);

    country_report **country_matrix = allocateCountryMatrix(&params);
    process_individuals = (individual*) malloc(params.N / world_size * sizeof(individual));

    //MPI_Scatterv(individuals, sizes, sizes, )

    // Finalize the MPI environment
    MPI_Finalize();
}