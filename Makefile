CC=cc
CFLAGS=-c -Wall -O2 -I.

# Gentoo, possibly others
LIBOPENSC=-lopensc

# Fedora 17 x86_64. Try this if GCC complains it can't find -lopensc
#LIBOPENSC=-l:libopensc.so.3


all: opensc-random opensc-entropy

opensc-random: opensc-random.o util.o
	$(CC) $(LIBOPENSC) opensc-random.o util.o -o opensc-random

opensc-entropy: opensc-entropy.o util.o
	$(CC) $(LIBOPENSC) opensc-entropy.o util.o -o opensc-entropy

opensc-random.o: opensc-random.c
	$(CC) $(CFLAGS) opensc-random.c

opensc-entropy.o: opensc-entropy.c
	$(CC) $(CFLAGS) opensc-entropy.c

util.o: util.c
	$(CC) $(CFLAGS) util.c

clean:
	rm *.o opensc-random opensc-entropy


