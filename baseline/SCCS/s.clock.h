h49848
s 00051/00000/00000
d D 1.1 19/04/02 14:03:28 wrc 1 0
c date and time created 19/04/02 14:03:28 by wrc
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
** File:	clock.h
**
** Author:	Warren R. Carithers and various CSCI-452 sections
**
** Contributor:
**
** Description:	Clock module declarations
*/

#ifndef _CLOCK_H_
#define _CLOCK_H_

#include "common.h"

/*
** General (C and/or assembly) definitions
*/

#ifndef __SP_ASM__

/*
** Start of C-only definitions
*/

/*
** Types
*/

/*
** Globals
*/

extern uint32_t _system_time;   // the current system time

/*
** Prototypes
*/

/*
** _clk_init()
**
** initialize the clock module
*/
void _clk_init( void );

#endif

#endif
E 1
