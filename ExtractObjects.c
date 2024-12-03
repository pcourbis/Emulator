#include <string.h>
#include <stdio.h>
#include "db.h"

int main()
{
	FILE *objects;
	unsigned long int a, f;
	int i, j;
	unsigned int l, theend;

	DB_OpenFiles();
	InitDisplay();
	objects = fopen("Objects", "w");

	for(j=0; j<NMOD; ++j) ActivesModules[j] = 0;
	ActivesModules[ROM_MOD] = 1;

	a = DB_ModStart(ROM_MOD);
	l = DB_ModSize(ROM_MOD);
	theend = f = a+l;
	while(a<f)
	{
		DisplayIt(a, objects);
		if(DB_Object(a))
		{
			fprintf(objects, "%05X\n", a);
		}
		++a;
	}

	DB_CloseFiles();
	fclose(objects);
}

