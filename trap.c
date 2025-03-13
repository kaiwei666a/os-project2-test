#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt descriptor table (shared by all CPUs).
struct gatedesc idt[256];
extern uint vectors[];  // in vectors.S: array of 256 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  lidt(idt, sizeof(idt));
}




//PAGEBREAK: 41
// void
// trap(struct trapframe *tf)
// {
//   if(tf->trapno == T_SYSCALL){
//     if(myproc()->killed)
//       exit();
//     myproc()->tf = tf;
//     syscall();
//     if(myproc()->killed)
//       exit();
//     return;
//   }

//   switch(tf->trapno){
//   case T_IRQ0 + IRQ_TIMER:
//     if(cpuid() == 0){
//       acquire(&tickslock);
//       ticks++;
//       wakeup(&ticks);
//       release(&tickslock);
//     }
//     if(myproc() && myproc()->state == RUNNING) {
//       myproc()->runticks++;
//     }
//     lapiceoi();
//     break;
//   case T_IRQ0 + IRQ_IDE:
//     ideintr();
//     lapiceoi();
//     break;
//   case T_IRQ0 + IRQ_IDE+1:
//     // Bochs generates spurious IDE1 interrupts.
//     break;
//   case T_IRQ0 + IRQ_KBD:
//     kbdintr();
//     lapiceoi();
//     break;
//   case T_IRQ0 + IRQ_COM1:
//     uartintr();
//     lapiceoi();
//     break;
//   case T_IRQ0 + 7:
//   case T_IRQ0 + IRQ_SPURIOUS:
//     cprintf("cpu%d: spurious interrupt at %x:%x\n",
//             cpuid(), tf->cs, tf->eip);
//     lapiceoi();
//     break;

//   //PAGEBREAK: 13
//   default:
//     if(myproc() == 0 || (tf->cs&3) == 0){
//       // In kernel, it must be our mistake.
//       cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
//               tf->trapno, cpuid(), tf->eip, rcr2());
//       panic("trap");
//     }
//     // In user space, assume process misbehaved.
//     cprintf("pid %d %s: trap %d err %d on cpu %d "
//             "eip 0x%x addr 0x%x--kill proc\n",
//             myproc()->pid, myproc()->name, tf->trapno,
//             tf->err, cpuid(), tf->eip, rcr2());
//     myproc()->killed = 1;
//   }

//   // Force process exit if it has been killed and is in user space.
//   // (If it is still executing in the kernel, let it keep running
//   // until it gets to the regular system call return.)
//   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
//     exit();

//   // Force process to give up CPU on clock tick.
//   // If interrupts were on while locks held, would need to check nlock.
//   if(myproc() && myproc()->state == RUNNING &&
//      tf->trapno == T_IRQ0+IRQ_TIMER)
//     yield();

//   // Check if the process has been killed since we yielded
//   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
//     exit();
// }


void trap(struct trapframe *tf)
{
  // 🔹 1. 打印陷阱号，查看 CPU 是否进入 trap 处理
  cprintf("Trap: CPU %d, trapno=%d, eip=0x%x, cs=0x%x\n", cpuid(), tf->trapno, tf->eip, tf->cs);

  if(tf->trapno == T_SYSCALL){
    if(myproc()->killed) {
      cprintf("Trap: Process %d killed before syscall\n", myproc()->pid);
      exit();
    }
    myproc()->tf = tf;
    syscall();
    if(myproc()->killed) {
      cprintf("Trap: Process %d killed after syscall\n", myproc()->pid);
      exit();
    }
    return;
  }

  switch(tf->trapno){
  case T_IRQ0 + IRQ_TIMER:
    if(cpuid() == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    if(myproc() && myproc()->state == RUNNING) {
      myproc()->runticks++;
    }
    cprintf("Trap: Timer interrupt on CPU %d, process %d\n", cpuid(), myproc() ? myproc()->pid : -1);
    lapiceoi();
    break;

  case T_IRQ0 + IRQ_IDE:
    cprintf("Trap: IDE interrupt on CPU %d\n", cpuid());
    ideintr();
    lapiceoi();
    break;

  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;

  case T_IRQ0 + IRQ_KBD:
    cprintf("Trap: Keyboard interrupt on CPU %d\n", cpuid());
    kbdintr();
    lapiceoi();
    break;

  case T_IRQ0 + IRQ_COM1:
    cprintf("Trap: COM1 interrupt on CPU %d\n", cpuid());
    uartintr();
    lapiceoi();
    break;

  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x:%x\n", cpuid(), tf->cs, tf->eip);
    lapiceoi();
    break;

  default:
    if(myproc() == 0 || (tf->cs & 3) == 0){
      // 内核模式下的 trap，可能是严重错误
      cprintf("Trap: Unexpected kernel trap %d from CPU %d eip=0x%x (halting)\n",
              tf->trapno, cpuid(), tf->eip);
      panic("trap");
    }
    
    cprintf("Trap: Process %d received unexpected trap %d at eip=0x%x\n",
            myproc()->pid, tf->trapno, tf->eip);
    
    myproc()->killed = 1;
  }

  // 🔹 2. 如果进程被 kill，则退出
  if(myproc() && myproc()->killed && (tf->cs & 3) == 3){
    cprintf("Trap: Process %d exiting due to kill flag\n", myproc()->pid);
    exit();
  }

  // 🔹 3. 如果进程进入 `RUNNABLE` 状态，触发调度
  if(myproc() && myproc()->state == RUNNABLE) {
    cprintf("Trap: Process %d yielding to scheduler\n", myproc()->pid);
    yield();
  }
}

