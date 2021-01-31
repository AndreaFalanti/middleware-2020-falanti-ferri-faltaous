#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

// TODO: making the list bidirectional would improve a lot the remove operation, because the list is
// ordered in crescent order of infection time (infects inserted in head), so searching from tail would make
// it O(1) instead of O(n)
void removeNodeWithId(node_ind **head, int search_id) {
    node_ind *temp = *head;
    node_ind *prev = NULL;
    bool deleted = false;

    while (!deleted && temp != NULL) {
        if (temp->ind->id == search_id) {
            // remove first list element, modifying the head
            if (prev == NULL) {
                *head = temp->next;
            }
            // remove element from list and memory
            else {
                prev->next = temp->next;
                free(temp);
            }

            deleted = true;
        }
        prev = temp;
        temp = temp->next;
    }

    if (!deleted) {
        printf("ID: %d not found!", search_id);
    }
}

void printIndividualList(node_ind *head) {
    node_ind *current = head;

    printf("\nInfected List:\n");
    while (current != NULL) {
        printIndividualState(*(current->ind));
        current = current->next;
    }
}