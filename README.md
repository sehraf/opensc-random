opensc-random
=============

Random generation and entropy gathering utilities for OpenSC. 
License: LGPL


This provides two more utilities for OpenSC:

opensc-random: Output random data from a smartcard to stdin

	Usage:

	1) Write a specific number of random bytes to stdout:

	$ opensc-tools 48 | base64
	mhM4rNRLci0QEtAXfRree+htPjHemieUKH9L5qzvmee+JeNtU2KelK6Hi91H1A4s

	2) Continuously write to stdout (warning, may be very slow!):
	
	$ opensc-tools | dd of=random.dat bs=1 count=4096
	4096+0 records in
	4096+0 records out
	4096 bytes (4.1 kB) copied, 132.724 s, 0.0 kB/s


opensc-entropy: Feed entropy to the kernel from a smart card. (must be root)

	A simple wrapper around libopensc for retrieving random data from a smart card HWRNG and feeding it into the kernel random pool using an ioctl. This makes it available to any application using /dev/random.


Building
--------

A simple makefile is provided (no autoconf/automake). You should be able to build by simply running "make". Install by copying the two binaries into your path.

You will need OpenSC installed on your system. Development headers for 0.12 are included in this project. 

Fedora 17 x86_64 does not symlink libopensc.so.3 to libopensc.so. If you receive an error that the linker cannot find opensc, uncomment the alternate LIBOPENSC line in the Makefile. You may need to alter this depending on your distribution.

