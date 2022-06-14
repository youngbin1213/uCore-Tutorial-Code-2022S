#ifndef VM_H
#define VM_H

#include "riscv.h"
#include "types.h"

void kvm_init(void);
void kvmmap(pagetable_t, uint64, uint64, uint64, int);
int mappages(pagetable_t, uint64, uint64, uint64, int);
pagetable_t uvmcreate(void);
void uvmfree(pagetable_t, uint64);
void uvmunmap(pagetable_t, uint64, uint64, int);
uint64 walkaddr(pagetable_t, uint64);
uint64 useraddr(pagetable_t, uint64);
int copyout(pagetable_t, uint64, char *, uint64);
int copyin(pagetable_t, char *, uint64, uint64);
int copyinstr(pagetable_t, char *, uint64, uint64);
int anommap(void*start,uint64 len,int port,int flag,int fd);

// user mmap and 
uint64 usermmap(pagetable_t pagetable,void*start,uint64 len,int port,int flag,int fd);
uint64 mmunmap(pagetable_t pagetable,void*start,uint len);
#endif // VM_H