#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>

#include "db.h"
#include "prologues.h"

#include "libs.h"

unsigned long int Libraries[100][4];

void ExtractLibrariesAddresses()
{
	int nlib, n, no, ad, sw, cmpl, start;
	int j;

	for(j=0; j<NMOD; ++j) ActivesModules[j] = 0;
	ActivesModules[RAM_MOD] = 1;

	start = DB_ModStart(RAM_MOD);

	if(start == 0)
	{
		fprintf(stderr, "Unable to read internal ram....\n");
		exit(333);
	}

	start += 0x09A3;
	nlib = DB_Read3(start);

	fprintf(stderr, "There are %d libraries....\n", nlib);
	start += 3;

	for(n=0; n<nlib;++n)
	{
		no = DB_Read3(start);
		ad = DB_Read5(start+3);
		sw = DB_Read5(start+8);
		cmpl = DB_Read3(start+13);

		start += 16;

		fprintf(stderr, "%2d : %03X %05X %05X %03X\n", n, no, ad, sw, cmpl);

		Libraries[n][0] = no;
		Libraries[n][1] = ad;
		Libraries[n][2] = sw;
		Libraries[n][3] = cmpl;
	}

	fprintf(stderr, "------------------------------------------\n");


}


#define MAXBUF	1000
char buffer[MAXBUF];

FILE *Code;
FILE *Prefixed;
FILE *Asm;

FILE *Errors;
FILE *Xlibs;


FILE *SB;
FILE *REAL;
FILE *LONGREAL;
FILE *COMPLEX;
FILE *LONGCOMPLEX;
FILE *CHARACTER;
FILE *ARRAY;
FILE *LINKEDARRAY;
FILE *STRING;
FILE *BINARY;
FILE *LIST;
FILE *DIRECTORY;
FILE *ALGEBRAIC;
FILE *UNIT;
FILE *TAGGED;
FILE *GRAPHIC;
FILE *LIBRARY;
FILE *BACKUP;
FILE *LIBRARYDATA;
FILE *RESERVED1;
FILE *RESERVED2;
FILE *RESERVED3;
FILE *RESERVED4;
FILE *PROGRAM;
FILE *CODE;
FILE *GLOBAL;
FILE *LOCAL;
FILE *XLIB;

FILE *new_labels;

struct label
{
	unsigned long int address;
	char *txt;

	struct label *next;
};

struct label *labels;

struct label *FindLabel(address)
unsigned long int address;
{
	struct label *l;

	l = labels;

	while(l!=NULL && l-> address!=address)
	{
		l = l->next;
	}

	return l;
}

void OutInTable(file, addr, txt)
FILE *file;
unsigned long int addr;
char *txt;
{
	struct label *lab;

	lab = FindLabel(addr);

	if(lab != NULL)
	{
		fprintf(file, "%05X\t%s\t%s\n", addr, txt, lab->txt);
	}
	else
	{
		fprintf(file, "%05X\t%s\n", addr, txt);
	}
	fflush(file);
}

char *NextSep(ptr)
char *ptr;
{
	while(*ptr != '\0')
	{
		if(strncmp(ptr, SEPARATOR, SEPARATORLENGTH) == 0)
					return ptr+SEPARATORLENGTH;

		++ptr;
	}
	return ptr;
}



int IsIn(n, l)
char *n;
char *l;
{
	int ln;
	char *ptr;

	char *name, *label;

	name = (char *)malloc(strlen(n) + SEPARATORLENGTH + 1);
	label= (char *)malloc(strlen(l) + SEPARATORLENGTH + 1);


	strcpy(name, n); strcat(name, SEPARATOR);
	strcpy(label, l); strcat(label, SEPARATOR);

	ln = strlen(name);


	ptr = label;

	while(*ptr != '\0')
	{
		if(ln>strlen(ptr)) break;

		if(strncmp(name, ptr, ln) == 0)
		{
			free(label);
			free(name);
			return 1;
		}

		ptr = NextSep(ptr);
	}

	free(label);
	free(name);
	
	return 0;
}

void RemoveNUH(address)
unsigned long int address;
{
	char comment[1000];
	char newcomment[1000];
	int n;
	int l;
	char *ptr;
	char *optr;
	char *ptr2;
	char *ToRemove = "Nibbles used here :";
	int flag = 0;

	l = DB_GetComment(address, comment);
	optr = ptr = comment;
	strcpy(newcomment, comment);
	ptr2 = newcomment;

	while(*ptr!='\0')
	{
		if(strncmp(ptr, ToRemove, strlen(ToRemove)))
		{
			ptr = NextSep(ptr);
			ptr2 += (ptr - optr);
			optr = ptr;
		}
		else
		{
			ptr = NextSep(ptr);
			optr = ptr;
			strcpy(ptr2, ptr);
			flag = 1;
		}
	}


	if(flag)
	{
		if(*newcomment == '\0')
		{
			DB_UnComment(address);
		}
		else
		{
			DB_Comment(address, newcomment, l);
		}
	}
			
}


void RemoveNUHs(address, i)
unsigned long int address;
int i;
{
	int j;
	for(j=0;j<i;++j) RemoveNUH(address+i);
}
void RemoveNUHa(address, fin)
unsigned long int address;
unsigned long int fin;
{
	unsigned long int a;
	for(a=address;a<fin;++a) RemoveNUH(a);
}

void AddLabel(address, name)
unsigned long int address;
char *name;
{
	struct label *nl;
	struct label *l;
	char *nt;

	if(strlen(name)==0) return;

	l = FindLabel(address);

	if(l!=NULL)
	{
		if(IsIn(name, l->txt))
		{
			return;
		}

		nt = (char *)malloc(SEPARATORLENGTH+1 + strlen(l->txt) + strlen(name));

		if(nt == 0)
		{
			fprintf(stderr, "UNABLE TO MALLOCATE MEMORY FOR NAME...\n");
			exit(666);
		}

		strcpy(nt, l->txt);
		strcat(nt, SEPARATOR);
		strcat(nt, name);

		free(l->txt);
		l->txt = nt;

		fprintf(new_labels, "> %05X %s\n", address, l->txt);
		fflush(new_labels);

		return;
	}

	nl = (struct label *)malloc(sizeof(struct label));

	if(nl==0)
	{
		fprintf(stderr, "FATAL ERROR : unable to mallocate memory for new lab\n");
		fflush(stderr);
		exit(99);
	}

	nl->txt = (char *)malloc(1+strlen(name));
	if(nl->txt==0)
	{
		fprintf(stderr, "FATAL ERROR : unable to mallocate memory for txt\n");
		fflush(stderr);
		exit(99);
	}

	strcpy(nl->txt, name);
	nl->address = address;

	fprintf(new_labels, "%05X %s\n", address, name);
	fflush(new_labels);

	nl->next = labels;
	labels = nl;
}

int GetEntry(file, address, name)
FILE *file;
char *name;
unsigned long int *address;
{
	int i;
	int c;

	if(fscanf(file, "%x", address)!=1) return 0;
	getc(file);
	i=0;
	while((c=getc(file))!='\n')
	{
		name[i++]=c;
	}
	name[i]='\0';

	return 1;
}

void LoadLabels()
{
	char name[100];
	FILE *hplabs;
	unsigned long int address;

	labels = (struct label *)0;
	new_labels = fopen("Book/entries.a", "w");
	hplabs = fopen("Entries.a", "r");


	while(GetEntry(hplabs, &address, name))
	{
		AddLabel(address, name);
	}

	fclose(hplabs);
}

unsigned long int OffsetU(address)
unsigned long int address;
{
	unsigned long int offset;

	offset = DB_Use5(address);

	if(offset == 0) return 0L;

	return (address+offset) % 0x100000;
}

void DecompileMsgTable(address, libn)
unsigned long int address, libn;
{
	unsigned long int prologue;
	unsigned long int i, n;

	prologue = DB_Use5(address);

	if(prologue == PROL_SB)
	{
		unsigned long int adresse;
		adresse = DB_Use5(address+5);
		sprintf(buffer, "<%05Xh>", adresse);
		DB_Comment(address, buffer, 0);
		sprintf(buffer, "Lib %03X's Message Table address", libn);
		DB_Title(address, buffer);
		AddLabel(address, buffer);
		DecompileMsgTable(adresse, libn);
		return;
	}
	else if(prologue == PROL_RESERVED1)
	{
		unsigned long int adresse;
		unsigned long int extension;
		adresse = DB_Use5(address+5);
		extension = DB_Use5(address+10);
		sprintf(buffer, "Far pointer : <%05Xh> <%05Xh>", adresse, extension);
		fprintf(RESERVED1, "%05X\t%s\n", address, buffer);
		DB_Comment(address, buffer, 0);
		sprintf(buffer, "Lib %03X's Message Table address", libn);
		DB_Title(address, buffer);
		AddLabel(address, buffer);
		DecompileMsgTable(adresse, libn);
		return;
	}

	sprintf(buffer, "Lib %03X's message table", libn);
	AddLabel(address, buffer);
	DB_Title(address, buffer);

	DB_UseN(15, address+5);

	n = DB_Use5(address+20);

	sprintf(buffer, "Array of %d strings", n);
	DB_Comment(address, buffer, 0);
	fprintf(ARRAY, "%05X\t%s\n", address, buffer);

	address+=25;

	for(i=1; i<=n; ++i)
	{
		unsigned long int ml, j, k;

		ml = (DB_Use5(address)-5)/2;

		sprintf(buffer, "Message %03X-%02X : \"", libn, i);

		fprintf(Errors, "%03X%02X\t", libn, i);

		k = strlen(buffer);

		for(j=0;j<ml;++j)
		{
			buffer[k] = DB_Use2(address+5+2*j);
			fprintf(Errors, "%c", buffer[k]);
			++k;
		}
		buffer[k]='"';
		buffer[k+1]='\0';

		fprintf(Errors, "\n");
		fflush(Errors);

		DB_Comment(address, buffer, 1);

		address += 5 + 2 * ml;
	}
}

char *Object2Txt(prologue)
unsigned long int prologue;
{
	switch(prologue)
	{
		case PROL_SB:
			return "System Binary";

		case PROL_REAL:
			return "Real";

		case PROL_LONGREAL:
			return "Extended Real";

		case PROL_COMPLEX:
			return "Complex";

		case PROL_LONGCOMPLEX:
			return "Extended Complex";

		case PROL_CHARACTER:
			return "Character";

		case PROL_ARRAY:
			return "Array";

		case PROL_LINKEDARRAY:
			return "Linked Array";

		case PROL_STRING:
			return "String";

		case PROL_BINARY:
			return "Binary Integer";

		case PROL_LIST:
			return "List";

		case PROL_DIRECTORY:
			return "Directory";

		case PROL_ALGEBRAIC:
			return "Algebraic";

		case PROL_UNIT:
			return "Unit";

		case PROL_TAGGED:
			return "Tagged";

		case PROL_GRAPHIC:
			return "Graphic";

		case PROL_LIBRARY:
			return "Library";

		case PROL_BACKUP:
			return "Backup";

		case PROL_LIBRARYDATA:
			return "Library Data";

		case PROL_RESERVED1:
			return "Far pointer";

		case PROL_RESERVED2:
			return "Reserved 1";

		case PROL_RESERVED3:
			return "Reserved 2";

		case PROL_RESERVED4:
			return "Reserved 3";

		case PROL_PROGRAM:
			return "Program";

		case PROL_CODE:
			return "Code";

		case PROL_GLOBAL:
			return "Global Name";

		case PROL_LOCAL:
			return "Local Name";

		case PROL_XLIB:
			return "XLIB Name";

		default:
			return "Unknown type...";
	}
}
char tmpname[100];

char *Object2Txt2(address)
unsigned long int address;
{
	unsigned long int prologue;
	prologue = DB_Read5(address);
	if(prologue == PROL_ARRAY || prologue == PROL_LINKEDARRAY)
	{
		sprintf(tmpname, "%s of %s", Object2Txt(prologue),
			Object2Txt(DB_Read5(address+10)));
	}
	else
	{
		sprintf(tmpname, "%s", Object2Txt(prologue));
	}

	return tmpname;
}

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

		case SPECIAL_GOTO:
		case SPECIAL_IF_GOTO:
		case SPECIAL_NOT_IF_GOTO:
			return 2;

		case SPECIAL_DUPANDTHEN:
			return 3;

		default:
			return 0;
	}
}

unsigned long int DecompileObject();

unsigned long int DecompileCollection(address, level, goin)
unsigned long int address;
int level;
int goin;
{
	unsigned long int prologue;
	struct label *l;
	char buffer[1000];

	while((prologue=DB_Use5(address))!=EPILOGUE)
	{
		if(IsAnObject(prologue) == 1 || IsAnObject(prologue) == 2)
		{
			address = DecompileObject(address, level, goin);
		}
		else
		{
			if(goin==1)
			{
				DecompileObject(prologue, 0, goin);
			}

			l = FindLabel(prologue);

			if(l!=NULL)
			{
				/* sprintf(buffer, "EXTERNAL %05X (%s)", prologue, l->txt); */
				sprintf(buffer, "%s", l->txt);
			}
			else
			{
				sprintf(buffer, "External %05X", prologue);
			}

			DB_SetExternal(address, 1);
			DB_Comment(address, buffer, level);
			
			address += 5;
		}
	}

	if(level==1) DB_NLA(address, 1);

	return address;
}

unsigned long int DecompileName(address, txt, level, file)
unsigned long int address;
char *txt;
int level;
FILE *file;
{
	unsigned long int nc,i;
	char name[127];
	char lab[127];

	nc = DB_Use2(address+5);

	for(i=0;i<nc;++i)
	{
		name[i] = (int)DB_Use2(address + 7 + i*2);
	}

	name[i]='\0';

	sprintf(lab, "%s '%s'", txt, name);
	AddLabel(address, lab);

	OutInTable(file, address, lab);

	sprintf(buffer, "'%s' (%s)", name, txt);
	DB_Comment(address, buffer, level);

	for(i=5;i<7 + 2*nc;++i) RemoveNUH(address+i);
	return address + 7 + 2*nc;
}

unsigned long int skip(address, txt, level, use)
unsigned long int address;
char *txt;
int level;
int use;
{
	unsigned long int length;
	int i;

	DB_Comment(address, txt, level);

	length = DB_Use5(address+5);

	if(use) DB_UseN(length, address+5); 

	for(i=5;i<5+length;++i) RemoveNUH(address+i);
	return address+5+length;
}

unsigned long int DecompileObject(address, level, goin)
unsigned long int address;
int level;
int goin;
{
	unsigned long int prologue;
	unsigned long int value;
	char lab[10000];
	char buffer[10000];
	int i;
	struct label *l;
	unsigned long int add, swi;

	l=FindLabel(address);

	if(DB_Used(address) || DB_COT(address)!=0)
	{
		/* object already decompiled... */
		/* no need to explode it */
		goin = 0;
	}
	else
	{
		if(l!=NULL)
		{
			fprintf(stderr, "            Decompiling : %s\n", l->txt);
			fflush(stderr);
		}
	}

	prologue = DB_Use5(address);

	if(level==0)
	{
		if(l!=NULL)
		{
			DB_Title(address, l->txt);
		}
		else
		{
			DB_Title(address, "");
		}
	}

	switch(prologue)
	{
		case PROL_SB:
			value = DB_Use5(address+5);
			sprintf(buffer, "<%05Xh> (System Binary)", value);
			DB_Comment(address, buffer, level);
			sprintf(buffer, "<%05Xh>", value);
			AddLabel(address, buffer);
			OutInTable(SB, address, buffer);
			
			for(i=5;i<10;++i) RemoveNUH(address+i);
			return address+10;

		case PROL_REAL:
			{
				int i, j;
				unsigned long int expo;
				char expon[100];

				i=0;

				if(DB_Use1(address+20)!=0)
				{
					buffer[i++] = '-';
				}

				buffer[i++] = HTOA((int)DB_Use1(address+19));
				buffer[i++] = '.';

				for(j=18;j>=8;--j)
				{
					buffer[i++]=HTOA((int)DB_Use1(address+j));
				}

				buffer[i++]='E';
				buffer[i++] = '\0';

				expo = DB_Use3(address+5);
				if(expo<0x500)
				{
					sprintf(expon, "%03X", expo);
				}
				else
				{
					sprintf(expon, "-%03X", expo);
				}

				strcat(buffer, expon);

				sprintf(lab, "%s", buffer);
				AddLabel(address, lab);
				OutInTable(REAL, address, lab);

				strcat(buffer, " (Real)");

				DB_Comment(address, buffer, level);

				for(i=5;i<21;++i) RemoveNUH(address+i);
				return address+21;
			}

		case PROL_LONGREAL:
			{
				int i, j;
				unsigned long int expo;
				char expon[100];

				i=0;

				if(DB_Use1(address+25)!=0)
				{
					buffer[i++] = '-';
				}

				buffer[i++] = HTOA((int)DB_Use1(address+24));
				buffer[i++] = '.';

				for(j=23;j>=10;--j)
				{
					buffer[i++]=HTOA((int)DB_Use1(address+j));
				}

				buffer[i++]='E';
				buffer[i++] = '\0';

				expo = DB_Use5(address+5);
				if(expo<0x50000)
				{
					sprintf(expon, "%05X", expo);
				}
				else
				{
					sprintf(expon, "-%05X", expo);
				}

				strcat(buffer, expon);

				sprintf(lab, "%s", buffer);
				AddLabel(address, lab);
				OutInTable(LONGREAL, address, lab);

				strcat(buffer, " (Extended Real)");

				DB_Comment(address, buffer, level);

				for(i=5;i<26;++i) RemoveNUH(address+i);
				return address+26;
			}

		case PROL_COMPLEX:
			DB_Comment(address, "Complex", level);
			OutInTable(COMPLEX, address, "");
			for(i=5;i<37;++i) RemoveNUH(address+i);
			return address+37;

		case PROL_LONGCOMPLEX:
			DB_Comment(address, "Extended Complex", level);
			OutInTable(LONGCOMPLEX,  address, "");
			for(i=5;i<47;++i) RemoveNUH(address+i);
			return address+47;

		case PROL_CHARACTER:
			{
				int c;

				c = (int) DB_Use2(address+5);

				sprintf(buffer, "Character %02Xh %03d", c, c);
				DB_Comment(address, buffer, level);
				AddLabel(address, buffer);

				OutInTable(CHARACTER, address, buffer);

				for(i=5;i<7;++i) RemoveNUH(address+i);
				return address+7;
			}

		case PROL_ARRAY:
			{
				unsigned long int type;
				char buf[100];

				type = DB_Use5(address+10);

				sprintf(buf, "Array of %s", Object2Txt(type));

				OutInTable(ARRAY, address, buf);

				return skip(address, buf, level, 1);
			}

		case PROL_LINKEDARRAY:
			OutInTable(LINKEDARRAY, address, "");
			return skip(address, "Linked Array", level, 1);

		case PROL_STRING:
			{
				unsigned long int length, nc, i, j;

				length = DB_Use5(address+5);

				nc = (length - 5)/2;

				j=0;
				buffer[j++]='"';

				for(i=0;i<nc;++i)
				{
					buffer[j++]=DB_Use2(address+10+2*i);
				}

				buffer[j] = '\0';

				sprintf(lab, "%s\"", buffer);
				AddLabel(address, lab);

				OutInTable(STRING, address, lab);

				strcat(buffer, "\" (String)");

				DB_Comment(address, buffer, level);

				for(i=5;i<5+length;++i) RemoveNUH(address+i);
				return address+5+length;
			}
		

		case PROL_BINARY:
			{
				unsigned long int length, nc, i, j;
				int x;

				length = DB_Use5(address+5);

				if(length > 0x25)
				{
					return skip(address, "Binary Integer", level, 1);
				}

				nc = (length - 5);

				j=0;
				buffer[j++]='#';

				for(i=0;i<nc;++i)
				{
					x = (int)DB_Use1(address+10+i);
					buffer[j++]=(char)HTOA(x);
				}

				buffer[j] = '\0';

				sprintf(lab, "%s", buffer);
				AddLabel(address, lab);

				OutInTable(BINARY, address, lab);

				strcat(buffer, " (Binary Integer)");

				DB_Comment(address, buffer, level);

				for(i=5;i<5+length;++i) RemoveNUH(address+i);
				return address+5+length;
			}
		

		case PROL_DIRECTORY:
			OutInTable(DIRECTORY, address, "");
			return skip(address, "Directory", level, 1);
		
		case PROL_LIST:
			OutInTable(LIST, address, "");
			DB_Comment(address, "{", level);
			address = DecompileCollection(address+5, level+1, goin);
			DB_Comment(address, "}", level);
			return address +5;

		case PROL_ALGEBRAIC:
			OutInTable(ALGEBRAIC, address, "");
			DB_Comment(address, "Start of algebraic", level);
			address = DecompileCollection(address+5, level+1, goin);
			DB_Comment(address, "End of algebraic", level);
			return address +5;

		case PROL_UNIT:
			OutInTable(UNIT, address, "");
			DB_Comment(address, "Start of unit", level);
			address = DecompileCollection(address+5, level+1, goin);
			DB_Comment(address, "End of unit", level);
			return address +5;

		case PROL_TAGGED:
			OutInTable(TAGGED, address, "");
			{
				unsigned long int nct, i, j;

				nct = DB_Use2(address+5);

				j=0;
				buffer[j++]=':';

				for(i=0;i<nct;++j)
				{
					buffer[j++]=DB_Use2(address+7+i*2);
				}
				buffer[j]='\0';

				strcat(buffer, ": (Tagged Object)");
				DB_Comment(address, buffer, level);

				return DecompileObject(address+7+2*nct, level+1, goin);
			}

		case PROL_GRAPHIC:
			OutInTable(GRAPHIC, address, "");
			return skip(address, "Graphic Object", level, 1);
		
		case PROL_LIBRARY:
			OutInTable(LIBRARY, address, "");
			return skip(address, "Library", level, 1);
		
		case PROL_BACKUP:
			OutInTable(BACKUP, address, "");
			return skip(address, "Backup Object", level, 1);
		
		case PROL_LIBRARYDATA:
			OutInTable(LIBRARYDATA, address, "");
			return skip(address, "Library Data", level, 1);

		case PROL_RESERVED1:
			add = DB_Use5(address+5);
			swi = DB_Use5(address+10);
			sprintf(buffer, "Far pointer <%05Xh> <%05Xh>", add, swi);
			DB_Comment(address, buffer, level);
			OutInTable(RESERVED1, address, buffer);
			for(i=5;i<15;++i) RemoveNUH(address+i);
			return address+15;

		case PROL_RESERVED2:
			OutInTable(RESERVED2, address, "");
			return skip(address, "Reserved Object 1", level, 1);

		case PROL_RESERVED3:
			OutInTable(RESERVED3, address, "");
			return skip(address, "Reserved Object 2", level, 1);

		case PROL_RESERVED4:
			OutInTable(RESERVED4, address, "");
			return skip(address, "Reserved Object 3", level, 1);

		case PROL_PROGRAM:
			OutInTable(PROGRAM, address, "");
			DB_Comment(address, "Start of program", level);
			address = DecompileCollection(address+5, level+1, goin);
			DB_Comment(address, "End of program", level);
			return address +5;

		case PROL_CODE:
			OutInTable(CODE, address, "");
			value = DB_Use5(address+5);
			sprintf(buffer, "Code object, ML starts at %05X", address + 10);
			DB_Comment(address, buffer, level);

			fprintf(Code, "%05X %05X %05X\n", address, address+10, address+5+value);
			fflush(Code);

			for(i=5;i<10;++i)
			{
				RemoveNUH(address+i);
				DB_SetLevel(address+i, level);
			}
			return address+5+value;

		case PROL_GLOBAL:
			return DecompileName(address, "Global Name", level, GLOBAL);

		case PROL_LOCAL:
			return DecompileName(address, "Local Name", level, LOCAL);

		case PROL_XLIB:
			{
				unsigned long libn, cmden;

				libn = DB_Use3(address+5);
				cmden= DB_Use3(address+8);

				sprintf(buffer, "XLIB %03X %03X", libn, cmden);
				DB_Comment(address, buffer, level);
				AddLabel(address, buffer);
				OutInTable(XLIB, address, buffer);

				for(i=5;i<11;++i) RemoveNUH(address+i);
				return address+11;
			}

		case SPECIAL_GOTO:
			value = DB_Use5(address + 5);
			sprintf(buffer, "GOTO %05X", value);
			DB_Comment(address, buffer, level);
			for(i=5;i<10;++i) RemoveNUH(address+i);
			return address+10;

		case SPECIAL_IF_GOTO:
			value = DB_Use5(address + 5);
			sprintf(buffer, "?GOTO %05X", value);
			DB_Comment(address, buffer, level);
			for(i=5;i<10;++i) RemoveNUH(address+i);
			return address+10;

		case SPECIAL_NOT_IF_GOTO:
			value = DB_Use5(address + 5);
			sprintf(buffer, "NOT?GOTO %05X", value);
			DB_Comment(address, buffer, level);
			for(i=5;i<10;++i) RemoveNUH(address+i);
			return address+10;

		case SPECIAL_DUPANDTHEN:
			DB_Comment(address, "DUP and call following object", level);
			return DecompileObject(address+5, level, goin);

		default:
			if(prologue==address+5)
			{
				sprintf(buffer, "Prefixed entry point, ML at %05X", prologue);
				DB_Comment(address, buffer, level);
				fprintf(Prefixed, "%05X\n", address+5);
				fflush(Prefixed);
				return 0; /* bof... */
			}
			if(level==0)
			{
				fprintf(stderr, "%05X seems to be ML (%d)\n", prologue, level);
				fflush(stderr);
				sprintf(buffer, "Call ML program at %05X", prologue);
				DB_Comment(address, buffer, level);

				fprintf(stderr, "%05x : %05x\n", address, prologue);
				fflush(stderr);

				sprintf(buffer, "ML called as external (%05X)", prologue);
				AddLabel(prologue, buffer);
				DB_Title(prologue, buffer);

				fprintf(Asm, "%05X\n", prologue);
				fflush(Asm);

				return 0;
			}
			fprintf(stderr, "%05X seems to be ML (%d)\n", prologue, level);
			fflush(stderr);
				
			fprintf(Asm, "%05X\n", prologue);
			fflush(Asm);

			exit(0);
			
	}
}

void DecompileConfig(address, libn)
unsigned long int address;
unsigned long int libn;
{
	unsigned long int prologue;

	prologue = DB_Use5(address);

	if(prologue == PROL_SB)
	{
		unsigned long int adresse;
		adresse = DB_Use5(address+5);
		sprintf(buffer, "<%05Xh>", adresse);
		DB_Comment(address, buffer, 0);
		sprintf(buffer, "Lib %03X's Config Object address", libn);
		DB_Title(address, buffer);
		AddLabel(address, buffer);
		DecompileConfig(adresse, libn);
		return;
	}
	else if(prologue == PROL_RESERVED1)
	{
		unsigned long int adresse;
		unsigned long int extension;
		adresse = DB_Use5(address+5);
		extension = DB_Use5(address+10);
		sprintf(buffer, "Far pointer : <%05Xh> <%05Xh>", adresse, extension);
		fprintf(RESERVED1, "%05X\t%s\n", address, buffer);
		DB_Comment(address, buffer, 0);
		sprintf(buffer, "Lib %03X's Config Object address", libn);
		DB_Title(address, buffer);
		AddLabel(address, buffer);
		DecompileConfig(adresse, libn);
		return;
	}

	sprintf(buffer, "Lib %03X's config object", libn);
	AddLabel(address, buffer);

	DecompileObject(address, 0, 1);
}

void RealHashDecompilation(address, libn)
unsigned long int address;
unsigned long int libn;
{
	unsigned long int i;
	unsigned long int class;
	unsigned long int eoo;
	unsigned long int xlibname;
	unsigned long int lstend;
	unsigned long int n;
	unsigned long int r;
	unsigned long int off;

	eoo = OffsetU(address+5); 

	RemoveNUHa(address, eoo);

	address += 10;

	for(i=0;i<16;++i)
	{
		off = OffsetU(address);
		if(off != 0)
		{
			sprintf(buffer, "Class %2d begins at %05X", i+1, off);
		}
		else
		{
			sprintf(buffer, "No name in class %02d", i+1);
		}
		DB_Comment(address, buffer, 1);
		DB_NLB(address, (i==0));
		address += 5;
	}

	lstend = OffsetU(address);
	sprintf(buffer, "End of namelist : %05X", lstend);
	DB_Comment(address, buffer, 1);
	DB_NLB(address, 1);

	address += 5;
	r=0;

	while(address<lstend)
	{
		unsigned int length, l;
		char name[127];
		unsigned long int addr;

		addr = address;

		length = DB_Use2(addr);
		addr +=2;

		for(l=0; l<length;++l)
		{
			name[l] = (int) DB_Use2(addr);
			addr+=2;
		}

		name[l] = '\0';

		n = DB_Use3(addr);

		sprintf(buffer, "XLIB %03X %03X's name is '%s'", libn, n, name);
		DB_Comment(address, buffer, 1);
		DB_NLB(address, (r==0));
		address = addr + 3;
		++r;
	}

	address = lstend;

	n=0;

	while(address<eoo)
	{
		unsigned long int offset;

		offset = DB_Use5(address);
		if(offset != 0)
		{
			xlibname = address - DB_Use5(address);
			sprintf(buffer, "XLIB %03X %03X name is at %05X", libn, n, xlibname);
		}
		else
		{
			sprintf(buffer, "XLIB %03X %03X has no name", libn, n);
		}
		DB_Comment(address, buffer, 1);
		DB_NLB(address, (n==0));
		address+=5;
		++n;
	}
}

char *Fflagnames[12][3] =
{
	{ " 0 : 0 => This is a function...           ",
	  "BUG.........                     ",
	  "ANOTHER BUG"                                },

	{ " 1 : 0 => No equation-writer object...     ",
	  " 1 : 1 => Equation-writer object ",
	  "Equation-writer object"                     },

	{ " 2 : 0                                    ",
	  " 2 : 1                           ",
	  "Object 2"                           },

	{ " 3 : 0 => No Algebraic-object...          ",
	  " 3 : 1 => Algebraic-object       ",
	  "Algebraic-object"                        },

	{ " 4 : 0 => No derivation-object...         ",
	  " 4 : 1 => Derivation-object      ",
	  "Derivation object"                           },
 
	{ " 5 : 0 => No isolate-object...            ",
	  " 5 : 1 => Isolate object         ",
	  "Isolate-object"                           },

	{ " 6 : 0                                    ",
	  " 6 : 1                           ",
	  "Object 6"                           },
  
	{ " 7 : 0                                    ",
	  " 7 : 1                           ",
	  "Object 7"                           },

	{ " 8 : 0 => No Rules-list...                ",
	  " 8 : 1 => Rules-list             ",
	  "Rules-list"                           },

	{ " 9 : 0 => No integration-object...        ",
	  " 9 : 1 => Integration object     ",
	  "Integration-object"                          },

	{ "10 : 0                                    ",
	  "10 : 1                           "           ,
	  "Object 10"                           },

	{ "11 : 0                                    ",
	  "11 : 1                           ",
	  "Object 11"                           }

};


char *FCflags[4][3] =
{
	{ " 0 : 0 => BUG !!!!!!            ",
	  " 0 : 1 => This is a command...  ",
	  "BUG" },

	{ " 1 : 0                          ",
	  " 1 : 1                          ",
	  "Object 1"                         },

	{ " 2 : 0 => No function version...",
	  " 2 : 1 => Function version      ",
	  "Function-version"                         },

	{ " 3 : 0                          ",
	  " 3 : 1                          ",
	  "Object 3"                         }
};

char *CCflags[4][3] =
{
	{ " 0 : 0                          ",
	  " 0 : 1                          ",
	  "???? - 0" },

	{ " 1 : 0                          ",
	  " 1 : 1                          ",
	  "???? - 1" },

	{ " 2 : 0                          ",
	  " 2 : 1                          ",
	  "???? - 2" },

	{ " 3 : 0                          ",
	  " 3 : 1                          ",
	  "???? - 3" }
};

void DecompileHiddenLibEntry();

void DecompileFunction(address, name, libn, c)
unsigned long int address;
char *name;
unsigned long int libn;
unsigned long int c;
{
	unsigned long int flags;
	unsigned long int ln, cmden;
	int i;
	unsigned long int adresses[12];
	unsigned long int current;
	unsigned long int prol;

	for(i=0;i<12;++i) adresses[i] = 0;

	ln = DB_Read3(address-6);
	cmden = DB_Read3(address-3);
	prol = DB_Read5(address);

	if(ln != libn || cmden != c || prol != PROL_PROGRAM)
	{
		DecompileHiddenLibEntry(address, name, libn, c);
		return;
	}
	ln = DB_Use3(address-6);
	cmden = DB_Use3(address-3);
	flags = DB_Use1(address-7)*0x100+DB_Use1(address-8)*0x10+DB_Use1(address-9);
	sprintf(buffer, "%s : Function - Informations", name);
	DB_Title(address-9, buffer);
	AddLabel(address-9, buffer);


	sprintf(buffer, "Library number :  %03X", ln);
	DB_Comment(address-6, buffer, 1);
	DB_NLB(address-6, 1);

	sprintf(buffer, "Function number : %03X", cmden);
	DB_Comment(address-3, buffer, 1);
	DB_NLB(address-3, 1);


	/* decompile objects */

	current = address;

	/* First let's decompile main object */

	sprintf(buffer, "Function %s", name);
	AddLabel(current, buffer);

	fprintf(Xlibs, "%05X\t%03X\t%03X\t%03X\t%s\t%s\n",
	current, libn, cmden, flags, name, Object2Txt2(current));
	fflush(Xlibs);

	current = DecompileObject(current, 0, 1);


	for(i=1;i<12; ++i)
	{
		if(flags&(1<<(11-i)))
		{
			unsigned long int prologue;

			prologue = DB_Use5(current);

			if(prologue != PROL_SB && IsAnObject(prologue)==1)
			{
				sprintf(buffer, "%s : %s", name, Fflagnames[i][2]);
				AddLabel(current, buffer);
				adresses[i] = current;

				current = DecompileObject(current, 0, 1);
			}
			else
			{
				int f;

				f=0;
				if(prologue == PROL_SB)
				{
					prologue = DB_Use5(current+5);
					f = 1;
				}
				sprintf(buffer, "%s : %s address", name, Fflagnames[i][2]);
				AddLabel(current, buffer);
				DB_Title(current, buffer);

				sprintf(buffer, "Object %s is at %05X",
											Fflagnames[i][2], prologue);
				DB_Comment(current, buffer, 0);

				sprintf(buffer, "%s : %s", name, Fflagnames[i][2]);
				AddLabel(prologue, buffer);

				adresses[i] = prologue;

				DecompileObject( prologue, 0, 1);

				if(f)
				{
					current = DecompileObject(current, 0, 1);
				}
				else
				{
					current +=5;
				}
			}
		}
	}


	/* Display flags value : */

	buffer[0] = '\0';

	for(i=0;i<12; ++i)
	{
		char tmpbuf[1000];
		if(flags&(1<<(11-i)))
		{
			sprintf(tmpbuf, "%s at %05X\n",
						Fflagnames[i][1], adresses[i]);
		}
		else
		{
			sprintf(tmpbuf, "%s\n", Fflagnames[i][0]);
		}
		strcat(buffer, tmpbuf);
	}
	buffer[strlen(buffer)-1]='\0'; /* remove last \n" */

	DB_Comment(address - 9, buffer, 1);
}

void DecompileFCommand(address, name, libn, c)
unsigned long int address;
char *name;
unsigned long int libn;
unsigned long int c;
{
	unsigned long int flags;
	unsigned long int ln, cmden;
	int i;
	unsigned long int adresses[12];
	unsigned long int current;
	unsigned long int prol;

	ln = DB_Read3(address-6);
	cmden = DB_Read3(address-3);
	prol = DB_Read5(address);
	

	if(ln != libn || cmden != c || prol != PROL_PROGRAM)
	{
		DecompileHiddenLibEntry(address, name, libn, c);
		return;
	}
	flags = DB_Use1(address-7);
	ln = DB_Use3(address-6);
	cmden = DB_Use3(address-3);
	sprintf(buffer, "%s : Command - Informations", name);
	DB_Title(address-7, buffer);
	AddLabel(address-7, buffer);

	sprintf(buffer, "Library number : %03X", ln);
	DB_Comment(address-6, buffer, 1);
	DB_NLB(address-6, 1);

	sprintf(buffer, "Command number : %03X", cmden);
	DB_Comment(address-3, buffer, 1);
	DB_NLB(address-3, 1);

	sprintf(buffer, "Command %s", name);
	AddLabel(address, buffer);

	fprintf(Xlibs, "%05X\t%03X\t%03X\t%X\t%s\t%s\n", 
	address, libn, cmden, flags, name, Object2Txt2(address));
	fflush(Xlibs);

	current = DecompileObject(address, 0, 1);
	
	for(i=1;i<4; ++i)
	{
		if((flags&(1<<(3-i))))
		{
			unsigned long int prologue;

			prologue = DB_Use5(current);

			if(prologue != PROL_SB && IsAnObject(prologue)==1)
			{
				sprintf(buffer, "%s : %s", name, FCflags[i][2]);
				AddLabel(current, buffer);
				adresses[i] = current;

				current = DecompileObject(current, 0, 1);
			}
			else
			{
				int f;

				f = 0;
				if(prologue == PROL_SB)
				{
					prologue = DB_Use5(current+5);
					f = 1;
				}
				sprintf(buffer, "%s : %s address", name, FCflags[i][2]);
				AddLabel(current, buffer);
				DB_Title(current, buffer);

				sprintf(buffer, "Object %s is at %05X",
											FCflags[i][2], prologue);
				DB_Comment(current, buffer, 0);

				sprintf(buffer, "%s : %s", name, FCflags[i][2]);
				AddLabel(prologue, buffer);

				adresses[i] = prologue;

				DecompileObject( prologue, 0, 1);

				if(f)
				{
					current = DecompileObject(current, 0, 1);
				}
				else
				{
					current +=5;
				}
			}
		}
	}
	
	buffer[0] = '\0';

	for(i=0;i<4; ++i)
	{
		char tmpbuf[1000];
		if(flags&(1<<(3-i)))
		{
			if(i==0)
			{
				sprintf(tmpbuf, "%s\n", FCflags[i][1]);
			}
			else
			{
				sprintf(tmpbuf, "%s at %05X\n", FCflags[i][1], adresses[i]);
			}
		}
		else
		{
			sprintf(tmpbuf, "%s\n", FCflags[i][0]);
		}
		strcat(buffer, tmpbuf);
	}
	buffer[strlen(buffer)-1]='\0'; /* remove last \n" */

	DB_Comment(address - 7, buffer, 1);
}

void DecompileHiddenLibEntry(address, name, libn, cmden)
unsigned long int address;
char *name;
unsigned long int libn;
unsigned long int cmden;
{
	unsigned long int l, c;

	l = DB_Read3(address-6);
	c = DB_Read3(address-3);

	if(l==libn && c == cmden)
	{
		DB_Use3(address-6);
		DB_Use3(address-3);
		sprintf(buffer, "Library number : %03X", l);
		DB_Comment(address-6, buffer, 1);
		DB_NLB(address-6, 1);
		sprintf(buffer, "Command number : %03X", c);
		DB_Comment(address-3, buffer, 1);
		DB_NLB(address-3, 1);
	}

	sprintf(buffer, "Command %s", name);
	AddLabel(address, buffer);

	fprintf(Xlibs, "%05X\t%03X\t%03X\t\t%s\t%s\n", address, libn, cmden, name, Object2Txt2(address));
	fflush(Xlibs);

	DecompileObject(address, 0, 1);
}

void DecompileFCLibEntry(address, name, libn, cmden)
unsigned long int address;
char *name;
unsigned long int libn;
unsigned long int cmden;
{
	/* first step : function or command ? */
	unsigned char q;
	unsigned long int libr;
	unsigned long int cmdr;
	unsigned long int prol;

	libr = DB_Read3(address - 6);
	cmdr = DB_Read3(address - 3);
	prol = DB_Read5(address );
	if(libn != libr || cmdr != cmden || prol != PROL_PROGRAM)
	{
		DecompileHiddenLibEntry(address, name, libn, cmden);
		return;
	}

	/* if(libn == HIDDEN_LIB)
	{
		DecompileHiddenLibEntry(address, name, libn, cmden);
		return;
	} */

	q= (unsigned char)DB_Read1(address - 7);

	if(q&0x8)
	{
		DecompileFCommand(address, name, libn, cmden);
	}
	else
	{
		DecompileFunction(address, name, libn, cmden);
	}
}

void DecompileCLibEntry(address, name, libn, c)
unsigned long int address;
char *name;
unsigned long int libn;
unsigned long int c;
{
	unsigned long int flags;
	unsigned long int ln, cmden;
	int i;
	unsigned long int prol;

	ln = DB_Read3(address-6);
	cmden = DB_Read3(address-3);
	prol = DB_Read5(address);

	if(ln != libn|| cmden!=c||prol!=PROL_PROGRAM)
	{
		DecompileHiddenLibEntry(address, name, libn, c);
		return;
	}

	ln = DB_Use3(address-6);
	cmden = DB_Use3(address-3);
	flags = DB_Use1(address-7);
	sprintf(buffer, "%s : Command - Informations", name);
	DB_Title(address-7, buffer);
	AddLabel(address-7, buffer);

	sprintf(buffer, "Library number : %03X", ln);
	DB_Comment(address-6, buffer, 1);
	DB_NLB(address-6, 1);

	sprintf(buffer, "Command number : %03X", cmden);
	DB_Comment(address-3, buffer, 1);
	DB_NLB(address-3, 1);

	sprintf(buffer, "Command %s", name);
	AddLabel(address, buffer);
	
	fprintf(Xlibs, "%05X\t%03X\t%03X\t%1X\t%s\t%s\n",
	address, libn, cmden, flags, name, Object2Txt2(address));
	fflush(Xlibs);

	DecompileObject(address, 0, 1);

	buffer[0] = '\0';

	for(i=0;i<4; ++i)
	{
		char tmpbuf[1000];
		if(flags&(1<<(3-i)))
		{
			sprintf(tmpbuf, "%s\n", CCflags[i][1]);
		}
		else
		{
			sprintf(tmpbuf, "%s\n", CCflags[i][0]);
		}
		strcat(buffer, tmpbuf);
	}
	buffer[strlen(buffer)-1]='\0'; /* remove last \n" */

	DB_Comment(address - 7, buffer, 1);
}


void RealLinkDecompilation(LinkTable, HashTable, libn)
unsigned long int LinkTable;
unsigned long int HashTable;
unsigned long int libn;
{
	unsigned long int base;
	unsigned long int eoht;
	unsigned long int eolt;
	unsigned long int address;
	unsigned long int n;
	char name[128];
	struct label *lab;

	eoht = HashTable + 5 + DB_Read5(HashTable + 5);
	base = HashTable + 5 + 5 + 80 + DB_Read5(HashTable + 5 + 5 + 80);

	eolt = LinkTable + 5 + DB_Use5(LinkTable + 5);
	LinkTable += 10;

	DB_NLB(LinkTable, 1);
	n=0;

	while(LinkTable < eolt)
	{
		address = OffsetU(LinkTable);

		if((HashTable!=0) && base < eoht && DB_Read5(base) != 0)
		{
			char tmpname[127];
			unsigned long int l, length;
			unsigned long int addrname;

			addrname = base - DB_Read5(base);

			length = DB_Read2(addrname);
			addrname += 2;

			for(l=0;l<length;++l)
			{
				tmpname[l] = (int) DB_Read2(addrname);
				addrname +=2;
			}
			tmpname[l]='\0';

			sprintf(name, "'%s' (XLIB %03X %03X)", tmpname, libn, n);
		}
		else
		{
			sprintf(name, "XLIB %03X %03X", libn, n);
		}

		lab = FindLabel(address);

		if(lab == NULL)
		{
			sprintf(buffer, "%s address is %05X", name, address);
		}
		else
		{
			sprintf(buffer, "%s address is %05X (%s)", name, address, lab->txt);
		}
		DB_Comment(LinkTable, buffer, 1);

		/* now, let's decompile command/function... */

		fprintf(stderr, "        Decompiling %s at %05X...\n", name, address);
		fflush(stderr);

		if(libn < 0x700)
		{
			DecompileFCLibEntry(address, name, libn, n);
		}
		else
		{
			DecompileCLibEntry(address, name, libn, n);
		}

		LinkTable += 5;
		base += 5;
		++n;
	}
}

void DecompileLinkTable(LinkTable, HashTable, libn)
unsigned long int LinkTable;
unsigned long int HashTable;
unsigned long int libn;
{
	unsigned long int prologue;

	prologue = DB_Use5(LinkTable);

	if(prologue == PROL_SB)
	{
		unsigned long int adresse;
		adresse = DB_Use5(LinkTable+5);
		sprintf(buffer, "<%05Xh>", adresse);
		DB_Comment(LinkTable, buffer, 0);
		sprintf(buffer, "Lib %03X's Link Table address", libn);
		DB_Title(LinkTable, buffer);
		AddLabel(LinkTable, buffer);
		DecompileLinkTable(adresse, HashTable, libn);
		return;
	}
	else if(prologue == PROL_RESERVED1)
	{
		unsigned long int adresse;
		unsigned long int extension;
		adresse = DB_Use5(LinkTable+5);
		extension = DB_Use5(LinkTable+10);
		sprintf(buffer, "Far pointer : <%05Xh> <%05Xh>", adresse, extension);
		fprintf(RESERVED1, "%05X\t%s\n", LinkTable, buffer);
		DB_Comment(LinkTable, buffer, 0);
		sprintf(buffer, "Lib %03X's Link Table address", libn);
		DB_Title(LinkTable, buffer);
		AddLabel(LinkTable, buffer);
		DecompileLinkTable(adresse, libn);
		return;
	}

	sprintf(buffer, "Lib %03X's link table", libn);
	AddLabel(LinkTable, buffer);
	DB_Title(LinkTable, buffer);
	DB_Comment(LinkTable, buffer, 0);

	RealLinkDecompilation(LinkTable, HashTable, libn);
}

unsigned long int DecompileHashTable(address, libn)
unsigned long int address;
unsigned long libn;
{
	unsigned long int prologue;

	prologue = DB_Use5(address);

	if(prologue == PROL_SB)
	{
		unsigned long int adresse;
		adresse = DB_Use5(address+5);
		sprintf(buffer, "<%05Xh>", adresse);
		DB_Comment(address, buffer, 0);
		sprintf(buffer, "Lib %03X's Hash Table address", libn);
		DB_Title(address, buffer);
		AddLabel(address, buffer);
		return DecompileHashTable(adresse, libn);
	}
	else if(prologue == PROL_RESERVED1)
	{
		unsigned long int adresse;
		unsigned long int extension;
		adresse = DB_Use5(address+5);
		extension = DB_Use5(address+10);
		sprintf(buffer, "Far pointer : <%05Xh> <%05Xh>", adresse, extension);
		fprintf(RESERVED1, "%05X\t%s\n", address, buffer);
		DB_Comment(address, buffer, 0);
		sprintf(buffer, "Lib %03X's Hash Table address", libn);
		DB_Title(address, buffer);
		AddLabel(address, buffer);
		DecompileHashTable(adresse, libn);
		return 0;
	}

	sprintf(buffer, "Lib %03X's hash table", libn);
	AddLabel(address, buffer);
	DB_Title(address, buffer);
	DB_Comment(address, buffer, 0);

	RealHashDecompilation(address, libn);

	return address;
}

void DecompileLibrary(libn, address)
unsigned long int libn;
unsigned long int address;
{
	unsigned long int libnv;
	unsigned long int HashTable;
	unsigned long int MessageTable;
	unsigned long int LinkTable;
	unsigned long int ConfigObject;
	char libname[1000];
	char name[500];
	int k;

	unsigned long int size, realstart, end, sizename;

	libnv = DB_Read3(address);

	if(libnv != libn)
	{
		fprintf(stderr, "Error : %05x does not contain infos for lib %03X\n", 
				address, libn);
				fflush(stderr);

		return;
	}

	fprintf(stderr, "Decompiling library %03X...\n", libn);
	fflush(stderr);

	DB_Use3(address);

	sizename = DB_Read2(address-2);
	if(sizename == 0)
	{
		sprintf(libname, "Library %03X has no name", libn);
		realstart = address-7;
		DB_Use2(address-2);
	}
	else
	{
		realstart = address - 9 - 2 * sizename;
		if(DB_Read2(realstart+5) != sizename)
		{
			fprintf(stderr, "BUG bad libname....\n");
			exit(33);
		}
		DB_Use2(realstart+5);
		DB_Use2(address-2);

		for(k=0; k<sizename;++k)
		{
			name[k]=(char)DB_Use2(realstart+7+2*k);
		}
		name[sizename]='\0';
		sprintf(libname, "Library %03X is '%s'", libn, name);
	}
	
	size = DB_Use5(realstart);
	end = realstart + size;
	sprintf(buffer, "Informations about library %03X", libn);
	DB_Title(realstart, buffer);
	AddLabel(realstart, buffer);

	sprintf(buffer, "Library size : %05X (ends at %05X)", size, end);
	DB_Comment(realstart, buffer, 0);

	DB_Comment(realstart+5, libname, 0);

	sprintf(buffer, "End of library %03X", libn);
	DB_Title(end, buffer);
	AddLabel(end, buffer);

	sprintf(buffer, "Library Number : %03X", libn);
	DB_Comment(address, buffer, 0);

	HashTable    = OffsetU(address + 3);
	MessageTable = OffsetU(address + 3 + 5);
	LinkTable    = OffsetU(address + 3 + 5 + 5);
	ConfigObject = OffsetU(address + 3 + 5 + 5 + 5);

	if(HashTable)
	{
		sprintf(buffer, "Hash table is at %05X", HashTable);
		DB_Comment(address + 3, buffer, 0);
		fprintf(stderr, "    Decompiling library %03X Hash Table...\n", libn);
		fflush(stderr);
		HashTable = DecompileHashTable(HashTable, libn);
	}
	else
	{
		DB_Comment(address + 3, "No Hash Table", 0);
	}

	if(MessageTable)
	{
		sprintf(buffer, "Message table is at %05X", MessageTable);
		DB_Comment(address + 8, buffer, 0);
		fprintf(stderr, "    Decompiling library %03X Msg Table...\n", libn);
		fflush(stderr);
		DecompileMsgTable(MessageTable, libn);
	}
	else
	{
		DB_Comment(address + 8, "No Message Table", 0);
	}

	if(LinkTable)
	{
		sprintf(buffer, "Link table is at %05X", LinkTable);
		DB_Comment(address + 13, buffer, 0);
		fprintf(stderr, "    Decompiling library %03X Link Table...\n", libn);
		fflush(stderr);
		DecompileLinkTable(LinkTable, HashTable, libn);
	}
	else
	{
		DB_Comment(address + 13, "No Link Table", 0);
	}

	if(ConfigObject)
	{
		sprintf(buffer, "Config Object is at %05X", ConfigObject);
		DB_Comment(address + 18, buffer, 0);
		fprintf(stderr, "    Decompiling library %03X's Config Object...\n",
									libn);
		fflush(stderr);
		DecompileConfig(ConfigObject, libn);
	}
	else
	{
		DB_Comment(address + 18, "No Config Object", 0);
	}

}


void ScanExternals()
{
	unsigned long int address;
	struct label *lab;
	unsigned long int prologue;
	int level;
	char title[1000];


	InitDisplay();

	address = 0;

	while(address<0x100000)
	{
		DisplayIt(address, stdout);

		DB_GetTitle(address, title);

		if(strlen(title))
		{	
			lab = FindLabel(address);
			if(lab!=NULL)
			{
				DB_Title(address, lab->txt);
			}
		}

		if(DB_External(address))
		{
			prologue = DB_Read5(address);
			lab = FindLabel(prologue);

			if(lab != NULL)
			{
				level = DB_Level(address);
				/* sprintf(buffer, "EXTERNAL %05X (%s)", prologue, lab->txt);*/
				sprintf(buffer, "%s", lab->txt);
				DB_Comment(address, buffer, level);
				DB_UnComment(address+1);
				DB_UnComment(address+2);
				DB_UnComment(address+3);
				DB_UnComment(address+4);
			}
			address += 5;
		}
		else
		{
			++address;
		}
	}
}


void ExplodeAFont(start, nl, nc)
unsigned long int start;
int nl, nc;
{
	unsigned long int address;
	char fontline[6];
	int charn;
	int bits;
	char buffer[1000];
	int i, j;

	sprintf(buffer, "Font %dx%d", nc, nl);
	DB_Title(start, buffer);
	
	for(charn=31;charn<256;++charn)
	{
		address = start + (2 * nl)*(charn-31);
		DB_NLB(address, 1);
		for(i=0;i<nl;++i)
		{
			bits = DB_Use2(address+i*2);
			for(j=0;j<nc;++j)
			{
				if(bits&(1<<j))
				{
					fontline[j] = '#';
				}
				else
				{
					fontline[j] = '.';
				}
			}
			fontline[nc]= '\0';

			if(i==0)
			{
				sprintf(buffer, "%s  (Character %d)", fontline, charn, charn);
			}
			else
			{
				sprintf(buffer, "%s", fontline);
			}

			DB_Comment(address+2*i, buffer, 0);
			DB_UnComment(address+2*i+1);
		}
	}
}

void ExplodeSmallFont(start, nl, cs, ce)
unsigned long int start;
int nl, cs, ce;
{
	unsigned long int address;
	char fontline[8];
	int charn;
	int bits;
	char buffer[1000];
	int i, j;
	int nc;

	sprintf(buffer, "Small font %d", nl);
	DB_Title(start, buffer);
	
	for(charn=cs;charn<=ce;++charn)
	{
		address = start + (2 * nl + 1)*(charn-cs);
		DB_NLB(address, 1);

		nc = DB_Use1(address);
		sprintf(buffer, "Small char %d is %d pixels wide :", charn, nc);
		DB_Comment(address, buffer, 0);

		for(i=0;i<nl;++i)
		{
			bits = DB_Use2(address+i*2+1);
			for(j=0;j<nc;++j)
			{
				if(bits&(1<<j))
				{
					fontline[j] = '#';
				}
				else
				{
					fontline[j] = '.';
				}
			}
			fontline[nc]= '\0';

			sprintf(buffer, "%s", fontline);

			DB_Comment(address+2*i+1, buffer, 0);
			DB_UnComment(address+2*i+2);
		}
	}
}


void ExplodeFonts()
{
	ExplodeAFont(0x7A2B3L, 10, 5);
	ExplodeAFont(0x7B447L, 8, 5);
	ExplodeSmallFont(0x7C257L, 5, 31, 152);
}

int main()
{
	unsigned long int essai;
	int i, j;
	FILE *objects;
	unsigned long int adresse;

	Code     = fopen("Book/Code", "w");
	Prefixed = fopen("Book/Prefixed", "w");
	Asm      = fopen("Book/Asm", "w");
	Errors   = fopen("Book/Errors", "w");
	Xlibs    = fopen ("Book/Xlibs", "w");

	SB = fopen("Book/SB", "w");
	REAL = fopen("Book/REAL", "w");
	LONGREAL = fopen("Book/LONGREAL", "w");
	COMPLEX = fopen("Book/COMPLEX", "w");
	LONGCOMPLEX = fopen("Book/LONGCOMPLEX", "w");
	CHARACTER = fopen("Book/CHARACTER", "w");
	ARRAY = fopen("Book/ARRAY", "w");
	LINKEDARRAY = fopen("Book/LINKEDARRAY", "w");
	STRING = fopen("Book/STRING", "w");
	BINARY = fopen("Book/BINARY", "w");
	LIST = fopen("Book/LIST", "w");
	DIRECTORY = fopen("Book/DIRECTORY", "w");
	ALGEBRAIC = fopen("Book/ALGEBRAIC", "w");
	UNIT = fopen("Book/UNIT", "w");
	TAGGED = fopen("Book/TAGGED", "w");
	GRAPHIC = fopen("Book/GRAPHIC", "w");
	LIBRARY = fopen("Book/LIBRARY", "w");
	BACKUP = fopen("Book/BACKUP", "w");
	LIBRARYDATA = fopen("Book/LIBRARYDATA", "w");
	RESERVED1 = fopen("Book/RESERVED1", "w");
	RESERVED2 = fopen("Book/RESERVED2", "w");
	RESERVED3 = fopen("Book/RESERVED3", "w");
	RESERVED4 = fopen("Book/RESERVED4", "w");
	PROGRAM = fopen("Book/PROGRAM", "w");
	CODE = fopen("Book/CODE", "w");
	GLOBAL = fopen("Book/GLOBAL", "w");
	LOCAL = fopen("Book/LOCAL", "w");
	XLIB = fopen("Book/XLIB", "w");

	fprintf(stderr, "Pass 1 : Loading files...\n\n");
	fflush(stderr);

	LoadLabels();
	DB_OpenFiles();

	ExtractLibrariesAddresses();

	for(j=0; j<NMOD; ++j) ActivesModules[j] = 0;
	ActivesModules[ROM_MOD] = 1;


	fprintf(stderr, "\n\nPass 2 : Decompiling libraries...\n\n");


	for(i=0;Libraries[i][1] != 0; ++i)
	{
		DecompileLibrary(Libraries[i][0], Libraries[i][1]);
	}


	fprintf(stderr, "\n\nPass 3 : Decompiling interesting objects\n\n");

	for(i=0;Interesting[i].address!=0; ++i)
	{
		fprintf(stderr, "    Decompiling %05X : %s\n",
						Interesting[i].address,
						Interesting[i].label);

		AddLabel(Interesting[i].address, Interesting[i].label);

		DecompileObject(Interesting[i].address, 0, 1);
	}

	fprintf(stderr, "\n\nPass 4 : Decompiling emulator objects\n\n");

	objects = fopen("Objects", "r");
	while(fscanf(objects, "%X", &adresse) == 1)
	{
		fprintf(stderr, "Decompiling %05X\n", adresse);
		DecompileObject(adresse, 0, 1);
	}
	fclose(objects);

	fprintf(stderr, "\n\nPass 5 : Re-examining labels\n\n");

	ScanExternals();

	fprintf(stderr, "\n\nPass 6 : Exploding fonts...\n\n");

	ExplodeFonts();

	DB_CloseFiles();

	fclose(Asm);
	fclose(Code);
	fclose(Prefixed);

	fclose(Errors);
	fclose(Xlibs);

	fclose(new_labels);

	fclose(SB);
	fclose(REAL);
	fclose(LONGREAL);
	fclose(COMPLEX);
	fclose(LONGCOMPLEX);
	fclose(CHARACTER);
	fclose(ARRAY);
	fclose(LINKEDARRAY);
	fclose(STRING);
	fclose(BINARY);
	fclose(LIST);
	fclose(DIRECTORY);
	fclose(ALGEBRAIC);
	fclose(UNIT);
	fclose(TAGGED);
	fclose(GRAPHIC);
	fclose(LIBRARY);
	fclose(BACKUP);
	fclose(LIBRARYDATA);
	fclose(RESERVED1);
	fclose(RESERVED2);
	fclose(RESERVED3);
	fclose(RESERVED4);
	fclose(PROGRAM);
	fclose(CODE);
	fclose(GLOBAL);
	fclose(LOCAL);
	fclose(XLIB);
};

