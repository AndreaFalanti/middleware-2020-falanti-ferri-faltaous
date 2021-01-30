#include "individual.h"

// nodes used in list of individuals
typedef struct node_ind {
    individual *ind;
    struct node_ind *next;
} node_ind;

node_ind* buildIndividualListNode(individual* ind);
void headInsertIndividualList(node_ind **l, node_ind *el);
void tailInsertIndividualList(node_ind **l, node_ind *el);
int pop(node_ind **head);
void printIndividualList(node_ind *head);