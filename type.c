#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include "db.h"
#include "prologues.h"

#define MAXBUF	1000
char buffer[MAXBUF];

char *Object2Txt(prologue)
unsigned long int prologue;
{
	switch(prologue)
	{
		case PROL_SB:
			return "SB";

		case PROL_REAL:
			return "R";

		case PROL_LONGREAL:
			return "ER";

		case PROL_COMPLEX:
			return "CMP";

		case PROL_LONGCOMPLEX:
			return "ECMP";

		case PROL_CHARACTER:
			return "CHR";

		case PROL_ARRAY:
			return "A_";

		case PROL_LINKEDARRAY:
			return "LA";

		case PROL_STRING:
			return "STR";

		case PROL_BINARY:
			return "BI";

		case PROL_LIST:
			return "LST";

		case PROL_DIRECTORY:
			return "DIR";

		case PROL_ALGEBRAIC:
			return "ALG";

		case PROL_UNIT:
			return "U";

		case PROL_TAGGED:
			return "T";

		case PROL_GRAPHIC:
			return "GR";

		case PROL_LIBRARY:
			return "LIB";

		case PROL_BACKUP:
			return "BCK";

		case PROL_LIBRARYDATA:
			return "LIBD";

		case PROL_RESERVED1:
			return "FPTR";

		case PROL_RESERVED2:
			return "R1";

		case PROL_RESERVED3:
			return "R2";

		case PROL_RESERVED4:
			return "R3";

		case PROL_PROGRAM:
			return "PGM";

		case PROL_CODE:
			return "CODE";

		case PROL_GLOBAL:
			return "GN";

		case PROL_LOCAL:
			return "LN";

		case PROL_XLIB:
			return "XN";

		default:
			return "Unknown type...";
	}
}

int main()
{
	unsigned long int essai;
	int i, j;
	FILE *objects;
	unsigned long int adresse;
	FILE *result;
	char type[100];

	DB_OpenFiles();

	for(j=0; j<NMOD; ++j) ActivesModules[j] = 0;
	ActivesModules[ROM_MOD] = 1;

	objects = fopen("types", "r");
	result = fopen("types.res", "w");
	while(fscanf(objects, "%X", &adresse) == 1)
	{
		strcpy(type, Object2Txt(DB_Read5(adresse)));
		if(!strcmp(type, "A_"))
		{
			strcat(type, Object2Txt(DB_Read5(adresse+10)));
		}

		fprintf(result, "%05X\t%s\n", adresse, type);
	}
	fclose(result);


}
