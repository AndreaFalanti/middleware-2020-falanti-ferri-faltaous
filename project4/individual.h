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
    long statusCumulatedTime;
} individual;

void printIndividualState(individual i);