

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  WINDOW.C
*   DESCRIP  :  Windowed Text Screen Code for MANOS v1.0
*   DATE     :  November 9, 1997
*
*
***************************************************************************/

#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "kernel.h"
#include "keyboard.h"
#include "screen.h"
#include "types.h"
#include "emit.h"
#include "dos.h"
#include "tss.h"
#include "os.h"
#include "mps.h"
#include "hal.h"
#include "window.h"
#include "malloc.h"
#include "free.h"

screen_frame_t frame[MAX_FRAME];

void scroll_window(LONG num, LONG up)
{

    register LONG x, y;

    x = frame[num].startx + 1;
    y = frame[num].starty + 1;

    ScrollDisplay(frame[num].screen,
		  y,
		  x,
		  frame[num].endy - y,
		  frame[num].endx - x,
		  up);
    return;

}

LONG fill_window(LONG num, LONG ch, LONG attr)
{
   register int i, j;
   BYTE *v;
   BYTE *t;

   v = frame[num].screen->pVidMem;
   t = v;
   for (i=frame[num].starty; i < frame[num].endy + 1; i++)
   {
      for (j=frame[num].startx; j < frame[num].endx + 1; j++)
      {
	 v = t;
	 v += (j * frame[num].screen->nCols * 2) + i * 2;
	 *v++ = (BYTE) ch;
	 *v = (BYTE) attr;
      }
      frame[num].active = 0;
   }
   return 0;

}

LONG close_window(LONG num)
{

   frame[num].curx = 0;
   frame[num].cury = 0;
   restore_window(num);

   if (!frame[num].PopUp)
      SetXY(frame[num].screen, frame[num].pcurx, frame[num].pcury);

   return 0;

}

LONG free_window(LONG num)
{

   frame[num].curx = 0;
   frame[num].cury = 0;
   restore_window(num);

   if (!frame[num].PopUp)
      SetXY(frame[num].screen, frame[num].pcurx, frame[num].pcury);

   if (frame[num].p)
      kfree((void *)frame[num].p);

   frame[num].p = 0;
   frame[num].owner = 0;

   return 0;

}

LONG activate_window(LONG num)
{
   LONG choice;
   LONG x, y;

   if (!frame[num].PopUp)
      GetXY(frame[num].screen, (LONG *)&frame[num].pcurx, (LONG *)&frame[num].pcury);

   if (!frame[num].active)
   {
      save_window(num);
      fill_window(num, ' ', frame[num].fill_color);
      frame[num].active = TRUE;
   }

   if (frame[num].border)
   {
      draw_border(num);
      display_header(num);
   }

   x = frame[num].startx + 1 + frame[num].curx;
   y = frame[num].starty + 1 + frame[num].cury;

   if (!frame[num].PopUp)
      SetXY(frame[num].screen, y, x);

   return 0;


}

LONG create_window(SCREEN *screen,
		   BYTE *header,
		   int startx,
		   int starty,
		   int endx,
		   int endy,
		   LONG border,
		   LONG hcolor,
		   LONG bcolor,
		   LONG fcolor,
		   LONG tcolor,
		   LONG PopUp)
{

   register int i;
   register LONG num;
   BYTE *p;

   for (num=1; num < MAX_FRAME; num++)
   {
      if (!frame[num].owner)
	 break;
   }

   if (!num || num > MAX_FRAME)
      return 0;

   if (startx > screen->nLines - 1 || startx < 0 ||
       starty > screen->nCols - 2 || starty < 0)
      return 0;

   if (endx > screen->nLines - 1 || endy > screen->nCols)
      return 0;

   frame[num].startx = startx;
   frame[num].endx = endx;
   frame[num].starty = starty;
   frame[num].endy = endy;
   for (i=0; i < 80; i++)
   {
      if (!header[i])
	 break;
      frame[num].header[i] = header[i];
   }
   frame[num].header[i] = 0x00;   // null terminate string

   frame[num].p = kmalloc(screen->nLines * screen->nCols * 2);
   if (!frame[num].p)
      return 0;

   frame[num].screen = screen;
   frame[num].border = border;
   frame[num].num = num;
   frame[num].active = 0;
   frame[num].curx = 0;
   frame[num].cury = 0;
   frame[num].PopUp = PopUp;
   frame[num].header_color = BRITEWHITE;
   frame[num].border_color = BRITEWHITE;
   frame[num].fill_color = BRITEWHITE;
   frame[num].text_color = BRITEWHITE;
   if (hcolor)
      frame[num].header_color = hcolor;
   if (bcolor)
      frame[num].border_color = bcolor;
   if (fcolor)
      frame[num].fill_color = fcolor;
   if (tcolor)
      frame[num].text_color = tcolor;

   frame[num].owner = 1;

   return num;


}

LONG create_window_internal(SCREEN *screen,
			    LONG num,
			    BYTE *header,
			   int startx,
			   int starty,
			   int endx,
			   int endy,
			   LONG border,
			   LONG hcolor,
			   LONG bcolor,
			   LONG fcolor,
			   LONG tcolor,
			   BYTE *buf,
			   LONG PopUp)
{

   register int i;
   BYTE *p;

   if (frame[num].owner)
      return -1;

   if (num > MAX_FRAME)
      return -1;

   if (startx > screen->nLines - 1 || startx < 0 ||
       starty > screen->nCols - 2 || starty < 0)
      return -2;

   if (endx > screen->nLines - 1 || endy > screen->nCols)
      return -3;

   frame[num].startx = startx;
   frame[num].endx = endx;
   frame[num].starty = starty;
   frame[num].endy = endy;
   for (i=0; i < 80; i++)
   {
      if (!header[i])
	 break;
      frame[num].header[i] = header[i];
   }
   frame[num].header[i] = 0x00;   // null terminate string

   frame[num].p = buf;
   if (!frame[num].p)
      return -1;

   frame[num].screen = screen;
   frame[num].border = border;
   frame[num].num = num;
   frame[num].active = 0;
   frame[num].curx = 0;
   frame[num].cury = 0;
   frame[num].PopUp = PopUp;
   frame[num].header_color = BRITEWHITE;
   frame[num].border_color = BRITEWHITE;
   frame[num].fill_color = BRITEWHITE;
   frame[num].text_color = BRITEWHITE;
   if (hcolor)
      frame[num].header_color = hcolor;
   if (bcolor)
      frame[num].border_color = bcolor;
   if (fcolor)
      frame[num].fill_color = fcolor;
   if (tcolor)
      frame[num].text_color = tcolor;

   frame[num].owner = 1;

   return 0;


}

LONG display_header(LONG num)
{

   register int y, len;

   y = frame[num].starty;

   len = strlen((BYTE *) &frame[num].header[0]);
   len = (frame[num].endy - y - len) / 2;
   if (len < 0)
      return -1;

   y = y + len;
   PutVidString(frame[num].screen, frame[num].header, y, frame[num].startx,
		frame[num].header_color);

   return 0;


}

LONG draw_border(LONG num)
{
   register int i;
   BYTE *v;
   BYTE *t;

   v = frame[num].screen->pVidMem;
   t = v;

   for (i=frame[num].startx + 1; i < frame[num].endx; i++)
   {
      v += (i * frame[num].screen->nCols * 2) + frame[num].starty * 2;
      *v++ = 179;
      *v = frame[num].border_color;
      v = t;
      v += (i * frame[num].screen->nCols * 2) + frame[num].endy * 2;
      *v++ = 179;
      *v = frame[num].border_color;
      v = t;
   }
   for (i=frame[num].starty + 1; i < frame[num].endy; i++)
   {
      v += (frame[num].startx * frame[num].screen->nCols * 2) + (i * 2);
      *v++ = 196;
      *v = frame[num].border_color;
      v = t;
      v += (frame[num].endx * frame[num].screen->nCols * 2) + (i * 2);
      *v++ = 196;
      *v = frame[num].border_color;
      v = t;
   }

   PutVidChar(frame[num].screen, 218, frame[num].starty, frame[num].startx,
	      frame[num].border_color);
   PutVidChar(frame[num].screen, 192, frame[num].starty, frame[num].endx,
	      frame[num].border_color);
   PutVidChar(frame[num].screen, 191, frame[num].endy, frame[num].startx,
	      frame[num].border_color);
   PutVidChar(frame[num].screen, 217, frame[num].endy, frame[num].endx,
	      frame[num].border_color);

   return 0;


}

LONG window_puts(LONG num, BYTE *str)
{

   if (!frame[num].active)
      return -1;

   for ( ; *str; str++)
      window_putchar(num, *str);
   return 0;

}

LONG window_write_string(LONG num, BYTE *p, LONG x, LONG y, LONG attr)
{

   if (!frame[num].active)
      return -1;

   PutVidString(frame[num].screen,
		p,
		frame[num].starty + 1 + y,
		frame[num].startx + 1 + x,
		attr);

   if (!frame[num].PopUp)
      SetXY(frame[num].screen,
	    frame[num].starty + y + strlen(p) + 1,
	    frame[num].startx + x + 1);

   return 0;

}

LONG window_putchar(LONG num, BYTE ch)
{
   register int x, y;
   BYTE *v;

   if (!frame[num].active)
      return -1;

   x = frame[num].curx + frame[num].startx;
   y = frame[num].cury + frame[num].starty;

   v = frame[num].screen->pVidMem;
   v += (x * frame[num].screen->nCols * 2) + y * 2;

   if (y > frame[num].endy)
      return 1;

   if (x > frame[num].endx)
      return 1;

   if ((ch == '\n') || (ch == '\r'))
   {
      if (frame[num].curx < frame[num].endx - frame[num].startx)
	 frame[num].curx++;
      frame[num].cury = 0;
   }
   else
   {
      frame[num].cury++;
      *v++ = ch;
      *v = frame[num].text_color;
   }
   window_xy(num, frame[num].curx, frame[num].cury);
   return 1;

}

LONG window_xy(LONG num, int x, int y)
{

   if (x < 0 || x + frame[num].startx > frame[num].endx)
      return -1;

   if (y < 0 || y + frame[num].starty > frame[num].endy)
      return -1;

   frame[num].curx = x;
   frame[num].cury = y;

   if (!frame[num].PopUp)
      SetXY(frame[num].screen, frame[num].starty + y, frame[num].startx + x);

   return 0;


}

void window_cls(LONG num)
{

   register int i, j;
   BYTE *v;
   BYTE *t;

   v = frame[num].screen->pVidMem;
   t = v;
   for (i=frame[num].starty; i < frame[num].endy; i++)
   {
      for (j=frame[num].startx; j < frame[num].endx; j++)
      {
	 v = t;
	 v += (j * frame[num].screen->nCols * 2) + (i * 2);
	 *v++ = ' ';
	 *v = frame[num].fill_color;
      }
   }
   frame[num].curx = 0;
   frame[num].cury = 0;
}

LONG save_window(LONG num)
{
   register int i, j;
   BYTE *buf_ptr;
   BYTE *v;
   BYTE *t;

   buf_ptr = (BYTE *) &frame[num].p[0];
   v = frame[num].screen->pVidMem;
   for (i=frame[num].starty; i < frame[num].endy + 1; i++)
   {
      for (j=frame[num].startx; j < frame[num].endx + 1; j++)
      {
	 t = (v + (j * frame[num].screen->nCols * 2) + i * 2);
	 *buf_ptr++ = *t++;
	 *buf_ptr++ = *t;
	 *(t - 1) = ' ';  // fill window
      }
   }
   return 0;

}

LONG restore_window(LONG num)
{
   register int i, j;
   BYTE *buf_ptr;
   BYTE *v;
   BYTE *t;

   buf_ptr = (BYTE *) &frame[num].p[0];
   v = frame[num].screen->pVidMem;
   t = v;
   for (i=frame[num].starty; i < frame[num].endy + 1; i++)
   {
      for (j=frame[num].startx; j < frame[num].endx + 1; j++)
      {
	 v = t;
	 v += (j * frame[num].screen->nCols * 2) + i * 2;
	 *v++ = *buf_ptr++;
	 *v = *buf_ptr++;
      }
      frame[num].active = 0;
   }
   return 0;

}


