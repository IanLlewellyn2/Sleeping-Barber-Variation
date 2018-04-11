PROG = csmc
OBJS = csmc.o
CC = gcc
CPPFLAGS = -c -pthread  #compile flags
#LDFLAGS =

vmm: csmc.o
	gcc -pthread -o csmc csmc.o
vmm.o: csmc.c 
	$(CC) $(CPPFLAGS) csmc.c
clean:
	rm -f core $(PROG) $(OBJS)
