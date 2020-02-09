/*
** SCCS ID:	@(#)syscalls.c	1.1	4/2/19
**
** File:	syscalls.c
**
** Author:	CSCI-452 class of 20185
**
** Contributor:
**
** Description:	System call module implementations
*/

#define	__SP_KERNEL__

#include "common.h"

#include <x86arch.h>
#include <x86pic.h>
#include <uart.h>

#include "support.h"
#include "klib.h"

#include "syscalls.h"
#include "scheduler.h"
#include "pcbs.h"
#include "stacks.h"
#include "clock.h"
#include "cio.h"
#include "sio.h"

/*
** PRIVATE DEFINITIONS
*/

/*
** PRIVATE DATA TYPES
*/

/*
** PRIVATE GLOBAL VARIABLES
*/

///
// system call jump table
//
// initialized by _sys_init() to ensure that the
// code::function mappings are correct
///

static void (*_syscalls[N_SYSCALLS])( pcb_t * );

/*
** PUBLIC GLOBAL VARIABLES
*/

// sleeping, waiting, and zombie queues
queue_t _sleeping;  // processes catching some Zs
queue_t _waiting;   // processes waiting (for Godot?)
queue_t _zombie;    // gone, but not forgotten

/*
** PRIVATE FUNCTIONS
*/

/*
** _sys_isr(vector,code)
**
** Common handler for the system call module.  Selects
** the correct second-level routine to invoke based on
** the contents of EAX.
**
** The second-level routine is invoked with a pointer to
** the PCB for the process.  It is the responsibility of
** that routine to assign all return values for the call.
*/
static void _sys_isr( int vector, int code ) {
    
    // which one does the user want?
    uint32_t which = REG(_current,eax);
    
    // verify that it's legal - if not, it's adios, muchacho!
    if( which >= N_SYSCALLS ) {
        which = SYS_exit;
        ARG(_current,1) = E_BAD_CODE;
        __cio_printf( "pid %d bad syscall code %d\n", _current->pid, which );
    }
    
    // invoke it
    _syscalls[which]( _current );
    
    // notify the PIC
    __outb( PIC_MASTER_CMD_PORT, PIC_EOI );
}

/*
** Second-level syscall handlers
**
** All have this prototype:
**
**    static void _sys_NAME( pcb_t * );
**
** Values being returned to the user are placed into the EAX
** field in the context save area of the provided PCB.
*/

/*
** _sys_time - retrieve the current system time
**
** implements:  uint32_t time( void );
**
** returns:
**    the current system time
*/
static void _sys_time( pcb_t *pcb ) {

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_time, pid %d", pcb->pid );
#endif

    RET(pcb) = _system_time;

#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_pid - retrieve the PID of the current process
**
** implements:  int32_t pid( void );
**
** returns:
**    the PID of this process
*/
static void _sys_getpid( pcb_t *pcb ) {

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_getpid, pid %d", pcb->pid );
#endif
#if TRACING_SYSRET
__cio_printf( " EAX @ %08x: %08x", &(pcb->context->eax),
              pcb->context->eax );
#endif

    RET(pcb) = pcb->pid;

#if TRACING_SYSRET
__cio_printf( " returns %08x", RET(pcb) );
#endif
#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_ppid - retrieve the parent PID of the current process
**
** implements:  int32_t ppid( void );
**
** returns:
**    the PID of the parent of this process
*/
static void _sys_getppid( pcb_t *pcb ) {

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_getppid, pid %d", pcb->pid );
#endif

    RET(pcb) = pcb->ppid;

#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_getprio - retrieve the priority of the specified process
**
** implements:  uint8_t getprio( int32_t pid );
**
** returns:
**    the priority of the process as an integer, or an error code
*/
static void _sys_getprio( pcb_t *pcb ) {
    int32_t pid = ARG(pcb,1);
    pcb_t *target;

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_getprio, pid %d", pcb->pid );
#endif
    
    // interpret -1 as an alias for the current process
    if( pid == -1 ) {
        target = pcb;
    } else {
        target = _pcb_locate( (pcb_t *) pid, _find_by_pid );
    }
    
#if TRACING_SYSRET
__cio_printf( " EAX @ %08x: %08x", &(pcb->context->eax),
              pcb->context->eax );
#endif

    // if we found it, return its priority; otherwise, an error code
    if( target ) {
        RET(pcb) = (uint32_t) (target->priority);
    } else {
        RET(pcb) = E_BAD_PID;
    }

#if TRACING_SYSRET
__cio_printf( " returns %08x", pcb->context->eax );
#endif
#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_setprio - set the priority of the specified process
**
** implements:  uint8_t setprio( int32_t pid, uint8_t prio );
**
** returns:
**    the old priority of the process as an integer, or an error code
*/
static void _sys_setprio( pcb_t *pcb ) {
    int32_t pid  = ARG(_current,1);
    int32_t prio = ARG(_current,2) & 0xff;  // only low-order 8 bits
    pcb_t *target;

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_setprio, pid %d", pcb->pid );
#endif
    
    // is this priority valid?
    if( !VALID_PRIO(prio) ) {
        RET(pcb) = E_BAD_PRIO;
        return;
    }
    
    // interpret -1 as an alias for the current process
    if( pid == -1 ) {
        target = pcb;
    } else {
        target = _pcb_locate( (pcb_t *) pid, _find_by_pid );
    }

    // did we find it?
    if( target == NULL ) {
        // no!
        RET(pcb) = E_BAD_PID;
        return;
    }
    
    // yes - return its old priority, and assign the new one

#if TRACING_SYSRET
__cio_printf( "setprio: pid %d pcb %08x context %08x", pcb->pid,
              pcb, pcb->context );
__cio_printf( " EAX @ %08x: %08x", &(pcb->context->eax),
              pcb->context->eax );
#endif

    RET(pcb) = target->priority;

#if TRACING_SYSRET
__cio_printf( " -> %08x\n", pcb->context->eax );
#endif
    target->priority = prio;
#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_sleep - put the current process to sleep
**
** implements:  void sleep( uint32_t msec );
**
** notes:
**    interprets a time of 0 as a request to yield the CPU
*/
static void _sys_sleep( pcb_t *pcb ) {
    uint32_t ms = ARG(pcb,1);

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_sleep, pid %d", pcb->pid );
#endif
    
    // interpret a sleep time of 0 as a 'yield'
    if( ms == 0 ) {

        _schedule( _current );

    } else {

        // calculate the wakeup time
        uint32_t wakeup = (ms * TICKS_PER_MS) + _system_time;
        pcb->wakeup = wakeup;
        pcb->state = ST_SLEEPING;

	// put the process to sleep
        status_t status = _queue_insert( _sleeping, pcb, (void *) wakeup );
        if( status != SUCCESS ) {
            _kpanic( "_sys_sleep", "sleepQ insert failed" );
        }
    }
    
    // either way, no current process, so we need to pick one
    _dispatch();

#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_read - read data from an input stream
**
** implements:  int32_t read( int32_t stream, void *buf, uint32_t size );
*/
static void _sys_read( pcb_t *pcb ) {
    int n = 0;
    int32_t stream = ARG(pcb,1);
    char *buf = (char *) ARG(pcb,2);
    uint32_t length = ARG(pcb,3);

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_read, pid %d", pcb->pid );
#endif

    // try to get the next character
    switch( stream ) {
    case STR_CONS:
        // console input doesn't block
        if( __cio_input_queue() < 1 ) {
            RET(pcb) = E_NO_DATA;
            return;
        }
        n = __cio_gets( buf, length );
        break;

    case STR_SIO:
        // this may block
        n = _sio_reads( buf, length );
        break;

    default:
        // bad channel code!
        RET(pcb) = E_BAD_STREAM;
        return;
    }

    // if there was data, return the byte count to the process;
    // otherwise, block the process until data is available
    if( n > 0 ) {

        RET(pcb) = n;

    } else {

        // mark it as blocked
        pcb->state = ST_BLOCKED;

        // no character; put this process on the
        // serial i/o input queue
        status_t status = _queue_insert( _reading, pcb, (void *) (pcb->pid) );
        if( status != SUCCESS ) {
            _kpanic( "_sys_read", "cannot insert into reading queue" );
        }

        // select a new current process
        _dispatch();
    }

#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_write - write data to an output stream
**
** implements:
**    int32_t write( int32_t stream, const void *buf, uint32_t size );
*/
static void _sys_write( pcb_t *pcb ) {
    int stream = ARG(pcb,1);
    const char *buf = (const char *) ARG(pcb,2);
    int length = ARG(pcb,3);

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_write, pid %d", pcb->pid );
#endif

    // this is almost insanely simple, but it does separate
    // the low-level device access fromm the higher-level
    // syscall implementation

    switch( stream ) {
    case STR_CONS:
        __cio_write( buf, length );
        RET(pcb) = length;
        break;

    case STR_SIO:
        // much simpler!
        _sio_write( buf, length );
        RET(pcb) = length;
        break;

    default:
        RET(pcb) = E_BAD_STREAM;
        break;
    }

#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_fork - create a new process
**
** implements:  int32_t fork( void );
*/
static void _sys_fork( pcb_t *pcb ) {

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_fork, pid %d", pcb->pid );
#endif

    // make sure there's room for another process!
    if( _active_procs >= MAX_PROCS ) {
        // no room at the inn!
        RET(pcb) = E_NO_PROCS;
        return;
    }

    // first, allocate a PCB
    pcb_t *child = _pcb_alloc();
    if( child == NULL ) {
        RET(pcb) = E_NO_PROCS;
        return;
    }

    // duplicate the parent's PCB in the child's PCB
    _kmemcpy( (void *)child, (void *)pcb, sizeof(pcb_t) );

    // create the stack for the child
    child->stack = _stk_alloc();
    if( child->stack == NULL ) {
        _pcb_free( child );
        RET(pcb) = E_NO_PROCS;
        return;
    }

    // set the child's identity
    child->pid = _next_pid++;
    child->ppid = pcb->pid;
    child->children = 0;

    // Copy working directory
    child->cwd_clstr = pcd->cwd_clstr; 

    // report the relevant information back to the parent
    pcb->children += 1;
    RET(pcb) = child->pid;

    // duplicate the parent's stack
    _kmemcpy( (void *)child->stack, (void *)pcb->stack, sizeof(stack_t) );

    /*
    ** Now, we need to update the ESP and EBP values in the
    ** child's stack.  The problem is that because we duplicated
    ** the parent's stack, these pointers are still pointing back
    ** into the parent's stack, which will cause problems as the
    ** two processes continue to execute.
    */

    // figure out the byte offset from one stack to the other
    int32_t adjust = (void *) child->stack - (void *) pcb->stack;

    // add this to the child's context pointer
    child->context = (context_t *)
            (adjust + (void *)pcb->context);

    // fix the child's ESP and EBP values
    REG(child,ebp) += adjust;
    REG(child,esp) += adjust;

    // follow the EBP chain through the child's stack
    uint32_t *bp = (uint32_t *) REG(child,ebp);
    while( bp && *bp ) {
        *bp += adjust;
        bp = (uint32_t *) *bp;
    }

    // add the new process to the process table
    int ix;
    for( ix = 0; ix < MAX_PROCS; ++ix ) {
        if( _ptable[ix] == NULL ) {
            break;
        }
    }

    // did we find an open slot?
    if( ix >= MAX_PROCS ) {
        _kpanic( "_sys_fork", "no empty slot in non-full ptable" );
    }

    // yes - record the new process
    child->pt_index = ix;
    _ptable[ix] = child;
    ++_active_procs;

    // assign the child's return value
    RET(child) = 0;

    // schedule the child, and let the parent continue
    _schedule( child );

#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_exec - replace the memory image of a process
**
** implements:  uint32_t exec( int (*entry)(int,void*,void*), uint8_t prio,
**             int argc, void *arg1, void *arg2 );
*/
static void _sys_exec( pcb_t *pcb ) {

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_exec, pid %d", pcb->pid );
#endif

    // grab the parameters from the stack
    uint32_t entry = ARG(pcb,1);
    uint8_t prio   = ARG(pcb,2);
    int32_t argc   = ARG(pcb,3);
    uint32_t arg1  = ARG(pcb,4);
    uint32_t arg2  = ARG(pcb,5);

    // create the new stack for the process
    pcb->context = _stk_setup( pcb->stack, entry, argc,
                               (void *) arg1, (void *) arg2 );

    // sanity check!
    if( pcb->context == NULL ) {
        _kpanic( "_sys_exec", "NULL context from _stk_setup" );
    }

    // set the priority for the new program
    pcb->priority = prio;

#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_wait - collect the exit status of a child process
**
** implements:  int32_t wait( int32_t *status );
*/
static void _sys_wait( pcb_t *pcb ) {
    int32_t *sptr = (int32_t *) ARG(pcb,1);
    status_t status;

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_wait, pid %d", pcb->pid );
#endif

    // if there are no children, nothing to do
    if( pcb->children < 1 ) {
        RET(pcb) = E_NO_CHILDREN;
        return;
    }

    // find the first child of this process that is a zombie
    pcb_t *child = _pcb_locate( pcb, _find_zombie_child );

    // if there aren't any, block this process
    if( child == NULL ) {
        pcb->state = ST_WAITING;
        status = _queue_insert( _waiting, pcb, NULL );
        if( status != SUCCESS ) {
            _kpanic( "_sys_wait", "wait Q insert failed" );
        }
	// need a new current process
        _dispatch();
        return;
    }

    // found one!  pull it off the zombie queue
    pcb_t *tmp;
    status = _queue_remove_by( _zombie, (void **)&tmp, child, _find_process );
    if( status != SUCCESS || tmp != child ) {
        _kpanic( "_sys_wait", "zombie Q remove failed" );
    }

    // give the status and PID to the parent
    *sptr = child->exitstatus;
    RET(pcb) = child->pid;

    // one fewer descendent
    pcb->children -= 1;

    // can now clean up the zombie
    _pcb_cleanup( child );

#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

/*
** _sys_exit - terminate the current process
**
** implements:  void exit( int32_t status );
*/
static void _sys_exit( pcb_t *pcb ) {
    int32_t xitstatus = ARG(pcb,1);
    status_t status;

#if TRACING_SYSCALLS
__cio_printf( "--- _sys_exit, pid %d", pcb->pid );
#endif

#if TRACING_EXIT
__cio_printf( " parent %d, status %d\n", pcb->ppid, xitstatus );
#endif

    // first, mark this process as "terminated but not yet cleaned up"
    pcb->state = ST_ZOMBIE;
    pcb->exitstatus = xitstatus;

#if TRACING_EXIT
__cio_printf( "--- exit: state %d estat %d\n", pcb->state, pcb->exitstatus );
#endif

    // next, locate its parent
    pcb_t *parent = _pcb_locate( pcb, _find_parent_of );

#if TRACING_EXIT
__cio_printf( "--- exit: parent %08x\n", parent );
#endif

    // if there isn't one, we have a serious problem
    if( parent == NULL ) {
        _kpanic( "_sys_exit", "no parent for exiting child" );
    }

    // we must also reparent any active children of this process
    if( pcb->children > 0 ) {
        pcb_t *tmp;

#if TRACING_EXIT
__cio_puts( "--- exit: reparenting" );
#endif

        // as long as we find a child,
        while( (tmp=_pcb_locate(pcb,_find_child_of)) != NULL ) {

#if TRACING_EXIT
__cio_printf( " %d", tmp->pid );
#endif
            // reparent it to the init process
            tmp->ppid = PID_INIT;
	    pcb->children -= 1;
	    _init_pcb->children += 1;
        }

#if TRACING_EXIT
__cio_putchar( '\n' );
#endif

    }

    // OK, we have reparented all the children of this process.
    // We now need to notify the parent of this process.  The
    // parent may already be wait()ing for a child, or it may
    // be executing along merrily doing something else....

    if( parent->state == ST_WAITING ) {

        // The parent is waiting.  We need to remove it from the
        // waiting queue so that it can be notified.

        pcb_t *tmp;
        status = _queue_remove_by(_waiting,(void **)&tmp,parent,_find_process);
        if( status != SUCCESS ) {
            _kpanic( "_sys_exit", "waiting parent not on waiting queue" );
        }

        // make sure we found the correct process
        if( parent != tmp ) {
            _kpanic( "_sys_exit", "found wrong waiting parent" );
        }

#if TRACING_EXIT
__cio_printf( "exit(): parent EAX %08x", tmp->context->eax );
#endif

        // next, return the PID of the child and the child's
        // status to the parent.
        RET(parent) = pcb->pid;

#if TRACING_EXIT
__cio_printf( " -> %08x", parent->context->eax );
#endif

        // the parent's call to wait() provided a pointer to the
        // variable into which the exit status is to be placed.
        int32_t *ptr = (int32_t *) ARG(parent,1);
        *ptr = xitstatus;

#if TRACING_EXIT
__cio_printf( ", status to %08x (stk %08x)\n", ptr, parent->stack );
#endif

        // schedule the parent
        _schedule( parent );

        // clean up the exited child
        _pcb_cleanup( pcb );

    } else {

        // the parent isn't currently waiting, so this child
        // must become a zombie.
        status = _queue_insert( _zombie, pcb, (void *) pcb->pid );
        if( status != SUCCESS ) {
            _kpanic( "_sys_exit", "zombie insert failed" );
        }
    }

    // the current process is dead; long live the current process!
    _dispatch();

#if TRACING_SYSCALLS
__cio_puts( " ---\n" );
#endif

}

static void _sys_chdir(pcb_t *pcb) {
    clstr_t clstr = ARG(pcb, 1);

    if (clstr != CLSTR_ERROR) {
        pcb->cwd_clstr = clstr;
    }

    RET(pcb) = clstr;
}

static void _sys_fopen(pcb_t *pcb) {
    char *path = ARG(pcb, 1);
    uint32_t flags = ARG(pcb, 2);

    char dir[512];
    char filename[MAX_FILENAME];
    uint8_t clstr_buf[CLUSTER_SIZE];
    clstr_t clstr;
    entry_t *entry;
    
    _fs_split_path(path, dir, filename);

    // Find the cluster of the parent dir
    clstr = _fs_find_entry(clstr_buf, dir, pcb->cwd_clstr);

    // Cannot find directory
    if (dir == CLSTR_ERROR) {
        RET(pcb) = CLSTR_ERROR;
        return;
    }

    // Load the directory
    _fs_load_clstr(clstr, clstr_buf);

    // See if the file already exists
    entry = _fs_find_in_dir(clstr_buf, filename);

    // File does not exist and should create it
    if (flags & O_CREATE && entry == NULL) {
        // Find next free entry in the directory
        int i;
        for (i = 0; i < MAX_ENTRIES; ++i) {
            entry = ENT(clstr_buf, i);

            if (entry->filename[0] == ENT_FREE || entry->filename[0] == ENT_REMOVED)
                break;
        }

        // Directory is out of space for entries
        if (i >= MAX_ENTRIES) {
            RET(pcb) = CLSTR_ERROR;
            return;
        }

        // entry now points to a new file in the directory
        _kstrcpy(entry->filename, filename);
        entry->size = 0;
        entry->cluster = _fs_get_free_clstr();
        entry->attributes = 0;
        entry->create_time = _system_time;
        entry->modified_time = _system_time

        // Save the directory
        _fs_set_clstr(clstr, clstr_buf);  
    }

    if (entry == NULL) {
        RET(pcb) = CLSTR_ERROR;
    } else {
        RET(pcb) = entry->cluster;
    }
}

/*
** PUBLIC FUNCTIONS
*/

/*
** _sys_init()
**
** initialize the syscall module
**
** MUST BE CALLED AFTER THE _sio_init AND _sched_init FUNCTIONS
** HAVE BEEN CALLED, SO THAT THE _reading QUEUE HAS BEEN CREATED,
** AND _all_queues CAN BE INITIALIZED
*/
void _sys_init( void ) {

    // report that we made it this far
    __cio_puts( " SYSCALL" );

    ///
    // Set up the syscall jump table.  We do this here
    // to ensure that the association between syscall
    // code and function address is correct even if the
    // codes change.
    ///

    _syscalls[ SYS_read ]      = _sys_read;
    _syscalls[ SYS_write ]     = _sys_write;
    _syscalls[ SYS_fork ]      = _sys_fork;
    _syscalls[ SYS_exec ]      = _sys_exec;
    _syscalls[ SYS_wait ]      = _sys_wait;
    _syscalls[ SYS_sleep ]     = _sys_sleep;
    _syscalls[ SYS_exit ]      = _sys_exit;
    _syscalls[ SYS_time ]      = _sys_time;
    _syscalls[ SYS_getpid ]    = _sys_getpid;
    _syscalls[ SYS_getppid ]   = _sys_getppid;
    _syscalls[ SYS_getprio ]   = _sys_getprio;
    _syscalls[ SYS_setprio ]   = _sys_setprio;

    // initialize queues that aren't set up elsewhere

    _sleeping = _queue_alloc( _order_wakeup );
    if( _sleeping == NULL ) {
        _kpanic( "_sys_init", "sleep Q alloc failed" );
    }

    _waiting = _queue_alloc( NULL );
    if( _waiting == NULL ) {
        _kpanic( "_sys_init", "wait Q alloc failed" );
    }

    _zombie = _queue_alloc( NULL );
    if( _zombie == NULL ) {
        _kpanic( "_sys_init", "zombie Q alloc failed" );
    }

    // install the second-stage ISR
    __install_isr( INT_VEC_SYSCALL, _sys_isr );
}
