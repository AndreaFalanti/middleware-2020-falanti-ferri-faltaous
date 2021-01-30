// contagion status of an individual
enum STATUS { susceptible, infected, immune };

typedef struct tuple {
    float x;
    float y;
} tuple;

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
    // consecutive time (in seconds) for which the individual stayed in contact with infected ones.
    // reset if the individual is not in contact with others within the time frame
    int infectionCumulatedTime;
} individual;

void printIndividualState(individual i);