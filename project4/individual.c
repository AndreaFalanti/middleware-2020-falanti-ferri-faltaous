#include <stdio.h>
#include "individual.h"

const char* statusNames[] = { "susceptible", "infected", "immune" };

void printIndividualState(individual i) {
    printf("[ID: %d] status: %s, position: (%f, %f), speed: (%f, %f), cumulated time: %ld\n",
        i.id, statusNames[i.status], i.pos.x, i.pos.y, i.vel.x, i.vel.y, i.statusCumulatedTime);
}