#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "db.h"

void mkrom()
{
	FILE *rom, *romdb;
	unsigned long int size;
	char romr[100];

	struct dbe q;
	int c;

	struct dbinfo info;

	size = 0;

	q.comment = 0;
	q.title = 0;
	q.flags = 0;
	q.level = 0;


	rom = fopen("rom", "r");
	sprintf(romr, "%s_r", DB_ROM);
	romdb=fopen(romr, "w");

	fwrite(&info, sizeof(struct dbinfo), 1, romdb);

	while((c=getc(rom))!=EOF)
	{
		q.quartet = ATOH(c);
		fwrite(&q, sizeof(struct dbe), 1, romdb);
		++size;

		if((size%0x1000)==0)
		{
			fprintf(stderr, "Rom -> current address : %05X\n", size);
		}
	}

	info.address = 0x0;
	info.size  = size;
	info.real_size  = size;
	info.configured = CONFIGURED; /* always configured at 0... */
	info.nbrofbanks = 1;
	info.currentbank = 0;

	fseek(romdb, 0L, 0);
	fwrite(&info, sizeof(struct dbinfo), 1, romdb);

	fclose(rom);
	fclose(romdb);
}

void mkempty(size, name, banks)
unsigned long int size;
char *name;
int banks;
{
	unsigned long int i;
	struct dbinfo info;
	struct dbe q;

	FILE *file;

	file = fopen(name, "w");

	info.nbrofbanks = banks;
	info.currentbank = 0;

	info.address = 0x0;
	info.real_size = size;
	info.configured = UNCONFIGURED;

	q.quartet = 0;
	q.comment = 0;
	q.title = 0;
	q.level = 0;
	q.flags = 0;

	fwrite(&info, sizeof(struct dbinfo), 1, file);

	for(i=0;i<size*banks;++i)
	{
		fwrite(&q, sizeof(struct dbe), 1, file);
		if((i%0x1000)==0)
		{
			fprintf(stderr, "%s -> current address : %02X-%05X\n",
					name, i/size, i%size);
		}
	}

	fclose(file);
}


void mkio()
{
	char io[100];
	sprintf(io, "%s_r", DB_IO);
	mkempty(IOSIZE, io, 1);
	sprintf(io, "%s_w", DB_IO);
	mkempty(IOSIZE, io, 1);
}

void mkcard(basename, size, ro, banks)
char *basename;
unsigned long int size;
int ro;
int banks;
{
	char namer[100], namew[100];
	sprintf(namer, "%s_r", basename);
	sprintf(namew, "%s_w", basename);
	mkempty(size, namer, banks);
	link(namer, namew);
	if(ro) unlink(namew);
}

int main()
{
	FILE *comments;
	int i;
	char buffer[100];

	mkrom();
	mkio();
	mkcard(DB_RAM, RAMSIZE, 0, 1);

 
	mkcard(DB_CARD1, BCNTRLSIZE, 1, 1);
	mkcard(DB_CARD2, CARD32SIZE, 1, 1);
	mkcard(DB_CARD3, CARD128SIZE, 1, 1);
	comments = fopen(COMMENTS, "w");
	fprintf(comments, "Comment's database\n");
	fclose(comments);
}
