#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

extern uint64 count_mem(void);
extern uint64 count_proc(void);

uint64
sys_sysinfo(void)
{
  struct proc *my_proc = myproc();
  uint64 st; // user pointer to struct stat

  argaddr(0, &st);

  struct sysinfo p1;
  p1.freemem = count_mem() * 4096;
  p1.nproc = count_proc();

  if (copyout(my_proc->pagetable, st, (char *)&p1, sizeof(p1)) < 0)
    return -1;
  return 0;
}