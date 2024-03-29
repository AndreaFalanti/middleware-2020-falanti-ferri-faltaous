#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
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

// set individual status to the one provided by the input file
void initializeIndividuals(FILE *fi, individual *individuals, individual *infected_arr, sim_params params) {
    int to_infect = params.I;
    for (int i = 0; i < params.N; i++) {
        if (to_infect > 0) {
            individuals[i].status = infected;
            // node_ind* el = buildIndividualListNode(&individuals[i]);
            // headInsertIndividualList(infected_head, el);
            to_infect--;
        }
        individuals[i].id = i;

        // read pos and vel from file
        fscanf(fi, "%f %f %f %f", &individuals[i].pos.x, &individuals[i].pos.y, &individuals[i].vel.x, &individuals[i].vel.y);
        fgetc(fi);
    }

    // copy all infected individuals into infected_array, this is a fast way to perform this operation
    memcpy(infected_arr, individuals, params.I * sizeof(individual));
}

void buildInfectedListFromArray(node_ind **infected_head, individual *infected_arr, int I) {
    //node_ind *infected_head;
    for (int i = 0; i < I; i++) {
        node_ind* el = buildIndividualListNode(infected_arr[i]);
        headInsertIndividualList(infected_head, el);
    }

    //return infected_head;
}

// check that, after movement, the individual is still in the simulation world space
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

// move an individual, based on their actual speed
void moveIndividual(individual *el, int W, int L) {
    if (isMovementOutOfBounds(el, W, L)) {
        // TODO: could still be out of bound by inverting, find a better method
        invertIndividualVelocity(el);
    }

    updateIndividualPosition(el);
}

// update the state of an infected individual
void updateInfectedIndividualStatus(individual *el, node_ind **infected_list, int t) {
    el->status_cumulated_time += t;
    if (el->status_cumulated_time >= INFECTION_TIME) {
        // extra time is converted into time of immunity
        el->status_cumulated_time = el->status_cumulated_time % INFECTION_TIME;
        el->status = immune;

        removeNodeWithId(infected_list, el->id);
    }
}

// update status timer of individual, potentially triggering a status change when conditions are met
bool updateIndividualStatus(individual *el, node_ind **infected_list, int t, float d) {
    bool exposure = false;
    node_ind *head_temp = *infected_list;
    int exposure_time = t;

    if (el->status == infected) {
        updateInfectedIndividualStatus(el, infected_list, t);
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
            if (computeDistance(el->pos, head_temp->ind.pos) <= d) {
                exposure = true;
                el->status_cumulated_time += exposure_time;
                if (el->status_cumulated_time >= INFECTION_THRESHOLD) {
                    el->status_cumulated_time = el->status_cumulated_time % INFECTION_THRESHOLD;
                    el->status = infected;

                    return true;

                    // node_ind *new_el = buildIndividualListNode(el);
                    // headInsertIndividualList(infected_list, new_el);
                }
            }

            head_temp = head_temp->next;
        }

        // if not in prossimity of any infected individual, reset the counter
        if (!exposure) {
            el->status_cumulated_time = 0;
        }   
    }

    return false;
}

// reset counters of structs to 0
void resetCountryMatrix(country_report *cr_matrix, sim_params params) {
    for (int i = 0; i < params.yc * params.xc; i++) {
        // reset counters
        cr_matrix[i].susceptible = 0;
        cr_matrix[i].infected = 0;
        cr_matrix[i].immune = 0;
    }
}

// update country counters after a day of simulation, so that the report can be performed
void computeCountriesStatus(country_report *cr_matrix, individual *individuals, sim_params params) {
    resetCountryMatrix(cr_matrix, params);

    for (int i = 0; i < params.N; i++) {
        int xc = floor(individuals[i].pos.x / params.l);
        int yc = floor(individuals[i].pos.y / params.w);

        enum STATUS status = individuals[i].status;
        if (status == susceptible) {
            cr_matrix[yc * params.yc + xc].susceptible += 1;
        }
        else if (status == infected) {
            cr_matrix[yc * params.yc + xc].infected += 1;
        }
        else {
            cr_matrix[yc * params.yc + xc].immune += 1;
        }
    }
}

// compute status updates for each individual, move them and print various debug information
int performSimulationStep(individual *individuals, node_ind **infected_list, sim_params params, individual *newInfected) {
    int infections = 0;
    for (int i = 0; i < params.N; i++) {
        bool infected = updateIndividualStatus(&individuals[i], infected_list, params.t, params.d);
        if (infected) {
            newInfected[infections] = individuals[i];
            infections++;
        }
    }

    // move individuals vector
    for (int i = 0; i < params.N; i++) {
        moveIndividual(&individuals[i], params.W, params.L);
    }

    // move and update infected individuals
    node_ind *temp = *infected_list;
    node_ind *el;
    while (temp != NULL) {
        // el used because updateInfectedIndividualStatus could delete the node
        el = temp;
        temp = temp->next;

        moveIndividual(&(el->ind), params.W, params.L);
        // el could be deleted if immune after the update, but it's no more accessed so it should work fine
        updateInfectedIndividualStatus(&(el->ind), infected_list, params.t);
    }

    return infections;
}

// used to debug variables, not used in normal program flow
void printSimulationStatus(individual *individuals, node_ind **infected_list, country_report *cr, sim_params params,
                            long total_simulation_time, int simulated_days, FILE *out_file) {
    for (int i = 0; i < params.N; i++) {
        printIndividualState(individuals[i]);
    }

    //print list of infected individuals
    printIndividualList(*infected_list);
}

// read initial settings from input text file
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

// perform simple checks to avoid running with inconsistent parameters
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

// allocate memory for storing data on country status
country_report* allocateCountryMatrix(sim_params *params) {
    // compute the number of countries per axis 
    params->xc = ceil(params->L / (float)params->l);
    params->yc = ceil(params->W / (float)params->w);

    // allocate country matrix
    country_report *country_matrix;
    country_matrix = (country_report*)malloc(params->yc * params->xc * sizeof(country_report));

    return country_matrix;
}

// compute correct sizes for each partition (will be used in scatterv)
void computePartitionsSizesAndOffsets(int *sizes, int *offsets, int N, int world_size) {
    int rest = N % world_size;
    for (int i = 0; i < world_size; i++) {
        sizes[i] = N / world_size;
        offsets[i] = (i == 0) ? 0 : sizes[i-1] + offsets[i-1];

        if (rest > 0) {
            sizes[i] += 1;
            rest--;
        }
        // DEBUG ONLY
        //printf("\n%d %d", sizes[i], offsets[i]);
    }
}

int main(int argc, char** argv) {
    // Init the MPI environment
    MPI_Init(NULL, NULL);

    // Commit MPI datatype versions of structs used by the program
    MPI_Datatype MPI_SIM_PARAMS, MPI_TUPLE, MPI_INDIVIDUAL, MPI_COUNTRY_REPORT;
    MPI_Op MPI_COUNTRY_SUM;

    commitSimParamsTypeMPI(&MPI_SIM_PARAMS);
    commitTupleTypeMPI(&MPI_TUPLE);
    commitIndividualTypeMPI(&MPI_INDIVIDUAL, MPI_TUPLE);
    commitCountryTypeMPI(&MPI_COUNTRY_REPORT);
    MPI_Op_create(sumStructTs, 1, &MPI_COUNTRY_SUM);

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

    // used only by root
    individual *individuals;
    clock_t begin, end;
    FILE *fo = NULL;

    // used by all processors
    sim_params params;
    individual *process_individuals = NULL;
    node_ind *infected_list = NULL;
    country_report *country_matrix = NULL;
    country_report *country_matrix_output = NULL;
    individual *infected_arr = NULL;
    individual *received_infected_arr = NULL;

    // temporary variables
    int *p_sizes = NULL;
    int *p_offsets = NULL;

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
        infected_arr = (individual*) malloc(params.I * sizeof(individual));
        // TODO: check allocation is not NULL

        initializeIndividuals(fi, individuals, infected_arr, params);

        // DEBUG ONLY
        //printf("------ Initial state --------\n");
        // for (int i = 0; i < params.N; i++) {
        //     printIndividualState(individuals[i]);
        // }
        //printIndividualList(infected_list);
        //printf("-------------------------------------------------------------------\n\n");

        fclose(fi);
    }

    // parameters are passed to all processors
    MPI_Bcast(&params, 1, MPI_SIM_PARAMS, 0, MPI_COMM_WORLD);
    // DEBUG ONLY
    //printf("\nProcessor %d: %d %d %d %d %f\n", processor_rank, params.N, params.I, params.w, params.l, params.d);

    country_matrix = allocateCountryMatrix(&params);
    received_infected_arr = (individual*)malloc(params.N * sizeof(individual));

    p_sizes = (int*)malloc(world_size * sizeof(int));
    p_offsets = (int*)malloc(world_size * sizeof(int));
    // TODO: insert allocation check (!= NULL)

    computePartitionsSizesAndOffsets(p_sizes, p_offsets, params.N, world_size);

    // update N parameter, to match the partition length
    params.N = p_sizes[processor_rank];
    printf("Processor %d: my new N value is %d\n", processor_rank, params.N);
    process_individuals = (individual*)malloc(params.N * sizeof(individual));

    MPI_Scatterv(individuals, p_sizes, p_offsets, MPI_INDIVIDUAL,
        process_individuals, p_sizes[processor_rank], MPI_INDIVIDUAL,
        0, MPI_COMM_WORLD);

    free(p_sizes);
    free(p_offsets);
    //free(individuals);

    // DEBUG ONLY
    //printIndividualState(process_individuals[0]);

    // pass array of infected individuals and build a list from that
    if (processor_rank != 0) {
        infected_arr = (individual*) malloc(params.I * sizeof(individual));
    }

    MPI_Bcast(infected_arr, params.I, MPI_INDIVIDUAL, 0, MPI_COMM_WORLD);
    buildInfectedListFromArray(&infected_list, infected_arr, params.I);

    // DEBUG ONLY
    // if (processor_rank == 1) {
    //     printf("Processor %d:\n", processor_rank);
    //     printIndividualList(infected_list);
    // }
    

    free(infected_arr);

    // array for storing new infected individuals during a step.
    // by setting it to dim = process_N, we make sure it is big enough even if all individuals are infected at the same step
    infected_arr = (individual*) malloc(params.N * sizeof(individual));


    // TODO: make an assumption on how much steps to do
    long simulation_time = 60 * 60 * 24 * 150;   // 150 days
    int simulation_steps = ceil(simulation_time / params.t);
    long total_simulation_time = 0;
    int simulated_days = 0;
    int infections, total_infections;   // total infections used only in root
    int *infection_counters = NULL;     // relevant only for root
    int *infection_offsets = NULL;      // relevant only for root

    if (processor_rank == 0) {
        fo = fopen("output.txt", "w");
        if (fo == NULL) {
            printf("Error while opening output file!\n");
            exit(FILE_ERROR);
        }

        // allocate variables used in root to perform MPI operations
        country_matrix_output = allocateCountryMatrix(&params);
        infection_counters = (int*)malloc(world_size * sizeof(int));
        infection_offsets = (int*)malloc(world_size * sizeof(int));

        // setup a timer for checking how much time it takes to perform the simulation
        begin = clock();
    }

    for (int i = 0; i < simulation_steps; i++) {
        infections = performSimulationStep(process_individuals, &infected_list, params, infected_arr);
        total_simulation_time += params.t;

        // DEBUG ONLY
        // if (processor_rank == 1) {
        //     printf("Processor 1: pre-list\n");
        //     printIndividualList(infected_list);
        // }

        //printf("Processor %d: infections are %d\n", processor_rank, infections);

        // after each step, get how many infections there are in all processes
        MPI_Gather(&infections, 1, MPI_INT,
            infection_counters, 1, MPI_INT,
            0, MPI_COMM_WORLD);

        // compute in root if there are infections (> 0)
        if (processor_rank == 0) {
            total_infections = 0;
            for (int i = 0; i < world_size; i++) {
                //printf("%d: %d\n", i, infection_counters[i]);
                total_infections += infection_counters[i];
                infection_offsets[i] = (i == 0) ? 0 : infection_counters[i-1] + infection_offsets[i-1];
            }
        }

        // broadcast total infection number of this simulation step
        MPI_Bcast(&total_infections, 1, MPI_INT, 0, MPI_COMM_WORLD);
        // if there are new infections, get in root the new infected individuals and propagate them to all the processes
        if (total_infections > 0) {
            MPI_Gatherv(infected_arr, infections, MPI_INDIVIDUAL,
                received_infected_arr, infection_counters, infection_offsets, MPI_INDIVIDUAL,
                0, MPI_COMM_WORLD);

            MPI_Bcast(received_infected_arr, total_infections, MPI_INDIVIDUAL, 0, MPI_COMM_WORLD);

            // DEBUG ONLY
            // if (processor_rank == 1) {
            //     printf("Processor 1: pre-list\n");
            //     printIndividualList(infected_list);
            // }

            // each process add in the list the infected individuals
            for (int i = 0; i < total_infections; i++) {
                node_ind *el = buildIndividualListNode(received_infected_arr[i]);
                headInsertIndividualList(&infected_list, el);

                // DEBUG ONLY
                // if (processor_rank == 1) {
                //     printf("el %d:\n", i);
                //     printIndividualState(el->ind);
                // }
            }

            // if (processor_rank == 0) {
            //     printf("Processor 0: new infections at step of time %ld are %d\n", total_simulation_time, total_infections);
            // }

            //DEBUG ONLY
            // if (processor_rank == 1) {
            //     printIndividualList(infected_list);
            // }
        }

        // block triggered when each day is passed in the simulation, builds the country report
        if (floor(total_simulation_time / DAY) > simulated_days) {
            simulated_days += 1;
            printf("[Processor %d] Days simulated: %d\n", processor_rank, simulated_days);
            computeCountriesStatus(country_matrix, process_individuals, params);

            // DEBUG ONLY
            //printCountryReports(country_matrix, params.xc, params.yc, stdout, simulated_days, total_simulation_time);

            MPI_Reduce(country_matrix, country_matrix_output, params.xc * params.yc, MPI_COUNTRY_REPORT,
                MPI_COUNTRY_SUM, 0, MPI_COMM_WORLD);

            // print country reports on output file and reset the matrix for next iteration
            if (processor_rank == 0) {
                printCountryReports(country_matrix_output, params.xc, params.yc, fo, total_simulation_time, simulated_days);
                resetCountryMatrix(country_matrix_output, params);
            }
        }
    }

    // stop simulation timer and print the timer value on screen
    if (processor_rank == 0) {
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("Simulation time: %g seconds\n", time_spent);

        fclose(fo);
    }

    // Finalize the MPI environment
    MPI_Finalize();
}