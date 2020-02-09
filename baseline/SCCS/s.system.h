h21641
s 00073/00000/00000
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
** File:	system.h
**
** Author:	CSCI-452 class of 20185
**
** Contributor:
**
** Description:	miscellaneous OS support functions
*/

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "common.h"

#include <x86arch.h>

#include "bootstrap.h"

#include "pcbs.h"

// copied from ulib.h
void exit_helper( void );

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

// default contents of EFLAGS register

#define DEFAULT_EFLAGS  (EFLAGS_MB1 | EFLAGS_IF)

/*
** Types
*/

/*
** Globals
*/

/*
** Prototypes
*/

/*
** _sysinit - system initialization routine
**
** Called by the startup code immediately before returning into the
** first user process.
*/
void _sysinit( void );

#ifdef CONSOLE_SHELL
/*
** _shell - extremely simple shell for handling console input
**
** Called whenever we want to take input from the console and
** act upon it (e.g., for debugging the kernel)
*/
void _shell( int );
#endif

#endif

#endif
E 1
