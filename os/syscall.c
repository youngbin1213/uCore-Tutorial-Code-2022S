#include "syscall.h"
#include "console.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"
#include "proc.h"
#include "vm.h"

uint64 console_write(uint64 va, uint64 len)
{
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	int size = copyinstr(p->pagetable, str, va, MIN(len, MAX_STR_LEN));
	tracef("write size = %d", size);
	for (int i = 0; i < size; ++i) {
		console_putchar(str[i]);
	}
	return len;
}

uint64 console_read(uint64 va, uint64 len)
{
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	tracef("read size = %d", len);
	for (int i = 0; i < len; ++i) {
		int c = consgetc();
		str[i] = c;
	}
	copyout(p->pagetable, va, str, len);
	return len;
}

uint64 sys_write(int fd, uint64 va, uint64 len)
{
	if (fd < 0 || fd > FD_BUFFER_SIZE)
		return -1;
	struct proc *p = curr_proc();
	struct file *f = p->files[fd];
	if (f == NULL) {
		errorf("invalid fd %d\n", fd);
		return -1;
	}
	switch (f->type) {
	case FD_STDIO:
		return console_write(va, len);
	case FD_INODE:
		return inodewrite(f, va, len);
	default:
		panic("unknown file type %d\n", f->type);
	}
}

uint64 sys_read(int fd, uint64 va, uint64 len)
{
	if (fd < 0 || fd > FD_BUFFER_SIZE)
		return -1;
	struct proc *p = curr_proc();
	struct file *f = p->files[fd];
	if (f == NULL) {
		errorf("invalid fd %d\n", fd);
		return -1;
	}
	switch (f->type) {
	case FD_STDIO:
		return console_read(va, len);
	case FD_INODE:
		return inoderead(f, va, len);
	default:
		panic("unknown file type %d\n", f->type);
	}
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

uint64 sys_gettimeofday(uint64 val, int _tz)
{
	struct proc *p = curr_proc();
	uint64 cycle = get_cycle();
	TimeVal t;
	t.sec = cycle / CPU_FREQ;
	t.usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	copyout(p->pagetable, val, (char *)&t, sizeof(TimeVal));
	return 0;
}

uint64 sys_getpid()
{
	return curr_proc()->pid;
}

uint64 sys_getppid()
{
	struct proc *p = curr_proc();
	return p->parent == NULL ? IDLE_PID : p->parent->pid;
}

uint64 sys_clone()
{
	debugf("fork!");
	return fork();
}

static inline uint64 fetchaddr(pagetable_t pagetable, uint64 va)
{
	uint64 *addr = (uint64 *)useraddr(pagetable, va);
	return *addr;
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
	if(ret==0){
		// uint64 npage = (len+PAGE_SIZE-1)/PAGE_SIZE;
		// reset max page  ,max page does not mean how many pages os has alloced to app ,
		// but mean the max virtual address's page num
		if(curr_proc()->max_page < (((uint64)start+len+PAGE_SIZE-1)/PAGE_SIZE)){
			curr_proc()->max_page = ((uint64)start+len+PAGE_SIZE-1)/PAGE_SIZE;
		}
	}
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

uint64 sys_exec(uint64 path,uint64 uargv)
{
	struct proc *p = curr_proc();
	char name[MAX_STR_LEN];
	copyinstr(p->pagetable, name, path, MAX_STR_LEN);
	uint64 arg;
	static char strpool[MAX_ARG_NUM][MAX_STR_LEN];
	char *argv[MAX_ARG_NUM];
	int i;
	for (i = 0; uargv && (arg = fetchaddr(p->pagetable, uargv));
	     uargv += sizeof(char *), i++) {
		copyinstr(p->pagetable, (char *)strpool[i], arg, MAX_STR_LEN);
		argv[i] = (char *)strpool[i];
	}
	argv[i] = NULL;
	return exec(name, (char **)argv);
}

uint64 sys_wait(int pid, uint64 va)
{
	struct proc *p = curr_proc();
	int *code = (int *)useraddr(p->pagetable, va);
	return wait(pid, code);
}

uint64 sys_spawn(uint64 va)
{
	// TODO: your job is to complete the sys call
	// va but here is kernel!!!
	
	// printf("sys_spawn:name is !! id is  %s",va);

	return kspawn(va);
}
uint64 sys_openat(uint64 va, uint64 omode, uint64 _flags)
{
	struct proc *p = curr_proc();
	char path[200];
	copyinstr(p->pagetable, path, va, 200);
	return fileopen(path, omode);
}
uint64 sys_set_priority(long long prio){
    // TODO: your job is to complete the sys call
	if(prio<2)return -1;
	curr_proc()->pro_level = (int)prio ; 
	// printf("now pro level is %d\n",curr_proc()->pro_level);
    return -1;
}

uint64 sys_close(int fd)
{
	if (fd < 0 || fd > FD_BUFFER_SIZE)
		return -1;
	struct proc *p = curr_proc();
	struct file *f = p->files[fd];
	if (f == NULL) {
		errorf("invalid fd %d", fd);
		return -1;
	}
	fileclose(f);
	p->files[fd] = 0;
	return 0;
}

int sys_fstat(int fd,uint64 stat){
	//TODO: your job is to complete the syscall
	// notice stat is a virtual address!!
	struct proc *p = curr_proc();
	struct file *f = p->files[fd];
	struct inode *ip;
	
	if(f==NULL || f->ip==NULL ||f->ip==0){
		errorf("invalid fd %d", fd);
		return -1;
	}
	// copy to stat
	struct Stat file_stat ;
	ip = f->ip;
	ivalid(ip);
	file_stat.dev = ip->dev;
	file_stat.ino =ip->inum;
	if(ip->type==T_DIR)
		file_stat.mode = DIR;
	else
		file_stat.mode = FILE;
	// should not ref but linkcount
	file_stat.nlink = ip->linkcount;

	copyout(p->pagetable, stat, (char*)&file_stat, sizeof(file_stat));
	return 0;
}

uint64 sys_linkat(int olddirfd, uint64 oldpath, int newdirfd, uint64 newpath, uint64 flags){
	//TODO: your job is to complete the syscall
	// oldpath and newpath are all virtual address
	// link at essentially link two file struct to **one** inode!! 

	if(oldpath==newpath)return -1;
	struct proc *p = curr_proc();

	char old_path_str[MAX_STR_LEN];
	char new_path_str[MAX_STR_LEN];
	copyinstr(p->pagetable, old_path_str, oldpath, MAX_STR_LEN);
	copyinstr(p->pagetable, new_path_str, newpath, MAX_STR_LEN);

	return hdlink( olddirfd, old_path_str,  newdirfd,new_path_str,  flags);

	
}

uint64 sys_unlinkat(int dirfd, uint64 name, uint64 flags){
	//TODO: your job is to complete the syscall
	char path[MAX_STR_LEN];
	struct proc *p = curr_proc();

	copyinstr(p->pagetable, path, name, MAX_STR_LEN);
	

	return hdunlink( dirfd,path,flags);
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
	case SYS_read:
		ret = sys_read(args[0], args[1], args[2]);
		break;
	case SYS_openat:
		ret = sys_openat(args[0], args[1], args[2]);
		break;
	case SYS_close:
		ret = sys_close(args[0]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
		// __builtin_unreachable();
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday(args[0], args[1]);
		break;
	case SYS_getpid:
		ret = sys_getpid();
		break;
	case SYS_getppid:
		ret = sys_getppid();
		break;
	case SYS_clone: // SYS_fork
		ret = sys_clone();
		break;
	case SYS_execve:
		ret = sys_exec(args[0], args[1]);
		break;
	case SYS_wait4:
		ret = sys_wait(args[0], args[1]);
		break;
	case SYS_fstat:
	    ret = sys_fstat(args[0],args[1]);
		break;
	case SYS_linkat:
	    ret = sys_linkat(args[0],args[1],args[2],args[3],args[4]);
		break;
	case SYS_unlinkat:
	    ret = sys_unlinkat(args[0],args[1],args[2]);
		break;
	case SYS_spawn:
		ret = sys_spawn(args[0]);
		break;
	case SYS_setpriority:
		ret = sys_set_priority((long long)args[0]);
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
