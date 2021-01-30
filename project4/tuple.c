#include <math.h>
#include "tuple.h"

double computeDistance(tuple pos1, tuple pos2) {
    return sqrt(pow(pos1.x - pos2.x, 2) + pow(pos1.x - pos2.x, 2));
}