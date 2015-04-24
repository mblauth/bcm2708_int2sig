# bcm2708_int2sig

This is a minimalistic example project mapping hardware interrupts of BCM2708-GPIO-ports (as installed on Raspberry Pis)
to POSIX signals.  This way it becomes possible to handle incoming hardware interrupts in pure Java applications when
using the Realtime Specification for Java (RTSJ).
