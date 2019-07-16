
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
*   FILE     :  WINDOW.H
*   DESCRIP  :  Windowed Text Screen Code for MANOS v1.0
*   DATE     :  November 9, 1997
*
*
***************************************************************************/

#include "types.h"

#define TRUE          1
#define FALSE         0

#define MAX_FRAME     100
#define BORDER        1

typedef struct _WINDOW_FRAME {
   LONG num;
   LONG startx;
   LONG endx;
   LONG starty;
   LONG endy;
   LONG curx;
   LONG cury;
   LONG pcurx;
   LONG pcury;
   LONG border;
   LONG active;
   LONG header_color;
   LONG border_color;
   LONG fill_color;
   LONG text_color;
   BYTE *p;
   BYTE header[80];
   SCREEN *screen;
   LONG PopUp;
   LONG owner;
} screen_frame_t;

extern screen_frame_t frame[MAX_FRAME];

extern LONG draw_border(LONG num);
extern LONG close_window(LONG num);
extern LONG free_window(LONG num);
extern LONG activate_window(LONG num);
extern LONG create_window(SCREEN *screen, BYTE *header, int startx, int starty,
		  int endx, int endy, LONG border,
		  LONG hcolor, LONG bcolor, LONG fcolor, LONG tcolor,
		  LONG PopUp);
extern LONG create_window_internal(SCREEN *screen, LONG num, BYTE *header, int startx, int starty,
		  int endx, int endy, LONG border,
		  LONG hcolor, LONG bcolor, LONG fcolor, LONG tcolor,
		  BYTE *buffer, LONG PopUp);
extern LONG display_header(LONG num);
extern LONG window_puts(LONG num, BYTE *str);
extern LONG window_putchar(LONG num, BYTE ch);
extern LONG window_xy(LONG num, int x, int y);
extern void window_cls(LONG num);
extern LONG save_window(LONG num);
extern LONG restore_window(LONG num);
extern LONG window_write_string(LONG num, BYTE *p, LONG x, LONG y, LONG attr);

