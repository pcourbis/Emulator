#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "db.h"
#include "interface.h"

extern unsigned long int Registre_IN, Registre_OUT;

extern unsigned char Interrupt_Pending, Interrupts_OK, Interrupt_Wakeup, Processing_Interrupt;
extern unsigned int InterruptClass;
extern int keyout[], keyin[];

void InitAnnShapes();
void PlcDrawCenteredString();
GC mygc;
Font myfont;

int ControlIconified = 0;

#define TOPGAP			20
#define BOTTOMGAP		20
#define ANNWIDTH		131
#define	ANNHEIGTH		10
#define SCREENWIDTH		ANNWIDTH
#define SCREENHEIGTH	64
#define SCRBOTGAT		20
#define KBDVSTART		(TOPGAP+ANNHEIGTH+SCREENHEIGTH+SCRBOTGAT)

#define DISPLAYW		SCREENWIDTH
#define DISPLAYH		(ANNHEIGTH+SCREENHEIGTH)

#define SCREENHMRGN		20
#define MAINWINDOWWIDTH	(2*SCREENHMRGN+SCREENWIDTH)

#define STDKEYWIDTH		18
#define KEYHEIGHT		10
#define KEYHGAP			5
#define KEYVGAP			5

#define BORDERWIDTH		10
#define BORDERHEIGTH	10

#define KBDWIDTH		(6*STDKEYWIDTH+5*KEYHGAP)
#define KBDHMRGN		((MAINWINDOWWIDTH-KBDWIDTH)/2)
#define ENTERWIDTH		(2*STDKEYWIDTH+KEYHGAP)
#define BIGKEYWIDTH		((KBDWIDTH-STDKEYWIDTH-4*KEYHGAP)/4)

#define KBDHEIGHT		(9*KEYHEIGHT+8*KEYVGAP)

#define FIRSTBKGAP		(KBDWIDTH-STDKEYWIDTH-4*BIGKEYWIDTH-3*KEYHGAP);

#define MAINWINDOWHEIGHT	(KBDVSTART+	KBDHEIGHT+BOTTOMGAP)

#define BORDER			1

Display *mydisplay;
unsigned long black, white;

Window PlcCreateKey(w, x, y, width, height)
Window w;
int x, y;
int width, height;
{
	Window win;
	win = XCreateSimpleWindow(mydisplay, w, 
							   x, y, width, height,
							   BORDER,
							   black,
							   white);
	return win;
}

Window PlcCreateBigKey(w, x, y)
Window w;
int x, y;
{
	return PlcCreateKey(w, x, y, BIGKEYWIDTH, KEYHEIGHT);
}

Window PlcCreateStdKey(w, x, y)
Window w;
int x, y;
{
	return PlcCreateKey(w, x, y, STDKEYWIDTH, KEYHEIGHT);
}

Window PlcCreateEnterKey(w, x, y)
Window w;
int x, y;
{
	return PlcCreateKey(w, x, y, ENTERWIDTH, KEYHEIGHT);
}

Window hp48;
	Window hp48Display;
		Window AnnunciatorsScreen;
		Window GraphicScreen;
	Window Keyboard;
		Window keys[9][6];

int keyheigth[9][6], keywidth[9][6];

char *KeyTexts[9][6] =
{
	{ "A", "B", "C", "D", "E",   "F"  },
	{ "G", "H", "I", "J", "K",   "L"  },
	{ "M", "N", "O", "P", "Q",   "R"  },
	{ "S", "T", "U", "V", "W",   "X"  },
	{ "ENTER",  "Y", "Z", "DEL", "<-" },
	{ "a",   "7",   "8",   "9",   "/" },
	{ "<",   "4",   "5",   "6",   "*" },
	{ ">",   "1",   "2",   "3",   "-" },
	{ "ON",  "0",   ".",   "SPC", "+" }
};

void RedrawKeyText(i, j)
int i, j;
{
	if(keys[i][j] != 0)
	{
		PlcDrawCenteredString(mydisplay, keys[i][j], mygc, keywidth[i][j],
								keyheigth[i][j], KeyTexts[i][j]);
	}
}

#define DESASSWIDTH		600
#define DESASSNLINE		12
#define LINECWIDTH		80 /* a regler ! */
#define DESASSLINEHEIGTH	10
#define DESASSHEIGTH	(DESASSNLINE*DESASSLINEHEIGTH)
#define DESASSHMRG		20
#define DESASSVMARGIN	20
#define CONTROLWIDTH	(DESASSHMRG*2+DESASSWIDTH)

int keypushed[9][6];

void PlcCreateKeyboard(w)
Window w;
{
	int i, j;
	int x, y;

	for(i=0;i<9;++i) for(j=0;j<6;++j) keypushed[i][j] = -1;

	Keyboard = XCreateSimpleWindow(mydisplay, w, KBDHMRGN, KBDVSTART,
								   KBDWIDTH+2, KBDHEIGHT+2,
								   0, black,
								   white);

	XMapWindow(mydisplay, Keyboard);

	y = 0;

	for(i=0;i<4;++i)
	{
		x = 0;
		for(j=0;j<6;++j)
		{
			keys[i][j] = PlcCreateStdKey(Keyboard, x, y, KeyTexts[i][j]);
			XSelectInput(mydisplay, keys[i][j], ButtonPressMask);
			XMapWindow(mydisplay, keys[i][j]);
			keypushed[i][j] = 0;
			x += STDKEYWIDTH + KEYHGAP;

			keyheigth[i][j] = KEYHEIGHT; 
			keywidth[i][j]  = STDKEYWIDTH;
		}
		y += KEYHEIGHT + KEYVGAP;
	}
	x = 0;
	keys[4][0] = PlcCreateEnterKey(Keyboard, x, y, KeyTexts[i][j]);
	XSelectInput(mydisplay, keys[4][0], ButtonPressMask);
	XMapWindow(mydisplay, keys[4][0]);
	keypushed[4][0] = 0;
	x += ENTERWIDTH + KEYHGAP;
	keyheigth[4][0] = KEYHEIGHT; 
	keywidth[4][0]  = ENTERWIDTH;
	for(i=1;i<5; ++i)
	{
		keys[4][i] = PlcCreateStdKey(Keyboard, x, y, KeyTexts[i][j]);
		XSelectInput(mydisplay, keys[4][i], ButtonPressMask);
		XMapWindow(mydisplay, keys[4][i]);
		keypushed[4][i] = 0;
		x += STDKEYWIDTH + KEYHGAP;
		keyheigth[4][i] = KEYHEIGHT; 
		keywidth[4][i]  = BIGKEYWIDTH;
	}

	y += KEYHEIGHT + KEYVGAP;

	for(i=5;i<9;++i)
	{
		x = 0;
		keys[i][0] = PlcCreateStdKey(Keyboard, x, y, KeyTexts[i][j]);
		XSelectInput(mydisplay, keys[i][0], ButtonPressMask);
		XMapWindow(mydisplay, keys[i][0]);
		keyheigth[i][0] = KEYHEIGHT; 
		keywidth[i][0]  = STDKEYWIDTH;
		keypushed[i][0] = 0;
		x += STDKEYWIDTH + FIRSTBKGAP;

		for(j=1; j<5; ++ j)
		{
			keys[i][j] = PlcCreateBigKey(Keyboard, x, y);
			XSelectInput(mydisplay, keys[i][j], ButtonPressMask);
			XMapWindow(mydisplay, keys[i][j]);
			keypushed[i][j] = 0;
			x += BIGKEYWIDTH  + KEYHGAP;
			keyheigth[i][j] = KEYHEIGHT; 
			keywidth[i][j]  = BIGKEYWIDTH;
		}

		 y += KEYHEIGHT + KEYVGAP;
	}
}

#define DESASSWIDTH		600
#define DESASSNLINE		12
#define LINECWIDTH		80 /* a regler ! */
#define DESASSLINEHEIGTH	10
#define DESASSHEIGTH	(DESASSNLINE*DESASSLINEHEIGTH)
#define DESASSHMRG		20
#define DESASSVMARGIN	20
#define CONTROLWIDTH	(DESASSHMRG*2+DESASSWIDTH)

#define COMMANDSSTART	(DESASSVMARGIN*2+DESASSHEIGTH)
#define COMMANDHEIGTH	20
#define COMMANDHMRG		20
#ifdef WORK_IN_MEMORY
#define COMMANDN		7
#else
#define COMMANDN		6
#endif
#define COMMANDBTN		((DESASSWIDTH-(COMMANDN-1)*COMMANDHMRG)/COMMANDN)


#define TEXTSTART		(COMMANDSSTART+COMMANDHEIGTH+DESASSVMARGIN)

#define BOXWIDTH		6
#define BOXHEIGTH		6
#define BOXHMRG			5
#define TEXTWIDTH		(DESASSWIDTH-BOXWIDTH-BOXHMRG)
#define BOXVMRG			(((TEXTHEIGTH-BOXHEIGTH) / 2)-3)
#define TEXTHEIGTH		DESASSLINEHEIGTH
#define TEXTNLINES		CONTROL_NLINES
#define ALLTEXTHEIGTH	(TEXTHEIGTH*TEXTNLINES)
#define TEXTLINECWIDTH  (LINECWIDTH-5)


#define ALLTEXTSWIDTH	TEXTWIDTH

#define BOTTOMMRG		20


#define CONTROLHEIGTH	(TEXTSTART + ALLTEXTHEIGTH + BOTTOMMRG /* +... */ )


Window Control;
	Window Desass;
		Window DesassLines[DESASSNLINE];
	Window Buttons[COMMANDN];
	Window AllTexts;
		Window Boxes[TEXTNLINES];
		Window TextLines[TEXTNLINES];

char desasslinestext[DESASSNLINE][1000];
unsigned char DrawBoxes[TEXTNLINES];
char textlinestext[TEXTNLINES][1000];

char ButtonTxt[10][100];
char *OriginalButtonText[COMMANDN] =
{
	"Step", "Run", "Break", "Update DSP", "Quit", "Wakeup",
#ifdef WORK_IN_MEMORY
	"Dump"
#endif
};

#define ASCEND	3

void WriteDesassText()
{
	int i;

	if(ControlIconified==1) return;

	for(i=0; i<DESASSNLINE; ++i)
	{
		XClearWindow(mydisplay, DesassLines[i]);
		XSetFunction(mydisplay, mygc, GXset);
		XDrawImageString(mydisplay, DesassLines[i], mygc, 0,
			DESASSLINEHEIGTH-ASCEND, desasslinestext[i],
			strlen(desasslinestext[i]));
	}
}

void DrawTextText(i)
int i;
{
	if(ControlIconified==1) return;

	XClearWindow(mydisplay, TextLines[i]);
	XSetFunction(mydisplay, mygc, GXset);
	XDrawImageString(mydisplay, TextLines[i], mygc, 0,
		TEXTHEIGTH-ASCEND, textlinestext[i], strlen(textlinestext[i]));
}

void RefreshTextText()
{
	int i;
	for(i=0;i<TEXTNLINES; ++i)
	{
		DrawTextText(i);
	}
}

void PlcDrawCenteredString(dsp, win, gc, width, heigth, txt)
Display *dsp;
Window win;
GC gc;
int width, heigth;
char *txt;
{
	int dir, asc, desc;
	XCharStruct infos;
	int w, h;
	int mh, mv;
	int sh, sv;

	XQueryTextExtents(dsp, myfont, txt, strlen(txt), &dir, &asc, &desc, &infos);

	w = infos.rbearing;
	h = infos.ascent;
	mh = (width-w)/2;
	mv = (heigth-h)/2;
	sh = mh;
	sv = mv+h;

	XDrawImageString(dsp, win, gc, sh, sv, txt, strlen(txt));
}

void RefreshButton(i)
int i;
{
	XClearWindow(mydisplay, Buttons[i]);
	XSetFunction(mydisplay, mygc, GXset);
	PlcDrawCenteredString(mydisplay, Buttons[i], mygc, COMMANDBTN,
				COMMANDHEIGTH,ButtonTxt[i]);
}

void RefreshButtons()
{
	int i;

	for(i=0; i<COMMANDN; ++i)
	{
		RefreshButton(i);
	} 
}

void PushTextIntoLineTextBuffer(txt)
char *txt;
{
	int n=0;
	int i, c, l, m;

	i = l = 0;
	n=1;
	while((c=txt[i++])!='\0')
	{
		if(c=='\n')
		{
			++n;
			l = 0;
		} else
		{
			++l;
			if(l>LINECWIDTH)
			{
				++n;
				l = 0;
			}
		}
	}

	/* n = nbr line to push */

	for(i=0; i<DESASSNLINE-n; ++i)
	{
		strcpy(desasslinestext[i], desasslinestext[i+n]);
	}

	/* scroll done, do push text */

	i = l = 0;
	m = DESASSNLINE-n;
	n = 0;
	while((c=txt[i++])!='\0')
	{
		if(c=='\n')
		{
			desasslinestext[m][n] = '\0';
			++m;
			n = 0;
			l = 0;
		}
		else
		{
			desasslinestext[m][n] = c;
			++n;
			++l;
			if(l>LINECWIDTH)
			{
				desasslinestext[m][n] = '\0';
				++m;
				n = 0;
				l = 0;
			}
		}
	}
	desasslinestext[m][n] = '\0';
		
	WriteDesassText();
}

void WriteTextIntoTextWidgets(n, txt)
int n;
char *txt;
{
	if(strcmp(txt, textlinestext[n])==0) return;
	if(n>=TEXTNLINES) return;
	strncpy(textlinestext[n], txt, TEXTLINECWIDTH);
	DrawTextText(n);
}

void InitDrawBoxes()
{
	int i;

	for(i=0; i<TEXTNLINES; ++i)
	{
		DrawBoxes[i] = 0;
	}


	DrawBoxes[REGISTERS_PC] = 1;
	DrawBoxes[REGISTERS_CYCLES] = 1;
	DrawBoxes[REGISTERS_D0] = 1;
	DrawBoxes[REGISTERS_D1] = 1;
	DrawBoxes[REGISTERS_A] = 1;
	DrawBoxes[REGISTERS_B] = 1;
	DrawBoxes[REGISTERS_C] = 1;
	DrawBoxes[REGISTERS_D] = 1;
	DrawBoxes[REGISTERS_R0] = 1;
	DrawBoxes[REGISTERS_R1] = 1;
	DrawBoxes[REGISTERS_R2] = 1;
	DrawBoxes[REGISTERS_R3] = 1;
	DrawBoxes[REGISTERS_R4] = 1;
	DrawBoxes[REGISTERS_RSTK7] = 1;
	DrawBoxes[REGISTERS_RSTK6] = 1;
	DrawBoxes[REGISTERS_RSTK5] = 1;
	DrawBoxes[REGISTERS_RSTK4] = 1;
	DrawBoxes[REGISTERS_RSTK3] = 1;
	DrawBoxes[REGISTERS_RSTK2] = 1;
	DrawBoxes[REGISTERS_RSTK1] = 1;
	DrawBoxes[REGISTERS_RSTK0] = 1;
	DrawBoxes[REGISTERS_OUTPUT] = 1;
	DrawBoxes[REGISTERS_INPUT] = 1;
	DrawBoxes[REGISTERS_CARRY] = 1;
	DrawBoxes[REGISTERS_P] = 1;
	DrawBoxes[REGISTERS_HST] = 1;
	DrawBoxes[REGISTERS_STATUS] = 1;
	DrawBoxes[REGISTERS_MODE] = 1;
	DrawBoxes[REGISTERS_INTER] = 1;
	DrawBoxes[REGISTERS_M0] = 1;
	DrawBoxes[REGISTERS_M1] = 1;
	DrawBoxes[REGISTERS_M2] = 1;
	DrawBoxes[REGISTERS_M3] = 1;
	DrawBoxes[REGISTERS_M4] = 1;
	DrawBoxes[REGISTERS_M5] = 1;
}

void PlcCreateControlWindow()
{
	int i;

	Control = XCreateSimpleWindow(mydisplay, DefaultRootWindow(mydisplay),
		0, 0, CONTROLWIDTH, CONTROLHEIGTH, 1, black, white);

	Desass = XCreateSimpleWindow(mydisplay, Control,
		DESASSHMRG, DESASSVMARGIN, DESASSWIDTH, DESASSHEIGTH, 0, black, white);

	XSelectInput(mydisplay, Desass, KeyPressMask);

	XMapWindow(mydisplay, Desass);

	for(i=0;i<DESASSNLINE;++i)
	{
		DesassLines[i] = XCreateSimpleWindow(mydisplay, Desass,
				0, i*DESASSLINEHEIGTH, DESASSWIDTH, DESASSLINEHEIGTH,
				0, black, white);

		XMapWindow(mydisplay, DesassLines[i]);
		XSelectInput(mydisplay, DesassLines[i], KeyPressMask);

		strcpy(desasslinestext[i], "");
	}

	for(i=0;i<COMMANDN;++i)
	{
		Buttons[i] = XCreateSimpleWindow(mydisplay, Control, 
						DESASSHMRG+i*(COMMANDBTN+COMMANDHMRG), COMMANDSSTART,
						COMMANDBTN, COMMANDHEIGTH, 1, black, white);

		XMapWindow(mydisplay, Buttons[i]);
		XMapWindow(mydisplay, Buttons[i]);
		XSelectInput(mydisplay, Buttons[i], KeyPressMask|ButtonPressMask);

		strcpy(ButtonTxt[i], OriginalButtonText[i]);
	}


				

	AllTexts = XCreateSimpleWindow(mydisplay, Control,
		DESASSHMRG, TEXTSTART, ALLTEXTSWIDTH,
		ALLTEXTHEIGTH, 0, black, white);

	XMapWindow(mydisplay, AllTexts);

	XSelectInput(mydisplay, AllTexts, KeyPressMask);

	InitDrawBoxes();

	for(i=0;i<TEXTNLINES;++i)
	{
		if(DrawBoxes[i])
		{
			Boxes[i] = XCreateSimpleWindow(mydisplay, AllTexts,
					0, i*TEXTHEIGTH+BOXVMRG, BOXWIDTH, BOXHEIGTH,
					1, black, white);

			XMapWindow(mydisplay, Boxes[i]);

			XSelectInput(mydisplay, Boxes[i], ButtonPressMask);

			TextLines[i] = XCreateSimpleWindow(mydisplay, AllTexts,
				BOXWIDTH+BOXHMRG, i*TEXTHEIGTH, TEXTWIDTH, TEXTHEIGTH,
				0, black, white);
		}
		else
		{
			TextLines[i] = XCreateSimpleWindow(mydisplay, AllTexts,
					0, i*TEXTHEIGTH, TEXTWIDTH+BOXWIDTH+BOXHMRG, TEXTHEIGTH,
					0, black, white);
		}

		XMapWindow(mydisplay, TextLines[i]);
		XSelectInput(mydisplay, TextLines[i], KeyPressMask);

		strcpy(textlinestext[i], "");
	}

	XSelectInput(mydisplay, Control, KeyPressMask | ExposureMask |
									 StructureNotifyMask);

	XMapRaised(mydisplay, Control);
}

char *title = "48 emulator-V2.1";


void CreateWindows(argc, argv)
int argc;
char **argv;
{
	XSizeHints myhint;
	int myscreen;

	mydisplay = XOpenDisplay("");

	if(mydisplay == NULL)
	{
		fprintf(stderr, "unable to open display...\n");
		exit(1);
	}

	myscreen = DefaultScreen(mydisplay);

	black = WhitePixel(mydisplay, myscreen);
	white = BlackPixel(mydisplay, myscreen);
	/*
	white = WhitePixel(mydisplay, myscreen);
	black = BlackPixel(mydisplay, myscreen);
	*/

	myhint.x = 200; myhint.y = 300;
	myhint.width=MAINWINDOWWIDTH; myhint.height = MAINWINDOWHEIGHT;
	myhint.flags = PPosition;

	hp48 = XCreateSimpleWindow(mydisplay, DefaultRootWindow(mydisplay),
		myhint.x, myhint.y, myhint.width, myhint.height,  5,
		black, white);

	mygc = XCreateGC(mydisplay, hp48, 0, 0);
	myfont = XLoadFont(mydisplay,
				"6x10");
				/* "-misc-*-medium-*-normal-*-10-110-75-*-c-75-*-*"); */
	XSetFont(mydisplay, mygc, myfont);
	XSetBackground(mydisplay, mygc, white);
	XSetForeground(mydisplay, mygc, black);

	hp48Display = XCreateSimpleWindow(mydisplay, hp48, SCREENHMRGN, TOPGAP,
			DISPLAYW, DISPLAYH, 1, black, white);

	XMapWindow(mydisplay, hp48Display);

	AnnunciatorsScreen = XCreateSimpleWindow(mydisplay, hp48Display, 
			0, 0, ANNWIDTH, ANNHEIGTH, 0, black, white);

	XMapWindow(mydisplay, AnnunciatorsScreen);

	GraphicScreen = XCreateSimpleWindow(mydisplay, hp48Display,
			0, ANNHEIGTH, SCREENWIDTH, SCREENHEIGTH, 0, black, white);

	XMapWindow(mydisplay, GraphicScreen);

	PlcCreateKeyboard(hp48);

	XSetStandardProperties(mydisplay, hp48, title, title, None, argv,
		argc, &myhint);

	XSelectInput(mydisplay, hp48, KeyPressMask | ExposureMask);

	XMapRaised(mydisplay, hp48);

	PlcCreateControlWindow();

	InitAnnShapes();

	ClearGraphicScreen();
}

Bool ChooseAll(dsp, evt, args)
Display *dsp;
XEvent *evt;
char *args;
{
	return True;
}

void DisplayScreen();
void DisplayAnnunciators();

int keymask[49];

int KeyWinToNumber(l, c)
int l, c;
{
    if(l<0 || l>8 || c<0 || c> 5) return -1;

    if(l<4) return l*6+c;

    if(c>4) return -1;

    return 24 + (l-4)*5+c;
}

void RedrawKey(i, j)
int i, j;
{
	if(keys[i][j] != 0)
	{
		XClearWindow(mydisplay, keys[i][j]);
		RedrawKeyText(i, j);
		if(keypushed[i][j])
		{
			XSetFunction(mydisplay, mygc, GXxor);
			XFillRectangle(mydisplay, keys[i][j], mygc,
						0, 0, keywidth[i][j], keyheigth[i][j]);
		}

	}
}
void RedrawKeys()
{
	int i, j;

	for(i=0;i<9;++i)
		for(j=0;j<6;++j)
		{
			RedrawKey(i, j);
		}
}


void TreatKey(key)
Window key;
{
	int i, j;
	int kn;

	for(i=0;i<9;++i)
	{
		for(j=0;j<6;++j)
		{
			if(key == keys[i][j])
			{
				kn = KeyWinToNumber(i, j);
				keymask[kn] = (keymask[kn]==0);
				keypushed[i][j] =(keypushed[i][j]==0);
				RedrawKey(i, j);

				if(keypushed[i][j])
				{
					Interrupt_Wakeup |= 1;

					if(Interrupts_OK && Processing_Interrupt == 0)
					{
						Interrupt_Pending |= 1;
						InterruptClass |= INTERRUPTION_KEYBOARD;
					}
				}

				return;
			}
		}
	}
}

extern unsigned long int Registre_PC;
extern void DisplayState();

int getaddr(char *txt, unsigned long int *v, char *def);

void TreatBox(box)
Window box;
{
	unsigned long int v;
	char buffer[100];

	if(box == Boxes[REGISTERS_PC])
	{
		DisplayState();
		sprintf(buffer, "%05X", Registre_PC);
		if(getaddr("Jump to address : ", &v, buffer)==0)
		{
			return;
		}
		Registre_PC=v;
		DisplayState();
		sprintf(buffer, "\n--> Jump to %05X\n", Registre_PC);
		PushTextIntoLineTextBuffer(buffer);
		return;
	}

	if(box == Boxes[REGISTERS_CYCLES])
	{
		return;
	}

	if(box == Boxes[REGISTERS_D0])
	{
		return;
	}

	if(box == Boxes[REGISTERS_D1])
	{
		return;
	}

	if(box == Boxes[REGISTERS_A])
	{
		return;
	}

	if(box == Boxes[REGISTERS_B])
	{
		return;
	}

	if(box == Boxes[REGISTERS_C])
	{
		return;
	}

	if(box == Boxes[REGISTERS_D])
	{
		return;
	}

	if(box == Boxes[REGISTERS_R0])
	{
		return;
	}

	if(box == Boxes[REGISTERS_R1])
	{
		return;
	}

	if(box == Boxes[REGISTERS_R2])
	{
		return;
	}

	if(box == Boxes[REGISTERS_R3])
	{
		return;
	}

	if(box == Boxes[REGISTERS_R4])
	{
		return;
	}

	if(box == Boxes[REGISTERS_RSTK7])
	{
		return;
	}

	if(box == Boxes[REGISTERS_RSTK6])
	{
		return;
	}

	if(box == Boxes[REGISTERS_RSTK5])
	{
		return;
	}

	if(box == Boxes[REGISTERS_RSTK4])
	{
		return;
	}

	if(box == Boxes[REGISTERS_RSTK3])
	{
		return;
	}

	if(box == Boxes[REGISTERS_RSTK2])
	{
		return;
	}

	if(box == Boxes[REGISTERS_RSTK1])
	{
		return;
	}

	if(box == Boxes[REGISTERS_RSTK0])
	{
		return;
	}

	if(box == Boxes[REGISTERS_OUTPUT])
	{
		return;
	}

	if(box == Boxes[REGISTERS_INPUT])
	{
		return;
	}

	if(box == Boxes[REGISTERS_CARRY])
	{
		return;
	}

	if(box == Boxes[REGISTERS_P])
	{
		return;
	}

	if(box == Boxes[REGISTERS_HST])
	{
		return;
	}

	if(box == Boxes[REGISTERS_STATUS])
	{
		return;
	}

	if(box == Boxes[REGISTERS_MODE])
	{
		return;
	}

	if(box == Boxes[REGISTERS_INTER])
	{
		return;
	}

	if(box == Boxes[REGISTERS_M0])
	{
		return;
	}

	if(box == Boxes[REGISTERS_M1])
	{
		return;
	}

	if(box == Boxes[REGISTERS_M2])
	{
		return;
	}

	if(box == Boxes[REGISTERS_M3])
	{
		return;
	}

	if(box == Boxes[REGISTERS_M4])
	{
		return;
	}

	if(box == Boxes[REGISTERS_M5])
	{
		return;
	}
}

extern int WhichMode;
extern int goon;
extern int breakf;
extern unsigned long int breaka;

void DisplayScreen();

void TreatButton(btn)
Window btn;
{
	if(btn == Buttons[BUTTON_STEP])
	{
		if(WhichMode == 's')
		{
			goon = 1;
		}
		else
		{
			WhichMode = 's';
			strcpy(ButtonTxt[BUTTON_STEP], "Step");
			RefreshButton(BUTTON_STEP);
		}
		return;
	}

	if(btn == Buttons[BUTTON_RUN])
	{
		if(WhichMode == 's')
		{
			WhichMode = 'r';
			strcpy(ButtonTxt[BUTTON_STEP], "Stop");
			RefreshButton(BUTTON_STEP);
		}
		return;
	}

	if(btn == Buttons[BUTTON_BREAK])
	{
		unsigned long int v;
		char buffer[100];

		if(breakf==1)
		{
			breakf = 0;
			WriteTextIntoTextWidgets(STATUS_BREAK, "");
			strcpy(ButtonTxt[BUTTON_BREAK], "Break");
			RefreshButton(BUTTON_BREAK);
			return;
		}

		if(getaddr("Set breakpoint : ", &v, "")==0)
		{
			return;
		}
		breaka = v;
		breakf = (breaka<0x100000);

		if(breakf)
		{
			sprintf(buffer, "Breakpoint at %05X", breaka);
			WriteTextIntoTextWidgets(STATUS_BREAK, buffer);
			strcpy(ButtonTxt[BUTTON_BREAK], "Clr Brk");
			RefreshButton(BUTTON_BREAK);
			return;
		}
		else
		{
			WriteTextIntoTextWidgets(STATUS_BREAK, "");
		}
		return;
	}

	if(btn == Buttons[BUTTON_UPDATE])
	{
		DisplayScreen();
		return;
	}

#ifdef WORK_IN_MEMORY
	if(btn == Buttons[BUTTON_DUMP])
	{
		WriteTextIntoTextWidgets(STATUS_MESSAGES, "Dumping files...");
		XFlush(mydisplay);
		DumpFiles();
		WriteTextIntoTextWidgets(STATUS_MESSAGES, "");
		return;
	}
#endif

	if(btn == Buttons[BUTTON_QUIT])
	{
		WhichMode = 'q';
		return;
	}

	if(btn == Buttons[BUTTON_WAKEUP])
	{
		Interrupt_Wakeup |= 1;
		return;
	}
}

extern int oldannstate;

void ClearGraphicScreen()
{
	XSetFunction(mydisplay, mygc, GXclear);
	XFillRectangle(mydisplay, GraphicScreen, mygc,
						0, 0, SCREENWIDTH, SCREENHEIGTH);
}

int ProcessXEventsIfNecessary()
{
	XEvent myevent;
	KeySym mykey;
	int i;
	char text[100];
	int chargot;
	char args[100];

	chargot = 0;

	while(chargot==0 && XCheckIfEvent(mydisplay, &myevent, ChooseAll, args))
	{
		switch(myevent.type)
		{
			case Expose:
				if(myevent.xexpose.count==0 && myevent.xexpose.window == hp48)
				{
					oldannstate = -1; DisplayAnnunciators();
					DisplayScreen();
					RedrawKeys();
				}
				else
				if(myevent.xexpose.count==0 &&
								myevent.xexpose.window == Control)
				{
					RefreshButtons();
					WriteDesassText();
					RefreshTextText();
				}
				break;

			case UnmapNotify:
				if(myevent.xunmap.window == Control)
				{
					ControlIconified = 1;
				}
				break;

			case MapNotify:
				if(myevent.xmap.window == Control)
				{
					ControlIconified = 0;
				}
				break;

			case MappingNotify:
				XRefreshKeyboardMapping(&myevent.xmapping);
				break;

			case ButtonPress:
				TreatKey(myevent.xbutton.window);
				if(ControlIconified == 0) TreatBox(myevent.xbutton.window);
				if(ControlIconified == 0) TreatButton(myevent.xbutton.window);
				break;

			case KeyPress:
				i = XLookupString(&myevent.xkey, text, 10, &mykey, 0);
				if(i==1)
				{
					chargot = text[0];
				}
				break;
		}
	}
	return chargot;
}

void CloseWindows()
{
	XFreeGC(mydisplay, mygc);
	XDestroyWindow(mydisplay, hp48);
	XCloseDisplay(mydisplay);
}
				

void PlcDrawPoint(x, y, p)
int x, y, p;
{
	if(p)
	{
		XSetFunction(mydisplay, mygc, GXset);
	}
	else
	{
		XSetFunction(mydisplay, mygc, GXclear);
	}
	XDrawPoint(mydisplay, GraphicScreen, mygc, x, y);
}
void PlcClearAnnunciators()
{
	XClearWindow(mydisplay, AnnunciatorsScreen);
}

unsigned char leftshift[8][8] =
{
	{0, 0, 0, 1, 0, 0, 0, 0},
	{0, 0, 1, 0, 0, 0, 0, 0},
	{0, 1, 1, 1, 1, 1, 1, 0},
	{0, 0, 1, 0, 0, 0, 1, 0},
	{0, 0, 0, 1, 0, 0, 1, 0},
	{0, 0, 0, 0, 0, 0, 1, 0},
	{0, 0, 0, 0, 0, 0, 1, 0},
	{0, 0, 0, 0, 0, 0, 1, 0}
};

unsigned char rightshift[8][8] =
{
	{0, 0, 0, 0, 1, 0, 0, 0}, 
	{0, 0, 0, 0, 0, 1, 0, 0}, 
	{0, 1, 1, 1, 1, 1, 1, 0}, 
	{0, 1, 0, 0, 0, 1, 0, 0}, 
	{0, 1, 0, 0, 1, 0, 0, 0}, 
	{0, 1, 0, 0, 0, 0, 0, 0}, 
	{0, 1, 0, 0, 0, 0, 0, 0}, 
	{0, 1, 0, 0, 0, 0, 0, 0} 
};

unsigned char alpha[8][8] =
{
	{0, 0, 0, 0, 0, 0, 0, 0}, 
	{0, 0, 0, 0, 0, 0, 0, 0}, 
	{0, 0, 1, 1, 0, 1, 0, 0}, 
	{0, 1, 0, 0, 1, 0, 0, 0}, 
	{0, 1, 0, 0, 1, 0, 0, 0}, 
	{0, 1, 0, 0, 1, 0, 0, 0}, 
	{0, 0, 1, 1, 0, 1, 0, 0}, 
	{0, 0, 0, 0, 0, 0, 0, 0} 
};

unsigned char warning[8][8] =
{
	{0, 1, 0, 0, 0, 1, 0, 0}, 
	{0, 1, 0, 0, 0, 1, 0, 0}, 
	{1, 0, 0, 0, 0, 0, 1, 0}, 
	{1, 0, 1, 0, 1, 0, 1, 0}, 
	{1, 0, 1, 0, 1, 0, 1, 0}, 
	{1, 0, 0, 0, 0, 0, 1, 0}, 
	{0, 1, 0, 0, 0, 1, 0, 0}, 
	{0, 1, 0, 0, 0, 1, 0, 0} 
};

unsigned char busy[8][8] =
{
	{0, 0, 0, 0, 0, 0, 0, 0}, 
	{1, 1, 1, 1, 1, 1, 1, 0}, 
	{0, 1, 0, 0, 0, 1, 0, 0}, 
	{0, 0, 1, 0, 1, 0, 0, 0}, 
	{0, 0, 0, 1, 0, 0, 0, 0}, 
	{0, 0, 1, 0, 1, 0, 0, 0}, 
	{0, 1, 1, 1, 1, 1, 0, 0}, 
	{1, 1, 1, 1, 1, 1, 1, 0} 
};

unsigned char transmitting[8][8] =
{
	{0, 0, 0, 0, 0, 0, 0, 0}, 
	{0, 0, 0, 0, 0, 0, 0, 0}, 
	{0, 1, 1, 0, 0, 1, 0, 0}, 
	{1, 0, 0, 1, 0, 1, 1, 0}, 
	{0, 1, 0, 1, 1, 1, 1, 1}, 
	{1, 0, 0, 1, 0, 1, 1, 0}, 
	{0, 1, 1, 0, 0, 1, 0, 0}, 
	{0, 0, 0, 0, 0, 0, 0, 0} 
};

XPoint leftshiftshape[100];
int    leftshiftn;

XPoint rightshiftshape[100];
int    rightshiftn;

XPoint alphashape[100];
int    alphan;

XPoint warningshape[100];
int    warningn;

XPoint busyshape[100]; 
int    busyn;

XPoint transmittingshape[100];
int    transmittingn;

int InitAnnShape(x, btm, points)
int x;
unsigned char btm[8][8];
XPoint *points;
{
	int i, j, k;

	k = 0;

	for(i=0;i< 8; ++i)
		for(j=0;j<8;++j)
		{
			if(btm[i][j]!=0)
			{
				points[k].x = j+x;
				points[k].y = i;
				++k;
			}
		}

	return k;
}
				
void InitAnnShapes()
{
	leftshiftn = InitAnnShape(5 + 0*(ANNWIDTH/6), leftshift, leftshiftshape);
	rightshiftn = InitAnnShape(5 + 1*(ANNWIDTH/6), rightshift, rightshiftshape);
	alphan = InitAnnShape(5 + 2*(ANNWIDTH/6), alpha, alphashape);
	warningn = InitAnnShape(5 + 3*(ANNWIDTH/6), warning, warningshape);
	busyn = InitAnnShape(5 + 4*(ANNWIDTH/6), busy, busyshape);
	transmittingn = InitAnnShape(5 + 5*(ANNWIDTH/6), transmitting, transmittingshape);
}

void DispIt(state, shape, n)
int state;
XPoint *shape;
int n;
{
	if(state) XSetFunction(mydisplay, mygc, GXset);
	else			 XSetFunction(mydisplay, mygc, GXclear);
	XDrawPoints(mydisplay, AnnunciatorsScreen, mygc, shape,
						n, CoordModeOrigin);
}

void PlcDisplayAnnunciators(state)
int state;
{
	DispIt(state&0x01, leftshiftshape, leftshiftn);
	DispIt(state&0x02, rightshiftshape, rightshiftn);
	DispIt(state&0x04, alphashape, alphan);
	DispIt(state&0x08, warningshape, warningn);
	DispIt(state&0x10, busyshape, busyn);
	DispIt(state&0x20, transmittingshape, transmittingn);
}
