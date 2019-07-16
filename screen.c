

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  SCREEN.C
*   DESCRIP  :  Screen Graphics Code for MANOS v1.0
*   DATE     :  November 9, 1997
*
*
***************************************************************************/

#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "types.h"
#include "kernel.h"
#include "keyboard.h"
#include "screen.h"
#include "os.h"
#include "emit.h"
#include "malloc.h"
#include "free.h"
#include "dos.h"
#include "dosfile.h"
#include "event.h"

#define DEBUGGING_SLD  0

#define  LINE_MODE_25  0x01
#define  LINE_MODE_43  0x02
#define  LINE_MODE_50  0x04
#define  LINE_MODE_28  0x08
#define  LINE_MODE_40  0x10
#define  LINE_MODE_35  0x20

extern void *AllocateMemoryBelow1MB(LONG);
extern void FreeMemoryBelow1MB(void *);

BYTE screensName[]={ "Active Screens" };
BYTE sourceName[]={ "SMP Source Debugger" };
BYTE debugName[]={ "SMP Debugger Console" };
BYTE consoleName[]={ "System Console" };

BYTE screensVideoMemory[80 * 50 * 2];
BYTE SourceVideoMemory[80 * 50 * 2];
BYTE DebugVideoMemory[80 * 50 * 2];
BYTE ConsoleVideoMemory[80 * 50 * 2];

SCREEN systemConsole = {
   (BYTE *) 0x000B8000,
   (BYTE *) &ConsoleVideoMemory[0],
   0,
   0,
   80,
   25,
   LINE_MODE_25,
   7,
   0x71,
   1,
   0,
   0,
   24,
   0,
   0,
};

SCREEN debugConsole = {
   (BYTE *) &DebugVideoMemory[0],  // this screen starts dormant
   (BYTE *) &DebugVideoMemory[0],
   0,
   0,
   80,
   25,
   LINE_MODE_25,
   7,
   0x71,
   1,
   0,
   0,
   24,
   0,
   0,
};

SCREEN sourceConsole = {
   (BYTE *) &SourceVideoMemory[0],  // this screen starts dormant
   (BYTE *) &SourceVideoMemory[0],
   0,
   0,
   80,
   25,
   LINE_MODE_25,
   7,
   0x71,
   1,
   0,
   0,
   24,
   0,
   0,
};

SCREEN screenScreensStruct = {
   (BYTE *) &screensVideoMemory[0],  // this screen starts dormant
   (BYTE *) &screensVideoMemory[0],
   0,
   0,
   80,
   25,
   LINE_MODE_25,
   7,
   0x71,
   1,
   0,
   0,
   24,
   0,
   0,
};

SCREEN *activeScreen = (SCREEN *) &systemConsole;
SCREEN *keyboardOwner = (SCREEN *) &systemConsole;
SCREEN *oldActiveScreen;
SCREEN *sourceScreen = (SCREEN *) &sourceConsole;
SCREEN *debugScreen = (SCREEN *) &debugConsole;
SCREEN *consoleScreen = (SCREEN *) &systemConsole;
SCREEN *screenScreens = (SCREEN *) &screenScreensStruct;

SCREEN *screenListHead = 0;
SCREEN *screenListTail = 0;

BYTE *VGATextBase = (BYTE *) 0x000B8000;
LONG screen_list_mutex;
LONG changeScreenMutex;
LONG TitleBarFlag = 0;
LONG currentScreenMode = LINE_MODE_25;  // MS-DOS default is 25 lines
LONG DisplayAdapter = 0; // assume lowest possible setting
LONG DisplayHardwareLock = 0;

extern void CopyData(LONG *src, LONG *dest, LONG len);
extern void SetData(LONG *dest, LONG value, LONG len);
extern void HardXY(LONG x, LONG y);
extern void InitVideo(void);
extern LONG PollKeyboard(void);
extern LONG ReadKeyboardFinal(void);
extern void MaskKeyboard(void);
extern void UnmaskKeyboard(void);
extern void KeyboardEnterDebugger(void);
extern LONG debuggerActive;

void ClearScreen(SCREEN *screen);
void ClrScn(SCREEN *screen);
void InitializeVideo(void *dos);
SCREEN *GetVidOwner(void);
LONG SetNormVid(LONG attr);
LONG GetNormVid(void);
LONG SetVideoOwner(SCREEN *screen);
void SetXY(SCREEN *screen, LONG x, LONG y);
void GetXY(SCREEN *screen, LONG *x, LONG *y);
long TTYOut(BYTE *buf, LONG len, LONG attr);
void PutChar(SCREEN *screen, BYTE ch, LONG x, LONG y, LONG attr);
void GetChar(SCREEN *screen, BYTE *ch, LONG x, LONG y);
void ScrollVid(SCREEN *screen);
void nextScreen(void);

LWP ListScreensLWP;
LONG ListScreensLock = 0;
LONG SystemDefaultScreenLines = 25;
LONG SystemDefaultScreenLineMode = LINE_MODE_25;

void InitializeVideo(void *dosTable)
{

    extern void SetScreenLines(LONG nLines);
    extern LONG MajorVersion;
    extern LONG MinorVersion;
    extern LONG BuildVersion;
    register int i;
    BYTE *p;
    register DOS_TABLE *dos = dosTable;

    // if a display code was available, set screen size based on
    // detected monitor and adapter types.  initialize 6845 cursor
    // as well.

    switch (dos->DISPLAY_CODE)
    {
       case 0x04:   // EGA Card with Color Monitor
       case 0x05:   // EGA Card with Mono Monitor
	  InitVideo();
	  DisplayAdapter = 1;
	  SystemDefaultScreenLines = 43;
	  SystemDefaultScreenLineMode = LINE_MODE_43;
	  SetScreenLines(43);
	  break;

       case 0x06:   // VGA Card with Color Monitor
       case 0x07:   // VGA Card with Mono Analog Monitor
       case 0x08:   // VGA Card with Color Analog Monitor
       case 0x0A:   // MCGA Card with digital Color Monitor
       case 0x0B:   // MCGA Card with analog Mono Monitor
       case 0x0C:   // MCGA Card with analog Color Monitor
	  InitVideo();
	  DisplayAdapter = 2;
#if DEBUGGING_SLD
	  SystemDefaultScreenLines = 25;
	  SystemDefaultScreenLineMode = LINE_MODE_25;
	  SetScreenLines(25);
#else
	  SystemDefaultScreenLines = 50;
	  SystemDefaultScreenLineMode = LINE_MODE_50;
	  SetScreenLines(50);
#endif
	  break;

       case 0x00:   // No display detected
       case 0x01:   // MDA Card with Monochrome Display
       case 0x02:   // CGA Card with Color Display
       case 0x03:   // reserved
       case 0x09:   // reserved
       case 0xFF:   // video adapter unknown
       default:
	  InitVideo();
	  DisplayAdapter = 0;
	  SystemDefaultScreenLines = 25;
	  SystemDefaultScreenLineMode = LINE_MODE_25;
	  SetScreenLines(25);
	  break;
    }

    consoleScreen->nLines = SystemDefaultScreenLines;
    consoleScreen->VidMode = SystemDefaultScreenLineMode;

    debugScreen->nLines = SystemDefaultScreenLines;
    debugScreen->VidMode = SystemDefaultScreenLineMode;

    sourceScreen->nLines = SystemDefaultScreenLines;
    sourceScreen->VidMode = SystemDefaultScreenLineMode;

    screenScreens->nLines = SystemDefaultScreenLines;
    screenScreens->VidMode = SystemDefaultScreenLineMode;


    ClearScreen(consoleScreen);
    ClearScreen(debugScreen);
    ClearScreen(sourceScreen);
    ClearScreen(screenScreens);

    for (i=0; i < 16; i++)
    {
       sourceScreen->commandRecall[i][0] = '\0';
       debugScreen->commandRecall[i][0] = '\0';
       consoleScreen->commandRecall[i][0] = '\0';
       screenScreens->commandRecall[i][0] = '\0';

       consoleScreen->kbuffer[i] = 0;
       debugScreen->kbuffer[i] = 0;
       sourceScreen->kbuffer[i] = 0;
       screenScreens->kbuffer[i] = 0;
    }

    consoleScreen->typeAheadCount = 0;
    debugScreen->typeAheadCount = 0;
    sourceScreen->typeAheadCount = 0;
    screenScreens->typeAheadCount = 0;

    consoleScreen->commandIndex = 0;
    debugScreen->commandIndex = 0;
    sourceScreen->commandIndex = 0;
    screenScreens->commandIndex = 0;

    consoleScreen->screen_mutex = 0;
    debugScreen->screen_mutex = 0;
    sourceScreen->screen_mutex = 0;
    screenScreens->screen_mutex = 0;

    sema_init(&consoleScreen->screen_sema, 0);
    sema_init(&debugScreen->screen_sema, 0);
    sema_init(&sourceScreen->screen_sema, 0);
    sema_init(&screenScreens->screen_sema, 0);

    consoleScreen->screenOwner = 0;
    debugScreen->screenOwner = 0;
    sourceScreen->screenOwner = 0;
    screenScreens->screenOwner = 0;
    consoleScreen->screenSignature = SCREEN_SIGNATURE;
    debugScreen->screenSignature = SCREEN_SIGNATURE;
    sourceScreen->screenSignature = SCREEN_SIGNATURE;
    screenScreens->screenSignature = SCREEN_SIGNATURE;

    p = consoleName;
    i = 0;
    while (*p)
       consoleScreen->screenName[i++] = *p++;
    consoleScreen->screenName[i] = '\0';

    p = debugName;
    i = 0;
    while (*p)
       debugScreen->screenName[i++] = *p++;
    debugScreen->screenName[i] = '\0';

    p = sourceName;
    i = 0;
    while (*p)
       sourceScreen->screenName[i++] = *p++;
    sourceScreen->screenName[i] = '\0';

    p = screensName;
    i = 0;
    while (*p)
       screenScreens->screenName[i++] = *p++;
    screenScreens->screenName[i] = '\0';

    screenListHead = consoleScreen;
    screenListTail = sourceScreen;
    screen_list_mutex = 0;
    changeScreenMutex = 0;

    consoleScreen->TabSize = 8;
    debugScreen->TabSize = 8;
    sourceScreen->TabSize = 8;
    screenScreens->TabSize = 8;

    consoleScreen->next = screenScreens;
    consoleScreen->prior = 0;
    consoleScreen->screenType = SWITCHABLE_SCREEN;

    screenScreens->next = debugScreen;
    screenScreens->prior = consoleScreen;
    screenScreens->screenType = NON_SWITCHABLE_SCREEN;

    debugScreen->next = sourceScreen;
    debugScreen->prior = screenScreens;
    debugScreen->screenType = NON_SWITCHABLE_SCREEN;

    sourceScreen->next = 0;
    sourceScreen->prior = debugScreen;
    sourceScreen->screenType = NON_SWITCHABLE_SCREEN;

    activeScreen = debugScreen;
    printfWithAttribute(LTCYAN, "Metropolitan Area Network Operating System SMP Debugger\n");
    printfWithAttribute(LTCYAN, "v%02d.%02d.%02d\n", MajorVersion, MinorVersion, BuildVersion);
    printfWithAttribute(LTCYAN, "Copyright (C) 1997, 1998 Timpanogas Research Group, Inc.\n");
    printfWithAttribute(LTCYAN, "All Rights Reserved.\n");

    activeScreen = consoleScreen;

    DisableCursor(sourceScreen);

    return;

}

void DisplayScreen(SCREEN *debugScreen, SCREEN *screen)
{

   register ACCELERATOR *accel;
   register int i;

   SetPauseMode(debugScreen, debugScreen->nLines - 3);

   printfScreenWithAttribute(debugScreen, BRITEWHITE, "Screen Structure at 0x%08X\n", screen);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->pVidMem              =  %08X\n", screen->pVidMem);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->pVirtVid             =  %08X\n", screen->pVirtVid);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->CrntX                =  %08X\n", screen->CrntX);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->CrntY                =  %08X\n", screen->CrntY);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->nCols                =  %08X\n", screen->nCols);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->nLines               =  %08X\n", screen->nLines);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->VidMode              =  %08X\n", screen->VidMode);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->NormVid              =  %08X\n", screen->NormVid);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->RevVid               =  %08X\n", screen->RevVid);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->CursOn               =  %08X\n", screen->CursOn);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->CursType             =  %08X\n", screen->CursType);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->ScrlCnt              =  %08X\n", screen->ScrlCnt);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->Pause                =  %08X\n", screen->Pause);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->typeAheadCount       =  %08X\n", screen->typeAheadCount);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->cmdIndex             =  %08X\n", screen->commandIndex);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->next                 =  %08X\n", screen->next);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->prior                =  %08X\n", screen->prior);

   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->screenName           =  %08X\n", screen->screenName);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->screen_mutex         =  %08X\n", screen->screen_mutex);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->screenType           =  %08X\n", screen->screenType);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->screenOwner          =  %08X\n", screen->screenOwner);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->screenSignature      =  %08X\n", screen->screenSignature);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->titleBarBuffer       =  %08X\n", screen->titleBarBuffer);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->titleBarActive       =  %08X\n", screen->titleBarActive);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->TabSize              =  %08X\n", screen->TabSize);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->CR_or_LF_Flag        =  %08X\n", screen->CR_or_LF_Flag);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->screenHistory        =  %08X\n", screen->screenHistory);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->screenHistorySize    =  %08X\n", screen->screenHistorySize);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->accelLock            =  %08X\n", screen->accelLock);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->accelHead            =  %08X\n", screen->accelHead);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->accelTail            =  %08X\n", screen->accelTail);
   printfScreenWithAttribute(debugScreen, LTGREEN, "screen->suppressAccelerators =  %08X\n", screen->suppressAccelerators);

   printfScreenWithAttribute(debugScreen, BRITEWHITE, "Screen Keyboard Buffer\n");
   for (i=0; i < 16; i++)
      printfScreenWithAttribute(debugScreen, LTGREEN, "screen->kbuffer[%i] = %08X\n",
			  i, screen->kbuffer[i]);

   printfScreenWithAttribute(debugScreen, BRITEWHITE, "Screen Command Recall Buffer\n");
   for (i=0; i < 16; i++)
      printfScreenWithAttribute(debugScreen, LTGREEN, "screen->commandRecall[%i][0] = %s\n",
			  i, (BYTE *) &screen->commandRecall[i][0]);

   printfScreenWithAttribute(debugScreen, BRITEWHITE, "Screen Accelerators\n");

   accel = screen->accelHead;
   while (accel)
   {
      printfScreenWithAttribute(debugScreen, LTGREEN,
	    "%08X:  %08X    %s\n", accel, accel->key,
	    accel->shortHelp);
      accel = accel->accelNext;
   }

   ClearPauseMode(debugScreen);
   return;

}

void ChangeSystemScreenSize(LONG nLines)
{
    register SCREEN *screen = activeScreen;
    register LONG flags;
    register SCREEN *displayScreen;
    extern void SetScreenLines(LONG);

    switch (nLines)
    {
       case 25:
	  SystemDefaultScreenLineMode = LINE_MODE_25;
	  break;

       case 43:
	  SystemDefaultScreenLineMode = LINE_MODE_43;
	  break;

       case 50:
	  SystemDefaultScreenLineMode = LINE_MODE_50;
	  break;

       default:
	  return;
    }
    SystemDefaultScreenLines = nLines;

    flags = get_flags();
    spin_lock(&screen_list_mutex);
    displayScreen = screenListHead;
    while (displayScreen)
    {
       displayScreen->nLines = SystemDefaultScreenLines;
       displayScreen->VidMode = SystemDefaultScreenLineMode;
       if (displayScreen->CrntY > nLines)
	  displayScreen->CrntY = displayScreen->CrntX = 0;
       displayScreen = displayScreen->next;
    }
    spin_unlock(&screen_list_mutex);

    SetScreenLines(SystemDefaultScreenLines);

    EventNotify(EVENT_RESIZE_SCREEN, nLines);

    spin_lock(&DisplayHardwareLock);

    if (!screen->CursOn)
       HardXY(100, 100);
    else
       HardXY(screen->CrntX, screen->CrntY);

    spin_unlock(&DisplayHardwareLock);
    set_flags(flags);

    return;

}

LONG GetActiveScreenMode(void)
{
   register SCREEN *screen = activeScreen;

   return (screen->VidMode);
}

LONG GetActiveScreenTextSize(void)
{
   register SCREEN *screen = activeScreen;

   return (screen->nLines);
}

void titleBarDisplay(SCREEN *screen)
{

    if (screen->screenSignature != SCREEN_SIGNATURE)
       return;

    if (!screen->titleBarActive)
    {
       screen->titleBarActive = 1;
       CopyData((LONG *)screen->pVidMem,
		(LONG *)&screen->titleBarBuffer,
		(screen->nCols * 2));
       PutVidStringCleol(screen, " %s ", 0, LTBLUE | BGWHITE);
    }
    else
    {
       screen->titleBarActive = 0;
       CopyData((LONG *)&screen->titleBarBuffer,
		(LONG *)screen->pVidMem,
		(screen->nCols * 2));
    }

}

/*****************************************************************

// THESE FUNCTIONS ARE NOW IN ASSEMLBER AND REMAIN FOR PORTABILITY
// DO NOT DELETE

void CopyData(LONG *src, LONG *dest, LONG len)
{
   register int i;

   for (i=0; i < ((len + 3) / 4); i++)
      *dest++ = *src++;
   return;
}

void SetData(LONG *dest, LONG value, LONG len)
{
   register int i;

   for (i=0; i < ((len + 3) / 4); i++)
      *dest++ = value;
   return;

}

***************************************************************/


void ClrScn(SCREEN *screen)
{

   register LONG fill;
   register BYTE ch;

   ch = (BYTE)(screen->NormVid & 0xFF);
   fill = 0x00200020;  //  ' ' space
   fill = fill | (LONG)(ch << 24);
   fill = fill | (LONG)(ch << 8);
   SetDataD((LONG *)screen->pVidMem, fill, (screen->nCols * screen->nLines * 2) / 4);
   return;
}


LONG deleteScreen(SCREEN *screen)
{

    register SCREEN *search;
    register PROCESS *p;
    register LONG flags;

    if (screen->screenSignature != SCREEN_SIGNATURE)
       return -1;

    if (screen == consoleScreen || screen == debugScreen)
       return -1;

    SetVideoOwner(consoleScreen);

    // free all processes blocked on screen, and signal screen invalid

    p->keyBuffer = -1;
    p->typeAheadCount++;

    //
    //

    flags = get_flags();
    spin_lock(&screen_list_mutex);
    search = screenListHead;
    while (search)
    {
       if (search == screen)
       {
	  if (screenListHead == screen)
	  {
	     screenListHead = (void *) screen->next;
	     if (screenListHead)
		screenListHead->prior = NULL;
	     else
		screenListTail = NULL;
	  }
	  else
	  {
	     screen->prior->next = screen->next;
	     if (screen != screenListTail)
		screen->next->prior = screen->prior;
	     else
		screenListTail = screen->prior;
	  }
       }
       search = search->next;
    }
    spin_unlock(&screen_list_mutex);
    set_flags(flags);

    kfree(screen->pVirtVid);
    kfree(screen);

    return 0;

}

SCREEN *createScreen(BYTE *name)
{

    register LONG flags;
    register int i;
    register SCREEN *screen;
    register BYTE *p;

    screen = kmalloc(sizeof(SCREEN));
    if (!screen)
       return (SCREEN *) 0;

    for (i=0; i < 16; i++)
    {
       screen->commandRecall[i][0] = '\0';
       screen->kbuffer[i] = 0;
    }

    screen->pVirtVid = kmalloc(80 * 50 * 2);
    if (!screen->pVirtVid)
    {
       kfree(screen);
       return 0;
    }

    screen->pVidMem = screen->pVirtVid;
    screen->CrntX = 0;
    screen->CrntY = 0;
    screen->nCols = 80;
    screen->nLines = SystemDefaultScreenLines;
    screen->VidMode = 0;
    screen->NormVid = 7;
    screen->RevVid = 0x71;
    screen->CursOn = 1;
    screen->CursType = 0;
    screen->ScrlCnt = 0;
    screen->CurrCnt = 24;

    screen->typeAheadCount = 0;
    screen->commandIndex = 0;
    screen->screen_mutex = 0;
    screen->screenType = SWITCHABLE_SCREEN;

    screen->screenOwner = 0;
    screen->screenSignature = SCREEN_SIGNATURE;
    screen->TabSize = 8;

    sema_init(&screen->screen_sema, 0);

    p = name;
    i = 0;
    while (*p)
       screen->screenName[i++] = *p++;
    screen->screenName[i] = '\0';

    ClearScreen(screen);

    flags = get_flags();
    spin_lock(&screen_list_mutex);
    if (!screenListHead)
    {
       screenListHead = screen;
       screenListTail = screen;
       screen->next = screen->prior = 0;
    }
    else
    {
       screenListTail->next = screen;
       screen->next = 0;
       screen->prior = screenListTail;
       screenListTail = screen;
    }
    spin_unlock(&screen_list_mutex);
    set_flags(flags);

    return screen;

}

SCREEN *getNextScreen(SCREEN *root)
{

   if (!root->next)
      return screenListHead;
   else
      return root->next;

}

void ListScreens(SCREEN *oldScreen)
{

    register LONG i, key, count;
    register SCREEN *displayScreen;
    BYTE screenNumber[20];
    extern LONG atol(char *);
    register LONG number;

    printfScreen(screenScreens, " Current Active Screens:\n");
    printfScreen(screenScreens, "\n");

    count = 0;
    displayScreen = screenListHead;
    for (i=0; displayScreen; )
    {
       if (displayScreen->screenType == SWITCHABLE_SCREEN)
       {
	  printfScreen(screenScreens, "    %i.  %s\n", i, displayScreen->screenName);
	  i++;
	  count++;
	  if (count > 18)
	  {
	     count = 0;
	     printfScreen(screenScreens, "\nEnter Screen Number To Select (Esc to Exit): ");
	     key = ScreenInputFromKeyboard(screenScreens, screenNumber, 0, 10, 7);
	     if ((key & 0xFF) == ESC)
	     {
		SetVideoOwner(oldScreen);
		ListScreensLock = 0;
		return;
	     }
	     if (!screenNumber[0])
	     {
		ClearScreen(screenScreens);

		printfScreen(screenScreens, " Current Active Screens:\n");
		printfScreen(screenScreens, "\n");

		displayScreen = displayScreen->next;
		continue;
	     }
	     number = atol(screenNumber);
	     if (number <= i)
	     {
		displayScreen = screenListHead;
		for (i=0; displayScreen; )
		{
		   if (displayScreen->screenType == SWITCHABLE_SCREEN)
		   {
		      if (i++ == number)
		      {
			 SetVideoOwner(displayScreen);
			 ListScreensLock = 0;
			 return;
		      }
		   }
		   displayScreen = displayScreen->next;
		}
	     }
	  }
       }
       displayScreen = displayScreen->next;
    }

    printfScreen(screenScreens, "\nEnter Screen Number To Select (Esc to Exit): ");
    ScreenInputFromKeyboard(screenScreens, screenNumber, 0, 10, 7);
    number = atol(screenNumber);
    displayScreen = screenListHead;
    for (i=0; displayScreen; )
    {
       if (displayScreen->screenType == SWITCHABLE_SCREEN)
       {
	  if (i++ == number)
	  {
	     SetVideoOwner(displayScreen);
	     ListScreensLock = 0;
	     return;
	  }
       }
       displayScreen = displayScreen->next;
    }

    SetVideoOwner(oldScreen);
    ListScreensLock = 0;
    return;

}

void SwitchScreens(void)
{

    SCREEN *screen = activeScreen;

    while (1)
    {
       screen = getNextScreen(screen);
       if (screen->screenType != NON_SWITCHABLE_SCREEN)
       {
	  SetVideoOwner(screen);
	  return;
       }
    }
}

void ClearScreen(SCREEN *screen)
{
     ClrScn(screen);
     SetXY(screen, 0, 0);
     return;
}

void ClearPauseMode(SCREEN *screen)
{
    if (screen->screenSignature != SCREEN_SIGNATURE)
       return;

    if (screen->Pause)
    {
       screen->Pause = 0;
       screen->CurrCnt = 0;
       screen->ScrlCnt = 0;
    }
    return;
}

void SetPauseMode(SCREEN *screen, LONG lines)
{
    if (screen->screenSignature != SCREEN_SIGNATURE)
       return;

    if (!screen->Pause)
    {
       screen->Pause = 1;
       screen->CurrCnt = 0;
       if (lines > screen->nLines)
	  screen->ScrlCnt = screen->nLines;
       else
	  screen->ScrlCnt = lines;
    }
    return;

}

void DisableCursor(SCREEN *screen)
{
     register SCREEN *aScreen = activeScreen;
     register LONG flags;

     screen->CursOn = 0;
     if (screen == aScreen)
     {
	flags = get_flags();
	spin_lock(&DisplayHardwareLock);
	HardXY(100, 100);
	spin_unlock(&DisplayHardwareLock);
	set_flags(flags);
     }
}

void EnableCursor(SCREEN *screen)
{
     register SCREEN *aScreen = activeScreen;
     register LONG flags;

     screen->CursOn = 1;
     if (screen == aScreen)
     {
	flags = get_flags();
	spin_lock(&DisplayHardwareLock);
	HardXY(screen->CrntX, screen->CrntY);
	spin_unlock(&DisplayHardwareLock);
	set_flags(flags);
     }
}

void PutChar(SCREEN *screen, BYTE ch, LONG x, LONG y, LONG attr)
{

   register BYTE *v;

   if (screen->screenSignature != SCREEN_SIGNATURE)
      return;

   v = screen->pVidMem;
   v += (y * (screen->nCols * 2)) + x * 2;
   *v++ = ch;
   *v = attr;
   return;

}

void GetChar(SCREEN *screen, BYTE *ch, LONG x, LONG y)
{

   register BYTE *v;

    if (screen->screenSignature != SCREEN_SIGNATURE)
       return;

   v = screen->pVidMem;
   v += (y * (screen->nCols * 2)) + x * 2;
   *ch = *v;
   return;

}

void ScrollScreen(SCREEN *screen)
{

    register LONG fill;
    register BYTE ch;
    register int i;
    register BYTE *v;

    if (screen->screenSignature != SCREEN_SIGNATURE)
       return;

    v = (BYTE *)((LONG)screen->pVidMem + (LONG) (screen->nCols * 2));
    CopyDataD((LONG *)v, (LONG *)screen->pVidMem, ((screen->nCols * screen->nLines * 2) - (screen->nCols * 2)) / 4);
    v = (BYTE *)((LONG)screen->pVidMem + (LONG) ((screen->nCols * screen->nLines * 2) - (screen->nCols * 2)));

    ch = (BYTE)(screen->NormVid & 0xFF);
    fill = 0x00200020;  // ' ' space
    fill = fill | (LONG)(ch << 24);
    fill = fill | (LONG)(ch << 8);
    SetDataD((LONG *)v, fill, (screen->nCols * 2) / 4);

}

void MoveVidString(SCREEN *screen,
		   LONG src_x, LONG src_y,
		   LONG dest_x, LONG dest_y,
		   LONG length)
{

    register LONG i;
    register BYTE *src_v, *dest_v;

    if (screen->screenSignature != SCREEN_SIGNATURE)
	return;

    src_v = screen->pVidMem;
    src_v += (src_y * (screen->nCols * 2)) + src_x * 2;

    dest_v = screen->pVidMem;
    dest_v += (dest_y * (screen->nCols * 2)) + dest_x * 2;

    for (i=0; i < length; i++)
    {
       *dest_v++ = *src_v++;
       *dest_v++ = *src_v++;
    }

}

LONG ScrollDisplay(SCREEN *screen, LONG x, LONG y,
		   LONG cols, LONG lines, LONG up)
{

    register LONG i, tx, ty;

    if (screen->screenSignature != SCREEN_SIGNATURE)
       return -1;

    if (!cols || !lines)
       return -1;

    if (x > screen->nCols - 1)
       return -1;

    if (cols > screen->nCols)
       return -1;

    if (y > screen->nLines - 1)
       return -1;

    if (lines > screen->nLines)
       return -1;

    if (up)
    {
       tx = x;
       ty = y;
       for (i=1; i < lines; i++)
       {
	  MoveVidString(screen,
			tx,
			ty + 1,
			tx,
			ty,
			cols);
	  ty++;
       }
       for (i=0; i < cols; i++)
       {
	  PutVidChar(screen,
		     ' ',
		     tx + i,
		     ty,
		     screen->NormVid);
       }
    }
    else
    {
       tx = x;
       ty = y + (lines - 1);
       for (i=1; i < lines; i++)
       {
	  MoveVidString(screen,
			tx,
			ty - 1,
			tx,
			ty,
			cols);
	  ty--;
       }
       for (i=0; i < cols; i++)
       {
	  PutVidChar(screen,
		     ' ',
		     tx + i,
		     y,
		     screen->NormVid);
       }
    }

    return 0;

}

SCREEN *GetVidOwner(void)
{
   register SCREEN *screen = activeScreen;
   return (SCREEN *) (screen);
}

LONG SetNormVid(LONG attr)
{
   register SCREEN *screen = activeScreen;
   screen->NormVid = attr;
   return 0;
}

LONG GetNormVid(void)
{
   register SCREEN *screen = activeScreen;
   return (screen->NormVid);
}

SCREEN *SetKeyboardOwner(SCREEN *screen)
{
    SCREEN *s;
    register LONG flags;

    flags = get_flags();
    s = keyboardOwner;
    keyboardOwner = screen;
    set_flags(flags);

    return s;

}

LONG SetVideoOwner(SCREEN *screen)
{

    extern void SetScreenLines(LONG nLines);
    register SCREEN *oldScreen;
    register LONG flags;

     if (screen->screenSignature != SCREEN_SIGNATURE)
	return 0;

     if (screen == activeScreen)
	return 0;

     if (!screen->pVidMem)
	return 0;

     if (!screen->pVirtVid)
	return 0;

     //  save the screen data;

     flags = get_flags();
     oldScreen = activeScreen;

     spin_lock(&oldScreen->screen_mutex);
     CopyData((LONG *)VGATextBase,
	      (LONG *)oldScreen->pVirtVid,
	      (oldScreen->nCols * oldScreen->nLines * 2));
     oldScreen->pVidMem = oldScreen->pVirtVid;
     spin_unlock(&oldScreen->screen_mutex);

     activeScreen = screen;
     SetKeyboardOwner(screen);
     SetScreenLines(screen->nLines);

     spin_lock(&screen->screen_mutex);
     screen->pVidMem = VGATextBase;
     CopyData((LONG *)screen->pVirtVid,
	      (LONG *)VGATextBase,
	      (screen->nCols * screen->nLines * 2));
     spin_unlock(&screen->screen_mutex);

     spin_lock(&DisplayHardwareLock);

     if (!screen->CursOn)
	HardXY(100, 100);
     else
	HardXY(screen->CrntX, screen->CrntY);

     spin_unlock(&DisplayHardwareLock);
     set_flags(flags);

     return (LONG) (screen);

}

LONG PopUpScreen(SCREEN *screen)
{

    extern void SetScreenLines(LONG nLines);
    register SCREEN *oldScreen;
    register LONG flags;

     if (screen->screenSignature != SCREEN_SIGNATURE)
	return 0;

     if (screen == activeScreen)
	return 0;

     if (!screen->pVidMem)
	return 0;

     if (!screen->pVirtVid)
	return 0;

     //  save the screen data;

     flags = get_flags();

     oldScreen = activeScreen;

     spin_lock(&oldScreen->screen_mutex);
     CopyData((LONG *)VGATextBase,
	      (LONG *)oldScreen->pVirtVid,
	      (oldScreen->nCols * oldScreen->nLines * 2));
     oldScreen->pVidMem = oldScreen->pVirtVid;
     spin_unlock(&oldScreen->screen_mutex);

     activeScreen = screen;
     SetScreenLines(screen->nLines);

     spin_lock(&screen->screen_mutex);
     screen->pVidMem = VGATextBase;
     CopyData((LONG *)screen->pVirtVid,
	      (LONG *)VGATextBase,
	      (screen->nCols * screen->nLines * 2));
     spin_unlock(&screen->screen_mutex);

     spin_lock(&DisplayHardwareLock);

     if (!screen->CursOn)
	HardXY(100, 100);
     else
	HardXY(screen->CrntX, screen->CrntY);

     spin_unlock(&DisplayHardwareLock);
     set_flags(flags);

     return (LONG) (screen);

}

void SetXY(SCREEN *screen, LONG x, LONG y)
{
    register SCREEN *tempActiveScreen = activeScreen;
    register LONG flags;

    if (screen->screenSignature != SCREEN_SIGNATURE)
	return;

    screen->CrntX = x;
    screen->CrntY = y;
    if (screen == tempActiveScreen)
    {
       flags = get_flags();
       spin_lock(&DisplayHardwareLock);
       HardXY(x, y);
       spin_unlock(&DisplayHardwareLock);
       set_flags(flags);
    }
    return;

}

void GetXY(SCREEN *screen, LONG *x, LONG *y)
{

    if (screen->screenSignature != SCREEN_SIGNATURE)
	return;

    *x = screen->CrntX;
    *y = screen->CrntY;
    return;

}

void PutVidString(SCREEN *screen, BYTE *s, LONG x, LONG y, LONG attr)
{

    register LONG tmp;
    register int i;
    register BYTE *v;

    if (screen->screenSignature != SCREEN_SIGNATURE)
	return;

    v = screen->pVidMem;
    v += (y * (screen->nCols * 2)) + x * 2;
    while (*s)
    {
       *v++ = *s++;
       *v++ = attr;
    }

}

void PutVidStringTransparent(SCREEN *screen, BYTE *s, LONG x, LONG y, LONG attr)
{

    register LONG tmp;
    register int i;
    register BYTE *v;

    if (screen->screenSignature != SCREEN_SIGNATURE)
	return;

    v = screen->pVidMem;
    v += (y * (screen->nCols * 2)) + x * 2;
    while (*s)
    {
       *v++ = *s++;
       if (attr)
	  *v++ |= attr;
       else
	  v++;
    }

}

void PutVidStringCleol(SCREEN *screen, BYTE *s, LONG line, LONG attr)
{

    register LONG tmp;
    register int r, i;
    register BYTE *v;

    if (screen->screenSignature != SCREEN_SIGNATURE)
	return;

    v = screen->pVidMem;
    v += (line * (screen->nCols * 2)) + 0 * 2;
    for (r = 0; r < screen->nCols; r++)
    {
       switch (*s)
       {
	  case '\t':
	     s++;
	     *v++ = ' ';
	     *v++ = attr;
	     for (i=0; i < screen->TabSize - 1; i++)
	     {
		if (r++ > screen->nCols)
		   break;

		*v++ = ' ';
		*v++ = attr;
	     }
	     break;

	  default:
	     if (!*s)
		*v++ = ' ';
	     else
		*v++ = *s++;
	     *v++ = attr;
	     break;
       }
    }

}

void PutVidStringToLength(SCREEN *screen, BYTE *s, LONG x,
			  LONG y, LONG attr, LONG len)
{

    register LONG tmp;
    register int r, i;
    register BYTE *v;

    if (screen->screenSignature != SCREEN_SIGNATURE)
	return;

    v = screen->pVidMem;
    v += (y * (screen->nCols * 2)) + x * 2;
    for (r = 0; r < len; r++)
    {
       switch (*s)
       {
	  case '\t':
	     s++;
	     *v++ = ' ';
	     *v++ = attr;
	     for (i=0; i < screen->TabSize - 1; i++)
	     {
		if (r++ > len)
		   break;

		*v++ = ' ';
		*v++ = attr;
	     }
	     break;

	  default:
	     if (!*s)
		*v++ = ' ';
	     else
		*v++ = *s++;
	     *v++ = attr;
	     break;
       }
    }

}

void PutVidCharCleol(SCREEN *screen, BYTE c, LONG line, LONG attr)
{

   register LONG i;

   for (i=0; i < screen->nCols; i++)
      PutVidChar(screen, c, i, line, attr);

}

void PutVidChar(SCREEN *screen, BYTE c, LONG x, LONG y, LONG attr)
{

    register BYTE *v;

    if (screen->screenSignature != SCREEN_SIGNATURE)
	return;

    v = screen->pVidMem;
    v += (y * (screen->nCols * 2)) + x * 2;
    *v++ = c;
    *v = attr;

}

BYTE GetVidChar(SCREEN *screen, LONG x, LONG y)
{

   register BYTE *v;

   if (screen->screenSignature != SCREEN_SIGNATURE)
      return 0;

   v = screen->pVidMem;
   v += (y * (screen->nCols * 2)) + x * 2;

   return (BYTE)(*v);

}

BYTE GetVidCharAttr(SCREEN *screen, LONG x, LONG y)
{

   register BYTE *v;

   if (screen->screenSignature != SCREEN_SIGNATURE)
      return 0;

   v = screen->pVidMem;
   v += (y * (screen->nCols * 2)) + x * 2;

   v++;
   return (BYTE)(*v);

}

long ScreenTTYOut(SCREEN *screen, BYTE *buf, LONG len, LONG attr)
{

	register int i, tab;

	if (screen->screenSignature != SCREEN_SIGNATURE)
	   return 0;

	if (!len)
	   return 0;

	for (i=0; i < len; i++)
	{
	   switch (*buf)
	   {
	      case '\t':        // TAB?
		 for (tab=0; tab < screen->TabSize; tab++)
		 {
		    PutChar(screen, ' ', screen->CrntX, screen->CrntY, attr);
		    screen->CrntX++;
		    if (screen->CrntX >= screen->nCols)
		    {
		       screen->CrntX = 0;
		       screen->CrntY++;
		       if (screen->CrntY >= screen->nLines)
		       {
			  ScrollScreen(screen);
			  screen->CrntY--;
		       }
		       if (screen->Pause)
		       {
			  if (screen->CurrCnt++ > screen->ScrlCnt)
			  {
			     screen->CurrCnt = 0;
			     if (Pause())
				return 1;
			  }
		       }
		    }
		 }
		 break;

	      case '\n':        // LF?
		 screen->CrntY++;
		 screen->CrntX = 0;
		 if (screen->CrntY >= screen->nLines)
		 {
		    ScrollScreen(screen);
		    screen->CrntY--;
		 }
		 if (screen->Pause)
		 {
		    if (screen->CurrCnt++ > screen->ScrlCnt)
		    {
		       screen->CurrCnt = 0;
		       if (Pause())
			  return 1;
		    }
		 }
		 break;

	      case '\r':        // CR?
		 screen->CrntX = 0;
		 break;

	      case '\b':        // BackSpace?
		 if (screen->CrntX)
		    screen->CrntX--;
		 else
		 {
		    if (screen->CrntY)
		    {
		       screen->CrntY--;
		       screen->CrntX = (screen->nCols - 1);
		       if (screen->Pause)
			  if (screen->CurrCnt)
			     screen->CurrCnt--;
		    }
		 }
		 break;

	      default:
		 PutChar(screen, *buf, screen->CrntX, screen->CrntY, attr);
		 screen->CrntX++;
		 if (screen->CrntX >= screen->nCols)
		 {
		    screen->CrntX = 0;
		    screen->CrntY++;
		    if (screen->CrntY >= screen->nLines)
		    {
		       ScrollScreen(screen);
		       screen->CrntY--;
		    }
		    if (screen->Pause)
		    {
		       if (screen->CurrCnt++ > screen->ScrlCnt)
		       {
			  screen->CurrCnt = 0;
			  if (Pause())
			     return 1;
		       }
		    }
		 }
		 break;
	   }
	   SetXY(screen, screen->CrntX, screen->CrntY);
	   buf++;
	}

	return 0;

}

LONG convertScreenMode(SCREEN *screen, LONG mode)
{
   register LONG value;

   if (screen->screenSignature != SCREEN_SIGNATURE)
      return -1;

   value = screen->CR_or_LF_Flag;
   screen->CR_or_LF_Flag = mode;

   return value;

}


void TextMode(LONG mode)
{

    register REGS *inregs, *outregs;

    inregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!inregs)
       return;

    outregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!outregs)
    {
       FreeMemoryBelow1MB(inregs);
       return;
    }

    inregs->eax = (mode & 0xFF) | 0x80; // do not clear the screen
    int86x(0x10, inregs, outregs);

    FreeMemoryBelow1MB(inregs);
    FreeMemoryBelow1MB(outregs);
    return;

}

LONG font_seg = -1;   //  segment of DOS buffer for 8x10 font

void SetScanLinesAndFont(LONG scan_lines, LONG font)
{

    register REGS *inregs, *outregs;

    inregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!inregs)
       return;

    outregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!outregs)
    {
       FreeMemoryBelow1MB(inregs);
       return;
    }

    //  Set 200/350/400 scan lines.

    inregs->eax = (0x12 << 8 | scan_lines);  // 0: 200, 1: 350, 2: 400
    inregs->ebx = 0x30;
    int86x(0x10, inregs, outregs);

    //  Scan lines setting only takes effect when video mode is set.

    inregs->eax = 0x83;
    int86x(0x10, inregs, outregs);

    //  Load a ROM BIOS font (0x11: 8x14, 0x12: 8x8, 0x14: 8x16).

    inregs->ebx = 0;                /* block zero */
    inregs->eax = (0x11 << 8 | font & 0xFF);
    int86x(0x10, inregs, outregs);


    //  Stretch a 8x8 font to the 8x10 character box.  This is required to
    //  use 80x40 mode on a VGA or 80x35 mode on an EGA, because the character
    //  box is 10 lines high, and the ROM BIOS doesn't have an appropriate font.
    //  So we create one from the 8x8 font by adding an extra blank line
    //  from each side.

    FreeMemoryBelow1MB(inregs);
    FreeMemoryBelow1MB(outregs);
    return;

}

LONG Create_8x10_Font(void)
{
    register LONG retCode;
    unsigned char *p;
    BYTE *src, *dest;
    BYTE val;
    LONG i, j;

    if (font_seg == (LONG) -1)
    {
       register REGS *inregs, *outregs;

       inregs = AllocateMemoryBelow1MB(sizeof(REGS));
       if (!inregs)
	  return -1;

       outregs = AllocateMemoryBelow1MB(sizeof(REGS));
       if (!outregs)
       {
	  FreeMemoryBelow1MB(inregs);
	  return -1;
       }

       //  Allocate buffer in conventional memory.
       font_seg = (LONG) AllocateMemoryBelow1MB(160);

       //  Get the pointer to the 8x8 font table.
       p = AllocateMemoryBelow1MB(2560); //  256 chars X 8x10 pixels
       if (!p)
       {
	  FreeMemoryBelow1MB((void *)font_seg);
	  font_seg = (LONG) -1;
	  FreeMemoryBelow1MB(inregs);
	  FreeMemoryBelow1MB(outregs);
	  return -1;
       }
       inregs->ebx = 0x0300;
       inregs->eax = 0x1130;
       retCode = int86x(0x10, inregs, outregs);
       if (retCode)
       {
	  FreeMemoryBelow1MB(inregs);
	  FreeMemoryBelow1MB(outregs);
	  return -1;
       }

       src = (BYTE *)(((unsigned)outregs->es) << 4) + outregs->ebp;
       dest = (BYTE *) font_seg;

       //  Now copy the font to our table, stretching it to 8x10.

       //  Fill first extra scan line with zeroes.
       for (i = 0; i < 256; i++)
       {
	  *dest++ = 0;
       }

       for (j = 0; j < 8; j++)
       {
	  val = *src++;
	  *dest++ = val;
       }

       //  Fill last extra scan line with zeroes.
       *dest = 0;

       FreeMemoryBelow1MB(inregs);
       FreeMemoryBelow1MB(outregs);
       return 0;
    }
    return 0;
}

//
//
//   Load the 8x10 font we created into character generator RAM.
//
//

void Load_8x10_Font(void)
{

    register REGS *inregs, *outregs;

    inregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!inregs)
       return;

    outregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!outregs)
    {
       FreeMemoryBelow1MB(inregs);
       return;
    }

    Create_8x10_Font();      //  create if needed
    inregs->es = font_seg;         //  pass pointer to our font in ES:BP
    inregs->ebp = 0;
    inregs->edx = 0;               //  1st char: ASCII 0
    inregs->ecx = 256;             //  256 chars
    inregs->ebx = 0x1000;          //  10 points per char
    inregs->eax = 0x1110;
    int86x(0x10, inregs, outregs);

    FreeMemoryBelow1MB(inregs);
    FreeMemoryBelow1MB(outregs);
    return;
}

//
//
//   Set screen scan lines and load 8x10 font.
//   SCAN_LINES is as required by Int 10h function 12h.
//
//

void SetScanLinesAnd_8x10_Font(LONG scan_lines)
{

    register REGS *inregs, *outregs;

    inregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!inregs)
       return;

    outregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!outregs)
    {
       FreeMemoryBelow1MB(inregs);
       return;
    }

    inregs->ebx = 0x30;
    inregs->eax = 0x12 << 8 | scan_lines;     //  0: 200, 1: 350, 2: 400
    int86x(0x10, inregs, outregs);

    // Set video mode, so that scan lines we set will take effect.

    inregs->eax = 0x83;
    int86x(0x10, inregs, outregs);

    // Load our 8x10 font and enable intensity bit.
    Load_8x10_Font();

    // Switch to screen lines given by NLINES.

    FreeMemoryBelow1MB(inregs);
    FreeMemoryBelow1MB(outregs);
    return;
}

void SetScreenLines(LONG nlines)
{
    extern LONG currentScreenLines;
    register REGS *inregs, *outregs;

    inregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!inregs)
       return;

    outregs = AllocateMemoryBelow1MB(sizeof(REGS));
    if (!outregs)
    {
       FreeMemoryBelow1MB(inregs);
       return;
    }

    switch (nlines)
    {
      case 25:
	  if (currentScreenMode != LINE_MODE_25)
	  {
	     if (DisplayAdapter)
	     {
		//  Set 350 scan lines for EGA, 400 for VGA.
		inregs->ebx = 0x30;
		inregs->eax = 0x12 << 8 | (DisplayAdapter > 1 ? 2 : 1);
		int86x(0x10, inregs, outregs);

		//  Load ROM BIOS font: 8x14 for EGA, 8x16 for VGA.
		inregs->ebx = 0;
		inregs->eax = 0x11 << 8 | (DisplayAdapter > 1 ? 0x14 : 0x11);
		int86x(0x10, inregs, outregs);
	     }
	     //  Set video mode.
	     inregs->eax = 0x83;
	     int86x(0x10, inregs, outregs);
	     currentScreenMode = LINE_MODE_25;
	  }
	  break;

      case 28:      //  VGA only
	  if (currentScreenMode != LINE_MODE_28)
	  {
	     if (DisplayAdapter > 1)
	     {
		SetScanLinesAndFont(2, 0x11);
		currentScreenMode = LINE_MODE_28;
	     }
	  }
	  break;

      case 35:      //  EGA or VGA
	  if (currentScreenMode != LINE_MODE_35)
	  {
	     if (DisplayAdapter)
	     {
		SetScanLinesAnd_8x10_Font(1);
		currentScreenMode = LINE_MODE_35;
	     }
	  }
	  break;

      case 40:      //  VGA only
	  if (currentScreenMode != LINE_MODE_40)
	  {
	     if (DisplayAdapter > 1)
	     {
		SetScanLinesAnd_8x10_Font(2);
		currentScreenMode = LINE_MODE_40;
	     }
	  }
	  break;

      case 43:      //  EGA or VGA
	  if (currentScreenMode != LINE_MODE_43)
	  {
	     if (DisplayAdapter)
	     {
		SetScanLinesAndFont(1, 0x12);
		currentScreenMode = LINE_MODE_43;
	     }
	  }
	  break;

      case 50:      //  VGA only
	  if (currentScreenMode != LINE_MODE_50)
	  {
	     if (DisplayAdapter > 1)
	     {
		SetScanLinesAndFont(2, 0x12);
		currentScreenMode = LINE_MODE_50;
	     }
	  }
	  break;
    }

    FreeMemoryBelow1MB(inregs);
    FreeMemoryBelow1MB(outregs);
    return;

}

LONG disableAccelerators(SCREEN *screen)
{
    register LONG retCode;

    retCode = screen->suppressAccelerators;
    if (!retCode)
       screen->suppressAccelerators = TRUE;

    return retCode;
}

LONG enableAccelerators(SCREEN *screen)
{
    register LONG retCode;

    retCode = screen->suppressAccelerators;
    if (retCode)
       screen->suppressAccelerators = 0;

    return retCode;
}

LONG ScreenInputFromKeyboard(SCREEN *screen, BYTE *buf, LONG current_len, LONG max_len, LONG attr)
{

    register LONG key, insert = 1;
    register BYTE *p, ch;
    register int i, r, temp;
    register LONG PosnX;
    LONG EditX, EditY;
    register LONG clen, cIndex;

    if (screen->screenSignature != SCREEN_SIGNATURE)
       return 0;

    if (current_len > max_len)
       return 0;

    if (!max_len)
       return 0;

    GetXY(screen, &EditX, &EditY);  // get current x:y coordinates

    clen = current_len;
    PosnX = EditX + current_len;    // get end of line

    p = (BYTE *)((LONG)buf + (LONG)current_len);
    for (i=0; i < (max_len - current_len); i++)   // fill with spaces to end of line
       *p++ = '\0';
    cIndex = screen->commandIndex;

    while (1)
    {
       SetXY(screen, PosnX, EditY);
       PutVidString(screen, buf, EditX, EditY, attr);
       if (debuggerActive)
	  key = KeyboardPollProcedure();
       else
	  key = ReadKeyboard(screen);

       if (!screen->suppressAccelerators)
       {
	  if (IsAccelerator(screen, key) && key != ENTER)
	  {
	     if (strncmp(screen->commandRecall[(screen->commandIndex - 1) & 0xF],
			  buf,
			  strlen(buf)))
	     {
		for (r=0; r < max_len; r++)
		{
		   if (buf[0])
		      screen->commandRecall[screen->commandIndex & 0xF][r] = buf[r];
		   PutVidChar(screen, ' ', (EditX + r), EditY, attr);
		}
		if (buf[0])
		   screen->commandIndex++;
	     }
	     return key;
	  }
       }
       ch = (BYTE) (key & 0x000000FF);
       switch (ch)
       {
	  case BKSP:
	     if (current_len)
	     {
		PosnX--;
		current_len--;
		PutVidChar(screen, ' ', PosnX, EditY, attr);
		if (!buf[current_len + 1])
		   buf[current_len] = '\0';
		else
		   buf[current_len] = ' ';
	     }
	     break;

	  case HOME:
	     current_len = clen;
	     PosnX = EditX + current_len;
	     break;

	  case END:
	     current_len = clen + strlen(buf);
	     PosnX = EditX + current_len;
	     break;

	  case LEFT_ARROW:
	     if (current_len)
	     {
		PosnX--;
		current_len--;
	     }
	     break;

	  case RIGHT_ARROW:
	     if (PosnX < (EditX + clen + strlen(buf)))
	     {
		PosnX++;
		current_len++;
	     }
	     break;

	  case INS:
	     if (insert)
		insert = 0;
	     else
		insert = 1;
	     break;

	  case DEL:
	     p = (BYTE *) &buf[current_len];
	     temp = current_len;
	     p++;
	     while ((*p) && (temp < max_len))
	     {
		buf[temp++] = *p++;
	     }
	     buf[temp] = '\0';
	     PutVidChar(screen, ' ', EditX + temp, EditY, attr);
	     break;

	  case ESC:
	  case ENTER:
	     if (strncmp(screen->commandRecall[(screen->commandIndex - 1) & 0xF],
			  buf,
			  strlen(buf))
		    || strlen(buf) != strlen(screen->commandRecall[(screen->commandIndex - 1) & 0xF]))
	     {
		for (r=0; r < max_len; r++)
		{
		   if (buf[0])
		      screen->commandRecall[screen->commandIndex & 0xF][r] = buf[r];
		}
		if (buf[0])
		   screen->commandIndex++;
	     }

	     if (!screen->CR_or_LF_Flag)
	     {
		screen->CrntY++;
		screen->CrntX = 0;
		if (screen->CrntY >= screen->nLines)
		{
		   ScrollScreen(screen);
		   screen->CrntY--;
		}
		if (screen->Pause)
		{
		   if (screen->CurrCnt++ > screen->ScrlCnt)
		   {
		      screen->CurrCnt = 0;
		      if (Pause())
			 return ENTER;
		   }
		}
	     }
	     return ENTER;

	  case UP_ARROW:
	     if (screen->commandRecall[(cIndex - 1) & 0xF][0])
	     {
		cIndex--;
		for (r=0; r < max_len; r++)
		{
		   buf[r] = screen->commandRecall[cIndex & 0xF][r];
		   PutVidChar(screen, ' ', (EditX + r), EditY, attr);
		}
		current_len = strlen(buf);
		PosnX = EditX + current_len;    // get end of line
	     }
	     break;

	  case DOWN_ARROW:
	     if (screen->commandRecall[cIndex & 0xF][0])
	     {
		cIndex++;
		for (r=0; r < max_len; r++)
		{
		   buf[r] = screen->commandRecall[cIndex & 0xF][r];
		   PutVidChar(screen, ' ', (EditX + r), EditY, attr);
		}
		current_len = strlen(buf);
		PosnX = EditX + current_len;    // get end of line
	     }
	     break;

	  default:
	     if ((ch > 0x7E) || (ch < 0x20))  // if above or below text
		break;
	     else
	     {
		if (!insert)
		{
		   buf[current_len] = ch;
		   if (current_len < max_len)
		   {
		      PosnX++;
		      current_len++;
		   }
		}
		else
		{
		   if (strlen(buf) < max_len)
		   {
		      for (i=max_len; i > current_len; i--)
			 buf[i] = buf[i-1];
		      buf[current_len] = ch;
		      if (current_len < max_len)
		      {
			 PosnX++;
			 current_len++;
		      }
		   }
		}
	     }
	     break;
       }
    }

}

LONG KeyboardPollProcedure(void)
{

    register LONG key;

    for (;;)
    {
       if (PollKeyboard())
       {
	  key = ReadKeyboardFinal();
	  if (key)
	  {
	     return (LONG) key;
	  }
       }
    }

}


LONG Pause(void)
{

   register LONG key;

   printfScreen(keyboardOwner, "    ---- More ----     ");

   if (debuggerActive)
      key = KeyboardPollProcedure();
   else
      key = ReadKeyboard(keyboardOwner);
   printfScreen(keyboardOwner, "\r                       \r");

   if ((key & 0x000000FF) == ESC)
      return 1;
   else
      return 0;

}

LONG GetKey(void)
{
   return (KeyboardPollProcedure());
}

LONG CheckKey(SCREEN *screen)
{

    register LONG key;

    if (screen->screenSignature != SCREEN_SIGNATURE)
       return 0;

    if (screen->typeAheadCount)
    {
       screen->typeAheadCount--;
       key = screen->kbuffer[0];
       screen->kbuffer[0] = 0;

       return (key);
    }

    return 0;

}

void ListScreensEvent(void)
{
    if (GetVidOwner() == screenScreens)
	  return;

    ClearScreen(screenScreens);
    SetVideoOwner(screenScreens);

    if (spin_try_lock(&ListScreensLock))
    {
       ListScreensLWP.procedureAddress = (LONG (*)(void *)) ListScreens;
       ListScreensLWP.lwpArgument = GetVidOwner();
       scheduleLWP(&ListScreensLWP);
    }
    return;
}

void SwitchScreensEvent(void)
{
    if (GetVidOwner() == screenScreens)
       return;
    SwitchScreens();
    return;
}

//
//  returns  (key) is key was pressed
//           -1 if screen context was destroyed
//

LONG ReadKeyboard(SCREEN *screen)
{
    register LONG key, flags;

    if (screen->screenSignature != SCREEN_SIGNATURE)
       return 0;

    for (;;)
    {
       sema_wait(&screen->screen_sema);

       flags = get_flags();
       spin_lock(&screen->screen_mutex);

       if (screen->typeAheadCount)
       {
	  key = screen->kbuffer[screen->typeAheadReader & 0xF];
	  screen->kbuffer[screen->typeAheadReader & 0xF] = 0;
	  screen->typeAheadReader++;
	  if (screen->typeAheadCount)
	     screen->typeAheadCount--;

	  spin_unlock(&screen->screen_mutex);
	  set_flags(flags);

	  return (key);
       }
       spin_unlock(&screen->screen_mutex);
       set_flags(flags);
    }

}

//
//  returns  (key) is key was pressed
//           -1 if screen context was destroyed
//

LONG CheckKeyboard(SCREEN *screen)
{
    register LONG flags;
    register LONG key = 0;

    if (screen->screenSignature != SCREEN_SIGNATURE)
       return 0;

    if (screen->typeAheadCount)
    {
       sema_wait(&screen->screen_sema);

       flags = get_flags();
       spin_lock(&screen->screen_mutex);

       key = screen->kbuffer[screen->typeAheadReader & 0xF];
       screen->kbuffer[screen->typeAheadReader & 0xF] = 0;
       screen->typeAheadReader++;
       if (screen->typeAheadCount)
	  screen->typeAheadCount--;

       spin_unlock(&screen->screen_mutex);
       set_flags(flags);

       return (key);
    }
    spin_unlock(&screen->screen_mutex);
    set_flags(flags);

    return key;

}

void KeyboardEvent(LONG key)
{
    //
    //  NOTE:  If you add local variables here, you may need to change
    //  the stack adjustment value in the routine KeyboardEnterDebugger()
    //  in STARTUP.386 so the debugger will access the correct stack frame
    //

    register LONG flags;
    register PROCESS *p;

    if ((LONG)(key & 0xFFFF) == (LONG)(LEFT_SHIFT_MASK | RIGHT_SHIFT_MASK | LEFT_ALT_MASK | ESC) ||
	(LONG)(key & 0xFFFF) == (LONG)(LEFT_ALT_MASK | 'D') ||
	(LONG)(key & 0xFFFF) == (LONG)(LEFT_ALT_MASK | 'd'))
    {
       KeyboardEnterDebugger();
       return;
    }
    if ((LONG)(key & 0xFFFF) == (LONG)(LEFT_CTRL_MASK | ESC))
    {
       ListScreensEvent();
       return;
    }
    if ((LONG)(key & 0xFFFF) == (LONG)(LEFT_ALT_MASK | ESC))
    {
       SwitchScreensEvent();
       return;
    }

    EventNotify(EVENT_KEY_PRESSED, key);

    flags = get_flags();
    spin_lock(&keyboardOwner->screen_mutex);

    keyboardOwner->kbuffer[keyboardOwner->typeAheadWriter & 0xF] = key;
    keyboardOwner->typeAheadWriter++;
    if (keyboardOwner->typeAheadCount < sizeof(keyboardOwner->kbuffer) - 1)
       keyboardOwner->typeAheadCount++;

    spin_unlock(&keyboardOwner->screen_mutex);
    set_flags(flags);

    sema_post(&keyboardOwner->screen_sema);

    return;
}

