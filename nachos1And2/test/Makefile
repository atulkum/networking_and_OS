# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = /auto/home-scf-22/csci402/testgnu/decstation-ultrix/bin/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt matmult sort testfiles test1 test2 test3 test4 test5 threadtest

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles
test1.o: test1.c
	$(CC) $(CFLAGS) -c test1.c
test1: test1.o start.o
	$(LD) $(LDFLAGS) start.o test1.o -o test1.coff
	../bin/coff2noff test1.coff test1
test2.o: test2.c
	$(CC) $(CFLAGS) -c test2.c
test2: test2.o start.o
	$(LD) $(LDFLAGS) start.o test2.o -o test2.coff
	../bin/coff2noff test2.coff test2
test3.o: test3.c
	$(CC) $(CFLAGS) -c test3.c
test3: test3.o start.o
	$(LD) $(LDFLAGS) start.o test3.o -o test3.coff
	../bin/coff2noff test3.coff test3
test4.o: test4.c
	$(CC) $(CFLAGS) -c test4.c
test4: test4.o start.o
	$(LD) $(LDFLAGS) start.o test4.o -o test4.coff
	../bin/coff2noff test4.coff test4
test5.o: test5.c
	$(CC) $(CFLAGS) -c test5.c
test5: test5.o start.o
	$(LD) $(LDFLAGS) start.o test5.o -o test5.coff
	../bin/coff2noff test5.coff test5	
threadtest.o: threadtest.c
	$(CC) $(CFLAGS) -c threadtest.c
threadtest: threadtest.o start.o
	$(LD) $(LDFLAGS) start.o threadtest.o -o threadtest.coff
	../bin/coff2noff threadtest.coff threadtest

clean:
	rm -f *.o *.coff
