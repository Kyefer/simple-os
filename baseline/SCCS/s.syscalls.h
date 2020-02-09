h52791
s 00090/00000/00000
d D 1.1 19/04/02 14:03:32 wrc 1 0
c date and time created 19/04/02 14:03:32 by wrc
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
** File:	syscalls.h
**
** Author:	CSCI-452 class of 20185
**
** Contributor:
**
** Description:	System call module declarations
*/

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
*/

// system call codes
//
// these are used in the user-level C library stub functions

#define	SYS_read	0
#define	SYS_write	1
#define	SYS_fork	2
#define	SYS_exec	3
#define	SYS_wait	4
#define	SYS_sleep	5
#define	SYS_exit	6
#define	SYS_time	7
#define	SYS_getpid	8
#define	SYS_getppid	9
#define	SYS_getprio	10
#define	SYS_setprio	11

// UPDATE THIS DEFINITION IF MORE SYSCALLS ARE ADDED!
#define	N_SYSCALLS	12

// dummy system call code to test our ISR

#define	SYS_bogus	0x0bad

// interrupt vector entry for system calls

#define	INT_VEC_SYSCALL		0x80

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

#ifdef	__SP_KERNEL__

// the following declarations should only be seen by the kernel

#include "queues.h"

/*
** Types
*/

/*
** Globals
*/

// sleeping, waiting, and zombie queues
extern queue_t _sleeping;  // processes catching some Zs
extern queue_t _waiting;   // processes waiting (for Godot?)
extern queue_t _zombie;    // gone, but not forgotten

/*
** Prototypes
*/

/*
** _sys_init()
**
** initializes all syscall-related data structures
*/
void _sys_init( void );

#endif

#endif

#endif
E 1
