#include <stdio.h>
#include <stdlib.h>
#include "individual_list.h"
#include "error_codes.h"

node_ind* buildIndividualListNode(individual* ind) {
    node_ind *el;
    el = (node_ind*)malloc(sizeof(node_ind));

    if (el != NULL) {
        el->ind = ind;
        el->next = NULL;
        return el;
    }
    else {
        printf("Allocation error");
        exit(ALLOCATION_ERROR);
    }
}

void headInsertIndividualList(node_ind **l, node_ind *el) {
    if (*l != NULL) {
        el -> next = *l;
        *l = el;
    }
    else {
        *l = el;
    }
}

void tailInsertIndividualList(node_ind **l, node_ind *el) {
    node_ind *temp;
    temp = *l;

    // if list is empty, then is equivalent to a head insert
    if (temp == NULL) {
        headInsertIndividualList(l, el);
    }
    else {
        while (temp -> next != NULL) {
            temp = temp -> next;
        }
        temp -> next = el;
    }
}

// remove from head
int pop(node_ind **head) {
    int retval = -1;
    node_ind *next_node = NULL;

    if (*head == NULL) {
        return -1;
    }

    next_node = (*head)->next;
    retval = (*head)->ind;
    free(*head);
    *head = next_node;

    return retval;
}

void printIndividualList(node_ind *head) {
    node_ind *current = head;

    printf("\nInfected List:\n");
    while (current != NULL) {
        printIndividualState(*(current->ind));
        current = current->next;
    }
}