

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   This program is an unpublished work of TRG, Inc. and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of TRG, Inc. can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  SCREEN.H
*   DESCRIP  :  Screen Code Defines for MANOS v1.0
*   DATE     :  December 23, 1997
*
*
***************************************************************************/

#include "types.h"

#define BLINK		0x80
#define	BLACK		0x00
#define BLUE		0x01
#define GREEN		0x02
#define CYAN		0x03
#define RED		0x04
#define MAGENTA		0x05
#define BROWN		0x06
#define WHITE		0x07
#define	GRAY		0x08
#define LTBLUE		0x09
#define LTGREEN		0x0A
#define LTCYAN		0x0B
#define LTRED		0x0C
#define LTMAGENTA	0x0D
#define YELLOW		0x0E
#define BRITEWHITE	0x0F

#define	BGBLACK		0x00
#define BGBLUE		0x10
#define BGGREEN		0x20
#define BGCYAN		0x30
#define BGRED		0x40
#define BGMAGENTA	0x50
#define BGBROWN		0x60
#define BGWHITE		0x70

#define UP    1
#define DOWN  0

#define NORMAL_CRLF        0
#define SUPRESS_CRLF       1

#define SWITCHABLE_SCREEN      0
#define NON_SWITCHABLE_SCREEN  1

#define SCREEN_SIGNATURE       0xFEEDFEED

typedef struct _ACCELERATOR
{
    struct _ACCELERATOR *accelNext;
    struct _ACCELERATOR *accelPrior;
    LONG (*accelRoutine)(struct _SCREEN *screen, LONG key, void *p, struct _ACCELERATOR *parser);
    LONG (*accelRoutineHelp)(struct _SCREEN *screen, LONG key, struct _ACCELERATOR *parser);
    LONG accelFlags;
    LONG key;
    LONG supervisorCommand;
    BYTE *shortHelp;
    void *moduleContext;
} ACCELERATOR;

typedef struct _SCREEN {
   BYTE *pVidMem;	   /* pointer to crnt video buffer */
   BYTE *pVirtVid;	   /* Virtual Video Buffer Address */
   LONG CrntX;		   /* Current cursor position */
   LONG CrntY;
   LONG nCols;		   /* Virtual Screen Size */
   LONG nLines;
   LONG VidMode;	   /* 0x00 = 80x25 VGA color text */
   LONG NormVid;	   /* 0x07 = WhiteOnBlack */
   LONG RevVid;	           /* 0x71 = RevWhiteOnBlack */
   LONG CursOn;		   /* FLAG 1 = Cursor is visible */
   LONG CursType;	   /* FLAG	(0=UL, 1 = Block) */
   LONG ScrlCnt;           /* Count since last pause  */
   LONG CurrCnt;           /* Count since last pause  */
   LONG Pause;	           /* Full screen pause (Text mode) */
   LONG typeAheadCount;    // number of buffered keys
   LONG typeAheadWriter;
   LONG typeAheadReader;
   LONG commandIndex;      // index into history buffer
   LONG kbuffer[16];       // current keyboard buffer
   BYTE commandRecall[16][80];     // keyboard history buffer
   struct _SCREEN *next;
   struct _SCREEN *prior;
   BYTE screenName[80];
   LONG screen_mutex;
   LONG screenType;
   LONG screenOwner;
   LONG screenSignature;
   BYTE titleBarBuffer[160];
   LONG titleBarActive;
   LONG TabSize;
   LONG CR_or_LF_Flag;
   BYTE *screenHistory;
   LONG screenHistorySize;
   LONG accelLock;
   ACCELERATOR *accelHead;
   ACCELERATOR *accelTail;
   LONG suppressAccelerators;
   sema_t screen_sema;
} SCREEN;

extern void ClrScn(SCREEN *screen);
extern void InitializeVideo(void *);
extern void ClearScreen(SCREEN *screen);
extern void ScrollScreen(SCREEN *screen);
extern SCREEN *GetVidOwner(void);
extern LONG SetNormVid(LONG attr);
extern LONG GetNormVid(void);
extern LONG PopUpScreen(SCREEN *screen);
extern LONG SetVideoOwner(SCREEN *screen);
extern SCREEN *SetKeyboardOwner(SCREEN *screen);
extern void SetXY(SCREEN *screen, LONG x, LONG y);
extern void GetXY(SCREEN *screen, LONG *x, LONG *y);
extern void PutVidString(SCREEN *screen, BYTE *s, LONG x, LONG y, LONG attr);
extern void PutVidStringTransparent(SCREEN *screen, BYTE *s, LONG x, LONG y, LONG attr);
extern void PutVidStringCleol(SCREEN *screen, BYTE *s, LONG line, LONG attr);
extern void PutVidStringToLength(SCREEN *screen, BYTE *s, LONG x, LONG y,
				 LONG attr, LONG len);
extern void PutVidChar(SCREEN *screen, BYTE c, LONG x, LONG y, LONG attr);
extern BYTE GetVidChar(SCREEN *screen, LONG x, LONG y);
extern BYTE GetVidCharAttr(SCREEN *screen, LONG x, LONG y);
extern void PutVidCharCleol(SCREEN *screen, BYTE c, LONG line, LONG attr);
extern LONG Pause(void);
extern void ClearPauseMode(SCREEN *screen);
extern void SetPauseMode(SCREEN *screen, LONG lines);
extern LONG deleteScreen(SCREEN *screen);
extern SCREEN *createScreen(BYTE *name);
extern long ScreenTTYOut(SCREEN *screen, BYTE *buf, LONG len, LONG attr);
extern LONG ScreenInputFromKeyboard(SCREEN *activeScreen, BYTE *buf, LONG current_len, LONG max_len, LONG attr);
extern LONG ReadKeyboard(SCREEN *screen);
extern LONG CheckKey(SCREEN *screen);
extern void DisableCursor(SCREEN *screen);
extern void EnableCursor(SCREEN *screen);
extern LONG ScrollDisplay(SCREEN *screen, LONG x, LONG y,
			  LONG cols, LONG lines, LONG up);
extern LONG GetActiveScreenMode(void);
extern LONG GetActiveScreenTextSize(void);
extern LONG convertScreenMode(SCREEN *screen, LONG mode);

extern SCREEN *activeScreen;
extern SCREEN *keyboardOwner;
extern SCREEN *oldActiveScreen;
extern SCREEN *sourceScreen;
extern SCREEN *debugScreen;
extern SCREEN *consoleScreen;
extern SCREEN *screenListHead;
extern SCREEN *screenListTail;
extern BYTE *VGATextBase;

extern LONG AccelRoutine(SCREEN *screen, LONG key, void *p);
extern LONG IsAccelerator(SCREEN *screen, LONG key);
extern LONG AccelHelpRoutine(SCREEN *screen, LONG key);
extern LONG AddAccelRoutine(SCREEN *screen, ACCELERATOR *newAccel);
extern LONG RemoveAccelRoutine(SCREEN *screen, ACCELERATOR *newAccel);
extern LONG disableAccelerators(SCREEN *screen);
extern LONG enableAccelerators(SCREEN *screen);
extern long ConsolePrintf(char *format, ...);
