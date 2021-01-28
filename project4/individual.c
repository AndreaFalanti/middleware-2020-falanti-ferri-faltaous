#include <stdio.h>
#include "individual.h"

const char* statusNames[] = { "susceptible", "infected", "immune" };

void printIndividualState(individual i) {
    printf("Status: %s, position: (%f, %f), speed: (%f, %f)\n",
        statusNames[i.status], i.pos.x, i.pos.y, i.v.x, i.v.y);
}