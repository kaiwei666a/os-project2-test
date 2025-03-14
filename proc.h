// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

#define NPROC        64  // maximum number of processes

// Performance metrics structure
struct PerfData {
  int pid;
  int creation_time;
  int start_time;
  int completion_time;
  int total_run_time;
  int total_ready_time;
  int valid;  // Flag to indicate if this entry contains valid data
};

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;          // Parent process
  struct trapframe *tf;         // Trap frame for current syscall
  struct context *context;      // swtch() here to run process
  void *chan;                   // If non-zero, sleeping on chan
  int killed;                   // If non-zero, have been killed
  int runticks;                 // Number of ticks this process has run
  struct file *ofile[NOFILE];   // Open files
  struct inode *cwd;             // Current directory
  char name[16];                 // Process name (debugging)
  int tickets;                   // Number of tickets for lottery scheduling
  int enqueue_time;              // Time when process was last enqueued
  
  // Performance metrics
  int creation_time;            // Time when process was created
  int start_time;                // Time when process first started running
  int completion_time;            // Time when process completed
  int total_run_time;             // Total time spent running
  int total_ready_time;           // Total time spent ready
  int total_sleep_time;            // Total time spent sleeping
  int num_run;                     // Number of times scheduled
  int priority;                     // Priority level
};

// Performance metrics storage
extern struct PerfData perf_data[NPROC];

// // Process management functions
// struct proc *myproc(void);
// void exit(void);
// int fork(void);
// int growproc(int);
// int kill(int);
// int sleep(void*, struct spinlock*);
// int wait(void);
// void wakeup(void*);

// // Job position function declaration
// int job_position(int pid);


// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
