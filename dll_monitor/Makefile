CC=x86_64-w64-mingw32-gcc
CFLAGS=-Wall -Wextra -O2

all: original.dll monitor.dll app.exe

original.dll: original.c original.h ; $(CC) $(CFLAGS) -shared -o $@ original.c

monitor.dll: monitor.c original.h ; $(CC) $(CFLAGS) -shared -o $@ monitor.c -L. -loriginal

app.exe: app.c original.h monitor.dll ; $(CC) $(CFLAGS) -o $@ app.c -L. -lmonitor

clean: ; rm -f *.o *.dll *.exe
