#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 15

typedef struct queue {

    int first,
            last,
            Q[SIZE],
            size;

} queue;

queue *new_queue() {

    queue *queue = malloc(sizeof(struct queue));

    queue->first = -1;
    queue->last = -1;
    queue->size = SIZE;

    return queue;
}

bool isEmpty(queue *queue) {

    return (queue->first == -1 && queue->last == -1);
}

bool isFull(queue *queue) {

    return ((queue->last + 1) % SIZE == queue->first);
}

int dequeue(queue *queue) {

    int to_return;
    if (queue->first == queue->last) {
        to_return = queue->Q[queue->first];
        queue->first = -1;
        queue->last = -1;

    } else {
        to_return = queue->Q[queue->first];
        queue->first = (queue->first + 1) % SIZE;

    }
    return to_return;
}

void enqueue(int number_to_enqueue, queue *queue) {

    if (queue->first == -1) {

        queue->first = 0;

    }

    queue->last = (queue->last + 1) % SIZE;
    (queue->Q)[queue->last] = number_to_enqueue;
}

void print_queue(queue *queue) {

    printf("Queue: ");

    if (queue->first <= queue->last) {

        for (int i = queue->first; i <= queue->last; i++) {

            printf("%d ", (queue->Q)[i]);
        }

        printf("\n");

    } else {

        for (int i = queue->first; i <= SIZE; i++) {

            printf("%d", (queue->Q)[i]);
        }

        for (int i = 0; i <= queue->last; i++) {

            printf("%d", (queue->Q)[i]);
        }

        printf("\n");
    }
}

//int main(void) {
//
//    queue *queue = new_queue();
//
//    print_queue(queue);
//
//    enqueue(5, queue);
//    enqueue(6, queue);
//    enqueue(4, queue);
//
//    print_queue(queue);
//
//    dequeue(queue);
//
//    print_queue(queue);
//
//    enqueue(90, queue);
//
//    print_queue(queue);
//
//
//}