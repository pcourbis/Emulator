#include <stdio.h>


/* #define WORK_ON_DISK  */

#ifndef WORK_ON_DISK
#define		WORK_IN_MEMORY
#ifndef		CMT_ON_DISK
#define			CMT_IN_MEM
#define			CMT_SPACE	(20*1024*1024) /* 10 Mo for comments */
#endif
#else
/* #define SYNC_FILES */
#endif

struct dbe
{
	unsigned char quartet;

	long int comment;
	long int title;

	unsigned char flags;

#define FLAG_USED	0x01
#define FLAG_EXTER	0x02
#define FLAG_NLA	0x04
#define FLAG_NLB	0x08
#define FLAG_ISOBJ	0x10

	unsigned char level;
};

struct dbinfo
{
	unsigned long int address;
	unsigned long int size;

	unsigned long int real_size;

	char configured;

	int nbrofbanks;
	int currentbank;
};

#define UNCONFIGURED	0
#define SIZECONFIGURED	1
#define CONFIGURED		2


struct module
{
	FILE *file_r;
	FILE *file_w;


#ifdef WORK_IN_MEMORY
	struct dbe *data_r;
	struct dbe *data_w;
#endif
	struct dbinfo infos;
};

#define ATOH(q)	(((q)<'A')?((q)-'0'):((q)-'A'+10))
#define HTOA(q) (((q)<0xA)?((q)+'0'):((q)+'A'-10))

#define NMOD		6

#define	IOSIZE		0x40
#define	RAMSIZE		0x40000
#define BCNTRLSIZE	0x01000
#define	CARD32SIZE	0x10000
#define	CARD128SIZE	0x40000

#define DB_ROM		"Rom.db"
#define DB_IO		"Io.db"
#define DB_RAM		"Ram.db"
#define DB_CARD1	"BankController.db"
#define DB_CARD2	"Card1.db"
#define DB_CARD3	"MultiBankCard2.db"

#define COMMENTS	"Comments.db"

#define NOWHERE	-1

/* Declaration des fichiers de la base */

extern FILE *comments;

/* DB functions : */

void DB_OpenFiles();
void DB_CloseFiles();
extern void DB_Comment();
extern void DB_Title();

unsigned long int DB_GetN();

#define DB_ReadN(n, address)	DB_GetN((int)(n), (unsigned long int)(address), 0)
#define DB_Read1(address)		DB_ReadN(1, (address))
#define DB_Read2(address)		DB_ReadN(2, (address))
#define DB_Read3(address)		DB_ReadN(3, (address))
#define DB_Read4(address)		DB_ReadN(4, (address))
#define DB_Read5(address)		DB_ReadN(5, (address))

#define DB_UseN(n, address)		DB_GetN((int)(n), (unsigned long int)(address), 1)
#define DB_Use1(address)		DB_UseN(1, (address))
#define DB_Use2(address)		DB_UseN(2, (address))
#define DB_Use3(address)		DB_UseN(3, (address))
#define DB_Use4(address)		DB_UseN(4, (address))
#define DB_Use5(address)		DB_UseN(5, (address))

void DB_WriteN(int n, unsigned long int address, unsigned long int data);
void DB_WriteIOW(int n, unsigned long int address, unsigned long int data);
void DB_WriteIOR(int n, unsigned long int address, unsigned long int data);
void DB_WriteIO(int n, unsigned long int address, unsigned long int data);


#define DB_Write1(address, data)		DB_WriteN(1, (address), data)
#define DB_Write2(address, data)		DB_WriteN(2, (address), data)
#define DB_Write3(address, data)		DB_WriteN(3, (address), data)
#define DB_Write4(address, data)		DB_WriteN(4, (address), data)
#define DB_Write5(address, data)		DB_WriteN(4, (address), data)

int DB_GetComment();
void DB_GetTitle();
int DB_COT();
int DB_C();
int DB_T();
int DB_Used();

void DB_NLB();
void DB_NLA();
int DB_GetNLB();
int DB_GetNLA();

int DB_External();
void DB_SetExternal();

unsigned long int DB_ReadIOW();
unsigned long int DB_ReadIOR();
void DB_WriteIOR();
void DB_WriteIO();

int DB_Level();

void InitDisplay();
void DisplayIt();

void DB_Unconfigure();
void DB_UnconfigureAll();

void DB_Config();

#define SEPARATOR		" / "
#define SEPARATORLENGTH	strlen(SEPARATOR)

#define MAX(x, y)    (((x)>(y))?(x):(y))

extern struct module module[NMOD];

#define ROM_MOD     0
#define IO_MOD      1
#define RAM_MOD     2
#define CARD1_MOD   3
#define CARD2_MOD   4
#define CARD3_MOD   5

extern unsigned long int DB_ID();

extern unsigned long int DB_ModSize();
extern unsigned long int DB_ModStart();

extern int ActivesModules[NMOD];

extern void DB_SetObject();
extern int DB_Object();

extern void DB_UnComment();
extern void DB_SetLevel();

extern int DB_WhichFile(unsigned long int address);

extern void DumpFiles();
