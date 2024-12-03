#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "db.h"

int main(argc, argv)
int argc;
char **argv;
{
	unsigned long int address;
	int i;

	if(argc!=2) exit(0);

	sscanf(argv[1], "%x", &address);

	DB_OpenFiles();

	printf("%05X (%d) : ", (int)address, DB_WhichFile(address));

	for(i=0; i<40; ++i)
	{
		printf("%X", DB_Read1(address+i));
		if(i%5==4) printf(" ");
	}
	printf("\n");

	DB_CloseFiles();
}


