OPT=-O2 -I/usr/X11R6/include -L/usr/X11R6/lib # -s -static
CC=gcc

go:
	./mk

all:			mkdb emulator Print peek ExtractObjects decompile odPrint oddecompile odemulator type

type:		type.o db.o
				$(CC) $(OPT) type.o db.o -o type

decompile:		decompile.o db.o
				$(CC) $(OPT) decompile.o db.o -o decompile

oddecompile:		oddecompile.o oddb.o
				$(CC) $(OPT) oddecompile.o oddb.o -o oddecompile -DWORK_ON_DISK

Display.o:		Display.c
				$(CC) $(OPT) -c Display.c

decompile.o:	decompile.c
				$(CC) $(OPT) -c decompile.c

oddecompile.o:	decompile.c
				$(CC) $(OPT) -c decompile.c -o oddecompile.o -DWORK_ON_DISK

emulator:		db.o em.o labels.o interface.o isobject.o 
				$(CC) $(OPT) db.o em.o labels.o interface.o isobject.o -o emulator -lX11

odemulator:		oddb.o em.o labels.o odinterface.o isobject.o 
				$(CC) $(OPT) oddb.o em.o labels.o odinterface.o isobject.o -o odemulator -lX11

interface.o:	interface.c
				$(CC) $(OPT) -c interface.c

odinterface.o:	interface.c
				$(CC) $(OPT) -c interface.c -o odinterface.o -DWORK_ON_DISK

isobject.o:		isobject.c 
				$(CC) $(OPT) -c isobject.c

em.o:			em.c 
				$(CC) $(OPT) -c em.c

db.o:			db.c 
				$(CC) $(OPT) -c db.c

oddb.o:			db.c 
				$(CC) $(OPT) -c db.c -DWORK_ON_DISK -o oddb.o

labels.o:		labels.c 
				$(CC) $(OPT) -c labels.c

mkdb:			mkdb.o db.o
				$(CC) $(OPT) db.o mkdb.o -o mkdb 

mkdb.o:			mkdb.c
				$(CC) $(OPT) -c mkdb.c

peek:			peek.o oddb.o
				$(CC) $(OPT) oddb.o peek.o -o peek 

peek.o:			peek.c
				$(CC) $(OPT) -c peek.c

Print:			Print.o db.o
				$(CC) $(OPT) Print.o db.o -o Print

Print.o:		Print.c
				$(CC) $(OPT) -c Print.c

odPrint:			odPrint.o oddb.o
				$(CC) $(OPT) odPrint.o oddb.o -o odPrint -DWORK_ON_DISK

odPrint.o:		Print.c
				$(CC) $(OPT) -c Print.c -DWORK_ON_DISK -o odPrint.o

ExtractObjects:	ExtractObjects.o db.o
				$(CC) $(OPT) ExtractObjects.o db.o -o ExtractObjects

ExtractObjects.o:	ExtractObjects.c
					$(CC) $(OPT) -c ExtractObjects.c


