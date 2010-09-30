all: dcf77

dcf77: dcf77.c Makefile
	gcc -Wall dcf77.c -L/usr/X11R6/lib -lpthread -lXxf86vm -lXaw -lX11 -lSDL -o dcf77
