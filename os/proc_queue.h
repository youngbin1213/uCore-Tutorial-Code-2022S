#include "proc.h"
#ifndef PROC_proc_queue_H
#define PROC_proc_queue_H
#define PROC_QUEUE_SIZE (1024)

// TODO: change the proc_queue to a priority proc_queue sorted by priority

struct proc_queue {
	int data[PROC_QUEUE_SIZE];
	int size;
	int empty;
};

void init_proc_queue(struct proc_queue *);
void push_proc_queue(struct proc_queue *, int);
int pop_proc_queue(struct proc_queue *,struct proc*);

#endif // PROC_proc_queue_H