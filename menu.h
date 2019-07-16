
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
*   FILE     :  MENU.H
*   DESCRIP  :  Text Menu Screen Code for MANOS v1.0
*   DATE     :  March 17, 1998
*
*
***************************************************************************/

#include "types.h"

#define TRUE            1
#define FALSE           0
#define MAX_MENU        100
#define BORDER_SINGLE   1
#define BORDER_DOUBLE   2

typedef struct _MENU_FRAME {
   LONG num;
   LONG startx;
   LONG endx;
   LONG starty;
   LONG endy;
   LONG curx;
   LONG cury;
   LONG pcurx;
   LONG pcury;
   LONG windowSize;
   LONG border;
   LONG active;
   LONG header_color;
   LONG border_color;
   LONG fill_color;
   LONG text_color;
   BYTE *p;
   BYTE header[80];
   SCREEN *screen;
   BYTE **elementStrings;
   LONG elementCount;
   LONG *elementValues;
   BYTE *elementStorage;
   LONG owner;
   LONG (*elementFunction)(SCREEN *, LONG, BYTE *);
   LONG (*warnFunction)(SCREEN *);
   long choice;
   long index;
   long top;
   long bottom;
   LONG selected;
   LONG (*keyboardHandler)(SCREEN *, LONG);
   LONG keyboardMask;
   LONG screenMode;
   LONG nLines;
} menu_frame_t;

extern menu_frame_t menu_frame[MAX_MENU];

extern LONG get_resp(LONG num);
extern LONG fill_menu(LONG num, LONG ch, LONG attr);
extern LONG save_menu(LONG num);
extern LONG restore_menu(LONG num);
extern LONG close_menu(LONG num);
extern LONG free_menu(LONG num);
extern LONG display_menu_header(LONG num);
extern LONG draw_menu_border(LONG num);
extern void display_menu(LONG num);
extern LONG add_item_to_menu(LONG num, BYTE *item, LONG value);
extern LONG activate_menu(LONG num);
extern LONG make_menu(SCREEN *screen,
		      BYTE *header,
		      int startx,
		      int starty,
		      LONG windowSize,
		      LONG border,
		      LONG hcolor,
		      LONG bcolor,
		      LONG fcolor,
		      LONG tcolor,
		      LONG (*elementFunction)(SCREEN *, LONG, BYTE *),
		      LONG (*warnFunction)(SCREEN *));

extern LONG menu_write_string(LONG num, BYTE *p, LONG x, LONG y, LONG attr);

extern LONG free_portal(LONG num);
extern LONG activate_portal(LONG num);
extern LONG update_portal(LONG num);
extern LONG update_static_portal(LONG num);
extern LONG make_portal(SCREEN *screen,
		      BYTE *header,
		      int startx,
		      int starty,
		      int endx,
		      int endy,
		      LONG numberOfLines,
		      LONG border,
		      LONG hcolor,
		      LONG bcolor,
		      LONG fcolor,
		      LONG tcolor,
		      LONG (*keyboardHandler)(SCREEN *, LONG));
extern LONG write_portal(LONG num, BYTE *p, int x, int y, LONG attr);
extern LONG write_screen_comment_line(SCREEN *screen, BYTE *p, LONG attr);
extern LONG activate_static_portal(LONG num);
extern LONG deactivate_static_portal(LONG num);
extern LONG disable_portal_input(LONG num);
extern LONG enable_portal_input(LONG num);



