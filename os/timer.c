#include "timer.h"
#include "riscv.h"
#include "sbi.h"

/// read the `mtime` regiser
uint64 get_cycle()
{
	return r_time();
}

/// Enable timer interrupt
void timer_init()
{
	// Enable supervisor timer interrupt
	w_sie(r_sie() | SIE_STIE);
	set_next_timer();
}

/// Set the next timer interrupt
void set_next_timer()
{
	const uint64 timebase = CPU_FREQ / TICKS_PER_SEC;
	set_timer(get_cycle() + timebase);
}
// kernel get time function

void kgettimeofday(TimeVal*val,int _ta){

	uint64 cycle = get_cycle();

	val->sec = cycle / CPU_FREQ;
	val->usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;

}