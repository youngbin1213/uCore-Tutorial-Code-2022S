#include "proc_queue.h"
#include "defs.h"
#include "proc.h"

void init_proc_queue(struct proc_queue*q){

    q->size = 0;
    q->empty=1;
}

void push_proc_queue(struct proc_queue *q,int value){

    if(!q->empty && q->size >PROC_QUEUE_SIZE){
		panic("proc queue shouldn't be overflow");

    }
	q->empty = 0;
	q->data[q->size] = value;
    q->size++;
}



int pop_proc_queue(struct proc_queue*q,struct proc* pool){

    if(q->empty)return -1;
    struct proc* tmp_proc;

    int min_value = __INT_MAX__;
    int min_value_idx = -1;
    for(int i=0;i<q->size;i++){
        tmp_proc = &pool[q->data[i]];
        if((tmp_proc->stride)<min_value){
            min_value = tmp_proc->stride;
            min_value_idx = i;
        }

    }

    //translate to the end and pop
    min_value = q->data[min_value_idx];
    q->data[min_value_idx] = q->data[(q->size)-1] ;
    q->size--;
    if(q->size==0)q->empty=1;
    return min_value;
}



