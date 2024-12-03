#include <X11/Xlib.h>
#include <X11/Xutil.h>

extern void CreateWindows();
extern int ProcessXEventsIfNecessary();
extern void CloseWindows();
extern void PlcDrawPoint();
extern void ClearGraphicScreen();
extern void PlcClearAnnunciators();
extern void PlcDisplayAnnunciators();
extern void WriteTextIntoTextWidgets();

#define STATUSLINES	8

#define STATUS_MESSAGES	0
#define	STATUS_BREAK	1
#define STATUS_INTR		2
#define STATUS_OBJECT	3
#define STATUS_ADDRESS	4
#define STATUS_DUMP		5

#define REGISTERS_FIRST	STATUSLINES

#define REGISTERS_PC		REGISTERS_FIRST

#define REGISTERS_CYCLES	(REGISTERS_PC+2)

#define REGISTERS_D0		(REGISTERS_CYCLES+2)
#define REGISTERS_D1		(REGISTERS_D0+1)

#define REGISTERS_A			(REGISTERS_D1+2)
#define REGISTERS_B			(REGISTERS_A+1)
#define REGISTERS_C			(REGISTERS_B+1)
#define REGISTERS_D			(REGISTERS_C+1)

#define REGISTERS_R0		(REGISTERS_D+2)
#define REGISTERS_R1		(REGISTERS_R0+1)
#define REGISTERS_R2		(REGISTERS_R1+1)
#define REGISTERS_R3		(REGISTERS_R2+1)
#define REGISTERS_R4		(REGISTERS_R3+1)

#define REGISTERS_RSTK7		(REGISTERS_R4+2)
#define REGISTERS_RSTK6		(REGISTERS_RSTK7+1)
#define REGISTERS_RSTK5		(REGISTERS_RSTK6+1)
#define REGISTERS_RSTK4		(REGISTERS_RSTK5+1)
#define REGISTERS_RSTK3		(REGISTERS_RSTK4+1)
#define REGISTERS_RSTK2		(REGISTERS_RSTK3+1)
#define REGISTERS_RSTK1		(REGISTERS_RSTK2+1)
#define REGISTERS_RSTK0		(REGISTERS_RSTK1+1)

#define REGISTERS_OUTPUT	(REGISTERS_RSTK0+2)
#define REGISTERS_INPUT		(REGISTERS_OUTPUT+1)

#define REGISTERS_CARRY		(REGISTERS_INPUT+2)
#define REGISTERS_P			(REGISTERS_CARRY+1)
#define REGISTERS_HST		(REGISTERS_P+1)
#define REGISTERS_STATUS	(REGISTERS_HST+1)

#define REGISTERS_MODE		(REGISTERS_STATUS+2)
#define REGISTERS_INTER		(REGISTERS_MODE+1)

#define REGISTERS_M0		(REGISTERS_INTER+2)
#define REGISTERS_M1		(REGISTERS_M0+1)
#define REGISTERS_M2		(REGISTERS_M1+1)
#define REGISTERS_M3		(REGISTERS_M2+1)
#define REGISTERS_M4		(REGISTERS_M3+1)
#define REGISTERS_M5		(REGISTERS_M4+1)

#define REGISTERS_LASTLINE	REGISTERS_M5

#define CONTROL_NLINES		(REGISTERS_LASTLINE + 1)

#define BUTTON_STEP     0
#define BUTTON_RUN      1
#define BUTTON_BREAK    2
#define BUTTON_UPDATE   3
#define BUTTON_QUIT     4
#define BUTTON_WAKEUP	5
#ifdef WORK_IN_MEMORY
#define BUTTON_DUMP     6
#endif

#define INTERRUPTION_TIMER1		0x1
#define INTERRUPTION_TIMER2		0x2
#define INTERRUPTION_KEYBOARD	0x4

#define INTERRUPT_HANDLER	0x0000f