#include <string.h>
#include "prologues.h"

int IsAnObject(prologue)
unsigned long int prologue;
{
	switch(prologue)
	{
		case PROL_SB:
		case PROL_REAL:
		case PROL_LONGREAL:
		case PROL_COMPLEX:
		case PROL_LONGCOMPLEX:
		case PROL_CHARACTER:
		case PROL_ARRAY:
		case PROL_LINKEDARRAY:
		case PROL_STRING:
		case PROL_BINARY:
		case PROL_LIST:
		case PROL_DIRECTORY:
		case PROL_ALGEBRAIC:
		case PROL_UNIT:
		case PROL_TAGGED:
		case PROL_GRAPHIC:
		case PROL_LIBRARY:
		case PROL_BACKUP:
		case PROL_LIBRARYDATA:
		case PROL_RESERVED1:
		case PROL_RESERVED2:
		case PROL_RESERVED3:
		case PROL_RESERVED4:
		case PROL_PROGRAM:
		case PROL_CODE:
		case PROL_GLOBAL:
		case PROL_LOCAL:
		case PROL_XLIB:
			return 1;

		default:
			return 0;
	}
}
