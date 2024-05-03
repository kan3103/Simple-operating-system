#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t * q) {
        if (q == NULL) return 1;
	return (q->size == 0);
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
        /* TODO: put a new process to queue [q] */
        if (q->size == MAX_QUEUE_SIZE)
                return;

        if (q->size == 0)
        {
                q->proc[0] = proc;
        }
        else{
                int i;
                for (i = q->size; i > 0 && proc->priority >= q->proc[i - 1]->priority; i--)
                {
                        q->proc[i] = q->proc[i - 1];
                } 
                q->proc[i] = proc;
        }
        q->size++;
}

struct pcb_t * dequeue(struct queue_t * q) {
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (empty(q))
                return NULL;
        if(q->size>0){
                struct pcb_t * temp = q->proc[q->size-1];
                q->proc[q->size-1]=NULL;
                q->size--;
                return temp;
        }
	return NULL;
}

