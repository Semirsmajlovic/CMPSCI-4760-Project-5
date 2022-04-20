#include <stdbool.h>
#include <stdlib.h>

#include "queue.h"

void queue_initializer(struct Queue* queue) {
    queue->front_ind = 0;
    queue->rear_ind = -1;
    queue->size = 0;
}

int queue_removal_action(struct Queue* queue) {
    if (queue_minimum_action(queue)) return -1;
    int element = queue->elements[queue->front_ind];
    queue->elements[queue->front_ind++] = 0;
    if (queue->front_ind == MAXIMUM_ELEMENTS) {
        queue->front_ind = 0;
    }
    queue->size--;
    return element;
}

int queue_lookin_action(struct Queue* queue) {
    if (queue_minimum_action(queue)) return -1;
    return queue->elements[queue->front_ind];
}

void queue_add_action(struct Queue* queue, int element) {
    if (queue_maximized_action(queue)) return;
    if (queue->rear_ind == MAXIMUM_ELEMENTS - 1) {
        queue->rear_ind = -1;
    }
    queue->elements[++queue->rear_ind] = element;
    queue->size++;
    
}

bool queue_maximized_action(struct Queue* queue) {
    return queue->size == MAXIMUM_ELEMENTS;
}

bool queue_minimum_action(struct Queue* queue) {
    return queue->size == 0;
}

void queue_output_action(struct Queue* queue) {
    printf("[");
    for (int i = queue->front_ind; i < queue->rear_ind; i++){
        printf("%d, ", queue->elements[i]);
    }
    printf("]");
}
