#
# SCCS ID: %W%	%G%
#
# Makefile to control the compiling, assembling and linking of standalone
# programs in the DSL.  Used for both individual interrupt handling
# assignments and the SP baseline OS (with appropriate tweaking).
#

#
# Application files
#

OS_C_SRC = klibc.c pcbs.c scheduler.c stacks.c system.c \
	clock.c memory.c queues.c sio.c syscalls.c

OS_C_OBJ = klibc.o pcbs.o scheduler.o stacks.o system.o \
	clock.o memory.o queues.o sio.o	 syscalls.o

OS_S_SRC = klibs.S
OS_S_OBJ = klibs.o

OS_LIBS =

OS_SRCS = $(OS_C_SRC) $(OS_S_SRC)
OS_OBJS = $(OS_C_OBJ) $(OS_S_OBJ)

USR_C_SRC = users.c ulibc.c
USR_C_OBJ = users.o ulibc.o

USR_S_SRC = ulibs.S
USR_S_OBJ = ulibs.o

USR_LIBS =

USR_SRCS = $(USR_C_SRC) $(USR_S_SRC)
USR_OBJS = $(USR_C_OBJ) $(USR_S_OBJ)

#
# Framework files
#

FMK_S_SRC = startup.S isr_stubs.S libs.S
FMK_S_OBJ = startup.o isr_stubs.o libs.o

FMK_C_SRC = cio.c libc.c support.c
FMK_C_OBJ = cio.o libc.o support.o

BOOT_SRC = bootstrap.S
BOOT_OBJ = bootstrap.o

FMK_SRCS = $(FMK_S_SRC) $(FMK_C_SRC)
FMK_OBJS = $(FMK_S_OBJ) $(FMK_C_OBJ)

# Collections of files

OBJECTS = $(FMK_OBJS) $(OS_OBJS) $(USR_OBJS)

SOURCES = $(BOOT_SRC) $(FMK_SRCS) $(OS_SRCS) $(USR_SRCS)

#
# Compilation/assembly definable options
#
# General options:
#	CLEAR_BSS		include code to clear all BSS space
#	GET_MMAP		get BIOS memory map via int 0x15 0xE820
#	SP_OS_CONFIG		enable SP OS-specific startup variations
#
# Debugging options:
#	CONSOLE_SHELL		compile in a simple console shell
#	DB_KMALLOC		debug the kernel allocator code
#	DB_KMALLOC_FREELIST	debug the freelist creation
#	DB_KMALLOC_INIT		debug km initialization
#	DB_UNEXP_INTS		debug any 'unexpected' interrupts
#	PANIC_DUMPS_QUEUES	_kpanic() does a full dump of the queues
#	REPORT_MYSTERY_INTS	print a message on interrupt 0x27 specifically
#	STATUS=n		dump queue & process info every n seconds
#	TRACE_CX		include context restore trace code
#	TRACE=n			enable internal tracing options
#	  any bit		general tracing
#	  bit 0x01		trace the PCB module
#	  bit 0x02		trace syscall return values
#	  bit 0x04		trace exit()
#	  bit 0x08		trace stack setup
#	  bit 0x10		trace SIO ISR
#	  bit 0x20		trace SIO output functions
#	  bit 0x40		trace syscall entries/exits
#

GEN_OPTIONS = -DCLEAR_BSS -DGET_MMAP -DSP_OS_CONFIG
DBG_OPTIONS = -DSTATUS=10 -DDEBUG_UNEXP_INTS -DTRACE_CX
# DBG_OPTIONS = -DSTATUS=10 -DCONSOLE_SHELL -DDEBUG_UNEXP_INTS -DTRACE_CX \
#	-DTRACE=0x06

ALL_OPTIONS = $(GEN_OPTIONS) $(DBG_OPTIONS)

#
# YOU SHOULD NOT NEED TO CHANGE ANYTHING BELOW THIS POINT!!!
#
# Compilation/assembly control
#

#
# We only want to include from the current directory and ~wrc/include
#
INCLUDES = -I. -I/home/fac/wrc/include

#
# Compilation/assembly/linking commands and options
#
CPP = cpp
CPPFLAGS = $(ALL_OPTIONS) -nostdinc $(INCLUDES)

#
# Compiler/assembler/etc. settings for 32-bit binaries
#
CC = gcc
CFLAGS = -m32 -fno-pie -std=c99 -fno-stack-protector -fno-builtin -Wall -Wstrict-prototypes $(CPPFLAGS)

AS = as
ASFLAGS = --32

LD = ld
LDFLAGS = -melf_i386 -no-pie

#		
# Transformation rules - these ensure that all compilation
# flags that are necessary are specified
#
# Note use of 'cpp' to convert .S files to temporary .s files: this allows
# use of #include/#define/#ifdef statements. However, the line numbers of
# error messages reflect the .s file rather than the original .S file. 
# (If the .s file already exists before a .S file is assembled, then
# the temporary .s file is not deleted.  This is useful for figuring
# out the line numbers of error messages, but take care not to accidentally
# start fixing things by editing the .s file.)
#
# The .c.X rule produces a .X file which contains the original C source
# code from the file being compiled mixed in with the generated
# assembly language code.  Very helpful when you need to figure out
# exactly what C statement generated which assembly statements!
#

.SUFFIXES:	.S .b .X .i

.c.X:
	$(CC) $(CFLAGS) -g -c -Wa,-adhln $*.c > $*.X

.c.s:
	$(CC) $(CFLAGS) -S $*.c

.S.s:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S

.S.o:
	$(CPP) $(CPPFLAGS) -o $*.s $*.S
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(RM) -f $*.s

.s.b:
	$(AS) $(ASFLAGS) -o $*.o $*.s -a=$*.lst
	$(LD) $(LDFLAGS) -Ttext 0x0 -s --oformat binary -e begtext -o $*.b $*.o

.c.o:
	$(CC) $(CFLAGS) -c $*.c

.c.i:
	$(CC) $(CFLAGS) -E $*.c > $*.i

#
# Targets for remaking bootable image of the program
#
# Default target:  usb.image
#

usb.image: bootstrap.b prog.b prog.nl BuildImage prog.dis Offsets
	./BuildImage -d usb -o usb.image -b bootstrap.b prog.b 0x10000

floppy.image: bootstrap.b prog.b prog.nl BuildImage prog.dis 
	./BuildImage -d floppy -o floppy.image -b bootstrap.b prog.b 0x10000

prog.out: $(OBJECTS)
	$(LD) $(LDFLAGS) -o prog.out $(OBJECTS)

prog.o:	$(OBJECTS)
	$(LD) $(LDFLAGS) -o prog.o -Ttext 0x10000 $(OBJECTS) $(A_LIBS)

prog.b:	prog.o
	$(LD) $(LDFLAGS) -o prog.b -s --oformat binary -Ttext 0x10000 prog.o

#
# Targets for copying bootable image onto boot devices
#

floppy:	floppy.image
	dd if=floppy.image of=/dev/fd0

usb:	usb.image
	/usr/local/dcs/bin/dcopy usb.image

#
# Special rule for creating the modification and offset programs
#
# These are required because we don't want to use the same options
# as for the standalone binaries.
#

BuildImage:	BuildImage.c
	$(CC) -o BuildImage BuildImage.c

Offsets:	Offsets.c
	$(CC) -mx32 -std=c99 $(INCLUDES) -o Offsets Offsets.c

#
# Clean out this directory
#

clean:
	rm -f *.nl *.nll *.lst *.i *.b *.o *.X *.image *.dis BuildImage Offsets

realclean:	clean

#
# Create a printable namelist from the prog.o file
#

prog.nl: prog.o
	nm -Bng prog.o | pr -w80 -3 > prog.nl

# prog.nll also includes local symbols
prog.nll: prog.o
	nm -Bn prog.o | pr -w80 -3 > prog.nll

#
# Generate a disassembly
#

prog.dis: prog.o
	objdump -d prog.o > prog.dis

#
# 'makedepend' is a program which creates dependency lists by looking
# at the #include lines in the source files.
#

depend:
	makedepend $(INCLUDES) $(SOURCES)

# DO NOT DELETE THIS LINE -- make depend depends on it.

bootstrap.o: bootstrap.h
startup.o: bootstrap.h
isr_stubs.o: bootstrap.h
cio.o: cio.h lib.h support.h /home/fac/wrc/include/x86arch.h
cio.o: /home/fac/wrc/include/x86pic.h
libc.o: lib.h
support.o: support.h lib.h cio.h /home/fac/wrc/include/x86arch.h
support.o: /home/fac/wrc/include/x86pic.h bootstrap.h
klibc.o: common.h types.h cio.h memory.h support.h system.h
klibc.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h queues.h
klibc.o: klib.h lib.h scheduler.h sio.h syscalls.h
pcbs.o: common.h types.h cio.h memory.h support.h system.h
pcbs.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h queues.h
pcbs.o: klib.h lib.h
scheduler.o: common.h types.h cio.h memory.h support.h system.h
scheduler.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h
scheduler.o: queues.h klib.h lib.h scheduler.h
stacks.o: common.h types.h cio.h memory.h support.h system.h
stacks.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h
stacks.o: queues.h klib.h lib.h
system.o: common.h types.h cio.h memory.h support.h system.h
system.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h
system.o: queues.h klib.h lib.h clock.h syscalls.h sio.h scheduler.h users.h
clock.o: /home/fac/wrc/include/x86arch.h /home/fac/wrc/include/x86pic.h
clock.o: /home/fac/wrc/include/x86pit.h common.h types.h cio.h memory.h
clock.o: support.h system.h bootstrap.h pcbs.h stacks.h queues.h klib.h lib.h
clock.o: clock.h scheduler.h sio.h syscalls.h
memory.o: common.h types.h cio.h memory.h support.h system.h
memory.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h
memory.o: queues.h klib.h lib.h
queues.o: common.h types.h cio.h memory.h support.h system.h
queues.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h
queues.o: queues.h klib.h lib.h
sio.o: common.h types.h cio.h memory.h support.h system.h
sio.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h queues.h
sio.o: klib.h lib.h /home/fac/wrc/include/uart.h
sio.o: /home/fac/wrc/include/x86pic.h sio.h scheduler.h
syscalls.o: common.h types.h cio.h memory.h support.h system.h
syscalls.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h
syscalls.o: queues.h klib.h lib.h /home/fac/wrc/include/x86pic.h
syscalls.o: /home/fac/wrc/include/uart.h syscalls.h scheduler.h clock.h sio.h
users.o: common.h types.h cio.h memory.h support.h system.h
users.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h queues.h
users.o: klib.h lib.h users.h
ulibc.o: common.h types.h cio.h memory.h support.h system.h
ulibc.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h queues.h
ulibc.o: klib.h lib.h
ulibs.o: syscalls.h common.h types.h cio.h memory.h support.h system.h
ulibs.o: /home/fac/wrc/include/x86arch.h bootstrap.h pcbs.h stacks.h queues.h
ulibs.o: klib.h lib.h
