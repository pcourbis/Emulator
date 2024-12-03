#include <string.h>
#include <malloc.h>
#include "db.h"

extern void exit();

FILE *comments;
#ifdef CMT_IN_MEM
long int CommentTell;
long int CommentEnd;
char *CommentsInMem;
#endif

struct module module[NMOD];

int ActivesModules[NMOD] = { 1, 1, 1, 1, 1, 1 };

long int old;

void InitDisplay()
{
	old = -1;
}

void DisplayIt(address, out)
unsigned long int address;
FILE *out;
{
    long int new;

    new = (long int) address / 0x1000;
    if(new<=old) return;
    old = new;
    fprintf(stderr, "Current address is %05X\n", address);
    fflush(out);
}

void openit(basename, i, j)
char *basename;
int i;
int j;
{
	char namer[100], namew[100];
	sprintf(namer, "%s_r", basename);
	sprintf(namew, "%s_w", basename);
	module[i].file_r = fopen(namer, "r+");
	module[i].file_w = fopen(namew, "r+");

	if(i != IO_MOD && module[i].file_w != NULL)
	{
		fclose(module[i].file_w);
		module[i].file_w = module[i].file_r;
	}

	if(module[i].file_r != NULL)
	{
		fread((char *)&module[i].infos, sizeof(struct dbinfo), 1, module[i].file_r);
	}
	else
	{
		return;
	}

#ifdef WORK_IN_MEMORY
	fprintf(stderr, "Loading <%s> into memory....\n", basename);

	module[i].data_r = (struct dbe *)
			malloc((unsigned)(module[i].infos.real_size * sizeof(struct dbe) * module[i].infos.nbrofbanks));

	if(module[i].data_r == NULL)
	{
		fprintf(stderr, "Malloc failed : not enough memory available !\n");
		exit(i);
	}

	fseek(module[i].file_r, (long)sizeof(struct dbinfo), 0);
	fread((char *) module[i].data_r,
	    sizeof(struct dbe), (int)module[i].infos.real_size * module[i].infos.nbrofbanks, module[i].file_r);

	if(module[i].file_w == NULL)  return;
	if(i != IO_MOD)
	{
		module[i].data_w = module[i].data_r;
		return;
	}

	module[i].data_w = (struct dbe *)
		malloc((unsigned)(module[i].infos.real_size * sizeof(struct dbe) * module[i].infos.nbrofbanks));

	if(module[i].data_w == NULL)
	{
		fprintf(stderr, "Malloc failed : no more memory available !\n");
		exit(i);
	}

	fseek(module[i].file_r, (long)sizeof(struct dbinfo), 0);
	fread((char *)module[i].data_w, sizeof(struct dbe),
				(int)module[i].infos.real_size * module[i].infos.nbrofbanks, module[i].file_w);
#endif
}

void DB_OpenFiles()
{
	openit(DB_ROM, ROM_MOD);
	openit(DB_IO, IO_MOD);
	openit(DB_RAM, RAM_MOD);
	openit(DB_CARD1, CARD1_MOD);
	openit(DB_CARD2, CARD2_MOD);
	openit(DB_CARD3, CARD3_MOD);

	comments = fopen(COMMENTS, "r+");
	fseek(comments, (long)0, 2);
#ifdef CMT_IN_MEM
	CommentTell = CommentEnd = ftell(comments);
	fseek(comments, 0L, 0);
	CommentsInMem = (char *)malloc(CMT_SPACE);
	if(CommentsInMem==NULL)
	{
		fprintf(stderr, "Unable to mallocate mem for comments...\n");
		exit(33);
	}
	fprintf(stderr, "Loading comments into memory...\n");
	fread(CommentsInMem, 1, CommentEnd, comments);
#endif

	module[IO_MOD].infos.real_size = IOSIZE;
	module[IO_MOD].infos.configured = CONFIGURED;
	module[IO_MOD].infos.address = 0x00100;
	module[IO_MOD].infos.size = IOSIZE;
}

void WriteInfos()
{
	int i;

	for(i=0;i<NMOD;++i)
	{
		if(module[i].file_r != NULL)
		{
			fseek(module[i].file_r, 0L, 0);
			fwrite((char *)&module[i].infos, sizeof(struct dbinfo), 1, module[i].file_r);
#ifdef SYNC_FILES
			fflush(module[i].file_r);
#endif
		}
	}
}

void DumpComments()
{
#ifdef CMT_IN_MEM
	fprintf(stderr, "Dumping comments on disk...\n");
	fseek(comments, 0L, 0);
	fwrite(CommentsInMem, 1, CommentEnd, comments);
#endif
}

#ifdef WORK_IN_MEMORY
void DumpFile(i)
int i;
{
	fprintf(stderr, "Dumping file %d back on disk...\n", i);

	fseek(module[i].file_r, 0L, 0);
	fwrite((char *)&module[i].infos, sizeof(struct dbinfo), 1, module[i].file_r);

	fseek(module[i].file_r, (long)sizeof(struct dbinfo), 0);

	fwrite((char *)module[i].data_r, sizeof(struct dbe), 
						(int) module[i].infos.real_size * module[i].infos.nbrofbanks, module[i].file_r);

	if(module[i].data_w != NULL && i == IO_MOD)
	{
		fseek(module[i].file_w, (long)sizeof(struct dbinfo), 0);
		fwrite((char *)module[i].data_w, sizeof(struct dbe),
					(int) module[i].infos.real_size * module[i].infos.nbrofbanks, module[i].file_w);
	}
}
void DumpFiles()
{
	int i;
	for(i=0; i<NMOD;++i)
	{
		DumpFile(i);
	}
	DumpComments();
}
#endif

void DB_CloseIt(i)
int i;
{
#ifdef WORK_IN_MEMORY
	DumpFile(i);
	free((char *)module[i].data_r);
	if(i==IO_MOD) free((char *)module[i].data_w);
#endif
	fclose(module[i].file_r);
	if(i==IO_MOD) fclose(module[i].file_w);
}

void DB_CloseFiles()
{
	int i;

	WriteInfos();

	for(i=0;i<NMOD;++i)
	{
		if(module[i].file_r != NULL)
		{
			DB_CloseIt(i);
		}
	}
	DumpComments();
}

int DB_IsIn(infos, address, i)
struct dbinfo infos;
unsigned long int address;
int i;
{
	if(ActivesModules[i]==0) return 0;
	if(infos.configured != CONFIGURED) return 0;
	if(address<infos.address) return 0;
	if(address>=(infos.address+infos.size)) return 0;

	return 1;
}

int DB_WhichFile(address)
unsigned long int address;
{
	int i;

	for(i=1;i<NMOD;++i)
	{
		if(DB_IsIn(module[i].infos, address, i)) return i;
	}

	if(DB_IsIn(module[ROM_MOD].infos, address, ROM_MOD)) return ROM_MOD;

	return NOWHERE;
}

unsigned long int DB_ModStart(i)
int i;
{
	if(module[i].file_r == NULL || module[i].infos.configured != CONFIGURED)
	{
		return 0L;
	}
	else
	{
		return module[i].infos.address;
	}
}
unsigned long int DB_ModSize(i)
int i;
{
	if(module[i].file_r == NULL || module[i].infos.configured != CONFIGURED)
	{
		return 0L;
	}
	else
	{
		return module[i].infos.size;
	}
}


void Deconfigure(i)
int i;
{
	if(module[i].file_r == NULL) return;

	module[i].infos.address = 0;
	module[i].infos.size = module[i].infos.real_size;
	module[i].infos.configured = UNCONFIGURED;
}

void DB_Unconfigure(address)
unsigned long int address;
{
	int i;

	i = DB_WhichFile(address);

	if(i == NOWHERE || i == ROM_MOD) return;

	Deconfigure(i);

#ifdef WORK_ON_DISK
	WriteInfos();
#endif
}

void DB_UnconfigureAll()
{
	int i;

	for(i=1; i<NMOD; ++i)
	{
		Deconfigure(i);
	}

#ifdef WORK_ON_DISK
	WriteInfos();
#endif
}

unsigned long int DB_ID()
{
	int i;

	i = 0;

	while(i<NMOD)
	{
		if(module[i].file_r!=NULL)
		{
			if(module[i].infos.configured != CONFIGURED)
			{
				fprintf(stderr, "Warning : C=ID is not sure !\n");

				switch(i)
				{
					case ROM_MOD:
						fprintf(stderr, "C=ID 0 Impossible !\n");
						exit(1);

					case IO_MOD:
					case RAM_MOD:
						fprintf(stderr, "C=ID 1/2 Doubtful !\n");
						exit(2);

					case CARD1_MOD:
						switch(module[i].infos.configured)
						{
							case UNCONFIGURED:
								return 0x100000-module[i].infos.real_size+0x005;
								break;

							case SIZECONFIGURED:
								return module[i].infos.address+0x0f6;
								break;
						}
						break;

					case CARD2_MOD:
						switch(module[i].infos.configured)
						{
							case UNCONFIGURED:
								return 0x100000-module[i].infos.real_size+0x007;
								break;

							case SIZECONFIGURED:
								return module[i].infos.address+0x0f8;
								break;
						}
						break;

					case CARD3_MOD:
						switch(module[i].infos.configured)
						{
							case UNCONFIGURED:
								return 0x100000-module[i].infos.real_size+0x001;
								break;

							case SIZECONFIGURED:
								return module[i].infos.address+0x0f2;
								break;
						}
						break;
				}
			}
		}
		++i;
	}

	return 0L;
}

void DB_Config(arg)
unsigned long int arg;
{
	int i;
			
	for(i=0;i<NMOD;++i)
	{
		if(module[i].file_r != NULL &&
				module[i].infos.configured != CONFIGURED) break;
	}

	if(i==NMOD) return;

	if(i==IO_MOD)
	{
		module[i].infos.size = module[i].infos.real_size;
		module[i].infos.configured = SIZECONFIGURED;
	}

	switch(module[i].infos.configured)
	{
		case UNCONFIGURED:
			arg = (0x100000 - arg) % 0x100000;
			module[i].infos.size = arg;
			module[i].infos.configured = SIZECONFIGURED;
			break;

		case SIZECONFIGURED:
			module[i].infos.address = arg;
			module[i].infos.configured = CONFIGURED;
			break;

		default:
			fprintf(stderr, "BUG : Bad configuration...\n");
			break;
	}

#ifdef WORK_ON_DISK
	WriteInfos();
#endif
}

int ItemIndex(address, infos)
unsigned long int address;
struct dbinfo infos;
{
	int index;

	index =  ((((address-infos.address)%infos.real_size))
				+infos.real_size*infos.currentbank);

	return index;
}


int DB_Put(data, address, ep, infos)
#ifdef WORK_ON_DISK
FILE *data;
#endif
#ifdef WORK_IN_MEMORY
struct dbe *data;
#endif
unsigned long int address;
struct dbe *ep;
struct dbinfo infos;
{

#ifdef WORK_ON_DISK
	fseek(data,
		(long)(sizeof(struct dbinfo)+
		ItemIndex(address, infos)*sizeof(struct dbe)), 0);

	fwrite((char *)ep, sizeof(struct dbe), 1, data);

#ifdef SYNC_FILES
	fflush(data);
#endif

#endif

#ifdef WORK_IN_MEMORY
	data[ItemIndex(address, infos)] = *ep;
#endif
}

void DB_Get(data, address, ep, use, infos)
#ifdef WORK_ON_DISK
FILE *data;
#endif
#ifdef WORK_IN_MEMORY
struct dbe *data;
#endif
unsigned long int address;
struct dbe *ep;
int use;
struct dbinfo infos;
{
#ifdef WORK_ON_DISK
	fseek(data, (long)(sizeof(struct dbinfo)+
			ItemIndex(address, infos)*sizeof(struct dbe)), 0);

	fread((char *)ep, sizeof(struct dbe), 1, data);
#endif

#ifdef WORK_IN_MEMORY
	*ep = data[ItemIndex(address, infos)];
#endif

	if(use)
	{
		ep->flags |= FLAG_USED;

#ifdef WORK_ON_DISK
		fseek(data, (long)(sizeof(struct dbinfo)+
		    ItemIndex(address, infos)*sizeof(struct dbe)), 0);

		fwrite((char *)ep, sizeof(struct dbe), 1, data);

#ifdef SYNC_FILES
		fflush(data);
#endif

#endif

#ifdef WORK_IN_MEMORY
		data[ItemIndex(address, infos)] = *ep;
#endif
	}
}

void DB_PutEntryR(address, ep)
unsigned long int address;
struct dbe *ep;
{
	int i;

	i =DB_WhichFile(address);

	if(i != NOWHERE)
	{
#ifdef WORK_ON_DISK
		DB_Put(module[i].file_r, address, ep, module[i].infos);
#endif

#ifdef WORK_IN_MEMORY
		DB_Put(module[i].data_r, address, ep, module[i].infos);
#endif
	}
}
	
void DB_PutEntryW(address, ep)
unsigned long int address;
struct dbe *ep;
{
	int i;

	i =DB_WhichFile(address);

	if(i != NOWHERE)
	{
#ifdef WORK_ON_DISK
		DB_Put(module[i].file_w, address, ep, module[i].infos);
#endif

#ifdef WORK_IN_MEMORY
		DB_Put(module[i].data_w, address, ep, module[i].infos);
#endif
	}
}
	
void DB_GetEntryW(address, ep, use)
unsigned long int address;
struct dbe *ep;
int use;
{
	int r;

	r=DB_WhichFile(address);

	if(r != NOWHERE)
	{
#ifdef WORK_ON_DISK
		DB_Get(module[r].file_w, address, ep, use, module[r].infos);
#endif

#ifdef WORK_IN_MEMORY
		DB_Get(module[r].data_w, address, ep, use, module[r].infos);
#endif
	}
	else
	{
		ep->quartet = 0;
		ep->flags=0;
		ep->comment=0;
		ep->level=0;
	}
}
void DB_GetEntryR(address, ep, use)
unsigned long int address;
struct dbe *ep;
int use;
{
	int r;

	r=DB_WhichFile(address);

	if(r != NOWHERE)
	{
#ifdef WORK_ON_DISK
		DB_Get(module[r].file_r, address, ep, use, module[r].infos);
#endif

#ifdef WORK_IN_MEMORY
		DB_Get(module[r].data_r, address, ep, use, module[r].infos);
#endif
	}
	else
	{
		ep->quartet = 0;
		ep->flags=0;
		ep->comment=0;
		ep->level=0;
	}
}
	
void DB_WriteN(n, address, data)
unsigned long int address;
int n;
unsigned long int data;
{
	int i;
	struct dbe quartet;

	i = DB_WhichFile(address);
	if(i==NOWHERE || module[i].file_w == NULL) return;

	for(i=0;i<n; ++i)
	{
		DB_GetEntryW(address+i, &quartet, 0);
		quartet.quartet = data % 0x10;
		DB_PutEntryW(address+i, &quartet);
		data /= 0x10;
	}
}

void DB_WriteIOW(n, address, data)
unsigned long int address;
int n;
unsigned long int data;
{
	int i;
	struct dbe quartet;

	for(i=0;i<n; ++i)
	{

#ifdef WORK_ON_DISK
		DB_Get(module[IO_MOD].file_w,
						address+i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						0,
						module[IO_MOD].infos);
#endif

#ifdef WORK_IN_MEMORY
		DB_Get(module[IO_MOD].data_w,
						address+i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						0,
						module[IO_MOD].infos);
#endif

		quartet.quartet = data % 0x10;

#ifdef WORK_ON_DISK
		DB_Put(module[IO_MOD].file_w,
						address+i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						module[IO_MOD].infos);
#endif
		
#ifdef WORK_IN_MEMORY
		DB_Put(module[IO_MOD].data_w,
						address+i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						module[IO_MOD].infos);
#endif
		
		data /= 0x10;
	}
}


void DB_WriteIOR(n, address, data)
unsigned long int address;
int n;
unsigned long int data;
{
	int i;
	struct dbe quartet;

	for(i=0;i<n; ++i)
	{

#ifdef WORK_ON_DISK
		DB_Get(module[IO_MOD].file_r,
						address+i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						0,
						module[IO_MOD].infos);
#endif

#ifdef WORK_IN_MEMORY
		DB_Get(module[IO_MOD].data_r,
						address+i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						0,
						module[IO_MOD].infos);
#endif

		quartet.quartet = data % 0x10;

#ifdef WORK_ON_DISK
		DB_Put(module[IO_MOD].file_r,
						address+i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						module[IO_MOD].infos);
#endif
		
#ifdef WORK_IN_MEMORY
		DB_Put(module[IO_MOD].data_r,
						address+i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						module[IO_MOD].infos);
#endif
		
		data /= 0x10;
	}
}

unsigned long int DB_GetN(n, address, use)
int n;
unsigned long int address;
int use;
{
	unsigned long int result;
	int i;
	struct dbe quartet;

	result = 0x0;

	for(i=1;i<=n; ++i)
	{
		DB_GetEntryR(address+n-i, &quartet, use);
		result = result * 0x10 + quartet.quartet;
	}

	return result;
}

unsigned long int DB_ReadIOR(n, address)
int n;
unsigned long int address;
{
	unsigned long int result;
	int i;
	struct dbe quartet;

	result = 0x0;

	for(i=1;i<=n; ++i)
	{

#ifdef WORK_ON_DISK
		DB_Get(module[IO_MOD].file_r,
						address+n-i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						0,
						module[IO_MOD].infos);
#endif

#ifdef WORK_IN_MEMORY
		DB_Get(module[IO_MOD].data_r,
						address+n-i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						0,
						module[IO_MOD].infos);
#endif

		result = result * 0x10 + quartet.quartet;
	}

	return result;
}

void DB_WriteIO(n, address, data)
int n;
unsigned long int address, data;
{
	DB_WriteIOR(n, address, data);
	DB_WriteIOW(n, address, data);
}

unsigned long int DB_ReadIOW(n, address)
int n;
unsigned long int address;
{
	unsigned long int result;
	int i;
	struct dbe quartet;

	result = 0x0;

	for(i=1;i<=n; ++i)
	{
#ifdef WORK_ON_DISK
		DB_Get(module[IO_MOD].file_w,
						address+n-i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						0,
						module[IO_MOD].infos);
#endif

#ifdef WORK_IN_MEMORY
		DB_Get(module[IO_MOD].data_w,
						address+n-i - 0x100 + module[IO_MOD].infos.address ,
						&quartet,
						0,
						module[IO_MOD].infos);
#endif

		result = result * 0x10 + quartet.quartet;
	}

	return result;
}

void WriteCommentString(str)
char *str;
{
#ifdef CMT_IN_MEM
	if((CommentTell+1+strlen(str))>CMT_SPACE)
	{
		fprintf(stderr, "Too Much Comments !!!\n");
		exit(66);
	}
	strcpy(CommentsInMem+CommentTell, str);
	CommentTell+=strlen(str)+1;
	if(CommentTell>CommentEnd) CommentEnd = CommentTell;
#else
	fprintf(comments, "%s", str);
	putc('\0', comments);
#endif
}

void DB_Title(address, text)
unsigned long int address;
char *text;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);

#ifdef CMT_IN_MEM
	q.title = CommentTell = CommentEnd;
#else
	fseek(comments, 0L, 2);
	q.title = ftell(comments);
#endif

	DB_PutEntryR(address, &q);
	if(strlen(text))
	{	
		WriteCommentString(text);
	}
	else
	{
		WriteCommentString(" ");
	}
#ifdef SYNC_FILES
	fflush(comments);
#endif
}

void DB_NLA(address, flag)
unsigned long int address;
int flag;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);
	q.flags = ((q.flags|FLAG_NLA)-FLAG_NLA)+FLAG_NLA*flag;
	DB_PutEntryR(address, &q);
}
void DB_NLB(address, flag)
unsigned long int address;
int flag;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);
	q.flags = ((q.flags|FLAG_NLB)-FLAG_NLB)+FLAG_NLB*flag;
	DB_PutEntryR(address, &q);
}

int DB_GetNLA(address)
unsigned long int address;
{
	struct dbe q;
	DB_GetEntryR(address, &q, 0);
	return (int)(q.flags&FLAG_NLA);
}

int DB_GetNLB(address)
unsigned long int address;
{
	struct dbe q;
	DB_GetEntryR(address, &q, 0);
	return (int)(q.flags&FLAG_NLB);
}

void DB_UnComment(address)
unsigned long int address;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);

	q.level = 0;
	q.comment = 0;

	DB_PutEntryR(address, &q);
}
void DB_SetLevel(address, level)
unsigned long int address;
int level;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);

	q.level = (unsigned char)MAX(level, q.level);
	DB_PutEntryR(address, &q);
}
void DB_Comment(address, text, level)
unsigned long int address;
char *text;
int level;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);

#ifdef CMT_IN_MEM
	q.comment = CommentTell = CommentEnd;
#else
	fseek(comments, 0L, 2);
	q.comment = ftell(comments);
#endif

	q.level = (unsigned char)MAX(level, q.level);
	DB_PutEntryR(address, &q);
	if(strlen(text))
	{
		WriteCommentString(text);
	}
	else
	{
		WriteCommentString(" ");
	}

#ifdef SYNC_FILES
	fflush(comments);
#endif
}

int DB_C(address)
unsigned long int address;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);
	if(q.comment != 0) return 1;
	return 0;
}
int DB_T(address)
unsigned long int address;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);

	if(q.title != 0) return 1;

	return 0;
}
int DB_COT(address)
unsigned long int address;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);

	if(q.comment != 0) return 1;
	if(q.title != 0) return 1;

	return 0;
}

int DB_Level(address)
unsigned long int address;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);
	return q.level;
}

int DB_Object(address)
unsigned long int address;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);

	return q.flags&FLAG_ISOBJ;
}

int DB_External(address)
unsigned long int address;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);

	return q.flags&FLAG_EXTER;
}

void DB_SetObject(address, flag)
unsigned long int address;
unsigned char flag;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);
	q.flags = ((q.flags|FLAG_ISOBJ)-FLAG_ISOBJ)+flag*FLAG_ISOBJ;
	DB_PutEntryR(address, &q);
}
void DB_SetExternal(address, flag)
unsigned long int address;
unsigned char flag;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);
	q.flags = ((q.flags|FLAG_EXTER)-FLAG_EXTER)+flag*FLAG_EXTER;
	DB_PutEntryR(address, &q);
}
int DB_Used(address)
unsigned long int address;
{
	struct dbe q;

	DB_GetEntryR(address, &q, 0);

	return q.flags&FLAG_USED;
}

int DB_GetComment(address, text)
unsigned long int address;
char *text;
{
	struct dbe q;
	int c;
	int i;

	DB_GetEntryR(address, &q, 0);

	if(q.comment == 0)
	{
		text[0]='\0';
	}
	else
	{
		i=0;
#ifdef CMT_IN_MEM
		CommentTell = (long)q.comment;
		c = CommentsInMem[CommentTell++];
#else
		fseek(comments, (long)q.comment, 0);
		c=getc(comments);
#endif
		while(c != '\0' && c != EOF)
		{
			text[i++] = c;
#ifdef CMT_IN_MEM
			c = CommentsInMem[CommentTell++];
#else
			c=getc(comments);
#endif
		}
#ifdef CMT_IN_MEM
		CommentTell = CommentEnd;
#else
		fseek(comments, 0L, 2);
#endif
		text[i] = '\0';
	}

	return q.level;
}

void DB_GetTitle(address, text)
unsigned long int address;
char *text;
{
	struct dbe q;
	int c;
	int i;

	DB_GetEntryR(address, &q, 0);

	if(q.title == 0)
	{
		text[0]='\0';
	}
	else
	{
		i=0;
#ifdef CMT_IN_MEM
		CommentTell = (long)q.title;
		c = CommentsInMem[CommentTell++];
#else
		fseek(comments, (long)q.title, 0);
		c=getc(comments);
#endif

		while(c != '\0' && c != EOF)
		{
			text[i++] = c;
#ifdef CMT_IN_MEM
			c = CommentsInMem[CommentTell++];
#else
			c=getc(comments);
#endif
		}

		text[i] = '\0';
#ifdef CMT_IN_MEM
		CommentTell = CommentEnd;
#else
		fseek(comments, 0L, 2);
#endif
	}
}

