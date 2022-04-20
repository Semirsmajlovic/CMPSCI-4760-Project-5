#ifndef __QUEUE_H
#define __QUEUE_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define MAXIMUM_ELEMENTS 100

struct Queue {
    int front_ind;
    int rear_ind;
    int elements[MAXIMUM_ELEMENTS];
    size_t size;
};

void queue_initializer(struct Queue* queue);
int queue_removal_action(struct Queue* queue);
int queue_lookin_action(struct Queue* queue);
void queue_add_action(struct Queue* queue, int element);
bool queue_maximized_action(struct Queue* queue);
bool queue_minimum_action(struct Queue* queue);
void queue_output_action(struct Queue* queue);

#endif
