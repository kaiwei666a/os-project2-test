#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "random.h"
extern int uptime(void);




struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

// Performance metrics storage
struct PerfData perf_data[NPROC];
struct spinlock perf_lock;  // Lock for accessing perf_data

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);



static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  initlock(&perf_lock, "perfdata");
  
  // Initialize performance data
  for(int i = 0; i < NPROC; i++) {
    perf_data[i].valid = 0;
  }
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
// static struct proc*
// allocproc(void)
// {
//   struct proc *p;
//   char *sp;

//   acquire(&ptable.lock);

//   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//     if(p->state == UNUSED)
//       goto found;

//   release(&ptable.lock);
//   return 0;

// found:
//   p->state = EMBRYO;
//   p->pid = nextpid++;
//   p->runticks = 0;
//   p->tickets = DEFAULT_TICKETS;
//   release(&ptable.lock);

//   // Allocate kernel stack.
//   if((p->kstack = kalloc()) == 0){
//     p->state = UNUSED;
//     return 0;
//   }
//   sp = p->kstack + KSTACKSIZE;

//   // Leave room for trap frame.
//   sp -= sizeof *p->tf;
//   p->tf = (struct trapframe*)sp;

//   // Set up new context to start executing at forkret,
//   // which returns to trapret.
//   sp -= 4;
//   *(uint*)sp = (uint)trapret;

//   sp -= sizeof *p->context;
//   p->context = (struct context*)sp;
//   memset(p->context, 0, sizeof *p->context);
//   p->context->eip = (uint)forkret;

//   return p;
// }
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED) {
      // cprintf("allocproc: Found UNUSED process slot, pid=%d\n", nextpid);
      goto found;
    }
  }

  release(&ptable.lock);
  // cprintf("allocproc: No UNUSED process found\n");
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->runticks = 0;
  p->tickets = DEFAULT_TICKETS;
  p->creation_time = ticks;
  p->start_time = 0;
  p->completion_time = 0;
  p->total_run_time = 0;
  p->total_ready_time = 0;
  p->total_sleep_time = 0;
  p->num_run = 0;
  p->priority = 0;
  // cprintf("allocproc: Process allocated, pid=%d, state=EMBRYO\n", p->pid);

  release(&ptable.lock);

  if((p->kstack = kalloc()) == 0){
    // cprintf("allocproc: kalloc failed for process pid=%d\n", p->pid);
    acquire(&ptable.lock);
    p->state = UNUSED; 
    release(&ptable.lock);
    return 0;
  }
  // cprintf("allocproc: kstack allocated at 0x%x for pid=%d\n", p->kstack, p->pid);


  sp = p->kstack + KSTACKSIZE;


  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  memset(p->tf, 0, sizeof *p->tf);
  // cprintf("allocproc: trapframe placed at 0x%x for pid=%d\n", p->tf, p->pid);


  sp -= 4;
  *(uint*)sp = (uint)trapret;
  // cprintf("allocproc: trapret address 0x%x stored at stack for pid=%d\n", sp, p->pid);


  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context); 
  p->context->eip = (uint)forkret; 
  // cprintf("allocproc: context set, eip=0x%x, context at 0x%x for pid=%d\n",
          // p->context->eip, p->context, p->pid);

  return p;
}



//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}




// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);
  np->enqueue_time = ticks;  // Record when process enters ready queue
  np->state = RUNNABLE;
  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Finalize all metrics before doing anything else
  acquire(&ptable.lock);
  if(curproc->completion_time <= curproc->creation_time) {
    curproc->completion_time = ticks;  // Set completion time if not already set properly
  }
  if(curproc->total_run_time == 0) {
    curproc->total_run_time = 1;  // Ensure minimum run time
  }
  if(curproc->enqueue_time > 0) {
    curproc->total_ready_time += ticks - curproc->enqueue_time;
    curproc->enqueue_time = 0;
  }

  // Store metrics in historical data
  acquire(&perf_lock);
  for(int i = 0; i < NPROC; i++) {
    if(!perf_data[i].valid) {
      perf_data[i].pid = curproc->pid;
      perf_data[i].creation_time = curproc->creation_time;
      perf_data[i].start_time = curproc->start_time;
      perf_data[i].completion_time = curproc->completion_time;
      perf_data[i].total_run_time = curproc->total_run_time;
      perf_data[i].total_ready_time = curproc->total_ready_time;
      perf_data[i].valid = 1;
      break;
    }
  }
  release(&perf_lock);
  release(&ptable.lock);

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}



// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        
        // Ensure all metrics are finalized
        if(p->completion_time <= p->creation_time) {
          p->completion_time = ticks;
        }
        if(p->total_run_time == 0) {
          p->total_run_time = 1;  // Ensure minimum run time
        }
        if(p->enqueue_time > 0) {
          p->total_ready_time += ticks - p->enqueue_time;
          p->enqueue_time = 0;
        }
        
        // Store performance metrics before cleanup
        acquire(&perf_lock);
        int stored = 0;
        for(int i = 0; i < NPROC; i++) {
          if(!perf_data[i].valid) {
            perf_data[i].pid = p->pid;
            perf_data[i].creation_time = p->creation_time;
            perf_data[i].start_time = p->start_time;
            perf_data[i].completion_time = p->completion_time;
            perf_data[i].total_run_time = p->total_run_time;
            perf_data[i].total_ready_time = p->total_ready_time;
            perf_data[i].valid = 1;
            stored = 1;
            break;
          }
        }
        release(&perf_lock);

        // If we couldn't store metrics, wait and try again
        if(!stored) {
          continue;
        }

        // Clean up process
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;

        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
// void
// scheduler(void)
// {
//   struct proc *p;
//   struct cpu *c = mycpu();
//   c->proc = 0;
  
//   for(;;){
//     // Enable interrupts on this processor.
//     sti();

//     // Loop over process table looking for process to run.
//     acquire(&ptable.lock);
//     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//       if(p->state != RUNNABLE)
//         continue;

//       // Switch to chosen process.  It is the process's job
//       // to release ptable.lock and then reacquire it
//       // before jumping back to us.
//       c->proc = p;
//       switchuvm(p);
//       p->state = RUNNING;

//       swtch(&(c->scheduler), p->context);
//       switchkvm();

//       // Process is done running for now.
//       // It should have changed its p->state before coming back.
//       c->proc = 0;
//     }
//     release(&ptable.lock);

//   }
// }





void scheduler(void)
{
#ifdef SCHEDULER_LOTTERY
    struct proc *p;
    struct cpu *c = mycpu();
    int total_tickets;
    int winning_ticket;

    c->proc = 0;

    for(;;) {
        sti();
        acquire(&ptable.lock);
        total_tickets = 0;

        // Update metrics for all processes first
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            // Update completion time for any process that's done
            if((p->state == ZOMBIE || p->state == UNUSED) && p->completion_time <= p->creation_time) {
                p->completion_time = ticks;
                if(p->total_run_time == 0) {
                    p->total_run_time = 1;  // Ensure minimum run time
                }
                // Store metrics immediately for completed processes
                acquire(&perf_lock);
                for(int i = 0; i < NPROC; i++) {
                    if(!perf_data[i].valid) {
                        perf_data[i].pid = p->pid;
                        perf_data[i].creation_time = p->creation_time;
                        perf_data[i].start_time = p->start_time;
                        perf_data[i].completion_time = p->completion_time;
                        perf_data[i].total_run_time = p->total_run_time;
                        perf_data[i].total_ready_time = p->total_ready_time;
                        perf_data[i].valid = 1;
                        break;
                    }
                }
                release(&perf_lock);
            }
            // Update ready time for RUNNABLE processes
            if(p->state == RUNNABLE && p->enqueue_time > 0) {
                p->total_ready_time += ticks - p->enqueue_time;
                p->enqueue_time = ticks;  // Reset enqueue time to prevent double counting
            }
        }

        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->state == RUNNABLE)
                total_tickets += p->tickets;
        }

        if(total_tickets > 0){
            winning_ticket = get_random(0, total_tickets);
            int current_count = 0;
            for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
                if(p->state != RUNNABLE)
                    continue;
                current_count += p->tickets;
                if(current_count > winning_ticket)
                    break;
            }

            if(p->state == RUNNABLE) {
                c->proc = p;
                switchuvm(p);
                p->state = RUNNING;

                // Update metrics before running
                if(p->start_time == 0 && p->total_run_time == 0) {
                    // Only set start_time if this is the first time the process runs
                    p->start_time = ticks;
                }
                if(p->enqueue_time > 0) {
                    p->total_ready_time += ticks - p->enqueue_time;
                    p->enqueue_time = 0;  // Clear enqueue time while running
                }
                uint run_start = ticks;

                swtch(&(c->scheduler), p->context);
                switchkvm();

                // Update metrics after running
                uint run_time = ticks - run_start;
                // Always count at least 1 tick for any process that gets CPU time
                if(run_time == 0) {
                    run_time = 1;
                }
                p->total_run_time += run_time;
                p->runticks += run_time;

                // Update completion time if process is done
                if(p->state == ZOMBIE || p->state == UNUSED) {
                    p->completion_time = ticks;
                    // Store metrics immediately
                    acquire(&perf_lock);
                    for(int i = 0; i < NPROC; i++) {
                        if(!perf_data[i].valid) {
                            perf_data[i].pid = p->pid;
                            perf_data[i].creation_time = p->creation_time;
                            perf_data[i].start_time = p->start_time;
                            perf_data[i].completion_time = p->completion_time;
                            perf_data[i].total_run_time = p->total_run_time;
                            perf_data[i].total_ready_time = p->total_ready_time;
                            perf_data[i].valid = 1;
                            break;
                        }
                    }
                    release(&perf_lock);
                }
                
                c->proc = 0;
            }
        }
        release(&ptable.lock);
    }
#elif defined(SCHEDULER_FIFO)
    struct proc *p;
    struct cpu *c = mycpu();
    c->proc = 0;

    for (;;) {
        sti();
        acquire(&ptable.lock);
        p = ptable.head;
        
        if (p && p->state == RUNNABLE) {
            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;

            // Update metrics
            if (p->enqueue_time > 0) {
                p->total_ready_time += ticks - p->enqueue_time;
            }
            if(p->start_time == 0) {
                p->start_time = ticks;
            }
            uint run_start = ticks;

            swtch(&(c->scheduler), p->context);
            switchkvm();

            p->total_run_time += ticks - run_start;
            c->proc = 0;

            if (p->state == ZOMBIE) {
                if(p->next && p->next->state == RUNNABLE) {
                    ptable.head = p->next;
                }
            }
        } else if(p && p->next && p->next->state == RUNNABLE) {
            ptable.head = p->next;
        }
        release(&ptable.lock);
    }
#else
    struct proc *p;
    struct cpu *c = mycpu();
    c->proc = 0;

    for(;;) {
        sti();
        acquire(&ptable.lock);

        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if(p->state != RUNNABLE)
                continue;

            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;
            
            // Update metrics
            if (p->enqueue_time > 0) {
                p->total_ready_time += ticks - p->enqueue_time;
            }
            if(p->start_time == 0) {
                p->start_time = ticks;
            }
            uint run_start = ticks;
            
            swtch(&(c->scheduler), p->context);
            switchkvm();
            
            p->total_run_time += ticks - run_start;
            c->proc = 0;
        }
        release(&ptable.lock);
    }
#endif
  }







// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&ptable.lock);
  p->state = RUNNABLE;
  p->enqueue_time = ticks;  // Record when process enters ready queue
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Update metrics before sleeping
  if(p->enqueue_time > 0) {
    p->total_ready_time += ticks - p->enqueue_time;
    p->enqueue_time = 0;
  }
  
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan){
       p->state = RUNNABLE;
       p->enqueue_time = ticks;
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}



int getrunticks(int pid) {
  struct proc *p;
  int val = -1;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) {
      // 确保进程状态有效，防止访问无效内存
      if (p->state != ZOMBIE && p->state != UNUSED) {
        val = p->runticks;
      }
      break;
    }
  }
  release(&ptable.lock);
  return val;
}



int
sys_ticks_run(void)
{
  int pid;

  if(argint(0, &pid) < 0){
    return -1;
  }


  return getrunticks(pid);
}

int
sys_set_tickets(void) {
  int t;
  if (argint(0, &t) < 0)
    return -1;

  if (t <= 0) {
    return -1;
  }

  struct proc* p = myproc();
  if (p == 0) {  // 确保进程有效
    return -1;
  }

  acquire(&ptable.lock);
  p->tickets = t;
  release(&ptable.lock);
  return 0;
}


int gettickets(int pid) {
  struct proc *p;
  int tickets_val = -1;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) { 
      // 确保状态是 RUNNABLE, RUNNING，并且进程不是 ZOMBIE
      if (p->state == RUNNABLE || p->state == RUNNING) {
        tickets_val = p->tickets;
      }
      break;
    }
  }
  release(&ptable.lock);
  return tickets_val;
}



int
sys_get_tickets(void)
{
  int pid;
  if(argint(0, &pid) < 0)
    return -1;
  return gettickets(pid);
}


int job_position(int pid) {
  struct proc *p;
  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if (p->pid == pid) {
          release(&ptable.lock);
          return p->pid;  
      }
  }
  release(&ptable.lock);
  return -1;  
}

int
get_creation_time(int pid)
{
  struct proc *p;
  int creation_time = -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid) {
      creation_time = p->creation_time;
      break;
    }
  }
  release(&ptable.lock);
  
  if(creation_time == -1) {
    // Look in historical data
    acquire(&perf_lock);
    for(int i = 0; i < NPROC; i++) {
      if(perf_data[i].valid && perf_data[i].pid == pid) {
        creation_time = perf_data[i].creation_time;
        break;
      }
    }
    release(&perf_lock);
  }
  
  return creation_time;
}

int
get_start_time(int pid)
{
  struct proc *p;
  int start_time = -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid) {
      start_time = p->start_time;
      break;
    }
  }
  release(&ptable.lock);
  
  if(start_time == -1) {
    // Look in historical data
    acquire(&perf_lock);
    for(int i = 0; i < NPROC; i++) {
      if(perf_data[i].valid && perf_data[i].pid == pid) {
        start_time = perf_data[i].start_time;
        break;
      }
    }
    release(&perf_lock);
  }
  
  return start_time;
}

int
get_completion_time(int pid)
{
  struct proc *p;
  int completion_time = -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid) {
      completion_time = p->completion_time;
      break;
    }
  }
  release(&ptable.lock);
  
  if(completion_time == -1) {
    // Look in historical data
    acquire(&perf_lock);
    for(int i = 0; i < NPROC; i++) {
      if(perf_data[i].valid && perf_data[i].pid == pid) {
        completion_time = perf_data[i].completion_time;
        break;
      }
    }
    release(&perf_lock);
  }
  
  return completion_time;
}

int
get_total_run_time(int pid)
{
  struct proc *p;
  int total_run_time = -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid) {
      total_run_time = p->total_run_time;
      break;
    }
  }
  release(&ptable.lock);
  
  if(total_run_time == -1) {
    // Look in historical data
    acquire(&perf_lock);
    for(int i = 0; i < NPROC; i++) {
      if(perf_data[i].valid && perf_data[i].pid == pid) {
        total_run_time = perf_data[i].total_run_time;
        break;
      }
    }
    release(&perf_lock);
  }
  
  return total_run_time;
}

int
get_total_ready_time(int pid)
{
  struct proc *p;
  int total_ready_time = -1;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->pid == pid) {
      total_ready_time = p->total_ready_time;
      break;
    }
  }
  release(&ptable.lock);
  
  if(total_ready_time == -1) {
    // Look in historical data
    acquire(&perf_lock);
    for(int i = 0; i < NPROC; i++) {
      if(perf_data[i].valid && perf_data[i].pid == pid) {
        total_ready_time = perf_data[i].total_ready_time;
        break;
      }
    }
    release(&perf_lock);
  }
  
  return total_ready_time;
}

