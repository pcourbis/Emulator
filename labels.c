#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include "db.h"
#include "labels.h"

struct label *labels;

FILE *new_labels;

struct label *FindLabel(address)
unsigned long int address;
{
	struct label *l;

	l = labels;

	while(l!=NULL && l->address!=address)
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
#ifdef SYNC_FILES
	fflush(file);
#endif
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
#ifdef SYNC_FILES
		fflush(new_labels);
#endif

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
#ifdef SYNC_FILES
	fflush(new_labels);
#endif

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
	new_labels = fopen("entries.a", "w");
	hplabs = fopen("Entries.a", "r");

	fprintf(stderr, "Loading labels...\n");

	while(GetEntry(hplabs, &address, name))
	{
		AddLabel(address, name);
	}

	fclose(hplabs);
}

void CloseLabels()
{
	fclose(new_labels);
}

