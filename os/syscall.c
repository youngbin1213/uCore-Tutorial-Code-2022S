#include "syscall.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"
#include "proc.h"
#include "vm.h"

uint64 sys_write(int fd, uint64 va, uint len)
{
	// debugf("sys_write fd = %d va = %x, len = %d", fd, va, len);
	if (fd != STDOUT)
		return -1;
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	int size = copyinstr(p->pagetable, str, va, MIN(len, MAX_STR_LEN));
	// debugf("size = %d", size);
	for (int i = 0; i < size; ++i) {
		console_putchar(str[i]);
	}
	return size;
}

__attribute__((noreturn)) void sys_exit(int code)
{
	exit(code);
	__builtin_unreachable();
}

uint64 sys_sched_yield()
{
	yield();
	return 0;
}

uint64 sys_gettimeofday(TimeVal *val, int _tz) // TODO: implement sys_gettimeofday in pagetable. (VA to PA)
{
	// YOUR CODE
	// in systemcall val is va ,if kernel write to va ,get wrong
	uint64 cycle = get_cycle();
	TimeVal tmp_val;
	tmp_val.sec = cycle / CPU_FREQ;
	tmp_val.usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	// debugf("get cycle success ,sec is %d,usec is %d",cycle / CPU_FREQ,(cycle % CPU_FREQ) * 1000000 / CPU_FREQ);
	// val->sec = 0;
	// val->usec = 0;
	// Essentially variable val ,a TimeVal pointer is actuall address ,so turn to (uint64)type 
	// will not trigger an exception
	uint64 ret = copyout(curr_proc()->pagetable,(uint64)val,(char*)&tmp_val,sizeof(tmp_val));
	/* The code in `ch3` will leads to memory bugs*/

	// uint64 cycle = get_cycle();
	// val->sec = cycle / CPU_FREQ;
	// val->usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	return ret;
}

// TODO: add support for mmap and munmap syscall.
// hint: read through docstrings in vm.c. Watching CH4 video may also help.
// Note the return value and PTE flags (especially U,X,W,R)
/*
* LAB1: you may need to define sys_task_info here
*/
uint64 sys_mmap(void*start,uint64 len,int port,int flag,int fd){

	// should return va address!!!
	// input start is only a key !!!!!
	uint64 ret = usermmap(curr_proc()->pagetable,start, len, port, flag, fd);
	return ret;
}

uint64 sys_task_info(TaskInfo*cur_task_info,TaskInfo* aim_task_info){
	// va to pa
	return copyout(curr_proc()->pagetable,(uint64)aim_task_info,(char*)cur_task_info,sizeof(TaskInfo));
	// aim_task_info->time = cur_task_info->time;
	// aim_task_info->status = cur_task_info->status;
	// for(int i=0;i<MAX_SYSCALL_NUM;i++){
	// 	aim_task_info->syscall_times[i] = cur_task_info->syscall_times[i];

	// }
}

uint64 sys_count_taskinfo(TaskInfo*cur_task_info,TimeVal*start_time,int id){
	if(id>MAX_SYSCALL_NUM){
		errorf("id %d over SYSCALL_NUM %d",id,MAX_SYSCALL_NUM);
	}

	cur_task_info->syscall_times[id] +=1;
	uint64 cycle = get_cycle();
	uint64 ms = cycle / CPU_FREQ*1000-start_time->sec*1000 + (cycle % CPU_FREQ) * 1000 / CPU_FREQ - start_time->usec/1000;
	cur_task_info->time = ms;

	return 0;

}

uint64 sys_munmap(void*start,uint64 len){

	uint64 ret = mmunmap(curr_proc()->pagetable,start,len);
	return ret;
}

extern char trap_page[];

void syscall()
{
	struct trapframe *trapframe = curr_proc()->trapframe;
	int id = trapframe->a7, ret;
	uint64 args[6] = { trapframe->a0, trapframe->a1, trapframe->a2,
			   trapframe->a3, trapframe->a4, trapframe->a5 };
	tracef("syscall %d args = [%x, %x, %x, %x, %x, %x]", id, args[0],
	       args[1], args[2], args[3], args[4], args[5]);
	/*
	* LAB1: you may need to update syscall counter for task info here
	*/
	TaskInfo *cur_task_info = curr_proc()->task_info;
	//TODO count task info
	TimeVal* start_time = curr_proc()->start_time;
	sys_count_taskinfo(cur_task_info,start_time,id);

	switch (id) {
	case SYS_write:
		ret = sys_write(args[0], args[1], args[2]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
		// __builtin_unreachable();
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday((TimeVal *)args[0], args[1]);
		break;
	case SYSCALL_TASK_INFO:
		ret = sys_task_info(cur_task_info,(TaskInfo*)args[0]);
		break;
	case SYS_mmap:
		ret = sys_mmap((void*)args[0],args[1],(int)args[2],(int)args[3],(int)args[4]);
		break;
	/*
	* LAB1: you may need to add SYS_taskinfo case here
	*/
	case SYS_munmap:
		// ret =0;
		ret = sys_munmap((void*)args[0],args[1]);
		break;
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}
	trapframe->a0 = ret;
	tracef("syscall ret %d", ret);
}
