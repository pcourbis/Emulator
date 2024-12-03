#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "db.h"
#include "addresses.h"
#include "labels.h"
#include "interface.h"

extern void exit();

extern Display *mydisplay;

void UpdateIO();

int keyout[] = 
{
	0x002, 0x100, 0x100, 0x100, 0x100, 0x100, 
	0x004, 0x080, 0x080, 0x080, 0x080, 0x080,
	0x001, 0x040, 0x040, 0x040, 0x040, 0x040,
	0x008, 0x020, 0x020, 0x020, 0x020, 0x020,
	0x010, 0x010, 0x010, 0x010, 0x010,
	0x008, 0x008, 0x008, 0x008, 0x008,
	0x004, 0x004, 0x004, 0x004, 0x004,
	0x002, 0x002, 0x002, 0x002, 0x002,
	0x400, 0x001, 0x001, 0x001, 0x001
};

int keyin[] =
{
	0x0010, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
	0x0010, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
	0x0010, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
	0x0010, 0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
	0x0010, 0x0008, 0x0004, 0x0002, 0x0001,
	0x0020, 0x0008, 0x0004, 0x0002, 0x0001,
	0x0020, 0x0008, 0x0004, 0x0002, 0x0001,
	0x0020, 0x0008, 0x0004, 0x0002, 0x0001,
	0x8000, 0x0008, 0x0004, 0x0002, 0x0001
};

extern int keymask[49];

/* #define DUBUGIT */

#ifdef DUBUGIT
FILE *debug;
#endif

int displayit;

char *FieldA[] =
{
	"(p)", "(wp)", "(xs)", "(x)", "(s)", "(m)", "(b)", "(w)",
	"BUG", "BUG", "BUG", "BUG", "BUG", "BUG", "BUG", "BUG"
};

char *FieldB[] =
{
	"BUG", "BUG", "BUG", "BUG", "BUG", "BUG", "BUG", "BUG",
	"(p)", "(wp)", "(xs)", "(x)", "(s)", "(m)", "(b)", "(w)"
};

char *FieldF[] =
{
	"(p)", "(wp)", "(xs)", "(x)", "(s)", "(m)", "(b)", "(w)",
	"BUG", "BUG", "BUG", "BUG", "BUG", "BUG", "BUG", "(a)"
};

char *FieldF2[] =
{
	"(p)", "(wp)", "(xs)", "(x)", "(s)", "(m)", "(b)", "(w)",
	"(a)", "BUG", "BUG", "BUG", "BUG", "BUG", "BUG", "BUG"
};

int Mode;

#define MODE_HEX	0
#define MODE_DEC	1

struct Register
{
	unsigned char qu[16];
};

unsigned char Interrupts_OK;
unsigned char Interrupt_Pending;
unsigned char Interrupt_Wakeup;
unsigned char Interrupt_Requested;
unsigned char Processing_Interrupt;
unsigned char Interrupt_When_Rti;

unsigned int InterruptClass;

unsigned long int Registre_PC;

struct Register Registre_A, Registre_B, Registre_C, Registre_D, Registres_R[5];
unsigned long int Registres_D[2];

unsigned long int Registre_OUT, Registre_IN;

unsigned char Registre_CARRY, Registre_HST;
unsigned int Registre_STATUS;

unsigned long int Registres_RSTK[8];

unsigned long int CYCLES;

int Registre_P;


#define FIELD_P		0
#define FIELD_WP	1
#define FIELD_XS	2
#define FIELD_X		3
#define FIELD_S		4
#define FIELD_M		5
#define FIELD_B		6
#define FIELD_W		7

#define FIELD_A		8

int FieldF2aA[16] =
{
	0, 1, 2, 3, 4, 5, 6, 7,
	999, 999, 999, 999, 999, 999, 999, 
	FIELD_A
};  

int fs(f)
int f;
{
	switch(f)
	{
		case FIELD_P:  return Registre_P;
		case FIELD_WP: return 0;
		case FIELD_XS: return 2;
		case FIELD_X:  return 0;
		case FIELD_S:  return 15;
		case FIELD_M:  return 3;
		case FIELD_B:  return 0;
		case FIELD_W:  return 0;
		case FIELD_A:  return 0;
	}
	return 999;
}

int fe(f)
int f;
{
	switch(f)
	{
		case FIELD_P:  return Registre_P;
		case FIELD_WP: return Registre_P;
		case FIELD_XS: return 2;
		case FIELD_X:  return 2;
		case FIELD_S:  return 15;
		case FIELD_M:  return 14;
		case FIELD_B:  return 1;
		case FIELD_W:  return 15;
		case FIELD_A:  return 4;
	}
	return 999;
}

void AddCycles(n)
int n;
{
	CYCLES += n;
}

#define Add2Cycles(n)	AddCycles((int)(n))
#define AddToCycles(n)	AddCycles((int)(n))

void AddRCN(b, q, a)
int b, q;
unsigned long int a;
{
    b += q + (q/2);

    if(q&1) b+= (a&1);

    AddToCycles(b);
}


void AddRCF(b, f, a)
int b, f;
unsigned long int a;
{
	int q;

	q = 1 + fe(f)-fs(f);

	b += q + (q/2);

	if(q&1) b+= (a&1);

	AddToCycles(b);
}

void DO_exit(e)
int e;
{
	CloseWindows();
	DB_CloseFiles();
	CloseLabels();
	exit(e);
}

void Decimal()
{
	if(Mode == MODE_HEX) return;
	fprintf(stderr, "DECIMAL NOT SUPPORTED !!!!!!!!\n");
	sleep(5);
	DO_exit(9);
}

unsigned char Add(q1, q2)
unsigned char q1, q2;
{
	unsigned char result;

	result = q1+q2+Registre_CARRY;

	if(Mode == MODE_HEX)
	{
		Registre_CARRY=((result/16)!=0);
		return result % 16;
	}
	else
	{
		Registre_CARRY = ((result/10)!=0);
		return result %10;
	}
}

unsigned char Or(q1, q2)
unsigned char q1, q2;
{
	return q1|q2;
}

unsigned char And(q1, q2)
unsigned char q1, q2;
{
	return q1&q2;
}

unsigned char Sub(q1, q2)
unsigned char q1, q2;
{
	unsigned char result;


	if(Mode == MODE_HEX)
	{
		if(q1 < (q2+Registre_CARRY))
		{	
			result = q1 + 0x10 -q2 - Registre_CARRY;
			Registre_CARRY = 1;
		}
		else
		{
			result = q1-q2 - Registre_CARRY;
			Registre_CARRY = 0;
		}
	}
	else
	{
		if(q1 < (q2+Registre_CARRY))
		{	
			result = q1 + 10 -q2 - Registre_CARRY;
			Registre_CARRY = 1;
		}
		else
		{
			result = q1-q2 - Registre_CARRY;
			Registre_CARRY = 0;
		}
	}

	return result;
}

void Perform2(Operation, Arg1, Arg2, Result, qs, qe, c)
unsigned char (*Operation)();
struct Register *Arg1, *Arg2, *Result;
int qs, qe;
int c;
{
	int car;
	if(c) Registre_CARRY=0;
	else car = Registre_CARRY;

	Result->qu[qs] = (*Operation)(Arg1->qu[qs], Arg2->qu[qs]);

	while(qs!=qe)
	{
		++qs; qs%=16;
		Result->qu[qs] = (*Operation)(Arg1->qu[qs], Arg2->qu[qs]);
	}

	if(!c) Registre_CARRY = car;
}

void Perform1(Operation, r, qs, qe, x, c)
unsigned char (*Operation)();
struct Register *r;
int qs, qe;
int x;
int c;
{
	int car;

	if(c) Registre_CARRY=0;
	else car = Registre_CARRY;

	r->qu[qs] = (*Operation)(r->qu[qs], x);

	while(qs!=qe)
	{
		++qs; qs%=16;
		r->qu[qs] = (*Operation)(r->qu[qs], 0);
	}

	if(!c) Registre_CARRY = car;
}

unsigned char Negate(q, dummy)
unsigned char q, dummy;
{
	return Sub(0, q);
}


unsigned char Zero(q, dummy)
int q, dummy;
{
	return 0;
}

unsigned char Copy(q, dummy)
int q, dummy;
{
	return q;
}

unsigned char Diff(q1, q2)
int q1, q2;
{
	Registre_CARRY |= (q1 != q2);
	return q1;
}

unsigned char NotZero(q1, dummy)
int q1, dummy;
{
	Registre_CARRY |= (q1!=0);
	return q1;
}

struct Register dummy;

#define AddRegisters(a1, a2, res, f)	Perform2(Add, &a1, &a2, &res, fs(f), fe(f), 1)
#define AndRegisters(a1, a2, res, f)	Perform2(And, &a1, &a2, &res, fs(f), fe(f), 0)
#define OrRegisters(a1, a2, res, f)	Perform2(Or, &a1, &a2, &res, fs(f), fe(f), 0)
#define SubRegisters(a1, a2, res, f)	Perform2(Sub, &a1, &a2, &res, fs(f), fe(f), 1)

#define IncRegister(r, f)               Perform1(Add, &r, fs(f), fe(f), 1, 1)
#define DecRegister(r, f)               Perform1(Sub, &r, fs(f), fe(f), 1, 1)

#define ZeroRegister(r, f)              Perform1(Zero, &r, fs(f), fe(f), 1, 0)
#define NegRegister(r, f)               Perform1(Negate, &r, fs(f), fe(f), 1, 1)

#define CopyRegisters(r1, r2, f)			Perform2(Copy, &r1, &r2, &r2, fs(f), fe(f), 0)
#define DiffRegisters(r1, r2, f)	Perform2(Diff, &r1, &r2, &dummy, fs(f), fe(f), 1)
#define NotZeroRegister(r1, f)	Perform2(NotZero, &r1, &dummy, &dummy, fs(f), fe(f), 1)

#define IncRegisterByX(r, f, x)             Perform1(Add, &r, fs(f), fe(f), x, 1)
#define DecRegisterByX(r, f, x)             Perform1(Sub, &r, fs(f), fe(f), x, 1)

void CommentIt(text)
char *text;
{
	char oldc[1000];

	if(DB_C(Registre_PC))
	{
		DB_GetComment(Registre_PC, oldc);
		if(!IsIn(text, oldc))
		{
			fprintf(stderr, "Warning : diff. coments at %05X\n", Registre_PC);
			strcat(oldc, SEPARATOR);
			strcat(oldc, text);
			DB_Comment(Registre_PC, oldc, 0);
		}
		return;
	}

	DB_Comment(Registre_PC, text, 0);
}

void Comment_TT(t1, t2)
char *t1, *t2;
{
	char buffer[80];

	strcpy(buffer, t1);

	while(strlen(buffer)<10)
	{
		strcat(buffer, " ");
	}

	strcat(buffer, t2);

	CommentIt(buffer);
}

void Comment_IA(str, arg, n)
char *str;
unsigned long int arg;
int n;
{
	char buffer[1000];
	char fmt[1000];
	char buf2[1000];

	if(n==0)
	{
		CommentIt(str);
		return;
	}

	strcpy(buffer, str);
	while(strlen(buffer)<10)
	{
		strcat(buffer, " ");
	}

	sprintf(fmt, "#%%0%dX", n);
	sprintf(buf2, fmt, arg);
	strcat(buffer, buf2);

	CommentIt(buffer);
}

void Comment_IG(str, arg)
char *str;
unsigned long int arg;
{
	char buffer[1000];
	struct label *lab;
	char buf2[10];

	strcpy(buffer, str);
	while(strlen(buffer)<10)
	{
		strcat(buffer, " ");
	}

	sprintf(buf2, "#%05X", arg);
	strcat(buffer, buf2);

	lab = FindLabel(arg);

	if(lab != NULL)
	{
		strcat(buffer, " (");
		strcat(buffer, lab->txt);
		strcat(buffer, ")");

		DB_Title(arg, lab->txt);
	}
	else
	{
		DB_Title(arg, "");
	}

	CommentIt(buffer);
}

void Comment_IF(str, arg)
char *str;
unsigned long int arg;
{
	char buffer[80];
	char buf2[10];

	strcpy(buffer, str);
	while(strlen(buffer)<10)
	{
		strcat(buffer, " ");
	}

	sprintf(buf2, "(%d)", arg);
	strcat(buffer, buf2);

	CommentIt(buffer);
}

void PushRstk(v)
unsigned long int v;
{
	int i;

	for(i=7; i>0; --i) Registres_RSTK[i] = Registres_RSTK[i-1];

	Registres_RSTK[0] = v;
}

unsigned long int PopRstk()
{
	int i;
	unsigned long int v;

	v = Registres_RSTK[0];

	for(i=1; i<8; ++i) Registres_RSTK[i-1] = Registres_RSTK[i];

	Registres_RSTK[7] = 0;

	return v;
}

unsigned long int Reg2UL(r, f)
struct Register r;
int f;
{
	int qs, qe;
	unsigned long int result;
	int i;

	qs = fs(f);
	qe = fe(f);

	result = 0;

	result = r.qu[qs];
	i=0;

	while(qs!=qe)
	{
		++i;
		++qs; qs %= 16;
		result += (r.qu[qs]<<(i*4));
	}

	return result;
}		

void UL2Reg(v, r, f)
unsigned long int v;
struct Register *r;
int f;
{
	int qs, qe;

	qs = fs(f);
	qe = fe(f);

	r->qu[qs] = v & 0xf;

	while(qs!=qe)
	{
		++qs; qs %= 16;
		v = v / 0x10;
		r->qu[qs] = v & 0xf;
	}

}		

void goyes(address, cyclesy, cyclesn)
unsigned long int address;
int cyclesy;
int cyclesn;
{
	unsigned long int off, to;
	int c;

	Registre_PC = address;

	off = DB_Use2(address);
	DB_NLA(Registre_PC, 1);

	if(off==0)
	{
		Comment_IA( "rtnyes", 0L, 0);
	}
	else
	{
		if(off<0x80)
		{
			to = address + off;
		}
		else
		{
			to = address + off - 0x100;
		}
		Comment_IA("goyes", to, 5);
		DB_NLB(to, 1);
	}

	if(Registre_CARRY==0)
	{
		to = Registre_PC + 2;
		c= cyclesn;
	}
	else
	{
		c = cyclesy;
		if(off == 0)
		{
			to = PopRstk();
		}
	}

	AddToCycles(c);

	Registre_PC = to;
}

int ReadQ(address)
unsigned long int address;
{
	int w;
	unsigned long int crc;
	int q;

	w = DB_WhichFile(address);

	q = (int) DB_Use1(address);

	if(w == RAM_MOD || w == CARD1_MOD || w == CARD2_MOD)
	{
		crc = DB_ReadIOR(4, IO_CRC);
		crc = (crc / 0x10) ^ (((q ^ crc) & 0xf) * 0x1081);
		DB_WriteIO(4, IO_CRC, crc);
	}

	UpdateIO();

	return q;
}

void ReadRegisterNP(address, r, n, p)
unsigned long int address;
struct Register *r;
int n, p;
{
	int i;
	char comment[1000];
	char length[100];
	int base;
	int mod, bank;

	if(DB_WhichFile(address)== CARD1_MOD)
	{
		base = address - module[CARD1_MOD].infos.address;
		if(base<0x40)
		{
			mod = CARD2_MOD;
			bank = (base/2)%module[mod].infos.nbrofbanks;
			module[mod].infos.currentbank = bank;
			fprintf(stderr, "Switching card 1 to bank %02X (asked: %02X)\n",bank, base/2);
			fflush(stderr);
		}
		else if(base <0x80)
		{
			mod = CARD3_MOD;
			bank = ((base-0x40)/2) %module[mod].infos.nbrofbanks;
			module[mod].infos.currentbank = bank;
			fprintf(stderr, "Switching card 2 to bank %02X (asked: %02X)\n",bank, (base-0x40)/2);
			fflush(stderr);
		}
		else
		{
			fprintf(stderr, "Warning : bad switcher area !\n");
			fflush(stderr);
		}
	}


	for(i=0; i<n; ++i)
	{
		r->qu[(i+p)%16] = ReadQ(address+i);
	}

	sprintf(length, "Nibbles used here : %d (R)", n);

	DB_GetComment(address, comment);

	if(strlen(comment) && IsIn(length, comment))
	{
		return;
	}

	if(strlen(comment)) strcat(comment, SEPARATOR);
	strcat(comment, length);

	DB_Comment(address, comment, 0);
}

void ReadRegisterF(address, r, f)
unsigned long int address;
struct Register *r;
int f;
{
	ReadRegisterNP(address, r, fe(f)-fs(f)+1, fs(f));
}

void WriteRegisterNP(address, r, n, p)
unsigned long int address;
struct Register *r;
int n, p;
{
	int i;
	char comment[1000];
	char length[100];

	for(i=0; i<n; ++i)
	{
		DB_Use1(address+i);
		DB_Write1(address+i, (unsigned long int) r->qu[(i+p)%16] );
	}

	sprintf(length, "Nibbles used here : %d (W)", n);

	DB_GetComment(address, comment);

	if(strlen(comment) && IsIn(length, comment))
	{
		return;
	}

	if(strlen(comment)) strcat(comment, SEPARATOR);
	strcat(comment, length);

	DB_Comment(address, comment, 0);
}

void WriteRegisterF(address, r, f)
unsigned long int address;
struct Register *r;
int f;
{
	WriteRegisterNP(address, r, fe(f)-fs(f)+1, fs(f));
}

void Execute_00()
{
	unsigned long int rtn;

	Comment_IA("rtnsxm", (unsigned long int)0, 0);
	DB_NLA(Registre_PC, 1);
	rtn = PopRstk();
	Registre_HST |= 0x1;
	AddToCycles(11);
	Registre_PC = rtn;
}

void Execute_01()
{
	unsigned long int rtn;
	Comment_IA("rtn", (unsigned long int)0, 0);
	DB_NLA(Registre_PC, 1);
	rtn = PopRstk();
	AddToCycles(11);
	Registre_PC = rtn;
}

void Execute_02()
{
	unsigned long int rtn;
	Comment_IA("rtnsc", (unsigned long int)0, 0);
	DB_NLA(Registre_PC, 1);
	rtn = PopRstk();
	Registre_CARRY = 1;
	AddToCycles(11);
	Registre_PC = rtn;
}

void Execute_03()
{
	unsigned long int rtn;
	Comment_IA("rtncc", (unsigned long int)0, 0);
	DB_NLA(Registre_PC, 1);
	rtn = PopRstk();
	Registre_CARRY = 0;
	AddToCycles(11);
	Registre_PC = rtn;
}

void Execute_04()
{
	Comment_IA("sethex", (unsigned long int)0, 0);
	Mode = MODE_HEX;
	AddToCycles(4);
	Registre_PC += 2;
}

void Execute_05()
{
	Comment_IA("setdec", (unsigned long int)0, 0);
	Mode = MODE_DEC;
	AddToCycles(4);
	Registre_PC += 2;
}

void Execute_06()
{
	Comment_IA("rstk=c", (unsigned long int)0, 0);
	PushRstk(Reg2UL(Registre_C, FIELD_A));
	AddToCycles(9);
	Registre_PC+=2;
}

void Execute_07()
{
	Comment_IA("c=rstk", (unsigned long int)0, 0);
	UL2Reg(PopRstk(), &Registre_C, FIELD_A);
	AddToCycles(9);
	Registre_PC += 2;
}

void Execute_08()
{
	Comment_IA("clrst", (unsigned long int)0, 0);
	Registre_STATUS = 0;
	AddToCycles(7);
	Registre_PC += 2;
}

void Execute_09()
{	
	Comment_IA("c=st", (unsigned long int)0, 0);
	UL2Reg((unsigned long int)(Registre_STATUS&0xfff), &Registre_C, FIELD_X);
	AddToCycles(7);
	Registre_PC+=2;
}

void Execute_0A()
{
	Comment_IA("st=c", (unsigned long int)0, 0);
	Registre_STATUS = (Registre_STATUS & 0xf000) | Reg2UL(Registre_C, FIELD_X);
	AddToCycles(7);
	Registre_PC+=2;
}

void Execute_0B()
{
	unsigned long int tempo;
	Comment_IA("cstex", (unsigned long int)0, 0);
	tempo = Reg2UL(Registre_C, FIELD_X);
	UL2Reg((unsigned long int)Registre_STATUS, &Registre_C, FIELD_X);
	Registre_STATUS = (Registre_STATUS&0xf000) | tempo;
	AddToCycles(7);
	Registre_PC+=2;
}

void Execute_0C()
{
	Comment_IA("p=p+1", (unsigned long int)0, 0);
	++Registre_P;
	Registre_CARRY = Registre_P > 0xf;
	Registre_P &= 0xf;
	AddToCycles(4);
	Registre_PC += 2;
}

void Execute_0D()
{
	Comment_IA("p=p-1", (unsigned long int)0, 0);
	if(Registre_P==0)
	{
		Registre_CARRY = 1;
		Registre_P =0xf;
	}
	else
	{
		--Registre_P;
		Registre_CARRY=0;
	}
	AddToCycles(4);
	Registre_PC += 2;
}

void Execute_0Ef0(f)
int f;
{
	Comment_TT("a=a&b", FieldF2[f]);
	AndRegisters(Registre_A, Registre_B, Registre_A, f);
}

void Execute_0Ef1(f)
int f;
{
	Comment_TT("b=b&c", FieldF2[f]);
	AndRegisters(Registre_B, Registre_C, Registre_B, f);
}

void Execute_0Ef2(f)
int f;
{
	Comment_TT("c=c&a", FieldF2[f]);
	AndRegisters(Registre_C, Registre_A, Registre_C, f);
}

void Execute_0Ef3(f)
int f;
{
	Comment_TT("d=d&c", FieldF2[f]);
	AndRegisters(Registre_D, Registre_C, Registre_D, f);
}

void Execute_0Ef4(f)
int f;
{
	Comment_TT("b=b&a", FieldF2[f]);
	AndRegisters(Registre_B, Registre_A, Registre_B, f);
}

void Execute_0Ef5(f)
int f;
{
	Comment_TT("c=c&b", FieldF2[f]);
	AndRegisters(Registre_C, Registre_B, Registre_C, f);
}

void Execute_0Ef6(f)
int f;
{
	Comment_TT("a=a&c", FieldF2[f]);
	AndRegisters(Registre_A, Registre_C, Registre_A, f);
}

void Execute_0Ef7(f)
int f;
{
	Comment_TT("c=c&d", FieldF2[f]);
	AndRegisters(Registre_C, Registre_D, Registre_C, f);
}

void Execute_0Ef8(f)
int f;
{
	Comment_TT("a=a!b", FieldF2[f]);
	OrRegisters(Registre_A, Registre_B, Registre_A, f);
}

void Execute_0Ef9(f)
int f;
{
	Comment_TT("b=b!c", FieldF2[f]);
	OrRegisters(Registre_B, Registre_C, Registre_B, f);
}

void Execute_0EfA(f)
int f;
{
	Comment_TT("c=c!a", FieldF2[f]);
	OrRegisters(Registre_C, Registre_A, Registre_C, f);
}

void Execute_0EfB(f)
int f;
{
	Comment_TT("d=d!c", FieldF2[f]);
	OrRegisters(Registre_D, Registre_C, Registre_D, f);
}

void Execute_0EfC(f)
int f;
{
	Comment_TT("b=b!a", FieldF2[f]);
	OrRegisters(Registre_B, Registre_A, Registre_B, f);
}

void Execute_0EfD(f)
int f;
{
	Comment_TT("c=c!b", FieldF2[f]);
	OrRegisters(Registre_C, Registre_B, Registre_C, f);
}

void Execute_0EfE(f)
int f;
{
	Comment_TT("a=a!c", FieldF2[f]);
	OrRegisters(Registre_A, Registre_C, Registre_A, f);
}

void Execute_0EfF(f)
int f;
{
	Comment_TT("c=c!d", FieldF2[f]);
	OrRegisters(Registre_C, Registre_D, Registre_C, f);
}


void Execute_0E()
{
	int f;
	int q;

	f = FieldF2aA[DB_Use1(Registre_PC + 2)];
	q = DB_Use1(Registre_PC + 3);


	switch(q)
	{
		case 0x0: Execute_0Ef0(f); break;
		case 0x1: Execute_0Ef1(f); break;
		case 0x2: Execute_0Ef2(f); break;
		case 0x3: Execute_0Ef3(f); break;
		case 0x4: Execute_0Ef4(f); break;
		case 0x5: Execute_0Ef5(f); break;
		case 0x6: Execute_0Ef6(f); break;
		case 0x7: Execute_0Ef7(f); break;
		case 0x8: Execute_0Ef8(f); break;
		case 0x9: Execute_0Ef9(f); break;
		case 0xA: Execute_0EfA(f); break;
		case 0xB: Execute_0EfB(f); break;
		case 0xC: Execute_0EfC(f); break;
		case 0xD: Execute_0EfD(f); break;
		case 0xE: Execute_0EfE(f); break;
		case 0xF: Execute_0EfF(f); break;
	}

	AddToCycles(6+1+fe(f)-fs(f));
	Registre_PC += 4;
}

void Execute_0F()
{
	unsigned long int rtn;
	Comment_IA("rti", (unsigned long int)0, 0);
	fprintf(stderr, "Warning : rti used at %05X\n", Registre_PC);
	DB_NLA(Registre_PC, 1);
	rtn = PopRstk();
	AddToCycles(11);
	Registre_PC = rtn;
	Processing_Interrupt = 0;
	if(Interrupt_When_Rti) Interrupt_Pending = 1;
}


void Execute_0()
{
	int q;

	q = DB_Use1(Registre_PC+1);

	switch(q)
	{
		case 0x0: Execute_00(); break;
		case 0x1: Execute_01(); break;
		case 0x2: Execute_02(); break;
		case 0x3: Execute_03(); break;
		case 0x4: Execute_04(); break;
		case 0x5: Execute_05(); break;
		case 0x6: Execute_06(); break;
		case 0x7: Execute_07(); break;
		case 0x8: Execute_08(); break;
		case 0x9: Execute_09(); break;
		case 0xA: Execute_0A(); break;
		case 0xB: Execute_0B(); break;
		case 0xC: Execute_0C(); break;
		case 0xD: Execute_0D(); break;
		case 0xE: Execute_0E(); break;
		case 0xF: Execute_0F(); break;
	}
}

void CopyR2intoR1(r1, r2, s1, s2)
struct Register *r1, *r2;
char *s1, *s2;
{
	int i;
	char buffer[40];
	sprintf(buffer, "%s=%s", s1, s2);
	Comment_IA(buffer, (unsigned long int)0, 0);
	for(i=0;i<16;++i)
	{
		r1->qu[i]=r2->qu[i];
	}
}

#define CopyR1intoR2(r1, r2, s1, s2) CopyR2intoR1(r2, r1, s2, s1)

void Execute_10()
{
	int f;

	f = (int) DB_Use1(Registre_PC+2);

	switch(f)
	{
		case 0x0: CopyR1intoR2(&Registre_A, &Registres_R[0], "a", "r0"); break;
		case 0x1: CopyR1intoR2(&Registre_A, &Registres_R[1], "a", "r1"); break;
		case 0x2: CopyR1intoR2(&Registre_A, &Registres_R[2], "a", "r2"); break;
		case 0x3: CopyR1intoR2(&Registre_A, &Registres_R[3], "a", "r3"); break;
		case 0x4: CopyR1intoR2(&Registre_A, &Registres_R[4], "a", "r4"); break;
		case 0x8: CopyR1intoR2(&Registre_C, &Registres_R[0], "c", "r0"); break;
		case 0x9: CopyR1intoR2(&Registre_C, &Registres_R[1], "c", "r1"); break;
		case 0xA: CopyR1intoR2(&Registre_C, &Registres_R[2], "c", "r2"); break;
		case 0xB: CopyR1intoR2(&Registre_C, &Registres_R[3], "c", "r3"); break;
		case 0xC: CopyR1intoR2(&Registre_C, &Registres_R[4], "c", "r4"); break;

		default : fprintf(stderr, "Unsup 10%X\n", f); DO_exit(7);
	}

	AddToCycles(20+(Registre_PC&1));
	Registre_PC+=3;
}


void Execute_11()
{
	int f;

	f = (int) DB_Use1(Registre_PC+2);

	switch(f)
	{
		case 0x0: CopyR2intoR1(&Registre_A, &Registres_R[0], "a", "r0"); break;
		case 0x1: CopyR2intoR1(&Registre_A, &Registres_R[1], "a", "r1"); break;
		case 0x2: CopyR2intoR1(&Registre_A, &Registres_R[2], "a", "r2"); break;
		case 0x3: CopyR2intoR1(&Registre_A, &Registres_R[3], "a", "r3"); break;
		case 0x4: CopyR2intoR1(&Registre_A, &Registres_R[4], "a", "r4"); break;
		case 0x8: CopyR2intoR1(&Registre_C, &Registres_R[0], "c", "r0"); break;
		case 0x9: CopyR2intoR1(&Registre_C, &Registres_R[1], "c", "r1"); break;
		case 0xA: CopyR2intoR1(&Registre_C, &Registres_R[2], "c", "r2"); break;
		case 0xB: CopyR2intoR1(&Registre_C, &Registres_R[3], "c", "r3"); break;
		case 0xC: CopyR2intoR1(&Registre_C, &Registres_R[4], "c", "r4"); break;

		default : fprintf(stderr, "Unsup 11%X\n", f); DO_exit(6);
	}

	AddToCycles(20+(Registre_PC&1));
	Registre_PC+=3;
}


void ExchangeRRn(r1, r2, s1, s2)
struct Register *r1, *r2;
char *s1, *s2;
{
	int i, v;
	char buffer[40];
	sprintf(buffer, "%s%sex", s1, s2);
	Comment_IA(buffer, (unsigned long int)0, 0);
	for(i=0;i<15;++i)
	{
		v = r1->qu[i];
		r1->qu[i]=r2->qu[i];
		r2->qu[i]=v;
	}
}

void Execute_12()
{
	int f;

	f = (int) DB_Use1(Registre_PC+2);

	switch(f)
	{
		case 0x0: ExchangeRRn(&Registre_A, &Registres_R[0], "a", "r0"); break;
		case 0x1: ExchangeRRn(&Registre_A, &Registres_R[1], "a", "r1"); break;
		case 0x2: ExchangeRRn(&Registre_A, &Registres_R[2], "a", "r2"); break;
		case 0x3: ExchangeRRn(&Registre_A, &Registres_R[3], "a", "r3"); break;
		case 0x4: ExchangeRRn(&Registre_A, &Registres_R[4], "a", "r4"); break;
		case 0x8: ExchangeRRn(&Registre_C, &Registres_R[0], "c", "r0"); break;
		case 0x9: ExchangeRRn(&Registre_C, &Registres_R[1], "c", "r1"); break;
		case 0xA: ExchangeRRn(&Registre_C, &Registres_R[2], "c", "r2"); break;
		case 0xB: ExchangeRRn(&Registre_C, &Registres_R[3], "c", "r3"); break;
		case 0xC: ExchangeRRn(&Registre_C, &Registres_R[4], "c", "r4"); break;

		default : fprintf(stderr, "Unsup 12%X\n", f); DO_exit(10);
	}

	AddToCycles(20+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_130()
{
	Comment_IA("d0=a", (unsigned long int)0, 0);
	Registres_D[0]=Reg2UL(Registre_A, FIELD_A);
	AddToCycles(9+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_131()
{
	Comment_IA("d1=a", (unsigned long int)0, 0);
	Registres_D[1]=Reg2UL(Registre_A, FIELD_A);
	AddToCycles(9+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_132()
{
	unsigned long int tmp;
	Comment_IA("ad0ex", (unsigned long int)0, 0);
	tmp = Registres_D[0];
	Registres_D[0]=Reg2UL(Registre_A, FIELD_A);
	UL2Reg(tmp, &Registre_A, FIELD_A);
	AddToCycles(9+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_133()
{
	unsigned long int tmp;
	Comment_IA("ad1ex", (unsigned long int)0, 0);
	tmp = Registres_D[1];
	Registres_D[1]=Reg2UL(Registre_A, FIELD_A);
	UL2Reg(tmp, &Registre_A, FIELD_A);
	AddToCycles(9+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_134()
{
	Comment_IA("d0=c", (unsigned long int)0, 0);
	Registres_D[0]=Reg2UL(Registre_C, FIELD_A);
	AddToCycles(9+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_135()
{
	Comment_IA("d1=c", (unsigned long int)0, 0);
	Registres_D[1]=Reg2UL(Registre_C, FIELD_A);
	AddToCycles(9+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_136()
{
	unsigned long int tmp;
	Comment_IA("cd0ex", (unsigned long int)0, 0);
	tmp = Registres_D[0];
	Registres_D[0]=Reg2UL(Registre_C, FIELD_A);
	UL2Reg(tmp, &Registre_C, FIELD_A);
	AddToCycles(9+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_137()
{
	unsigned long int tmp;
	Comment_IA("cd1ex", (unsigned long int)0, 0);

	tmp = Registres_D[1];
	Registres_D[1]=Reg2UL(Registre_C, FIELD_A);
	UL2Reg(tmp, &Registre_C, FIELD_A);

	AddToCycles(9+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_138()
{
	Comment_IA("d0=as", (unsigned long int)0, 0);
	Registres_D[0] = (Registres_D[0]&0xf0000) +
					 (Reg2UL(Registre_A, FIELD_A) &0x0ffff);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_139()
{
	Comment_IA("d1=as", (unsigned long int)0, 0);
	Registres_D[1] = (Registres_D[1]&0xf0000) +
					 (Reg2UL(Registre_A, FIELD_A) &0x0ffff);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_13A()
{
	unsigned long int r, ptr;
	Comment_IA("ad0xs", (unsigned long int)0, 0);
	r = Reg2UL(Registre_A, FIELD_A);
	ptr=Registres_D[0];
	Registres_D[0] = (ptr&0xf0000)+(r&0x0ffff);
	UL2Reg((r&0xf0000)+(ptr&0x0ffff), &Registre_A, FIELD_A);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_13B()
{
	unsigned long int r, ptr;
	Comment_IA("ad1xs", (unsigned long int)0, 0);
	r = Reg2UL(Registre_A, FIELD_A);
	ptr=Registres_D[1];
	Registres_D[1] = (ptr&0xf0000)+(r&0x0ffff);
	UL2Reg((r&0xf0000)+(ptr&0x0ffff), &Registre_A, FIELD_A);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_13C()
{
	Comment_IA("d0=cs", (unsigned long int)0, 0);
	Registres_D[0] = (Registres_D[0]&0xf0000) +
					 (Reg2UL(Registre_C, FIELD_A) &0x0ffff);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_13D()
{
	Comment_IA("d1=cs", (unsigned long int)0, 0);
	Registres_D[1] = (Registres_D[1]&0xf0000) +
					 (Reg2UL(Registre_C, FIELD_A) &0x0ffff);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_13E()
{
	unsigned long int r, ptr;
	Comment_IA("cd0xs", (unsigned long int)0, 0);
	r = Reg2UL(Registre_C, FIELD_A);
	ptr=Registres_D[0];
	Registres_D[0] = (ptr&0xf0000)+(r&0x0ffff);
	UL2Reg((r&0xf0000)+(ptr&0x0ffff), &Registre_C, FIELD_A);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_13F()
{
	unsigned long int r, ptr;
	Comment_IA("cd1xs", (unsigned long int)0, 0);
	r = Reg2UL(Registre_C, FIELD_A);
	ptr=Registres_D[1];
	Registres_D[1] = (ptr&0xf0000)+(r&0x0ffff);
	UL2Reg((r&0xf0000)+(ptr&0x0ffff), &Registre_C, FIELD_A);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC += 3;
}


void Execute_13()
{
	int q;

	q = (int) DB_Use1(Registre_PC + 2);

	switch(q)
	{
		case 0x0: Execute_130(); break;
		case 0x1: Execute_131(); break;
		case 0x2: Execute_132(); break;
		case 0x3: Execute_133(); break;
		case 0x4: Execute_134(); break;
		case 0x5: Execute_135(); break;
		case 0x6: Execute_136(); break;
		case 0x7: Execute_137(); break;
		case 0x8: Execute_138(); break;
		case 0x9: Execute_139(); break;
		case 0xA: Execute_13A(); break;
		case 0xB: Execute_13B(); break;
		case 0xC: Execute_13C(); break;
		case 0xD: Execute_13D(); break;
		case 0xE: Execute_13E(); break;
		case 0xF: Execute_13F(); break;
	}
}

void Execute_140()
{
	Comment_TT("dat0=a", "(a)");
	WriteRegisterF(Registres_D[0], &Registre_A, FIELD_A);
	AddToCycles(19 + (Registre_PC&1));
	Registre_PC += 3;
}

void Execute_141()
{
	Comment_TT("dat1=a", "(a)");
	WriteRegisterF(Registres_D[1], &Registre_A, FIELD_A);
	AddToCycles(19 + (Registre_PC&1));
	Registre_PC += 3;
}

void Execute_142()
{
	Comment_TT("a=dat0", "(a)");
	ReadRegisterF(Registres_D[0], &Registre_A, FIELD_A);
	AddRCN((int)(12 + (Registre_PC&1)), 5, Registres_D[0]);
	Registre_PC += 3;
}

void Execute_143()
{
	Comment_TT("a=dat1", "(a)");
	ReadRegisterF(Registres_D[1], &Registre_A, FIELD_A);
	AddRCN((int)(12 + (Registre_PC&1)), 5, Registres_D[1]);
	Registre_PC += 3;
}

void Execute_144()
{
	Comment_TT("dat0=c", "(a)");
	WriteRegisterF(Registres_D[0], &Registre_C, FIELD_A);
	AddToCycles(19 + (Registre_PC&1));
	Registre_PC += 3;
}

void Execute_145()
{
	Comment_TT("dat1=c", "(a)");
	WriteRegisterF(Registres_D[1], &Registre_C, FIELD_A);
	AddToCycles(19 + (Registre_PC&1));
	Registre_PC += 3;
}

void Execute_146()
{
	Comment_TT("c=dat0", "(a)");
	ReadRegisterF(Registres_D[0], &Registre_C, FIELD_A);
	AddToCycles(20 + (Registre_PC&1) + 3 + (Registres_D[0]&1));
	Registre_PC+=3;
}

void Execute_147()
{
	Comment_TT("c=dat1", "(a)");
	ReadRegisterF(Registres_D[1], &Registre_C, FIELD_A);
	AddToCycles(20 + (Registre_PC & 1) + 3 + (Registres_D[1]&1));
	Registre_PC+=3;
}

void Execute_148()
{
	Comment_TT("dat0=a", "(b)");
	WriteRegisterF(Registres_D[0], &Registre_A, FIELD_B);
	AddToCycles(19 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_149()
{
	Comment_TT("dat1=a", "(b)");
	WriteRegisterF(Registres_D[1], &Registre_A, FIELD_B);
	AddToCycles(19 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_14A()
{
	Comment_TT("a=dat0", "(b)");
	ReadRegisterF(Registres_D[0], &Registre_A, FIELD_B);
	AddToCycles(19 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_14B()
{
	Comment_TT("a=dat1", "(b)");
	ReadRegisterF(Registres_D[1], &Registre_A, FIELD_B);
	AddToCycles(19 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_14C()
{
	Comment_TT("dat0=c", "(b)");
	WriteRegisterF(Registres_D[0], &Registre_C, FIELD_B);
	AddToCycles(19 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_14D()
{
	Comment_TT("dat1=c", "(b)");
	WriteRegisterF(Registres_D[1], &Registre_C, FIELD_B);
	AddToCycles(19 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_14E()
{
	Comment_TT("c=dat0", "(b)");
	ReadRegisterF(Registres_D[0], &Registre_C, FIELD_B);
	AddToCycles(19 + (Registre_PC & 1));
	Registre_PC+=3;
}

void Execute_14F()
{
	Comment_TT("c=dat1", "(b)");
	ReadRegisterF(Registres_D[1], &Registre_C, FIELD_B);
	AddToCycles(19 + (Registre_PC & 1));
	Registre_PC+=3;
}

void Execute_14()
{
	int q;

	q = (int) DB_Use1(Registre_PC + 2);

	switch(q)
	{
		case 0x0: Execute_140(); break;
		case 0x1: Execute_141(); break;
		case 0x2: Execute_142(); break;
		case 0x3: Execute_143(); break;
		case 0x4: Execute_144(); break;
		case 0x5: Execute_145(); break;
		case 0x6: Execute_146(); break;
		case 0x7: Execute_147(); break;
		case 0x8: Execute_148(); break;
		case 0x9: Execute_149(); break;
		case 0xA: Execute_14A(); break;
		case 0xB: Execute_14B(); break;
		case 0xC: Execute_14C(); break;
		case 0xD: Execute_14D(); break;
		case 0xE: Execute_14E(); break;
		case 0xF: Execute_14F(); break;
	}
}

void Execute_150()
{
	int f;
	f=(int)DB_Use1(Registre_PC+3);
	Comment_TT("dat0=a", FieldA[f]);
	WriteRegisterF(Registres_D[0], &Registre_A, f);
	AddToCycles(19+1+fe(f)-fs(f));
	Registre_PC += 4;
}

void Execute_151()
{
	int f;
	f=(int)DB_Use1(Registre_PC+3);
	Comment_TT("dat1=a", FieldA[f]);
	WriteRegisterF(Registres_D[1], &Registre_A, f);
	AddToCycles(19+1+fe(f)-fs(f));
	Registre_PC += 4;
}

void Execute_152()
{
	int f;
	f = (int) DB_Use1(Registre_PC + 3);
	Comment_TT("a=dat0", FieldA[f]);
	ReadRegisterF(Registres_D[0], &Registre_A, f);
	AddRCF(20, f, Registres_D[0]);
	Registre_PC += 4;
}

void Execute_153()
{
	int f;
	f = (int) DB_Use1(Registre_PC + 3);
	Comment_TT("a=dat1", FieldA[f]);
	ReadRegisterF(Registres_D[1], &Registre_A, f);
	AddRCF(20, f, Registres_D[1]);
	Registre_PC += 4;
}

void Execute_154()
{
	int f;
	f=(int)DB_Use1(Registre_PC+3);
	Comment_TT("dat0=c", FieldA[f]);
	WriteRegisterF(Registres_D[0], &Registre_C, f);
	AddToCycles(19+1+fe(f)-fs(f));
	Registre_PC += 4;
}

void Execute_155()
{
	int f;

	f = (int) DB_Use1(Registre_PC + 3);
	Comment_TT("dat1=c", FieldA[f]);
	WriteRegisterF(Registres_D[1], &Registre_C, f);
	AddToCycles(19 + 1 + fe(f) - fs(f));
	Registre_PC += 4;

}

void Execute_156()
{
	int f;
	f = (int) DB_Use1(Registre_PC + 3);
	Comment_TT("c=dat0", FieldA[f]);
	ReadRegisterF(Registres_D[0], &Registre_C, f);
	AddRCF(20, f, Registres_D[0]);
	Registre_PC += 4;
}

void Execute_157()
{
	int f;
	f = (int) DB_Use1(Registre_PC + 3);
	Comment_TT("c=dat1", FieldA[f]);
	ReadRegisterF(Registres_D[1], &Registre_C, f);
	AddRCF(20, f, Registres_D[1]);
	Registre_PC += 4;
}

void Execute_158()
{
	int l;
	unsigned long int address;

	l = 1 + DB_Use1(Registre_PC+3);
	Comment_IF("dat0=a", (unsigned long int)l);
	address = Registres_D[0];
	WriteRegisterNP(address, &Registre_A, l, 0);
	AddToCycles(18 + l);
	Registre_PC += 4;
}

void Execute_159()
{
	int l;
	unsigned long int address;

	l = 1 + DB_Use1(Registre_PC+3);
	Comment_IF("dat1=a", (unsigned long int)l);
	address = Registres_D[1];
	WriteRegisterNP(address, &Registre_A, l, 0);
	AddToCycles(18 + l);
	Registre_PC += 4;
}

void Execute_15A()
{
	int n;
	unsigned long int address;

	n = DB_Use1(Registre_PC + 3);
	Comment_IF("a=dat0", (unsigned long int)n+1);
	address = Registres_D[0];
	ReadRegisterNP(address, &Registre_A, n+1, 0);
	AddRCN(19, n+1, address);
	Registre_PC += 4;
}

void Execute_15B()
{
	int n;
	unsigned long int address;

	n = DB_Use1(Registre_PC + 3);
	Comment_IF("a=dat1", (unsigned long int)n+1);
	address = Registres_D[1];
	ReadRegisterNP(address, &Registre_A, n+1, 0);
	AddRCN(19, n+1, address);
	Registre_PC += 4;
}

void Execute_15C()
{
	int l;
	unsigned long int address;

	l = 1 + DB_Use1(Registre_PC+3);
	Comment_IF("dat0=c", (unsigned long int)l);
	address = Registres_D[0];
	WriteRegisterNP(address, &Registre_C, l, 0);
	AddToCycles(18 + l);
	Registre_PC += 4;
}

void Execute_15D()
{
	int l;
	unsigned long int address;

	l = 1 + DB_Use1(Registre_PC+3);
	Comment_IF("dat1=c", (unsigned long int)l);
	address = Registres_D[1];
	WriteRegisterNP(address, &Registre_C, l, 0);
	AddToCycles(18 + l);
	Registre_PC += 4;
}

void Execute_15E()
{
	int n;
	unsigned long int address;

	n = DB_Use1(Registre_PC + 3);
	Comment_IF("c=dat0", (unsigned long int)n+1);
	address = Registres_D[0];
	ReadRegisterNP(address, &Registre_C, n+1, 0);
	AddRCN(19, n+1, address);
	Registre_PC += 4;
}

void Execute_15F()
{
	int n;
	unsigned long int address;

	n = DB_Use1(Registre_PC + 3);
	Comment_IF("c=dat1", (unsigned long int)n+1);
	address = Registres_D[1];
	ReadRegisterNP(address, &Registre_C, n+1, 0);
	AddRCN(19, n+1, address);
	Registre_PC += 4;
}


void Execute_15()
{
	int q;

	q = (int) DB_Use1(Registre_PC + 2);

	switch(q)
	{
		case 0x0: Execute_150(); break;
		case 0x1: Execute_151(); break;
		case 0x2: Execute_152(); break;
		case 0x3: Execute_153(); break;
		case 0x4: Execute_154(); break;
		case 0x5: Execute_155(); break;
		case 0x6: Execute_156(); break;
		case 0x7: Execute_157(); break;
		case 0x8: Execute_158(); break;
		case 0x9: Execute_159(); break;
		case 0xA: Execute_15A(); break;
		case 0xB: Execute_15B(); break;
		case 0xC: Execute_15C(); break;
		case 0xD: Execute_15D(); break;
		case 0xE: Execute_15E(); break;
		case 0xF: Execute_15F(); break;
	}
}

void Execute_16()
{
	int n;

	n = 1 + DB_Use1(Registre_PC+2);
	Comment_IA("d0=d0+", (unsigned long int)n, 1);
	Registre_CARRY = 0;
	Registres_D[0]=Registres_D[0]+n;
	if(Registres_D[0]>0xfffff)
	{
		Registre_CARRY=1;
		Registres_D[0] &= 0xfffff;
	}
	AddToCycles(8 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_17()
{
	int n;

	n = 1 + DB_Use1(Registre_PC+2);
	Comment_IA("d1=d1+", (unsigned long int)n, 1);
	Registre_CARRY = 0;
	Registres_D[1]=Registres_D[1]+n;
	if(Registres_D[1]>0xfffff)
	{
		Registre_CARRY=1;
		Registres_D[1] &= 0xfffff;
	}
	AddToCycles(8 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_18()
{
	int n;

	n = 1 + DB_Use1(Registre_PC+2);
	Comment_IA("d0=d0-", (unsigned long int)n, 1);
	Registre_CARRY = 0;
	if(Registres_D[0]<n)
	{
		Registre_CARRY = 1;
		Registres_D[0] += 0x100000;
	}
	Registres_D[0]=Registres_D[0]-n;
	AddToCycles(8 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_19()
{
	unsigned long int arg;

	arg = DB_Use2(Registre_PC + 2);
	Comment_IA("d0(2)=", arg, 2);
	Registres_D[0] = (Registres_D[0] & 0xFFF00) + arg;
	AddToCycles(6);
	Registre_PC += 4;
}

void Execute_1A()
{
	unsigned long int arg;

	arg = DB_Use4(Registre_PC + 2);
	Comment_IA("d0(4)=", arg, 4);
	Registres_D[0] = (Registres_D[0] & 0xF0000) + arg;
	AddToCycles(9);
	Registre_PC += 6;
}

void Execute_1B()
{
	unsigned long int arg;

	arg = DB_Use5(Registre_PC + 2);
	Comment_IA("d0(5)=", arg, 5);
	Registres_D[0] = arg;
	AddToCycles(10 + (Registre_PC & 1));
	Registre_PC += 7;
}

void Execute_1C()
{
	int n;

	n = 1 + DB_Use1(Registre_PC+2);
	Comment_IA("d1=d1-", (unsigned long int)n, 1);
	Registre_CARRY = 0;
	if(Registres_D[1]<n)
	{
		Registre_CARRY = 1;
		Registres_D[1] += 0x100000;
	}
	Registres_D[1]=Registres_D[1]-n;
	AddToCycles(8 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_1D()
{
	unsigned long int arg;

	arg = DB_Use2(Registre_PC + 2);

	Comment_IA("d1(2)=", arg, 2);

	Registres_D[1] = (Registres_D[1] & 0xFFF00) + arg;

	AddToCycles(6);

	Registre_PC += 4;
}

void Execute_1E()
{
	unsigned long int arg;

	arg = DB_Use4(Registre_PC + 2);

	Comment_IA("d1(4)=", arg, 4);

	Registres_D[1] = (Registres_D[1] & 0xF0000) + arg;

	AddToCycles(9);

	Registre_PC += 6;
}

void Execute_1F()
{
	unsigned long int arg;

	arg = DB_Use5(Registre_PC + 2);

	Comment_IA("d1(5)=", arg, 5);

	Registres_D[1] = arg;

	AddToCycles(10 + (Registre_PC & 1));

	Registre_PC += 7;
}


void Execute_1()
{
	int q;

	q = (int) DB_Use1(Registre_PC + 1);

	switch(q)
	{
		case 0x0: Execute_10(); break;
		case 0x1: Execute_11(); break;
		case 0x2: Execute_12(); break;
		case 0x3: Execute_13(); break;
		case 0x4: Execute_14(); break;
		case 0x5: Execute_15(); break;
		case 0x6: Execute_16(); break;
		case 0x7: Execute_17(); break;
		case 0x8: Execute_18(); break;
		case 0x9: Execute_19(); break;
		case 0xA: Execute_1A(); break;
		case 0xB: Execute_1B(); break;
		case 0xC: Execute_1C(); break;
		case 0xD: Execute_1D(); break;
		case 0xE: Execute_1E(); break;
		case 0xF: Execute_1F(); break;
	}
}

void Execute_2()
{
	unsigned long int v;

	v=DB_Use1(Registre_PC+1);
	Comment_IA("p=", v, 1);
	Registre_P = (int) v;
	AddToCycles(3);
	Registre_PC+=2;
}

void Execute_3()
{
	char buffer1[80], buffer2[80];
	int l;
	int i;
	char q[2];

	l = DB_Use1(Registre_PC + 1);

	for(i=0; i<=l; ++i)
	{
		Registre_C.qu[(Registre_P+i)%16] =
							(unsigned char) DB_Use1(Registre_PC + 2 + i);
	}

	sprintf(buffer1, "lchex(%d)", l+1);

	q[1] = '\0';

	strcpy(buffer2, "#");

	for(i=l;i>=0; --i)
	{
		q[0] = HTOA((int) DB_Read1(Registre_PC+2+i));
		strcat(buffer2, q);
	}

	Comment_TT(buffer1, buffer2);

	AddToCycles(2 + l + 1 + (3+l)/2 + (Registre_PC &1) * ((l+1) & 1));

	Registre_PC += 2 + l + 1;
}

void Execute_4()
{
	int l;
	unsigned long int to;

	l = DB_Use2(Registre_PC + 1);

	if(l==0)
	{
		Comment_IA("rtnc", (unsigned long int)0, 0);
		to = 0;
	}
	else
	{
		if(l<0x80)
		{
			to = Registre_PC + 1 +l;
		}
		else
		{
			to = Registre_PC +1 -0x100 + l;
		}
		if(l == 2)
		{
			Comment_IA("nop3", (unsigned long int)0, 0);
		}
		else
		{
			Comment_IA("goc", to, 5);
		}
	}

	DB_NLA(Registre_PC, 1);

	if(Registre_CARRY == 0)
	{
		AddToCycles(4+(Registre_PC&1));
		Registre_PC += 3;
		return;
	}

	AddToCycles(12 + (Registre_PC &1));

	if(l!= 0)
	{
		Registre_PC = to;
		DB_NLB(Registre_PC, 1);
	}
	else
	{
		Registre_PC = PopRstk();
	}
}

void Execute_5()
{
	int l;
	unsigned long int to;

	l = DB_Use2(Registre_PC + 1);

	if(l==0)
	{
		Comment_IA("rtnnc", (unsigned long int)0, 0);
		to = 0;
	}
	else
	{
		if(l<0x80)
		{
			to = Registre_PC + 1 +l;
		}
		else
		{
			to = Registre_PC +1 -0x100 + l;
		}
		if(l == 2)
		{
			Comment_IA("nop3", (unsigned long int)0, 0);
		}
		else
		{
			Comment_IA("gonc", to, 5);
		}
	}

	DB_NLA(Registre_PC, 1);


	if(Registre_CARRY != 0)
	{
		AddToCycles(4+(Registre_PC&1));
		Registre_PC += 3;
		return;
	}

	AddToCycles(12 + (Registre_PC &1));

	if(l!= 0)
	{
		Registre_PC = to;
		DB_NLB(Registre_PC, 1);
	}
	else
	{
		Registre_PC = PopRstk();
	}
}

void Execute_6()
{
	unsigned long int arg;
	unsigned long int to;

	arg = DB_Use3(Registre_PC+1);

	if(arg < 0x800)
	{
		to = Registre_PC + 1 + arg;
	}
	else
	{
		to = (Registre_PC + 1 - 0x1000 + arg) % 0x100000;
	}

	if(arg == 3)
	{
		Comment_IA("nop4", (unsigned long int)0, 0);
	}
	else if(arg == 4)
	{
		Comment_IA("nop5", (unsigned long int)0, 0);
	}
	else
	{
		Comment_IA("goto", to, 5);
		DB_NLA(Registre_PC, 1);
		DB_NLB(to, 1);
	}

	AddToCycles(14);

	Registre_PC=to;
}

void Execute_7()
{
	unsigned long int arg, to;

	arg = DB_Use3(Registre_PC + 1);

	if(arg<0x800)
	{
		to = Registre_PC + 4 + arg;
	}
	else
	{
		to = Registre_PC + 4 + arg - 0x1000;
	}

	Comment_IG("gosub", to);
	DB_NLB(to, 1);
	PushRstk(Registre_PC+4);
	AddToCycles(15);
	Registre_PC=to;
}

void Execute_800()
{
	Comment_IA("out=cs", (unsigned long int)0, 0);
	Registre_OUT = (Registre_OUT&0xff0) | Registre_C.qu[0];
	AddToCycles(5+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_801()
{
	Comment_IA("out=c", (unsigned long int)0, 0);
	Registre_OUT = Reg2UL(Registre_C, FIELD_X);
	AddToCycles(7+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_802()
{
	unsigned long int v;
	Comment_IA("a=in", (unsigned long int)0, 0);
	v = Reg2UL(Registre_A, FIELD_A);
	v = (v & 0xf0000) + Registre_IN;
	UL2Reg(v, &Registre_A, FIELD_A);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_803()
{
	unsigned long int v;
	Comment_IA("c=in", (unsigned long int)0, 0);
	v = Reg2UL(Registre_C, FIELD_A);
	v = (v & 0xf0000) + Registre_IN;
	UL2Reg(v, &Registre_C, FIELD_A);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_804()
{
	unsigned long int a;
	Comment_IA("uncnfg", (unsigned long int)0, 0);
	a = Reg2UL(Registre_C, FIELD_A);
	DB_Unconfigure(a);
	AddToCycles(14+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_805()
{
	unsigned long int arg;
	arg = Reg2UL(Registre_C, FIELD_A);
	DB_Config(arg);
	Comment_IA("config", (unsigned long int)0, 0);
	AddToCycles(13 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_806()
{
	unsigned long int id;
	Comment_IA("c=id", (unsigned long int)0, 0);
	id=DB_ID();
	UL2Reg(id, &Registre_C, FIELD_A);
	AddToCycles(13+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_807()
{
	Comment_IA("shutdwn", (unsigned long int)0, 0);
	AddToCycles(6+(Registre_PC&1));
	Registre_PC += 3;
	Interrupt_Requested = 1;
	fprintf(stderr, "Shutdowned !\n"); fflush(stderr);
}

void Execute_8080()
{
	Comment_IA("inton", (unsigned long int)0, 0);
	Interrupts_OK = 1;
	AddToCycles(7);
	Registre_PC +=4;
}


void Execute_8081()
{
	int q;

	q = (int) DB_Use1(Registre_PC + 4);

	if(q != 0)
	{
		fprintf(stderr, "unsup op. 8081%X at %05X\n", q, Registre_PC);
		DO_exit(7);
	}

	Comment_IA("rsi", (unsigned long int)0, 0);

	AddToCycles(8+(Registre_PC&1));
	Registre_PC  += 5;

	if(!Processing_Interrupt) Interrupt_Pending = 1;
	else Interrupt_When_Rti=1;
}


void Execute_8082()
{
	char buffer1[80], buffer2[80];
	int l;
	int i;
	char q[2];

	l = DB_Use1(Registre_PC + 4);

	for(i=0; i<=l; ++i)
	{
		Registre_A.qu[(Registre_P+i)%16] =
							(unsigned char) DB_Use1(Registre_PC + 5 + i);
	}

	sprintf(buffer1, "lahex(%d)", l+1);

	q[1] = '\0';

	strcpy(buffer2, "#");

	for(i=l;i>=0; --i)
	{
		q[0] = HTOA((int) DB_Read1(Registre_PC+5+i));
		strcat(buffer2, q);
	}

	Comment_TT(buffer1, buffer2);

	AddToCycles(5+l+1+(l+6)/2+(l&1)*(Registre_PC&1));

	Registre_PC += 5 + l + 1;
}


void Execute_8083()
{
	Comment_IA("buscb", (unsigned long int)0, 0);
	fprintf(stderr, "Warning : use of buscb at %05X\n", Registre_PC);
	AddToCycles(10);
	Registre_PC += 4;
}


void Execute_8084()
{
	int d;
	unsigned long int a;
	d = (int) DB_Use1(Registre_PC+4);
	Comment_IF("abit=0", (unsigned long int)d);
	a = Reg2UL(Registre_A, FIELD_A);
	a = a & (0xfffff ^ (1 << d));
	UL2Reg(a, &Registre_A, FIELD_A);
	AddToCycles(7+(Registre_PC&1));
	Registre_PC+=5;
}


void Execute_8085()
{
	int d;
	unsigned long int a;
	d = (int) DB_Use1(Registre_PC+4);
	Comment_IF("abit=1", (unsigned long int)d);
	a = Reg2UL(Registre_A, FIELD_A);
	a = a | (1 << d);
	UL2Reg(a, &Registre_A, FIELD_A);
	AddToCycles(7+(Registre_PC&1));
	Registre_PC+=5;
}


void Execute_8086()
{
	int f;
	f = (int) DB_Use1(Registre_PC + 4);
	Comment_IF("?abit=0", (unsigned long int)f);
	Registre_CARRY = (~((Reg2UL(Registre_A, FIELD_A) >> f) &1))&1;
	goyes(Registre_PC + 5, 20 + (Registre_PC & 1), 12 + (Registre_PC &1));
}


void Execute_8087()
{
	int f;

	f = (int) DB_Use1(Registre_PC + 4);
	Comment_IF("?abit=1", (unsigned long int)f);
	Registre_CARRY = ((Reg2UL(Registre_A, FIELD_A) >> f) &1);
	goyes(Registre_PC + 5, 20 + (Registre_PC & 1), 12 + (Registre_PC &1));
}


void Execute_8088()
{
	int d;
	unsigned long int a;
	d = (int) DB_Use1(Registre_PC+4);
	Comment_IF("cbit=0", (unsigned long int)d);
	a = Reg2UL(Registre_C, FIELD_A);
	a = a & (0xfffff ^ (1 << d));
	UL2Reg(a, &Registre_C, FIELD_A);
	AddToCycles(7+(Registre_PC&1));
	Registre_PC+=5;
}


void Execute_8089()
{
	int d;
	unsigned long int a;
	d = (int) DB_Use1(Registre_PC+4);
	Comment_IF("cbit=1", (unsigned long int)d);
	a = Reg2UL(Registre_C, FIELD_A);
	a = a | (1 << d);
	UL2Reg(a, &Registre_C, FIELD_A);
	AddToCycles(7+(Registre_PC&1));
	Registre_PC+=5;
}



void Execute_808A()
{
	int f;
	f = (int) DB_Use1(Registre_PC + 4);
	Comment_IF("?cbit=0", (unsigned long int)f);
	Registre_CARRY = (~((Reg2UL(Registre_C, FIELD_A) >> f) &1))&1;
	goyes(Registre_PC + 5, 20 + (Registre_PC & 1), 12 + (Registre_PC &1));
}


void Execute_808B()
{
	int f;
	f = (int) DB_Use1(Registre_PC + 4);
	Comment_IF("?cbit=1", (unsigned long int)f);
	Registre_CARRY = (Reg2UL(Registre_C, FIELD_A) >> f) &1;
	goyes(Registre_PC + 5, 20 + (Registre_PC & 1), 12 + (Registre_PC &1));
}

int IsAnObject(unsigned long int prologue);

void Execute_808C()
{
	unsigned long int a;
	unsigned long int to;
	struct label *lab;
	char buffer[100];
	Comment_IA("pc=(a)", (unsigned long int)0, 0);
	DB_NLA(Registre_PC, 1);
	a = Reg2UL(Registre_A, FIELD_A);
	to = DB_Read5(a);
	AddToCycles(26+(a&1));
	Registre_PC = to;
	DB_NLB(Registre_PC, 1);

	if(IsAnObject(to)|| to == a+5)
	{
		DB_SetObject(a, 1);
	}
	lab = FindLabel(a);
	if(lab != NULL)
	{
		WriteTextIntoTextWidgets(STATUS_OBJECT, lab->txt);
		WriteTextIntoTextWidgets(STATUS_ADDRESS, "");
	}
	else
	{
		sprintf(buffer, "%05X", a);
		WriteTextIntoTextWidgets(STATUS_ADDRESS, buffer);
	}
		
}


void Execute_808D()
{
	Comment_IA("buscd", (unsigned long int)0, 0);
	fprintf(stderr, "Warning : use of buscd at %05X\n", Registre_PC);
	AddToCycles(10);
	Registre_PC += 4;
}


void Execute_808E()
{
	unsigned long int to, a;
	struct label *lab;
	char buffer[100];
	Comment_IA("pc=(c)", (unsigned long int)0, 0);
	DB_NLA(Registre_PC, 1);
	a = Reg2UL(Registre_C, FIELD_A);
	to = DB_Read5(a);
	AddToCycles(26+(a&1));
	Registre_PC = to;
	DB_NLB(Registre_PC, 1);

	if(IsAnObject(to)|| to == a+5)
	{
		DB_SetObject(a, 1);
	}
	lab = FindLabel(a);
	if(lab != NULL)
	{
		WriteTextIntoTextWidgets(STATUS_OBJECT, lab->txt);
		WriteTextIntoTextWidgets(STATUS_ADDRESS, "");
	}
	else
	{
		sprintf(buffer, "%05X", a);
		WriteTextIntoTextWidgets(STATUS_ADDRESS, buffer);
	}
}

void Execute_808F()
{
	Comment_IA("intoff", (unsigned long int)0, 0);
	Interrupts_OK = 0;
	AddToCycles(7);
	Registre_PC +=4;
}



void Execute_808()
{
	int q;

	q = (int) DB_Use1(Registre_PC+3);

	switch(q)
	{
		case 0x0: Execute_8080(); break;
		case 0x1: Execute_8081(); break;
		case 0x2: Execute_8082(); break;
		case 0x3: Execute_8083(); break;
		case 0x4: Execute_8084(); break;
		case 0x5: Execute_8085(); break;
		case 0x6: Execute_8086(); break;
		case 0x7: Execute_8087(); break;
		case 0x8: Execute_8088(); break;
		case 0x9: Execute_8089(); break;
		case 0xA: Execute_808A(); break;
		case 0xB: Execute_808B(); break;
		case 0xC: Execute_808C(); break;
		case 0xD: Execute_808D(); break;
		case 0xE: Execute_808E(); break;
		case 0xF: Execute_808F(); break;
	}
}

void Execute_809()
{
	Comment_IA("c+p+1", (unsigned long int)0, 0);
	IncRegisterByX(Registre_C, FIELD_A, Registre_P+1);
	AddToCycles(9+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_80A()
{
	Comment_IA("reset", (unsigned long int)0, 0);
	DB_UnconfigureAll();
	AddToCycles(7 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_80B()
{
	Comment_IA("buscc", (unsigned long int)0, 0);
	fprintf(stderr, "Warning : use of buscc at %05X\n", Registre_PC);
	AddToCycles(8+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_80C()
{
	int arg;

	arg = DB_Use1(Registre_PC + 3);
	Comment_IF("c=p", (unsigned long int)arg);
	Registre_C.qu[arg] = Registre_P;
	AddToCycles(8);
	Registre_PC += 4;
}

void Execute_80D()
{
	int arg;

	arg = DB_Use1(Registre_PC + 3);
	Comment_IF("p=c", (unsigned long int)arg);
	Registre_P = Registre_C.qu[arg];
	AddToCycles(8);
	Registre_PC += 4;
}

void Execute_80E()
{
	Comment_IA("sreq?", (unsigned long int)0, 0);
	fprintf(stderr, "Warning : use of sreq? at %05X\n", Registre_PC);
	AddToCycles(9+(Registre_PC&1));
	Registre_C.qu[0] = 0; /* A VERIFIER !!! */
	Registre_PC += 3;
}

void Execute_80F()
{
	int x;
	int v;
	x = (int) DB_Use1(Registre_PC+3);
	Comment_IF("cpex", (unsigned long int)x);
	v = Registre_C.qu[x];
	Registre_C.qu[x] = Registre_P;
	Registre_P=v;
	AddToCycles(8);
	Registre_PC +=4;
}


void Execute_80()
{
	int q;

	q = (int) DB_Use1(Registre_PC+2);

	switch(q)
	{
		case 0x0: Execute_800(); break;
		case 0x1: Execute_801(); break;
		case 0x2: Execute_802(); break;
		case 0x3: Execute_803(); break;
		case 0x4: Execute_804(); break;
		case 0x5: Execute_805(); break;
		case 0x6: Execute_806(); break;
		case 0x7: Execute_807(); break;
		case 0x8: Execute_808(); break;
		case 0x9: Execute_809(); break;
		case 0xA: Execute_80A(); break;
		case 0xB: Execute_80B(); break;
		case 0xC: Execute_80C(); break;
		case 0xD: Execute_80D(); break;
		case 0xE: Execute_80E(); break;
		case 0xF: Execute_80F(); break;
	}
}

void RegSLC(r)
struct Register *r;
{
	int i;
	int q1, q2;

	q1 = (int) r->qu[15];

	for(i=0;i<16;++i)
	{
		q2 = r->qu[i];
		r->qu[i] = q1;
		q1 = q2;
	}
}

void RegSRC(r)
struct Register *r;
{
	int i;
	int q1, q2;

	q1 = (int) r->qu[0];

	for(i=15;i>=0;--i)
	{
		q2 = r->qu[i];
		r->qu[i] = q1;
		q1 = q2;
	}
}

void Execute_810()
{
	Comment_IA("aslc", (unsigned long int)0, 0);
	RegSLC(&Registre_A);
	AddToCycles(22+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_811()
{
	Comment_IA("bslc", (unsigned long int)0, 0);
	RegSLC(&Registre_B);
	AddToCycles(22+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_812()
{
	Comment_IA("cslc", (unsigned long int)0, 0);
	RegSLC(&Registre_C);
	AddToCycles(22+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_813()
{
	Comment_IA("dslc", (unsigned long int)0, 0);
	RegSLC(&Registre_D);
	AddToCycles(22+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_814()
{
	Comment_IA("asrc", (unsigned long int)0, 0);
	RegSRC(&Registre_A);
	AddToCycles(22+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_815()
{
	Comment_IA("bsrc", (unsigned long int)0, 0);
	RegSRC(&Registre_B);
	AddToCycles(22+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_816()
{
	Comment_IA("csrc", (unsigned long int)0, 0);
	RegSRC(&Registre_C);
	AddToCycles(22+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_817()
{
	Comment_IA("dsrc", (unsigned long int)0, 0);
	RegSRC(&Registre_D);
	AddToCycles(22+(Registre_PC&1));
	Registre_PC+=3;
}

void Execute_818()
{
	int f, q, x;
	char instr[100];

	f = FieldF2aA[(int) DB_Use1(Registre_PC +3)];
	q = (int) DB_Use1(Registre_PC +4);
	x = (int) DB_Use1(Registre_PC +5)+1;

	switch(q)
	{
		case 0x0:
			IncRegisterByX(Registre_A, f, x);
			sprintf(instr, "a=a+%d", x);
			break;

		case 0x1:
			IncRegisterByX(Registre_B, f, x);
			sprintf(instr, "b=b+%d", x);
			break;

		case 0x2:
			IncRegisterByX(Registre_C, f, x);
			sprintf(instr, "c=c+%d", x);
			break;

		case 0x3:
			IncRegisterByX(Registre_D, f, x);
			sprintf(instr, "d=d+%d", x);
			break;

		case 0x8:
			DecRegisterByX(Registre_A, f, x);
			sprintf(instr, "a=a-%d", x);
			break;

		case 0x9:
			DecRegisterByX(Registre_B, f, x);
			sprintf(instr, "b=b-%d", x);
			break;

		case 0xA:
			DecRegisterByX(Registre_C, f, x);
			sprintf(instr, "c=c-%d", x);
			break;

		case 0xB:
			DecRegisterByX(Registre_D, f, x);
			sprintf(instr, "d=d-%d", x);
			break;

		default:
			fprintf(stderr, "Unsup 818%X%X%X at %05X\n",
				f, q, x, Registre_PC);
			DO_exit(11);

	}

	Comment_TT(instr, FieldF2[f]);
	AddToCycles(8+fe(f)-fs(f)+1);
	Registre_PC += 6;
}

void RegSRB(r, f)
struct Register *r;
int f;
{
	int qs, qe;
	int b;
	int q;

	qs = fs(f);
	qe = fe(f);

	q = r->qu[qe];
	r->qu[qe] = q/2;
	b=(q&0x1);

	while(qe != qs)
	{
		--qe; if(qe<0) qe = 0xf;
		q = r->qu[qe] + (b*0x10);
		r->qu[qe] = q/2;
		b=(q&0x1);
	} 

	Registre_HST |= (b*0x2);
}


void Execute_819()
{
	int f, q;
	char r;
	char instr[10];

	f = FieldF2aA[(int) DB_Use1(Registre_PC +3)];
	q = (int) DB_Use1(Registre_PC+4);

	switch(q)
	{
		case 0x0:  r = 'a';  RegSRB(&Registre_A, f); break;
		case 0x1:  r = 'b';  RegSRB(&Registre_B, f); break;
		case 0x2:  r = 'c';  RegSRB(&Registre_C, f); break;
		case 0x3:  r = 'd';  RegSRB(&Registre_D, f); break;

		default:
			fprintf(stderr, "Unsup 819%X%X at %05X\n",
				f, q, Registre_PC);
			DO_exit(12);
	}

	sprintf(instr, "%csrb", r);
	Comment_TT(instr, FieldF2[f]);
	AddToCycles(8+(Registre_PC&1)+fe(f)-fs(f)+1);
	Registre_PC += 5;
}

void Execute_81Af0q(f, q)
int f, q;
{
	char buffer[100];
	int n;
	char r;

	switch(q)
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
			n = q;
			r = 'a';
			CopyRegisters(Registre_A, Registres_R[n], f);
			break;

		case 0x8:
		case 0x9:
		case 0xA:
		case 0xB:
		case 0xC:
			n = q-8;
			r = 'c';
			CopyRegisters(Registre_C, Registres_R[n], f);
			break;

		default:
			fprintf(stderr, "Unsup. op at %05X\n", Registre_PC);
			DO_exit(1);
			break;
	}

	sprintf(buffer, "r%d=%c", n, r);
	Comment_TT(buffer, FieldF2[f]);
}


void Execute_81Af1q(f, q)
int f, q;
{
	char buffer[100];
	int n;
	char r;

	switch(q)
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
			n = q;
			r = 'a';
			CopyRegisters(Registres_R[n], Registre_A, f);
			break;

		case 0x8:
		case 0x9:
		case 0xA:
		case 0xB:
		case 0xC:
			n = q-8;
			r = 'c';
			CopyRegisters(Registres_R[n], Registre_C, f);
			break;

		default:
			fprintf(stderr, "Unsup. op at %05X\n", Registre_PC);
			DO_exit(1);
			break;
	}

	sprintf(buffer, "%c=r%d", r, n);
	Comment_TT(buffer, FieldF2[f]);
}


void Execute_81Af2q(f, q)
int f, q;
{
	struct Register tmp;
	char buffer[100];
	int n;
	char r;

	switch(q)
	{
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
			n = q;
			r = 'a';
			CopyRegisters(Registre_A, tmp, f);
			CopyRegisters(Registres_R[n], Registre_A, f);
			CopyRegisters(tmp, Registres_R[n], f);
			break;

		case 0x8:
		case 0x9:
		case 0xA:
		case 0xB:
		case 0xC:
			n = q-8;
			r = 'c';
			CopyRegisters(Registre_C, tmp, f);
			CopyRegisters(Registres_R[n], Registre_C, f);
			CopyRegisters(tmp, Registres_R[n], f);
			break;

		default:
			fprintf(stderr, "Unsup. op at %05X\n", Registre_PC);
			DO_exit(1);
			break;
	}

	sprintf(buffer, "%cr%dex", r, n);
	Comment_TT(buffer, FieldF2[f]);
}

void Execute_81A()
{
	int f, d, q;

	f = FieldF2aA[(int) DB_Use1(Registre_PC + 3)];
	d = (int) DB_Use1(Registre_PC + 4);
	q = (int) DB_Use1(Registre_PC + 5);

	if(f>7&&f!=0x8) 
	{
		fprintf(stderr, "unsup. op 81A%X%X%X at %05X\n",
				f, d, q, Registre_PC);
		DO_exit(1);
	}	

	switch(d)
	{
		case 0:	Execute_81Af0q(f, q); break;
		case 1:	Execute_81Af1q(f, q); break;
		case 2:	Execute_81Af2q(f, q); break;

		default:
			fprintf(stderr, "unsup. op 81A%X%X%X at %05X\n",
					f, d, q, Registre_PC);
			DO_exit(1);
	}



	AddToCycles(9+fe(f)-fs(f)+1);
	Registre_PC += 6;
}

void Execute_81B2()
{
	Comment_IA("pc=a", (unsigned long int)0, 0);
	DB_NLA(Registre_PC, 1);
	AddToCycles(19);
	Registre_PC = Reg2UL(Registre_A, FIELD_A);
	DB_NLB(Registre_PC, 1);

}

void Execute_81B3()
{
	Comment_IA("pc=c", (unsigned long int)0, 0);
	DB_NLA(Registre_PC, 1);
	AddToCycles(19);
	Registre_PC = Reg2UL(Registre_C, FIELD_A);
	DB_NLB(Registre_PC, 1);
}

void Execute_81B4()
{
	Comment_IA("a=pc", (unsigned long int)0, 0);
	Registre_PC += 4;
	UL2Reg(Registre_PC, &Registre_A, FIELD_A);
	AddToCycles(11);
}

void Execute_81B5()
{
	Comment_IA("c=pc", (unsigned long int)0, 0);
	Registre_PC += 4;
	UL2Reg(Registre_PC, &Registre_C, FIELD_A);
	AddToCycles(11);
}

void Execute_81B6()
{
	unsigned long int tmp;
	Comment_IA("apcex", (unsigned long int)0, 0);
	tmp = Registre_PC+4;
	Registre_PC = Reg2UL(Registre_A, FIELD_A);
	UL2Reg(tmp, &Registre_A, FIELD_A);
	AddToCycles(19);
}

void Execute_81B7()
{
	unsigned long int tmp;
	Comment_IA("cpcex", (unsigned long int)0, 0);
	tmp = Registre_PC+4;
	Registre_PC = Reg2UL(Registre_C, FIELD_A);
	UL2Reg(tmp, &Registre_C, FIELD_A);
	AddToCycles(19);
}


void Execute_81B()
{
	int q;

	q = (int) DB_Use1(Registre_PC+3);

	switch(q)
	{
		case 0x2: Execute_81B2(); break;
		case 0x3: Execute_81B3(); break;
		case 0x4: Execute_81B4(); break;
		case 0x5: Execute_81B5(); break;
		case 0x6: Execute_81B6(); break;
		case 0x7: Execute_81B7(); break;

		default:
			fprintf(stderr, "Unsup. instr 81B%X at %05X\n", q, Registre_PC);
			DO_exit(13);
	}
}

void Execute_81C()
{
	Comment_IA("asrb", (unsigned long int)0, 0);
	RegSRB(&Registre_A, FIELD_W);
	AddToCycles(21+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_81D()
{
	Comment_IA("bsrb", (unsigned long int)0, 0);
	RegSRB(&Registre_B, FIELD_W);
	AddToCycles(21+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_81E()
{
	Comment_IA("csrb", (unsigned long int)0, 0);
	RegSRB(&Registre_C, FIELD_W);
	AddToCycles(21+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_81F()
{
	Comment_IA("dsrb", (unsigned long int)0, 0);
	RegSRB(&Registre_D, FIELD_W);
	AddToCycles(21+(Registre_PC&1));
	Registre_PC += 3;
}


void Execute_81()
{
	int q;

	q = (int) DB_Use1(Registre_PC+2);

	switch(q)
	{
		case 0x0: Execute_810(); break;
		case 0x1: Execute_811(); break;
		case 0x2: Execute_812(); break;
		case 0x3: Execute_813(); break;
		case 0x4: Execute_814(); break;
		case 0x5: Execute_815(); break;
		case 0x6: Execute_816(); break;
		case 0x7: Execute_817(); break;
		case 0x8: Execute_818(); break;
		case 0x9: Execute_819(); break;
		case 0xA: Execute_81A(); break;
		case 0xB: Execute_81B(); break;
		case 0xC: Execute_81C(); break;
		case 0xD: Execute_81D(); break;
		case 0xE: Execute_81E(); break;
		case 0xF: Execute_81F(); break;
	}
}

void Execute_82()
{	
	int arg;
	char buffer[80];

	arg = (int) DB_Use1(Registre_PC+2);

	if(arg==0)
	{
		strcpy(buffer, "nop3");
	}
	else
	{
		if(arg&0x1) strcpy(buffer, "xm=");
		if(arg&0x2) strcpy(buffer, "sb=");
		if(arg&0x4) strcpy(buffer, "sr=");
		if(arg&0x8) strcpy(buffer, "mp=");

		strcat(buffer, "0");
	}

	Comment_IA(buffer, (unsigned long int)0, 0);

	Registre_HST = ~(~Registre_HST | arg);

	AddToCycles(4 + (Registre_PC & 1));

	Registre_PC += 3;
}

void Execute_83()
{
	int f;

	f = DB_Use1(Registre_PC + 2);

	switch(f)
	{
		case 0x1:
			Comment_IA("?xm=0", (unsigned long int)0, 0);
			break;

		case 0x2:
			Comment_IA("?sb=0", (unsigned long int)0, 0);
			break;

		case 0x4:
			Comment_IA("?sr=0", (unsigned long int)0, 0);
			break;

		case 0x8:
			Comment_IA("?mp=0", (unsigned long int)0, 0);
			break;

		default:
			fprintf(stderr, "Unsupported : 83%X\n", f);
			DO_exit(14);
	}

	Registre_CARRY = ((Registre_HST & f)==0);
	goyes(Registre_PC + 3, 15+(Registre_PC&1), 7+(Registre_PC &1));
}

void Execute_84()
{
	int n;
	
	n = (int) DB_Use1(Registre_PC+2);
	
	Comment_IF("st=0", (unsigned long int)n);

	Registre_STATUS = ~((~Registre_STATUS) | (1<<n));

	AddToCycles(5 + (Registre_PC & 1));

	Registre_PC+=3;

}

void Execute_85()
{
	int n;
	
	n = (int) DB_Use1(Registre_PC+2);
	
	Comment_IF("st=1", (unsigned long int)n);

	Registre_STATUS = Registre_STATUS | (1<<n);

	AddToCycles(5 + (Registre_PC & 1));

	Registre_PC+=3;
}

void Execute_86()
{
	int f;

	f = (int) DB_Use1(Registre_PC + 2);

	Comment_IF("?st=0", (unsigned long int)f);

	Registre_CARRY = (~((Registre_STATUS >> f) & 1))&1;

	goyes(Registre_PC + 3, 16 + (Registre_PC & 1), 8 + (Registre_PC & 1));
}

void Execute_87()
{
	int f;

	f = (int) DB_Use1(Registre_PC + 2);

	Comment_IF("?st=1", (unsigned long int)f);

	Registre_CARRY = (Registre_STATUS >> f) & 1;

	goyes(Registre_PC + 3, 16 + (Registre_PC & 1), 8 + (Registre_PC & 1));
}

void Execute_88()
{
	int f;

	f = (int) DB_Use1(Registre_PC + 2);
	Comment_IA("?p#", (unsigned long int)f, 1);
	Registre_CARRY = Registre_P != f;
	goyes(Registre_PC + 3, 15 + (Registre_PC & 1), 7 + (Registre_PC & 1));
}

void Execute_89()
{
	int f;
	f = (int) DB_Use1(Registre_PC + 2);
	Comment_IA("?p=", (unsigned long int)f, 1);
	Registre_CARRY = Registre_P == f;
	goyes(Registre_PC + 3, 15 + (Registre_PC & 1), 7 + (Registre_PC & 1));
}

void Execute_8A0()
{
	unsigned long int tmp1, tmp2;
	Comment_TT("?a=b", "(a)");
	tmp1 = Reg2UL(Registre_A, FIELD_A);
	tmp2 = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (tmp1 == tmp2);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8A1()
{
	unsigned long int tmp1, tmp2;
	Comment_TT("?b=c", "(a)");
	tmp1 = Reg2UL(Registre_C, FIELD_A);
	tmp2 = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (tmp1 == tmp2);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8A2()
{
	unsigned long int tmp1, tmp2;
	Comment_TT("?a=c", "(a)");
	tmp1 = Reg2UL(Registre_A, FIELD_A);
	tmp2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (tmp1 == tmp2);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8A3()
{
	unsigned long int tmp1, tmp2;
	Comment_TT("?c=d", "(a)");
	tmp1 = Reg2UL(Registre_C, FIELD_A);
	tmp2 = Reg2UL(Registre_D, FIELD_A);
	Registre_CARRY = (tmp1 == tmp2);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8A4()
{
	unsigned long int tmp1, tmp2;
	Comment_TT("?a#b", "(a)");
	tmp1 = Reg2UL(Registre_A, FIELD_A);
	tmp2 = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (tmp1 != tmp2);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8A5()
{
	unsigned long int tmp1, tmp2;
	Comment_TT("?b#c", "(a)");
	tmp1 = Reg2UL(Registre_C, FIELD_A);
	tmp2 = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (tmp1 != tmp2);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8A6()
{
	unsigned long int tmp1, tmp2;
	Comment_TT("?a#c", "(a)");
	tmp1 = Reg2UL(Registre_A, FIELD_A);
	tmp2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (tmp1 != tmp2);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8A7()
{
	unsigned long int tmp1, tmp2;
	Comment_TT("?c#d", "(a)");
	tmp1 = Reg2UL(Registre_C, FIELD_A);
	tmp2 = Reg2UL(Registre_D, FIELD_A);
	Registre_CARRY = (tmp1 != tmp2);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8A8()
{
	unsigned long int tmp;
	Comment_TT("?a=0", "(a)");
	tmp = Reg2UL(Registre_A, FIELD_A);
	Registre_CARRY = (tmp == 0);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8A9()
{
	unsigned long int tmp;
	Comment_TT("?b=0", "(a)");
	tmp = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (tmp == 0);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8AA()
{
	unsigned long int tmp;
	Comment_TT("?c=0", "(a)");
	tmp = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (tmp == 0);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8AB()
{
	unsigned long int tmp;
	Comment_TT("?d=0", "(a)");
	tmp = Reg2UL(Registre_D, FIELD_A);
	Registre_CARRY = (tmp == 0);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8AC()
{
	unsigned long int tmp;
	Comment_TT("?a#0", "(a)");
	tmp = Reg2UL(Registre_A, FIELD_A);
	Registre_CARRY = (tmp != 0);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8AD()
{
	unsigned long int tmp;
	Comment_TT("?b#0", "(a)");
	tmp = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (tmp != 0);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8AE()
{
	unsigned long int tmp;
	Comment_TT("?c#0", "(a)");
	tmp = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (tmp != 0);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}

void Execute_8AF()
{
	unsigned long int tmp;
	Comment_TT("?d#0", "(a)");
	tmp = Reg2UL(Registre_D, FIELD_A);
	Registre_CARRY = (tmp != 0);
	goyes(Registre_PC+3, 21 + (Registre_PC &1), 13 + (Registre_PC & 1));
}


void Execute_8A()
{
	int q;

	q=DB_Use1(Registre_PC+2);

	switch(q)
	{
		case 0x0: Execute_8A0(); break;
		case 0x1: Execute_8A1(); break;
		case 0x2: Execute_8A2(); break;
		case 0x3: Execute_8A3(); break;
		case 0x4: Execute_8A4(); break;
		case 0x5: Execute_8A5(); break;
		case 0x6: Execute_8A6(); break;
		case 0x7: Execute_8A7(); break;
		case 0x8: Execute_8A8(); break;
		case 0x9: Execute_8A9(); break;
		case 0xA: Execute_8AA(); break;
		case 0xB: Execute_8AB(); break;
		case 0xC: Execute_8AC(); break;
		case 0xD: Execute_8AD(); break;
		case 0xE: Execute_8AE(); break;
		case 0xF: Execute_8AF(); break;
	}
}


void Execute_8B0()
{
	unsigned long int v1, v2;
	Comment_TT("?a>b", "(a)");
	v1 = Reg2UL(Registre_A, FIELD_A);
	v2 = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (v1>v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B1()
{
	unsigned long int v1, v2;
	Comment_TT("?b>c", "(a)");
	v1 = Reg2UL(Registre_B, FIELD_A);
	v2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (v1>v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B2()
{
	unsigned long int v1, v2;
	Comment_TT("?c>a", "(a)");
	v1 = Reg2UL(Registre_C, FIELD_A);
	v2 = Reg2UL(Registre_A, FIELD_A);
	Registre_CARRY = (v1>v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B3()
{
	unsigned long int v1, v2;
	Comment_TT("?d>c", "(a)");
	v1 = Reg2UL(Registre_D, FIELD_A);
	v2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (v1>v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B4()
{
	unsigned long int v1, v2;
	Comment_TT("?a<b", "(a)");
	v1 = Reg2UL(Registre_A, FIELD_A);
	v2 = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (v1<v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B5()
{
	unsigned long int v1, v2;
	Comment_TT("?b<c", "(a)");
	v1 = Reg2UL(Registre_B, FIELD_A);
	v2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (v1<v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B6()
{
	unsigned long int v1, v2;
	Comment_TT("?c<a", "(a)");
	v1 = Reg2UL(Registre_C, FIELD_A);
	v2 = Reg2UL(Registre_A, FIELD_A);
	Registre_CARRY = (v1<v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B7()
{
	unsigned long int v1, v2;
	Comment_TT("?d<c", "(a)");
	v1 = Reg2UL(Registre_D, FIELD_A);
	v2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (v1<v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B8()
{
	unsigned long int v1, v2;
	Comment_TT("?a>=b", "(a)");
	v1 = Reg2UL(Registre_A, FIELD_A);
	v2 = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (v1>=v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B9()
{
	unsigned long int v1, v2;
	Comment_TT("?b>=c", "(a)");
	v1 = Reg2UL(Registre_B, FIELD_A);
	v2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (v1>=v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8BA()
{
	unsigned long int v1, v2;
	Comment_TT("?c>=a", "(a)");
	v1 = Reg2UL(Registre_C, FIELD_A);
	v2 = Reg2UL(Registre_A, FIELD_A);
	Registre_CARRY = (v1>=v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8BB()
{
	unsigned long int v1, v2;
	Comment_TT("?d>=c", "(a)");
	v1 = Reg2UL(Registre_D, FIELD_A);
	v2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (v1>=v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8BC()
{
	unsigned long int v1, v2;
	Comment_TT("?a<=b", "(a)");
	v1 = Reg2UL(Registre_A, FIELD_A);
	v2 = Reg2UL(Registre_B, FIELD_A);
	Registre_CARRY = (v1<=v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8BD()
{
	unsigned long int v1, v2;
	Comment_TT("?b<=c", "(a)");
	v1 = Reg2UL(Registre_B, FIELD_A);
	v2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (v1<=v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8BE()
{
	unsigned long int v1, v2;
	Comment_TT("?c<=a", "(a)");
	v1 = Reg2UL(Registre_C, FIELD_A);
	v2 = Reg2UL(Registre_A, FIELD_A);
	Registre_CARRY = (v1<=v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8BF()
{
	unsigned long int v1, v2;
	Comment_TT("?d<=c", "(a)");
	v1 = Reg2UL(Registre_D, FIELD_A);
	v2 = Reg2UL(Registre_C, FIELD_A);
	Registre_CARRY = (v1<=v2);
	goyes(Registre_PC+3, 21+(Registre_PC&1), 13+(Registre_PC&1));
}

void Execute_8B()
{
	int q;

	q=DB_Use1(Registre_PC+2);

	switch(q)
	{
		case 0x0: Execute_8B0(); break;
		case 0x1: Execute_8B1(); break;
		case 0x2: Execute_8B2(); break;
		case 0x3: Execute_8B3(); break;
		case 0x4: Execute_8B4(); break;
		case 0x5: Execute_8B5(); break;
		case 0x6: Execute_8B6(); break;
		case 0x7: Execute_8B7(); break;
		case 0x8: Execute_8B8(); break;
		case 0x9: Execute_8B9(); break;
		case 0xA: Execute_8BA(); break;
		case 0xB: Execute_8BB(); break;
		case 0xC: Execute_8BC(); break;
		case 0xD: Execute_8BD(); break;
		case 0xE: Execute_8BE(); break;
		case 0xF: Execute_8BF(); break;
	}
}

void Execute_8C()
{
	unsigned long int to;
	unsigned long int off;
	off = DB_Use4(Registre_PC+2);
	if(off<0x8000)
	{
		to = Registre_PC + 2 + off;
	}
	else
	{
		to = Registre_PC  + 2 - 0x10000 + off;
	}
	Comment_IA("golong", to, 5);
	DB_NLA(Registre_PC, 1);
	DB_NLB(to, 1);
	AddToCycles(17);
	Registre_PC=to;
}

void Execute_8D()
{
	unsigned long int to;
	to = DB_Use5(Registre_PC+2);
	Comment_IA("govlng", to, 5);
	DB_NLA(Registre_PC, 1);
	DB_NLB(to, 1);
	AddToCycles(18 + (Registre_PC & 1));
	Registre_PC=to;
}

void Execute_8E()
{
	unsigned long int to;
	unsigned long int off;
	off = DB_Use4(Registre_PC+2);
	if(off<0x8000)
	{
		to = Registre_PC + 6 + off;
	}
	else
	{
		to = Registre_PC + 6 - 0x10000 + off;
	}
	Comment_IG("gosubl", to);
	DB_NLB(to, 1);
	AddToCycles(18);
	PushRstk(Registre_PC+6);
	Registre_PC=to;
}

void Execute_8F()
{
	unsigned long int to;
	to = DB_Use5(Registre_PC+2);
	Comment_IG("gosbvl", to);
	DB_NLB(to, 1);
	AddToCycles(19 + (Registre_PC & 1));
	PushRstk(Registre_PC+7);
	Registre_PC=to;
}

void Execute_8()
{
	int q;

	q = (int) DB_Use1(Registre_PC+1);

	switch(q)
	{
		case 0x0: Execute_80(); break;
		case 0x1: Execute_81(); break;
		case 0x2: Execute_82(); break;
		case 0x3: Execute_83(); break;
		case 0x4: Execute_84(); break;
		case 0x5: Execute_85(); break;
		case 0x6: Execute_86(); break;
		case 0x7: Execute_87(); break;
		case 0x8: Execute_88(); break;
		case 0x9: Execute_89(); break;
		case 0xA: Execute_8A(); break;
		case 0xB: Execute_8B(); break;
		case 0xC: Execute_8C(); break;
		case 0xD: Execute_8D(); break;
		case 0xE: Execute_8E(); break;
		case 0xF: Execute_8F(); break;
	}
}



void Execute_9a0(f)
int f;
{
	Comment_TT("?a=b", FieldA[f]);

	DiffRegisters(Registre_A, Registre_B, f);
	Registre_CARRY = (Registre_CARRY == 0);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9a1(f)
int f;
{
	Comment_TT("?b=c", FieldA[f]);

	DiffRegisters(Registre_B, Registre_C, f);
	Registre_CARRY = (Registre_CARRY == 0);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9a2(f)
int f;
{
	Comment_TT("?c=a", FieldA[f]);

	DiffRegisters(Registre_C, Registre_A, f);
	Registre_CARRY = (Registre_CARRY == 0);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9a3(f)
int f;
{
	Comment_TT("?c=d", FieldA[f]);

	DiffRegisters(Registre_C, Registre_D, f);
	Registre_CARRY = (Registre_CARRY == 0);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}


void Execute_9a4(f)
int f;
{
	Comment_TT("?a#b", FieldA[f]);

	DiffRegisters(Registre_A, Registre_B, f);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9a5(f)
int f;
{
	Comment_TT("?b#c", FieldA[f]);

	DiffRegisters(Registre_B, Registre_C, f);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9a6(f)
int f;
{
	Comment_TT("?c#a", FieldA[f]);

	DiffRegisters(Registre_C, Registre_A, f);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9a7(f)
int f;
{
	Comment_TT("?c#d", FieldA[f]);

	DiffRegisters(Registre_C, Registre_D, f);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}


void Execute_9a8(f)
int f;
{
	Comment_TT("?a=0", FieldA[f]);

	NotZeroRegister(Registre_A, f);
	Registre_CARRY = (Registre_CARRY == 0);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9a9(f)
int f;
{
	Comment_TT("?b=0", FieldA[f]);

	NotZeroRegister(Registre_B, f);
	Registre_CARRY = (Registre_CARRY == 0);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9aA(f)
int f;
{
	Comment_TT("?c=0", FieldA[f]);

	NotZeroRegister(Registre_C, f);
	Registre_CARRY = (Registre_CARRY == 0);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9aB(f)
int f;
{
	Comment_TT("?d=0", FieldA[f]);

	NotZeroRegister(Registre_D, f);
	Registre_CARRY = (Registre_CARRY == 0);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9aC(f)
int f;
{
	Comment_TT("?a#0", FieldA[f]);

	NotZeroRegister(Registre_A, f);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9aD(f)
int f;
{
	Comment_TT("?b#0", FieldA[f]);

	NotZeroRegister(Registre_B, f);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9aE(f)
int f;
{
	Comment_TT("?c#0", FieldA[f]);

	NotZeroRegister(Registre_C, f);

	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9aF(f)
int f;
{
	Comment_TT("?d#0", FieldA[f]);
	NotZeroRegister(Registre_D, f);
	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9b0(f)
int f;
{
	struct Register dummy;
	Comment_TT("?a>b", FieldA[f]);
	SubRegisters(Registre_B, Registre_A, dummy, f);
	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9b1(f)
int f;
{
	struct Register dummy;
	Comment_TT("?b>c", FieldA[f]);
	SubRegisters(Registre_C, Registre_B, dummy, f);
	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9b2(f)
int f;
{
	struct Register dummy;
	Comment_TT("?c>a", FieldA[f]);
	SubRegisters(Registre_A, Registre_C, dummy, f);
	goyes(Registre_PC+3,
		16 + (Registre_PC&1) + 1 + fe(f)-fs(f), 8+(Registre_PC&1) +1 +fe(f)-fs(f));
}

void Execute_9b3(f)
int f;
{
	struct Register dummy;
	Comment_TT("?d>c", FieldA[f]);
	SubRegisters(Registre_C, Registre_D, dummy, f);
	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9b4(f)
int f;
{
	struct Register dummy;
	Comment_TT("?a<b", FieldA[f]);
	SubRegisters(Registre_A, Registre_B, dummy, f);
	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9b5(f)
int f;
{
	struct Register dummy;
	Comment_TT("?b<c", FieldA[f]);
	SubRegisters(Registre_B, Registre_C, dummy, f);
	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9b6(f)
int f;
{
	struct Register dummy;
	Comment_TT("?c<a", FieldA[f]);
	SubRegisters(Registre_C, Registre_A, dummy, f);
	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9b7(f)
int f;
{
	struct Register dummy;
	Comment_TT("?d<c", FieldA[f]);
	SubRegisters(Registre_D, Registre_C, dummy, f);
	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9b8(f)
int f;
{
	struct Register dummy;
	Comment_TT("?a>=b", FieldA[f]);
	SubRegisters(Registre_A, Registre_B, dummy, f);
	Registre_CARRY = (Registre_CARRY == 0);
	goyes(Registre_PC+3, 16 + (Registre_PC &1) + fe(f) - fs(f) + 1,
						  8 + (Registre_PC &1) + fe(f) - fs(f) + 1);
}

void Execute_9b9(f)
int f;
{
	struct Register dummy;
	Comment_TT("?b>=c", FieldA[f]);
	SubRegisters(Registre_B, Registre_C, dummy, f);
	Registre_CARRY = (Registre_CARRY == 0);
	goyes(Registre_PC+3,
		16 + (Registre_PC&1) + 1 + fe(f)-fs(f), 8+(Registre_PC&1) +1 +fe(f)-fs(f));
}

void Execute_9bA(f)
int f;
{
	struct Register dummy;
	Comment_TT("?c>=a", FieldA[f]);
	SubRegisters(Registre_C, Registre_A, dummy, f);
	Registre_CARRY = (Registre_CARRY == 0);
	goyes(Registre_PC+3,
		16 + (Registre_PC&1) + 1 + fe(f)-fs(f), 8+(Registre_PC&1) +1 +fe(f)-fs(f));
}

void Execute_9bB(f)
int f;
{
	struct Register dummy;
	Comment_TT("?d>=c", FieldA[f]);
	SubRegisters(Registre_D, Registre_C, dummy, f);
	Registre_CARRY = (Registre_CARRY == 0);
	goyes(Registre_PC+3,
		16 + (Registre_PC&1) + 1 + fe(f)-fs(f), 8+(Registre_PC&1) +1 +fe(f)-fs(f));
}

void Execute_9bC(f)
int f;
{
	struct Register dummy;
	Comment_TT("?a<=b", FieldA[f]);
	SubRegisters(Registre_B, Registre_A, dummy, f);
	Registre_CARRY = (Registre_CARRY == 0);
	goyes(Registre_PC+3,
		16 + (Registre_PC&1) + 1 + fe(f)-fs(f), 8+(Registre_PC&1) +1 +fe(f)-fs(f));
}

void Execute_9bD(f)
int f;
{
	struct Register dummy;
	Comment_TT("?b<=c", FieldA[f]);
	SubRegisters(Registre_C, Registre_B, dummy, f);
	Registre_CARRY = (Registre_CARRY == 0);
	goyes(Registre_PC+3,
		16 + (Registre_PC&1) + 1 + fe(f)-fs(f), 8+(Registre_PC&1) +1 +fe(f)-fs(f));
}

void Execute_9bE(f)
int f;
{
	struct Register dummy;
	Comment_TT("?c<=a", FieldA[f]);
	SubRegisters(Registre_A, Registre_C, dummy, f);
	Registre_CARRY = (Registre_CARRY == 0);
	goyes(Registre_PC+3,
		16 + (Registre_PC&1) + 1 + fe(f)-fs(f), 8+(Registre_PC&1) +1 +fe(f)-fs(f));
}

void Execute_9bF(f)
int f;
{
	struct Register dummy;
	Comment_TT("?d<=c", FieldA[f]);
	SubRegisters(Registre_C, Registre_D, dummy, f);
	Registre_CARRY = (Registre_CARRY == 0);
	goyes(Registre_PC+3,
		16 + (Registre_PC&1) + 1 + fe(f)-fs(f), 8+(Registre_PC&1) +1 +fe(f)-fs(f));
}


void Execute_9()
{
	int f, q;

	f = (int) DB_Use1(Registre_PC + 1);
	q = (int) DB_Use1(Registre_PC + 2);

	if(f < 0x8)
	{
		switch(q)
		{
			case 0x0: Execute_9a0(f); break;
			case 0x1: Execute_9a1(f); break;
			case 0x2: Execute_9a2(f); break;
			case 0x3: Execute_9a3(f); break;
			case 0x4: Execute_9a4(f); break;
			case 0x5: Execute_9a5(f); break;
			case 0x6: Execute_9a6(f); break;
			case 0x7: Execute_9a7(f); break;
			case 0x8: Execute_9a8(f); break;
			case 0x9: Execute_9a9(f); break;
			case 0xA: Execute_9aA(f); break;
			case 0xB: Execute_9aB(f); break;
			case 0xC: Execute_9aC(f); break;
			case 0xD: Execute_9aD(f); break;
			case 0xE: Execute_9aE(f); break;
			case 0xF: Execute_9aF(f); break;
		}
	}
	else
	{
		switch(q)
		{
			case 0x0: Execute_9b0(f-0x8); break;
			case 0x1: Execute_9b1(f-0x8); break;
			case 0x2: Execute_9b2(f-0x8); break;
			case 0x3: Execute_9b3(f-0x8); break;
			case 0x4: Execute_9b4(f-0x8); break;
			case 0x5: Execute_9b5(f-0x8); break;
			case 0x6: Execute_9b6(f-0x8); break;
			case 0x7: Execute_9b7(f-0x8); break;
			case 0x8: Execute_9b8(f-0x8); break;
			case 0x9: Execute_9b9(f-0x8); break;
			case 0xA: Execute_9bA(f-0x8); break;
			case 0xB: Execute_9bB(f-0x8); break;
			case 0xC: Execute_9bC(f-0x8); break;
			case 0xD: Execute_9bD(f-0x8); break;
			case 0xE: Execute_9bE(f-0x8); break;
			case 0xF: Execute_9bF(f-0x8); break;
		}
	}
}

void Execute_Aa0(f)
int f;
{
	Comment_TT("a=a+b", FieldA[f]);
	AddRegisters(Registre_A, Registre_B, Registre_A, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Aa1(f)
int f;
{
	Comment_TT("b=b+c", FieldA[f]);
	AddRegisters(Registre_B, Registre_C, Registre_B, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Aa2(f)
int f;
{
	Comment_TT("c=c+a", FieldA[f]);
	AddRegisters(Registre_C, Registre_A, Registre_C, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Aa3(f)
int f;
{
	Comment_TT("d=d+c", FieldA[f]);
	AddRegisters(Registre_D, Registre_C, Registre_D, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Aa4(f)
int f;
{
	Comment_TT("a=a+a", FieldA[f]);
	AddRegisters(Registre_A, Registre_A, Registre_A, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Aa5(f)
int f;
{
	Comment_TT("b=b+b", FieldA[f]);
	AddRegisters(Registre_B, Registre_B, Registre_B, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Aa6(f)
int f;
{
	Comment_TT("c=c+c", FieldA[f]);
	AddRegisters(Registre_C, Registre_C, Registre_C, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Aa7(f)
int f;
{
	Comment_TT("d=d+d", FieldA[f]);
	AddRegisters(Registre_D, Registre_D, Registre_D, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Aa8(f)
int f;
{
	AddRegisters(Registre_B, Registre_A, Registre_B, f);
	Comment_TT("b=b+a", FieldA[f]);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Aa9(f)
int f;
{
	Comment_TT("c=c+b", FieldA[f]);
	AddRegisters(Registre_C, Registre_B, Registre_C, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_AaA(f)
int f;
{
	Comment_TT("a=a+c", FieldA[f]);
	AddRegisters(Registre_A, Registre_C, Registre_A, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_AaB(f)
int f;
{
	Comment_TT("c=c+d", FieldA[f]);
	AddRegisters(Registre_C, Registre_D, Registre_C, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_AaC(f)
int f;
{
	Comment_TT("a=a-1", FieldA[f]);
	DecRegister(Registre_A, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_AaD(f)
int f;
{
	Comment_TT("b=b-1", FieldA[f]);
	DecRegister(Registre_B, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_AaE(f)
int f;
{
	Comment_TT("c=c-1", FieldA[f]);
	DecRegister(Registre_C, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_AaF(f)
int f;
{
	Comment_TT("d=d-1", FieldA[f]);
	DecRegister(Registre_D, f);
	Add2Cycles(4 + (Registre_PC&1) + fe(f)-fs(f)+1);
	Registre_PC += 3;
}

void Execute_Ab0(f)
int f;
{
	Comment_TT("a=0", FieldA[f]);
	ZeroRegister(Registre_A, f);
	AddToCycles(4+1+fe(f)-fe(f)+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_Ab1(f)
int f;
{
	Comment_TT("b=0", FieldA[f]);
	ZeroRegister(Registre_B, f);
	AddToCycles(4+1+fe(f)-fe(f)+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_Ab2(f)
int f;
{
	Comment_TT("c=0", FieldA[f]);
	ZeroRegister(Registre_C, f);
	AddToCycles(4+1+fe(f)-fe(f)+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_Ab3(f)
int f;
{
	Comment_TT("d=0", FieldA[f]);
	ZeroRegister(Registre_D, f);
	AddToCycles(4+1+fe(f)-fe(f)+(Registre_PC&1));
	Registre_PC +=3;
}

void Execute_Ab4(f)
int f;
{
	Comment_TT("a=b", FieldA[f]);
	CopyRegisters(Registre_B, Registre_A, f);
	AddToCycles(4 + (Registre_PC&1) + 1 + fe(f) - fs(f));
	Registre_PC += 3;
}

void Execute_Ab5(f)
int f;
{
	Comment_TT("b=c", FieldA[f]);
	CopyRegisters(Registre_C, Registre_B, f);
	AddToCycles(4 + (Registre_PC &1) + 1 + fe(f) - fs(f));
	Registre_PC += 3;
}

void Execute_Ab6(f)
int f;
{
	Comment_TT("c=a", FieldA[f]);
	CopyRegisters(Registre_A, Registre_C, f);
	AddToCycles(4 + (Registre_PC &1) + 1 + fe(f) - fs(f));
	Registre_PC += 3;
}

void Execute_Ab7(f)
int f;
{
	Comment_TT("d=c", FieldA[f]);
	CopyRegisters(Registre_C, Registre_D, f);
	AddToCycles(4 + (Registre_PC &1) + 1 + fe(f) - fs(f));
	Registre_PC += 3;
}

void Execute_Ab8(f)
int f;
{
	Comment_TT("b=a", FieldA[f]);
	CopyRegisters(Registre_A, Registre_B, f);
	AddToCycles(4 + (Registre_PC &1) + 1 + fe(f) - fs(f));
	Registre_PC += 3;
}

void Execute_Ab9(f)
int f;
{
	Comment_TT("c=b", FieldA[f]);
	CopyRegisters(Registre_B, Registre_C, f);
	AddToCycles(4 + (Registre_PC &1) + 1 + fe(f) - fs(f));
	Registre_PC += 3;
}

void Execute_AbA(f)
int f;
{
	Comment_TT("a=c", FieldA[f]);
	CopyRegisters(Registre_C, Registre_A, f);
	AddToCycles(4 + (Registre_PC &1) + 1 + fe(f) - fs(f));
	Registre_PC += 3;
}

void Execute_AbB(f)
int f;
{
	Comment_TT("c=d", FieldA[f]);
	CopyRegisters(Registre_D, Registre_C, f);
	AddToCycles(4 + (Registre_PC &1) + 1 + fe(f) - fs(f));
	Registre_PC += 3;
}

void Execute_AbC(f)
int f;
{
	struct Register tmp;

	Comment_TT("abex", FieldA[f]);
	CopyRegisters(Registre_A, tmp, f);
	CopyRegisters(Registre_B, Registre_A, f);
	CopyRegisters(tmp, Registre_B, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC += 3;
}

void Execute_AbD(f)
int f;
{
	struct Register tmp;

	Comment_TT("bcex", FieldA[f]);
	CopyRegisters(Registre_B, tmp, f);
	CopyRegisters(Registre_C, Registre_B, f);
	CopyRegisters(tmp, Registre_C, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC += 3;
}

void Execute_AbE(f)
int f;
{
	struct Register tmp;

	Comment_TT("acex", FieldA[f]);
	CopyRegisters(Registre_A, tmp, f);
	CopyRegisters(Registre_C, Registre_A, f);
	CopyRegisters(tmp, Registre_C, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC += 3;
}

void Execute_AbF(f)
int f;
{
	struct Register tmp;

	Comment_TT("cdex", FieldA[f]);
	CopyRegisters(Registre_C, tmp, f);
	CopyRegisters(Registre_D, Registre_C, f);
	CopyRegisters(tmp, Registre_D, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC += 3;
}


void Execute_A()
{
	int f, q;

	f = (int) DB_Use1(Registre_PC + 1);
	q = (int) DB_Use1(Registre_PC + 2);

	if(f < 0x8)
	{
		switch(q)
		{
			case 0x0: Execute_Aa0(f); break;
			case 0x1: Execute_Aa1(f); break;
			case 0x2: Execute_Aa2(f); break;
			case 0x3: Execute_Aa3(f); break;
			case 0x4: Execute_Aa4(f); break;
			case 0x5: Execute_Aa5(f); break;
			case 0x6: Execute_Aa6(f); break;
			case 0x7: Execute_Aa7(f); break;
			case 0x8: Execute_Aa8(f); break;
			case 0x9: Execute_Aa9(f); break;
			case 0xA: Execute_AaA(f); break;
			case 0xB: Execute_AaB(f); break;
			case 0xC: Execute_AaC(f); break;
			case 0xD: Execute_AaD(f); break;
			case 0xE: Execute_AaE(f); break;
			case 0xF: Execute_AaF(f); break;
		}
	}
	else
	{
		switch(q)
		{
			case 0x0: Execute_Ab0(f-0x8); break;
			case 0x1: Execute_Ab1(f-0x8); break;
			case 0x2: Execute_Ab2(f-0x8); break;
			case 0x3: Execute_Ab3(f-0x8); break;
			case 0x4: Execute_Ab4(f-0x8); break;
			case 0x5: Execute_Ab5(f-0x8); break;
			case 0x6: Execute_Ab6(f-0x8); break;
			case 0x7: Execute_Ab7(f-0x8); break;
			case 0x8: Execute_Ab8(f-0x8); break;
			case 0x9: Execute_Ab9(f-0x8); break;
			case 0xA: Execute_AbA(f-0x8); break;
			case 0xB: Execute_AbB(f-0x8); break;
			case 0xC: Execute_AbC(f-0x8); break;
			case 0xD: Execute_AbD(f-0x8); break;
			case 0xE: Execute_AbE(f-0x8); break;
			case 0xF: Execute_AbF(f-0x8); break;
		}
	}
}

void Execute_Ba0(f)
int f;
{
	Comment_TT("a=a-b", FieldA[f]);
	SubRegisters(Registre_A, Registre_B, Registre_A, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_Ba1(f)
int f;
{
	Comment_TT("b=b-c", FieldA[f]);
	SubRegisters(Registre_B, Registre_C, Registre_B, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_Ba2(f)
int f;
{
	Comment_TT("c=c-a", FieldA[f]);
	SubRegisters(Registre_C, Registre_A, Registre_C, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_Ba3(f)
int f;
{
	Comment_TT("d=d-c", FieldA[f]);
	SubRegisters(Registre_D, Registre_C, Registre_D, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_Ba4(f)
int f;
{
	Comment_TT("a=a+1", FieldA[f]);
	IncRegister(Registre_A, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_Ba5(f)
int f;
{
	Comment_TT("b=b+1", FieldA[f]);
	IncRegister(Registre_B, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_Ba6(f)
int f;
{
	Comment_TT("c=c+1", FieldA[f]);
	IncRegister(Registre_C, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_Ba7(f)
int f;
{
	Comment_TT("d=d+1", FieldA[f]);
	IncRegister(Registre_D, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_Ba8(f)
int f;
{
	Comment_TT("b=b-a", FieldA[f]);
	SubRegisters(Registre_B, Registre_A, Registre_B, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_Ba9(f)
int f;
{
	Comment_TT("c=c-b", FieldA[f]);
	SubRegisters(Registre_C, Registre_B, Registre_C, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_BaA(f)
int f;
{
	Comment_TT("a=a-c", FieldA[f]);
	SubRegisters(Registre_A, Registre_C, Registre_A, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_BaB(f)
int f;
{
	Comment_TT("c=c-d", FieldA[f]);
	SubRegisters(Registre_C, Registre_D, Registre_C, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_BaC(f)
int f;
{
	Comment_TT("a=b-a", FieldA[f]);
	SubRegisters(Registre_B, Registre_A, Registre_A, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_BaD(f)
int f;
{
	Comment_TT("b=c-b", FieldA[f]);
	SubRegisters(Registre_C, Registre_B, Registre_B, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_BaE(f)
int f;
{
	Comment_TT("c=a-c", FieldA[f]);
	SubRegisters(Registre_A, Registre_C, Registre_C, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}

void Execute_BaF(f)
int f;
{
	Comment_TT("d=c-d", FieldA[f]);
	SubRegisters(Registre_C, Registre_D, Registre_D, f);
	AddToCycles(4 + 1 + fe(f)-fs(f) + 1 + (Registre_PC & 1));
	Registre_PC += 3;
}


void RegSL(r, f)
struct Register *r;
int f;
{
	int qs, qe;
	int q;
	int qn;

	qs = fs(f);
	qe = fe(f);

	q = r->qu[qe];
	Registre_HST |= ((q!=0)*0x2);

	while(qe != qs)
	{
		qn = qe - 1; if(qn<0) qn = 0xf;
		q = r->qu[qn];
		r->qu[qe] = q;
		qe = qn;
	} 

	r->qu[qs] = 0;
}

void Execute_Bb0(f)
int f;
{
	Comment_TT("asl", FieldA[f]);
	RegSL(&Registre_A, f);
	AddToCycles(5+1+fe(f)-fs(f)+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_Bb1(f)
int f;
{
	Comment_TT("bsl", FieldA[f]);
	RegSL(&Registre_B, f);
	AddToCycles(5+1+fe(f)-fs(f)+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_Bb2(f)
int f;
{
	Comment_TT("csl", FieldA[f]);
	RegSL(&Registre_C, f);
	AddToCycles(5+1+fe(f)-fs(f)+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_Bb3(f)
int f;
{
	Comment_TT("dsl", FieldA[f]);
	RegSL(&Registre_D, f);
	AddToCycles(5+1+fe(f)-fs(f)+(Registre_PC&1));
	Registre_PC += 3;
}

void RegSR(r, f)
struct Register *r;
int f;
{
	int qs, qe;
	int q;
	int qn;

	qs = fs(f);
	qe = fe(f);

	q = r->qu[qs];
	Registre_HST |= ((q!=0)*0x2);

	while(qe != qs)
	{
		qn = qs + 1; if(qn>0xf) qn = 0x0;
		q = r->qu[qn];
		r->qu[qs] = q;
		qs = qn;
	} 

	r->qu[qe] = 0;
}

void Execute_Bb4(f)
int f;
{
	Comment_TT("asr", FieldA[f]);
	RegSR(&Registre_A, f);
	AddToCycles(5+1+fe(f)-fs(f)+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_Bb5(f)
int f;
{
	Comment_TT("bsr", FieldA[f]);
	RegSR(&Registre_B, f);
	AddToCycles(5+1+fe(f)-fs(f)+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_Bb6(f)
int f;
{
	Comment_TT("csr", FieldA[f]);
	RegSR(&Registre_C, f);
	AddToCycles(5+1+fe(f)-fs(f)+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_Bb7(f)
int f;
{
	Comment_TT("dsr", FieldA[f]);
	RegSR(&Registre_D, f);
	AddToCycles(5+1+fe(f)-fs(f)+(Registre_PC&1));
	Registre_PC += 3;
}

void Execute_Bb8(f)
int f;
{
	Comment_TT("a=-a", FieldA[f]);
	NegRegister(Registre_A, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC +=3;
}

void Execute_Bb9(f)
int f;
{
	Comment_TT("b=-b", FieldA[f]);
	NegRegister(Registre_B, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC +=3;
}

void Execute_BbA(f)
int f;
{
	Comment_TT("c=-c", FieldA[f]);
	NegRegister(Registre_C, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC +=3;
}

void Execute_BbB(f)
int f;
{
	Comment_TT("d=-d", FieldA[f]);
	NegRegister(Registre_D, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC +=3;
}

void Execute_BbC(f)
int f;
{
	Comment_TT("a=-a-1", FieldA[f]);
	IncRegister(Registre_A, f);
	NegRegister(Registre_A, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC +=3;
	Registre_CARRY = 0;
}

void Execute_BbD(f)
int f;
{
	Comment_TT("b=-b-1", FieldA[f]);
	IncRegister(Registre_B, f);
	NegRegister(Registre_B, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC +=3;
	Registre_CARRY = 0;
}

void Execute_BbE(f)
int f;
{
	Comment_TT("c=-c-1", FieldA[f]);
	IncRegister(Registre_C, f);
	NegRegister(Registre_C, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC +=3;
	Registre_CARRY = 0;
}

void Execute_BbF(f)
int f;
{
	Comment_TT("d=-d-1", FieldA[f]);
	IncRegister(Registre_D, f);
	NegRegister(Registre_D, f);
	AddToCycles(4+(Registre_PC&1)+1+fe(f)-fs(f));
	Registre_PC +=3;
	Registre_CARRY = 0;
}

void Execute_B()
{
	int f, q;

	f = (int) DB_Use1(Registre_PC + 1);
	q = (int) DB_Use1(Registre_PC + 2);

	if(f < 0x8)
	{
		switch(q)
		{
			case 0x0: Execute_Ba0(f); break;
			case 0x1: Execute_Ba1(f); break;
			case 0x2: Execute_Ba2(f); break;
			case 0x3: Execute_Ba3(f); break;
			case 0x4: Execute_Ba4(f); break;
			case 0x5: Execute_Ba5(f); break;
			case 0x6: Execute_Ba6(f); break;
			case 0x7: Execute_Ba7(f); break;
			case 0x8: Execute_Ba8(f); break;
			case 0x9: Execute_Ba9(f); break;
			case 0xA: Execute_BaA(f); break;
			case 0xB: Execute_BaB(f); break;
			case 0xC: Execute_BaC(f); break;
			case 0xD: Execute_BaD(f); break;
			case 0xE: Execute_BaE(f); break;
			case 0xF: Execute_BaF(f); break;
		}
	}
	else
	{
		switch(q)
		{
			case 0x0: Execute_Bb0(f-0x8); break;
			case 0x1: Execute_Bb1(f-0x8); break;
			case 0x2: Execute_Bb2(f-0x8); break;
			case 0x3: Execute_Bb3(f-0x8); break;
			case 0x4: Execute_Bb4(f-0x8); break;
			case 0x5: Execute_Bb5(f-0x8); break;
			case 0x6: Execute_Bb6(f-0x8); break;
			case 0x7: Execute_Bb7(f-0x8); break;
			case 0x8: Execute_Bb8(f-0x8); break;
			case 0x9: Execute_Bb9(f-0x8); break;
			case 0xA: Execute_BbA(f-0x8); break;
			case 0xB: Execute_BbB(f-0x8); break;
			case 0xC: Execute_BbC(f-0x8); break;
			case 0xD: Execute_BbD(f-0x8); break;
			case 0xE: Execute_BbE(f-0x8); break;
			case 0xF: Execute_BbF(f-0x8); break;
		}
	}
}

void Execute_C0()
{
	Comment_TT("a=a+b", "(a)");
	AddRegisters(Registre_A, Registre_B, Registre_A, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_C1()
{
	Comment_TT("b=b+c", "(a)");
	AddRegisters(Registre_B, Registre_C, Registre_B, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_C2()
{
	Comment_TT("c=c+a", "(a)");
	AddRegisters(Registre_C, Registre_A, Registre_C, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_C3()
{
	Comment_TT("d=d+c", "(a)");
	AddRegisters(Registre_D, Registre_C, Registre_D, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_C4()
{
	Comment_TT("a=a+a", "(a)");
	AddRegisters(Registre_A, Registre_A, Registre_A, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_C5()
{
	Comment_TT("b=b+b", "(a)");
	AddRegisters(Registre_B, Registre_B, Registre_B, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_C6()
{
	Comment_TT("c=c+c", "(a)");
	AddRegisters(Registre_C, Registre_C, Registre_C, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_C7()
{
	Comment_TT("d=d+d", "(a)");
	AddRegisters(Registre_D, Registre_D, Registre_D, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_C8()
{
	Comment_TT("b=b+a", "(a)");
	AddRegisters(Registre_B, Registre_A, Registre_B, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_C9()
{
	Comment_TT("c=c+b", "(a)");
	AddRegisters(Registre_C, Registre_B, Registre_C, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_CA()
{
	Comment_TT("a=a+c", "(a)");
	AddRegisters(Registre_A, Registre_C, Registre_A, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_CB()
{
	Comment_TT("c=c+d", "(a)");
	AddRegisters(Registre_C, Registre_D, Registre_C, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_CC()
{
	Comment_TT("a=a-1", "(a)");
	DecRegister(Registre_A, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_CD()
{
	Comment_TT("b=b-1", "(a)");
	DecRegister(Registre_B, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_CE()
{
	Comment_TT("c=c-1", "(a)");
	DecRegister(Registre_C, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}

void Execute_CF()
{
	Comment_TT("d=d-1", "(a)");
	DecRegister(Registre_D, FIELD_A);
	Add2Cycles(8);
	Registre_PC += 2;
}


void Execute_C()
{
	int q;

	q =  (int) DB_Use1(Registre_PC+1);

	switch(q)
	{
		case 0x0: Execute_C0(); break;
		case 0x1: Execute_C1(); break;
		case 0x2: Execute_C2(); break;
		case 0x3: Execute_C3(); break;
		case 0x4: Execute_C4(); break;
		case 0x5: Execute_C5(); break;
		case 0x6: Execute_C6(); break;
		case 0x7: Execute_C7(); break;
		case 0x8: Execute_C8(); break;
		case 0x9: Execute_C9(); break;
		case 0xA: Execute_CA(); break;
		case 0xB: Execute_CB(); break;
		case 0xC: Execute_CC(); break;
		case 0xD: Execute_CD(); break;
		case 0xE: Execute_CE(); break;
		case 0xF: Execute_CF(); break;
	}
}

void Execute_D0()
{
	Comment_TT("a=0", "(a)");
	ZeroRegister(Registre_A, FIELD_A);
	AddToCycles(8);
	Registre_PC +=2;
}

void Execute_D1()
{
	Comment_TT("b=0", "(a)");
	ZeroRegister(Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_PC +=2;
}

void Execute_D2()
{
	Comment_TT("c=0", "(a)");
	ZeroRegister(Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC +=2;
}

void Execute_D3()
{
	Comment_TT("d=0", "(a)");
	ZeroRegister(Registre_D, FIELD_A);
	AddToCycles(8);
	Registre_PC +=2;
}

void Execute_D4()
{
	Comment_TT("a=b", "(a)");
	UL2Reg(Reg2UL(Registre_B, FIELD_A), &Registre_A, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_D5()
{
	Comment_TT("b=c", "(a)");
	UL2Reg(Reg2UL(Registre_C, FIELD_A), &Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_D6()
{
	Comment_TT("c=a", "(a)");
	UL2Reg(Reg2UL(Registre_A, FIELD_A), &Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_D7()
{
	Comment_TT("d=c", "(a)");
	UL2Reg(Reg2UL(Registre_C, FIELD_A), &Registre_D, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_D8()
{
	Comment_TT("b=a", "(a)");
	UL2Reg(Reg2UL(Registre_A, FIELD_A), &Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_D9()
{
	Comment_TT("c=b", "(a)");
	UL2Reg(Reg2UL(Registre_B, FIELD_A), &Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_DA()
{
	Comment_TT("a=c", "(a)");
	UL2Reg(Reg2UL(Registre_C, FIELD_A), &Registre_A, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_DB()
{
	Comment_TT("c=d", "(a)");
	UL2Reg(Reg2UL(Registre_D, FIELD_A), &Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_DC()
{
	unsigned long int v;
	Comment_TT("abex", "(a)");
	v = Reg2UL(Registre_A, FIELD_A);
	UL2Reg(Reg2UL(Registre_B, FIELD_A), &Registre_A, FIELD_A);
	UL2Reg(v, &Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_DD()
{
	unsigned long int v;
	Comment_TT("bcex", "(a)");
	v = Reg2UL(Registre_B, FIELD_A);
	UL2Reg(Reg2UL(Registre_C, FIELD_A), &Registre_B, FIELD_A);
	UL2Reg(v, &Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_DE()
{
	unsigned long int v;
	Comment_TT("acex", "(a)");
	v = Reg2UL(Registre_A, FIELD_A);
	UL2Reg(Reg2UL(Registre_C, FIELD_A), &Registre_A, FIELD_A);
	UL2Reg(v, &Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_DF()
{
	unsigned long int v;
	Comment_TT("cdex", "(a)");
	v = Reg2UL(Registre_C, FIELD_A);
	UL2Reg(Reg2UL(Registre_D, FIELD_A), &Registre_C, FIELD_A);
	UL2Reg(v, &Registre_D, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}


void Execute_D()
{
	int q;

	q =  (int) DB_Use1(Registre_PC+1);

	switch(q)
	{
		case 0x0: Execute_D0(); break;
		case 0x1: Execute_D1(); break;
		case 0x2: Execute_D2(); break;
		case 0x3: Execute_D3(); break;
		case 0x4: Execute_D4(); break;
		case 0x5: Execute_D5(); break;
		case 0x6: Execute_D6(); break;
		case 0x7: Execute_D7(); break;
		case 0x8: Execute_D8(); break;
		case 0x9: Execute_D9(); break;
		case 0xA: Execute_DA(); break;
		case 0xB: Execute_DB(); break;
		case 0xC: Execute_DC(); break;
		case 0xD: Execute_DD(); break;
		case 0xE: Execute_DE(); break;
		case 0xF: Execute_DF(); break;
	}
}

void Execute_E0()
{
	Comment_TT("a=a-b", "(a)");
	SubRegisters(Registre_A, Registre_B, Registre_A, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_E1()
{
	Comment_TT("b=b-c", "(a)");
	SubRegisters(Registre_B, Registre_C, Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_E2()
{
	Comment_TT("c=c-a", "(a)");
	SubRegisters(Registre_C, Registre_A, Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_E3()
{
	Comment_TT("d=d-c", "(a)");
	SubRegisters(Registre_D, Registre_C, Registre_D, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_E4()
{
	Comment_TT("a=a+1", "(a)");
	IncRegister(Registre_A, FIELD_A);
	AddToCycles(8);
	Registre_PC+=2;
}

void Execute_E5()
{
	Comment_TT("b=b+1", "(a)");
	IncRegister(Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_PC+=2;
}

void Execute_E6()
{
	Comment_TT("c=c+1", "(a)");
	IncRegister(Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC+=2;
}

void Execute_E7()
{
	Comment_TT("d=d+1", "(a)");
	IncRegister(Registre_D, FIELD_A);
	AddToCycles(8);
	Registre_PC+=2;
}

void Execute_E8()
{
	Comment_TT("b=b-a", "(a)");
	SubRegisters(Registre_B, Registre_A, Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_E9()
{
	Comment_TT("c=c-b", "(a)");
	SubRegisters(Registre_C, Registre_B, Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_EA()
{
	Comment_TT("a=a-c", "(a)");
	SubRegisters(Registre_A, Registre_C, Registre_A, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_EB()
{
	Comment_TT("c=c-d", "(a)");
	SubRegisters(Registre_C, Registre_D, Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_EC()
{
	Comment_TT("a=b-a", "(a)");
	SubRegisters(Registre_B, Registre_A, Registre_A, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_ED()
{
	Comment_TT("b=c-b", "(a)");
	SubRegisters(Registre_C, Registre_B, Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_EE()
{
	Comment_TT("c=a-c", "(a)");
	SubRegisters(Registre_A, Registre_C, Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}

void Execute_EF()
{
	Comment_TT("d=c-d", "(a)");
	SubRegisters(Registre_C, Registre_D, Registre_D, FIELD_A);
	AddToCycles(8);
	Registre_PC += 2;
}


void Execute_E()
{
	int q;

	q =  (int) DB_Use1(Registre_PC+1);

	switch(q)
	{
		case 0x0: Execute_E0(); break;
		case 0x1: Execute_E1(); break;
		case 0x2: Execute_E2(); break;
		case 0x3: Execute_E3(); break;
		case 0x4: Execute_E4(); break;
		case 0x5: Execute_E5(); break;
		case 0x6: Execute_E6(); break;
		case 0x7: Execute_E7(); break;
		case 0x8: Execute_E8(); break;
		case 0x9: Execute_E9(); break;
		case 0xA: Execute_EA(); break;
		case 0xB: Execute_EB(); break;
		case 0xC: Execute_EC(); break;
		case 0xD: Execute_ED(); break;
		case 0xE: Execute_EE(); break;
		case 0xF: Execute_EF(); break;
	}
}

void Execute_F0()
{
	unsigned long int v;
	Comment_TT("asl", "(a)");
	v = Reg2UL(Registre_A, FIELD_A);
	UL2Reg(v*0x10, &Registre_A, FIELD_A);
	Registre_HST |= ((v&0xf0000)!=0)*0x2;
	AddToCycles(9);
	Registre_PC+=2;
}

void Execute_F1()
{
	unsigned long int v;
	Comment_TT("bsl", "(a)");
	v = Reg2UL(Registre_B, FIELD_A);
	UL2Reg(v*0x10, &Registre_B, FIELD_A);
	Registre_HST |= ((v&0xf0000)!=0)*0x2;
	AddToCycles(9);
	Registre_PC+=2;
}

void Execute_F2()
{
	unsigned long int v;
	Comment_TT("csl", "(a)");
	v = Reg2UL(Registre_C, FIELD_A);
	UL2Reg(v*0x10, &Registre_C, FIELD_A);
	Registre_HST |= ((v&0xf0000)!=0)*0x2;
	AddToCycles(9);
	Registre_PC+=2;
}

void Execute_F3()
{
	unsigned long int v;
	Comment_TT("dsl", "(a)");
	v = Reg2UL(Registre_D, FIELD_A);
	UL2Reg(v*0x10, &Registre_D, FIELD_A);
	Registre_HST |= ((v&0xf0000)!=0)*0x2;
	AddToCycles(9);
	Registre_PC+=2;
}

void Execute_F4()
{
	unsigned long int v;
	Comment_TT("asr", "(a)");
	v = Reg2UL(Registre_A, FIELD_A);
	UL2Reg(v/0x10, &Registre_A, FIELD_A);
	Registre_HST |= ((v&0xf)!=0)*0x2;
	AddToCycles(9);
	Registre_PC+=2;
}

void Execute_F5()
{
	unsigned long int v;
	Comment_TT("bsr", "(a)");
	v = Reg2UL(Registre_B, FIELD_A);
	UL2Reg(v/0x10, &Registre_B, FIELD_A);
	Registre_HST |= ((v&0xf)!=0)*0x2;
	AddToCycles(9);
	Registre_PC+=2;
}

void Execute_F6()
{
	unsigned long int v;
	Comment_TT("csr", "(a)");
	v = Reg2UL(Registre_C, FIELD_A);
	UL2Reg(v/0x10, &Registre_C, FIELD_A);
	Registre_HST |= ((v&0xf)!=0)*0x2;
	AddToCycles(9);
	Registre_PC+=2;
}

void Execute_F7()
{
	unsigned long int v;
	Comment_TT("dsr", "(a)");
	v = Reg2UL(Registre_D, FIELD_A);
	UL2Reg(v/0x10, &Registre_D, FIELD_A);
	Registre_HST |= ((v&0xf)!=0)*0x2;
	AddToCycles(9);
	Registre_PC+=2;
}

void Execute_F8()
{
	int a;
	a=Reg2UL(Registre_A, FIELD_A);
	Comment_TT("a=-a", "(a)");
	UL2Reg((unsigned long int)(0x100000-a)&0xfffff, &Registre_A, FIELD_A);
	AddToCycles(8);
	Registre_CARRY = (a!=0);
	Registre_PC+=2;
}

void Execute_F9()
{
	int a;
	a=Reg2UL(Registre_B, FIELD_A);
	Comment_TT("b=-b", "(a)");
	UL2Reg((unsigned long int)(0x100000-a)&0xfffff, &Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_CARRY = (a!=0);
	Registre_PC+=2;
}

void Execute_FA()
{
	int a;
	a=Reg2UL(Registre_C, FIELD_A);
	Comment_TT("c=-c", "(a)");
	UL2Reg((unsigned long int)(0x100000-a)&0xfffff, &Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_CARRY = (a!=0);
	Registre_PC+=2;
}

void Execute_FB()
{
	int a;
	a=Reg2UL(Registre_D, FIELD_A);
	Comment_TT("d=-d", "(a)");
	UL2Reg((unsigned long int)(0x100000-a)&0xfffff, &Registre_D, FIELD_A);
	AddToCycles(8);
	Registre_CARRY = (a!=0);
	Registre_PC+=2;
}

void Execute_FC()
{
	Comment_TT("a=-a-1", "(a)");
	UL2Reg((0xfffff-Reg2UL(Registre_A, FIELD_A))&0xfffff, &Registre_A, FIELD_A);
	AddToCycles(8);
	Registre_PC+=2;
	Registre_CARRY = 0;
}

void Execute_FD()
{
	Comment_TT("b=-b-1", "(a)");
	UL2Reg((0xfffff-Reg2UL(Registre_B, FIELD_A))&0xfffff, &Registre_B, FIELD_A);
	AddToCycles(8);
	Registre_PC+=2;
	Registre_CARRY = 0;
}

void Execute_FE()
{
	Comment_TT("c=-c-1", "(a)");
	UL2Reg((0xfffff-Reg2UL(Registre_C, FIELD_A))&0xfffff, &Registre_C, FIELD_A);
	AddToCycles(8);
	Registre_PC+=2;
	Registre_CARRY = 0;
}

void Execute_FF()
{
	Comment_TT("d=-d-1", "(a)");
	UL2Reg((0xfffff-Reg2UL(Registre_D, FIELD_A))&0xfffff, &Registre_D, FIELD_A);
	AddToCycles(8);
	Registre_PC+=2;
	Registre_CARRY = 0;
}

void Execute_F()
{
	int q;

	q =  (int) DB_Use1(Registre_PC+1);

	switch(q)
	{
		case 0x0: Execute_F0(); break;
		case 0x1: Execute_F1(); break;
		case 0x2: Execute_F2(); break;
		case 0x3: Execute_F3(); break;
		case 0x4: Execute_F4(); break;
		case 0x5: Execute_F5(); break;
		case 0x6: Execute_F6(); break;
		case 0x7: Execute_F7(); break;
		case 0x8: Execute_F8(); break;
		case 0x9: Execute_F9(); break;
		case 0xA: Execute_FA(); break;
		case 0xB: Execute_FB(); break;
		case 0xC: Execute_FC(); break;
		case 0xD: Execute_FD(); break;
		case 0xE: Execute_FE(); break;
		case 0xF: Execute_FF(); break;
	}
}

void PushTextIntoLineTextBuffer(char * txt);

void Execute()
{
	int q;
	unsigned long int address, a;
	char text[100];
	char buffer[1000];

	address = Registre_PC;

	sprintf(buffer, "%05X (%d) : ", address, DB_WhichFile(address));

	for(a=0;a<20; ++a)
	{
		sprintf(buffer+strlen(buffer), "%X", DB_Read1(address+a));
		if(a%5==4) strcat(buffer, " ");
	}
	WriteTextIntoTextWidgets(STATUS_DUMP, buffer);

	q = (int) DB_Use1(address);

	switch(q)
	{
		case 0x0: Execute_0(); break;
		case 0x1: Execute_1(); break;
		case 0x2: Execute_2(); break;
		case 0x3: Execute_3(); break;
		case 0x4: Execute_4(); break;
		case 0x5: Execute_5(); break;
		case 0x6: Execute_6(); break;
		case 0x7: Execute_7(); break;
		case 0x8: Execute_8(); break;
		case 0x9: Execute_9(); break;
		case 0xA: Execute_A(); break;
		case 0xB: Execute_B(); break;
		case 0xC: Execute_C(); break;
		case 0xD: Execute_D(); break;
		case 0xE: Execute_E(); break;
		case 0xF: Execute_F(); break;
	}

	DB_GetComment(address, text);


	sprintf(buffer, "%05X : %s", address, text);
	PushTextIntoLineTextBuffer(buffer);

#ifdef DUBUGIT
	fprintf(debug, "%05X (%d) : %s\n", address, DB_WhichFile(address), text);
#endif

	if(*text == '?') /* c est un test */
	{
		++address;

		while(DB_Used(address) && ! DB_C(address))
		{
			++address;
		}
		DB_GetComment(address, text);


		sprintf(buffer, "%05X : %s", address, text);
		PushTextIntoLineTextBuffer(buffer);

#ifdef DUBUGIT
		fprintf(debug, "%05X (%d) : %s\n", address, DB_WhichFile(address),text);
#endif

	}

#ifdef DUBUGIT
	fflush(debug);
#endif

}


void DisplayRegister(l, r, name)
int l;
struct Register r;
char *name;
{
	int i;
	char buffer[100];

	sprintf(buffer, "%s=", name);

	for(i=15;i>=0;--i)
	{
		sprintf(buffer+strlen(buffer), "%X", r.qu[i]);
	}
	WriteTextIntoTextWidgets(l, buffer);
}
		
void DisplayBits(l, n, r, name)
int l;
int n;
unsigned long int r;
char *name;
{
	int i;
	char buffer[100];

	sprintf(buffer, "%s=", name);
	for(i=n-1;i>=0;--i)
	{
		sprintf(buffer+strlen(buffer), "%X", (r&1<<i)!=0);
	}
	WriteTextIntoTextWidgets(l, buffer);
}


void DisplayCycle(l)
int l;
{
	char buffer[100];
	sprintf(buffer, "Cycles : %lu", CYCLES);
	WriteTextIntoTextWidgets(l, buffer);
}

void DisplayPointer(n, l)
int n, l;
{
	char buffer[100];
	int i;

	sprintf(buffer, "D%d=%05X [", n, Registres_D[n]);
	for(i=0; i<8*5;++i)
	{
		sprintf(buffer+strlen(buffer), "%X", DB_Read1(Registres_D[n]+i));
		if(i%5==4) sprintf(buffer+strlen(buffer), " ");
	}
	sprintf(buffer+strlen(buffer)-1, "]");

	WriteTextIntoTextWidgets(l, buffer);
}
void DisplayState()
{
	int i;
	char buffer[100];

	sprintf(buffer, "PC=%05X", Registre_PC); WriteTextIntoTextWidgets(REGISTERS_PC, buffer);

	DisplayCycle(REGISTERS_CYCLES);

	DisplayPointer(0, REGISTERS_D0);
	DisplayPointer(1, REGISTERS_D1);


	DisplayRegister(REGISTERS_A, Registre_A, "A ");
	DisplayRegister(REGISTERS_B, Registre_B, "B ");
	DisplayRegister(REGISTERS_C, Registre_C, "C ");
	DisplayRegister(REGISTERS_D, Registre_D, "D ");

	DisplayRegister(REGISTERS_R0, Registres_R[0], "R0");
	DisplayRegister(REGISTERS_R1, Registres_R[1], "R1");
	DisplayRegister(REGISTERS_R2, Registres_R[2], "R2");
	DisplayRegister(REGISTERS_R3, Registres_R[3], "R3");
	DisplayRegister(REGISTERS_R4, Registres_R[4], "R4");

	for(i=0;i<8;++i)
	{
		sprintf(buffer, "RSTK[%d]=%05X", i, Registres_RSTK[i]); WriteTextIntoTextWidgets(REGISTERS_RSTK7+7-i, buffer);
	}

	sprintf(buffer, "OUTPUT=%03X", Registre_OUT); WriteTextIntoTextWidgets(REGISTERS_OUTPUT, buffer);
	sprintf(buffer, "INPUT =%04X", Registre_IN); WriteTextIntoTextWidgets(REGISTERS_INPUT, buffer);

	sprintf(buffer, "CARRY =%X", Registre_CARRY); WriteTextIntoTextWidgets(REGISTERS_CARRY, buffer);
	sprintf(buffer, "P     =%X", Registre_P); WriteTextIntoTextWidgets(REGISTERS_P, buffer);
	DisplayBits(REGISTERS_HST, 4, (unsigned long int)Registre_HST, "HST   ");
	DisplayBits(REGISTERS_STATUS, 16, (unsigned long int)Registre_STATUS, "STATUS");

	sprintf(buffer, "Mode       : ");

	if(Mode == MODE_HEX)
	{
		strcat(buffer, "Hexadecimal");
	}
	else				
	{
		strcat(buffer, "Decimal");
	}
	WriteTextIntoTextWidgets(REGISTERS_MODE, buffer);

	sprintf(buffer, "Interrupts : ");
	if(Interrupts_OK)
	{
		strcat(buffer, "Enabled");
	} 
	else
	{
		strcat(buffer, "Disabled");
	}
	WriteTextIntoTextWidgets(REGISTERS_INTER, buffer);

	for(i=0; i<NMOD;++i)
	{
		sprintf(buffer, "%d ", i);

		if(module[i].file_r == NULL)
		{
			strcat(buffer, "   : EMPTY SLOT");
		}
		else
		{
			if(module[i].file_w == NULL)
			{
				strcat(buffer, "R  : ");
			}
			else
			{
				strcat(buffer, "RW : ");
			}
			if(module[i].infos.configured == UNCONFIGURED)
			{
				strcat(buffer, "UNCONFIGURED");
			}
			else if(module[i].infos.configured == SIZECONFIGURED)
			{
				sprintf(buffer+strlen(buffer), "Size : %05X", module[i].infos.size);
			}
			else if(module[i].infos.configured == CONFIGURED)
			{
				sprintf(buffer+strlen(buffer), "%05X-%05X", module[i].infos.address,
						module[i].infos.address+module[i].infos.size-1);
			}

			if(i==CARD2_MOD || i==CARD3_MOD)
			{
				sprintf(buffer+strlen(buffer), " Bank %02X/%02X", 
					 module[i].infos.currentbank+1,  module[i].infos.nbrofbanks);
			}
			WriteTextIntoTextWidgets(REGISTERS_M0+i, buffer);
		}
	}
}

void DisplayMainScreen()
{
	unsigned long int sep;
	unsigned long int grob;
	unsigned long int lmargin;
	unsigned long int rmargin;
	int l;
	int c;
	int b;
	int p;
	unsigned char o;

	sep = DB_ReadIOW(2, IO_SEP) & 0x3f;
	grob= DB_ReadIOW(5, IO_BTMD)&0xffffe;
	lmargin = DB_ReadIOW(1, IO_LMRG) & 0x7;
	rmargin = DB_ReadIOW(3, IO_RMRG);

	for(l=0;l<=sep;++l)
	{
		o = (unsigned char) DB_Read2(grob);
		grob +=2;
		c=0;
		b=lmargin;

		while(c<131)
		{
			p = o & (1<<b);

			PlcDrawPoint(c, l, p);
			++c;

			++b;
			if(b==8)
			{
				o = (unsigned char) DB_Read2(grob);
				grob +=2;
				b=0;
			}
		}

		grob += rmargin;

	}
}

void DisplayMenuScreen()
{
	unsigned long int sep;
	int l;
	int c;
	unsigned long int grob;
	int o;
	int i;

	sep = (int) DB_ReadIOW(2, IO_SEP) & 0x3f;
	grob= DB_ReadIOW(5, IO_BTMM)&0xffffe;

	for(l = sep+1; l<64; ++l)
	{
		c = 0;
		while(c<131)
		{
			o = (unsigned char) DB_Read2(grob);
			grob += 2;

			for(i=0;i<8;++i)
			{
				if(c<131)
				{
					PlcDrawPoint(c, l, o & (1<<i));
					++c;
				}
			}
		}
	}
}

void DisplayScreen()
{
	int state;

	state =  (DB_ReadIOW(1, IO_SOO) & 0x8 ) != 0;

	if(state == 0 ) 
	{
		ClearGraphicScreen();
		return;
	}

	WriteTextIntoTextWidgets(STATUS_MESSAGES, "Refreshing screen display...");

	DisplayMainScreen();
	DisplayMenuScreen();

	WriteTextIntoTextWidgets(STATUS_MESSAGES, "");
}


int DisplayLine = 0;
int CurrentMainDisplayLine = 0;
int DisplayCol  = 0;

void UpdateOneLineMainDisplay()
{
	unsigned long int grob;
	unsigned long int lmargin, rmargin;
	unsigned char o;
	int p;
	int b;

	grob    = DB_ReadIOW(5, IO_BTMD)&0xffffe;
	lmargin = DB_ReadIOW(1, IO_LMRG) & 0x7;
	rmargin = DB_ReadIOW(3, IO_RMRG) & 0xffe;

	grob += (34+rmargin) * DisplayLine;

	o = (unsigned char) DB_Read2(grob);
	grob +=2;
	DisplayCol=0;
	b=lmargin;

	while(DisplayCol<131)
	{
		p = o & (1<<b);

		PlcDrawPoint(DisplayCol, DisplayLine, p);
		++DisplayCol;

		++b;
		if(b==8)
		{
			o = (unsigned char) DB_Read2(grob);
			grob +=2;
			b=0;
		}
	}
	CurrentMainDisplayLine = DisplayLine;

	++DisplayLine;
	if(DisplayLine>63) DisplayLine = 0;
}

void UpdateOneLineMenuDisplay(sep)
unsigned long int sep;
{
	unsigned long int grob;
	int grobl;
	int bit;
	int o;
	int i;

	grobl = DisplayLine - sep - 1;
	grob = DB_ReadIOW(5, IO_BTMM)&0xffffe;
	grob += (grobl*34);

	DisplayCol = 0;
	while(DisplayCol<131)
	{
		o = (unsigned char) DB_Read2(grob);
		grob += 2;

		for(i=0;i<8;++i)
		{
			if(DisplayCol<131)
			{
				PlcDrawPoint(DisplayCol, DisplayLine, o & (1<<i));
				++DisplayCol;
			}
		}
	}
	CurrentMainDisplayLine = DisplayLine;

	++DisplayLine;
	if(DisplayLine > 63) DisplayLine = 0;
}

void UpdateOneLine()
{
	int state;
	unsigned long int sep;

	state =  (DB_ReadIOW(1, IO_SOO) & 0x8 ) != 0;

	if(state == 0) 
	{
		void ClearGraphicScreen();
		return;
	}

	sep = (int) DB_ReadIOW(2, IO_SEP) & 0x3f;

	if(DisplayLine<=sep)
	{
		UpdateOneLineMainDisplay();
	}
	else
	{
		UpdateOneLineMenuDisplay(sep);
	}

}

int oldannstate = -1;

void DisplayAnnunciators()
{
	unsigned char ann;

	ann = DB_ReadIOW(2, IO_ANN);

	if(ann == 0)
	{
		if(oldannstate != ann) PlcClearAnnunciators();
		oldannstate = ann;
		return;
	}

	if(ann != oldannstate) PlcDisplayAnnunciators(ann);
	oldannstate = ann;
}

unsigned long int horl;
unsigned long int horl1;
unsigned long int horloge;
unsigned long int horloge1;

void InitIO()
{
	unsigned long int i;

	i = 0x00100;

	while(i<0x140)
	{
		DB_WriteIO(1, i++, 0x0L);
	}
	return;
}

void CopyIO(a, i)
unsigned long int a;
unsigned long int i;
{
	unsigned long int j, q;
	
	for(j=0;j<i;++j)
	{
		q = DB_ReadIOW(1, a+j);
		DB_WriteIOR(1, a+j, q);
	}
}

#define TIMER1_TICK	(214 * 512)     /* cycles */
#define TIMER2_TICK	214 			/* cycles */
#define DISPLAY_TICK (TIMER2_TICK*2)/* cycles */

unsigned long int horl3;

void UpdateIO()
{
	int q;
	int ft1, ft2;
	int k;
	int o;

	CopyIO(0x00100, 0x20);
	CopyIO(0x0012A, 0x16);

	/* update PORT infos */

	q = 0;
	if(module[CARD2_MOD].file_r != NULL)
	{
		if(module[CARD2_MOD].file_w != NULL) q |= 0x5;
		else q |= 0x1;
	}
	if(module[CARD3_MOD].file_r != NULL)
	{
		if(module[CARD3_MOD].file_w != NULL) q |= 0xA;
		else q |= 0x2;
	}

	DB_WriteIO(1, IO_INFOP, q);

	/* update Vsync */
	o = (DB_ReadIOW(2, IO_VSYNC) &0xC0);
	o |= CurrentMainDisplayLine;
	DB_WriteIOR(2, IO_VSYNC, o);


	ft1 = DB_ReadIOR(1, IO_T1C);
	ft2 = DB_ReadIOR(1, IO_T2C);

	/* Update Timer 1 */

	if((CYCLES-horl1) > (TIMER1_TICK))
	{
		horloge1 = DB_ReadIOR(1, IO_TM1);

		while(CYCLES-horl1 > (TIMER1_TICK))
		{
			if(horloge1 == 0)
			{
				horloge1 = 0xf;
				if(ft1&0x2 && Processing_Interrupt == 0)
				{
					Interrupt_Pending = 1;
					InterruptClass |= INTERRUPTION_TIMER1;
				}
				if(ft1&0x4)
				{
					Interrupt_Wakeup |= 1;
				}
			}
			else
			{
				--horloge1;
			}
			horl1 +=(TIMER1_TICK);
		}
		if(ft1 & 0x1) DB_WriteIO(1, IO_TM1, horloge1);
	}

	/* update TIMER2 */

	if((CYCLES-horl) > TIMER2_TICK)
	{
		horloge = DB_ReadIOR(8, IO_TM2);

		while(CYCLES-horl > TIMER2_TICK)
		{
			if(horloge == 0)
			{
				horloge = 0xffffffff;
				if(ft1&0x4 && Processing_Interrupt == 0)
				{
					Interrupt_Pending = 1;
					InterruptClass |= INTERRUPTION_TIMER2;
				}
				if(ft2&0x4)
				{
					Interrupt_Wakeup |= 1;
				}
			}
			else
			{
				--horloge;
			}
			horl +=TIMER2_TICK;
		}
		if(ft2 & 0x1) DB_WriteIO(8, IO_TM2, horloge);
	}

	/* update display, 1 pixel each tick */

	if((CYCLES-horl3) > DISPLAY_TICK)
	{
		while(CYCLES-horl3 > DISPLAY_TICK)
		{
			horl3 +=DISPLAY_TICK;
			UpdateOneLine();
		}
	}

	/* update INPUT value */

	Registre_IN = 0;
	for(k=0;k<49;++k)
	{
		if(keymask[k] && ((keyout[k]&Registre_OUT)||k==44/*ON*/))
		{
			Registre_IN |= keyin[k];
		}
	}
}
	
int keyn(v)
int v;
{
	int l, c;

	l = (v/0x10)-1;
	c = (v%0x10)-1;

	if(l<0 || l>8 || c<0 || c> 5) return -1;

	if(l<4) return l*6+c;

	if(c>4) return -1;

	return 24 + (l-4)*5+c;
}

int getachar()
{
	int c;
	while((c=ProcessXEventsIfNecessary())==0)
		;

	return c;
}

void em_gettext(question, answer, def)
char *question, *answer;
char *def;
{
	int c;
	int i;
	char buffer[1000];

	strcpy(answer, def);
	i = strlen(answer);

	sprintf(buffer, "%s%s_", question, answer);
	WriteTextIntoTextWidgets(STATUS_MESSAGES, buffer);

	while((c=getachar())!=13)
	{
		if(c<' '||c>=127) switch(c)
		{
			case 8: /* BACKSPACE: */
			case 127: /* delete */
				--i; if(i<0) i=0;

			default:
				break;
		}
		else
		{
			answer[i++] = c;
		}
		answer[i]='\0';

		sprintf(buffer, "%s%s_", question, answer);
		WriteTextIntoTextWidgets(STATUS_MESSAGES, buffer);
	}
	WriteTextIntoTextWidgets(STATUS_MESSAGES, "");
}

int getaddr(txt, v, def)
char *txt;
unsigned long int *v;
char *def;
{
	char buffer[1000];
	int r;

	em_gettext(txt, buffer, def);

	r = sscanf(buffer, "%x", v);

	return (r==1);
}

void ClearPendingInterrupts()
{
	InterruptClass = 0;
	Interrupt_Pending = 0;
}

void Interruptions()
{
	int ft1, ft2, srq;

	if(Interrupt_Pending == 0) return;
	if(InterruptClass==0)
	{
		return;
	}

	Processing_Interrupt = 1;

	fprintf(stderr, "An interrupt has occured - class %X...\n",
							InterruptClass);

	if((InterruptClass & INTERRUPTION_KEYBOARD) && !Interrupts_OK)
	{
		InterruptClass -= INTERRUPTION_KEYBOARD;
		if(InterruptClass==0)
		{
			return;
		}
	}

	Interrupt_Pending = 0;
	PushRstk(Registre_PC);
	Registre_PC = INTERRUPT_HANDLER;

	/* update IO with int. class */

	srq = (int) DB_ReadIOR(2, IO_SRQ);
	ft1 = (int) DB_ReadIOR(1, IO_T1C);
	ft2 = (int) DB_ReadIOR(1, IO_T2C);

	if(InterruptClass&INTERRUPTION_KEYBOARD)
	{
		srq |= 0x80;
	}

	if(InterruptClass&INTERRUPTION_TIMER1)
	{
		ft1 |=0x8;
	}

	if(InterruptClass&INTERRUPTION_TIMER2)
	{
		ft2 |=0x8;
	}

	DB_WriteIO(2, IO_SRQ, (unsigned long int) srq);
	DB_WriteIO(1, IO_T1C, (unsigned long int) ft1);
	DB_WriteIO(1, IO_T2C, (unsigned long int) ft2);

	InterruptClass = 0;

}

int WhichMode;
unsigned long int breaka; 
int breakf;
int goon;

extern char ButtonTxt[10][100];

void RefreshButton(int i);

int main(argc, argv)
int argc;
char **argv;
{
	int c;
	unsigned long int a;
	int flag;

	displayit = 1;

	Interrupt_Pending = 0;
	Processing_Interrupt = 0;

#ifdef DUBUGIT
	debug = fopen("Debug.dbg", "w");
#endif

	DB_OpenFiles();
	LoadLabels();

	CreateWindows(argc, argv);

	InitIO();

	breakf = 0;

	WhichMode = 's';

	horl3 = horl1 = horl = CYCLES = 0;
	DB_WriteIOR(8, IO_TM2, 0L);

	fcntl(0, F_SETFL, fcntl(0, F_GETFL, 0) | O_NDELAY);

	ClearPendingInterrupts();	

	while(WhichMode != 'q')
	{
		if(Interrupt_Pending) Interruptions();

		UpdateIO();
		DisplayState();
		
		DisplayAnnunciators();
		a = Registre_PC;

		Interrupt_Requested = 0;
		Interrupt_Wakeup = 0;
		Execute();

		if(Interrupt_Requested)
		{
			WriteTextIntoTextWidgets(STATUS_INTR,"Wake-up requested...");
			XFlush(mydisplay);
		}

		if(breakf && a == breaka)
		{
			WhichMode = 's';
			strcpy(ButtonTxt[BUTTON_STEP], "Step");
			RefreshButton(BUTTON_STEP);
			XFlush(mydisplay);
			fprintf(stderr, "Halted...\n"); fflush(stderr);
		}

		/* get order */

		goon = 0;
		do
		{
			c = ProcessXEventsIfNecessary();

			if(Interrupt_Requested)
			{
				AddToCycles(TIMER2_TICK);
				DisplayCycle(REGISTERS_CYCLES);
				UpdateIO();
			}

			switch(c)
			{
				case ' ':
					goon = 1;
					break;
			}
		}
		while(
				(
					(Interrupt_Requested && !Interrupt_Wakeup  ) ||
					(WhichMode == 's' && goon == 0)
				)
				&& WhichMode != 'q'
			 );

		if(Interrupt_Requested)
		{
			WriteTextIntoTextWidgets(STATUS_INTR,"");
		}
	}

#ifdef DUBUGIT
	fclose(debug);
#endif

	CloseWindows();

	DB_CloseFiles();
	CloseLabels();

}
