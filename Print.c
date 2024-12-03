#include <string.h>
#include "db.h"

/*#define NODUMP*/

#define MAXBUF	10000
char buffer[MAXBUF];
FILE *out;

int wasNLA = 0;

unsigned long int theend;

void PrintTitle(txt)
char *txt;
{
	int width, wm;
	int i, j;


	i= 0;
	width = 0;
	wm = 0;

	if(strlen(txt)==1 && txt[0]==' ')
	{
		fprintf(out, "\n\n*\n*\n*\n\n");
		return;
	}

	while(txt[i]!='\0' )
	{
		if(txt[i]=='\n' )
		{
			wm = MAX(wm, width);
			width = 0;
		}
		else if(strncmp(txt+i, SEPARATOR, SEPARATORLENGTH)==0)
		{
			wm = MAX(wm, width);
			width = 0;
			i+=SEPARATORLENGTH-1;
		}
		else
		{
			++width;
		}
		++i;
	}
	wm = MAX(width, wm);

	fprintf(out, "\n\n");
	for(i=0;i<wm+4;++i) fprintf(out, "*"); fprintf(out, "\n");

	fprintf(out, "* ");
	i=0;
	width = 0;
	while(txt[i]!='\0')
	{
		if(txt[i]=='\n')
		{
			for(j=width; j<wm; ++j) fprintf(out, " ");
			fprintf(out, " *\n* ");
			width = 0;
		}
		else if(strncmp(txt+i, SEPARATOR, SEPARATORLENGTH)==0)
		{
			for(j=width; j<wm; ++j) fprintf(out, " ");
			fprintf(out, " *\n* ");
			width = 0;
			i+=SEPARATORLENGTH-1;
		}
		else
		{
			fprintf(out, "%c", txt[i]);
			++width;
		}
		++i;
	}

	for(j=width; j<wm; ++j) fprintf(out, " ");
	fprintf(out, " *\n");
	for(i=0;i<wm+4;++i) fprintf(out, "*"); fprintf(out, "\n\n");
							 
}

unsigned long int PrintComment(address, txt, level)
unsigned long int address;
char *txt;
int level;
{
	unsigned char quartets[100000];

	unsigned long int addr;
	int i, j, k;
	int l;
	int n;

	quartets[0] = HTOA(DB_Read1(address));
	i=1;
	addr = address + 1;
	while(DB_Used(addr) == 1 && DB_COT(addr)==0)
	{
		quartets[i] = HTOA(DB_Read1(addr));
		++addr;
		++i;
	}

	quartets[i]='\0';

	i=j=0;


	while(txt[j]!='\0'||quartets[i]!='\0')
	{
		if(quartets[i]!='\0')
		{
			int k;
			fprintf(out, "%05X : ", address);
			k=0;

			while(k<(txt[j]=='\0'?60:10)  && quartets[i]!='\0')
			{
				fprintf(out, "%c", quartets[i]);
				++i;
				++k;

				if((k%5==0 && txt[j]=='\0') || (k==5) || (k==10)) fprintf(out, " ");
			}

			address+=k;

			while(k<(txt[j]=='\0'?60:10))
			{
				fprintf(out, " ");
				++k;
				if((k%5==0 && txt[j]=='\0') || (k==5) || (k==10)) fprintf(out, " ");
			}
		}
		else
		{
			fprintf(out, "                    ");
		}

		if(txt[j]!='\0')
		{
			fprintf(out, " ");
		
			for(l=0; l<level; ++l)
			{
				fprintf(out, "  ");
			}

			k = 0;

			while(txt[j]!='\n'&&txt[j]!='\0'&&
					/* (strncmp(txt+j, SEPARATOR, SEPARATORLENGTH) != 0)&& */
					k<(55-2*level))
			{
				fprintf(out, "%c", txt[j]);
				++j;
				++k;
			}
			if(txt[j]=='\n') ++j;
			/*
			if(strncmp(txt+j, SEPARATOR, SEPARATORLENGTH) == 0)
			{
				j += SEPARATORLENGTH;
			}
			*/
		}

		fprintf(out, "\n");
	}

	return addr;
}

unsigned long int DumpIt(address)
unsigned long int address;
{
	int n;
	int flag;

#ifdef NODUMP

	++address;

	while(address < theend && ! DB_COT(address))
	{
		DisplayIt(++address, out);
	}

	return address;

#endif

	fprintf(out, "\n\n");
	for(n=0;n<80;++n) fprintf(out, "-");
	fprintf(out, "\n");

	n = 0;
	flag = 1; /* dump at least 1 nib */

	while(flag || DB_COT(address) == 0)
	{
		if(n==0)
		{
			if(address>=theend) return address;
			fprintf(out, "%05X : ", address);
			DisplayIt(address, out);
		}

		flag = 0;
		fprintf(out, "%X", DB_Read1(address));
		++address;
		++n;

		if((n%5)==0) fprintf(out, " ");

		if(n==60)
		{
			n=0;
			fprintf(out, "\n");
		}
	}

	fprintf(out, "\n");
	for(n=0;n<80;++n) fprintf(out, "-");
	fprintf(out, "\n\n");

	return address;
}


unsigned long int PrintIt(address)
unsigned long int address;
{
	int level;
	int flag = 0;
	unsigned long int next;
	int flag2;

	DB_GetTitle(address, buffer);

	if(strlen(buffer) > 0)
	{
		PrintTitle(buffer);
		flag = 1;
	}

	if(DB_GetNLB(address) && !wasNLA)
	{
		fprintf(out, "\n");
	}

	flag2 = DB_GetNLA(address);

	level = DB_GetComment(address, buffer);

	if(strlen(buffer))
	{
		next = PrintComment(address, buffer, level);

		if(flag2)
		{
			fprintf(out, "\n");
		}

		wasNLA = flag2;

		if(next != address) return next;
	}
	else
	{
		wasNLA = 0;
	}
	return DumpIt(address);
}

void LetsDumpIt(file, i)
char *file;
int i;
{
	int j;
	int a, l, f;

	InitDisplay();
	fprintf(stderr, "-> %s <-\n", file);
	out = fopen(file, "w");

	for(j=0; j<NMOD; ++j) ActivesModules[j] = 0;
	ActivesModules[i] = 1;

	a = DB_ModStart(i);
	l = DB_ModSize(i);

	theend = f = a+l;

	fprintf(stderr, "Start : %05X - Size : %05X - End : %05X\n", a, l, f-1);

	while(a<f)
	{
		DisplayIt(a, out);
		a = PrintIt(a);
	}

	DisplayIt(a, out);

	fprintf(stderr, "\n\n\n");

	fclose(out);
}

int main(argc, argv)
int argc;
char **argv;
{
	int i;
	unsigned long int start[5];
	unsigned long int size[5];

	fprintf(stderr, "\n\n-------> Printing database...\n\n");

	DB_OpenFiles();

	LetsDumpIt("Dump_rom", ROM_MOD);
	LetsDumpIt("Dump_io",   IO_MOD);
	LetsDumpIt("Dump_ram", RAM_MOD);
	LetsDumpIt("Dump_card1", CARD1_MOD);
	LetsDumpIt("Dump_card2", CARD2_MOD);
	LetsDumpIt("Dump_card3", CARD3_MOD);

	DB_CloseFiles();
}
