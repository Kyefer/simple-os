h46708
s 00073/00000/00000
d D 1.1 19/04/02 14:02:50 wrc 1 0
c date and time created 19/04/02 14:02:50 by wrc
e
u
U
f e 0
t
T
I 1
/*
** SCCS ID:	%W%	%G%
**
** File:	ulibs.S
**
** Author:	CSCI-452 class of 20185
**
** Contributor:
**
** Description:	assembly-language user-level library functions
*/

#define	__SP_ASM__

#include "syscalls.h"

/*
** System call stubs
**
** All have the same structure:
**
**      move a code into EAX
**      generate the interrupt
**      return to the caller
**
** As these are simple "leaf" routines, we don't use
** the standard enter/leave method to set up a stack
** frame - that takes time, and we don't really need it.
*/

#define	SYSCALL(name) \
	.globl	name			; \
name:					; \
	movl	$SYS_##name, %eax	; \
	int	$INT_VEC_SYSCALL	; \
	ret

SYSCALL(read)
SYSCALL(write)
SYSCALL(fork)
SYSCALL(exec)
SYSCALL(wait)
SYSCALL(sleep)
SYSCALL(exit)
SYSCALL(time)
SYSCALL(getpid)
SYSCALL(getppid)
SYSCALL(getprio)
SYSCALL(setprio)

/*
** This is a bogus system call; it's here so that we can test
** our handling of out-of-range syscall codes in the syscall ISR.
*/

SYSCALL(bogus)

/*
** Other library functions
*/

/*
** exit_helper() - dummy "startup" function
**
** support function used as the "return to" address
** for user main() routines; it just calls exit(%eax)
*/

        .globl  exit_helper
exit_helper:
        pushl   %eax    // use whatever was in EAX as the status
        call    exit    // terminate this process

E 1
