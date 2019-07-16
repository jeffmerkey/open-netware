

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
*   FILE     :  MENU.C
*   DESCRIP  :  Text Menu Screen Code for MANOS v1.0
*   DATE     :  March 17, 1998
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
#include "menu.h"
#include "malloc.h"
#include "free.h"

#define UP_CHAR       0x1E
#define DOWN_CHAR     0x1F

menu_frame_t menu_frame[MAX_MENU];
extern LONG debuggerActive;

void scroll_menu(LONG num, LONG up)
{

    register LONG x, y;

    if (strlen(menu_frame[num].header))
       x = menu_frame[num].startx + 3;
    else
       x = menu_frame[num].startx + 1;

    y = menu_frame[num].starty + 1;

    ScrollDisplay(menu_frame[num].screen,
		  y,
		  x,
		  menu_frame[num].endy - y,
		  menu_frame[num].endx - x,
		  up);
    return;

}

LONG get_resp(LONG num)
{

    BYTE buf[255];
    register long key, x, y, width;
    register LONG i;
    register LONG retCode;

    menu_frame[num].choice = 0;
    menu_frame[num].index = 0;

    if (strlen(menu_frame[num].header))
       x = menu_frame[num].startx + 3;
    else
       x = menu_frame[num].startx + 1;

    y = menu_frame[num].starty + 1;
    width = menu_frame[num].endy - menu_frame[num].starty;
    if (width >= 1)
       width -= 1;
    menu_frame[num].top = 0;
    menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;

    sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
    PutVidStringToLength(menu_frame[num].screen,
			 buf,
			 y,
			 x,
			 BLUE | BGWHITE,
			 width);

    if (menu_frame[num].elementCount > menu_frame[num].windowSize && menu_frame[num].top)
    {
       PutVidChar(menu_frame[num].screen,
		  UP_CHAR,
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
    }
    else
    {
       PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize
       && menu_frame[num].bottom < menu_frame[num].elementCount)
    {
       PutVidChar(menu_frame[num].screen,
		  DOWN_CHAR,
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
    }
    else
    {
       PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
    }

    for (;;)
    {
       if (debuggerActive)
	  key = KeyboardPollProcedure();
       else
	  key = ReadKeyboard(menu_frame[num].screen);

       if (menu_frame[num].keyboardMask)
	  continue;

       sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
       PutVidStringToLength(menu_frame[num].screen,
			    buf,
			    y,
			    x + menu_frame[num].index,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);

       switch (key & 0x00FFFFFF)
       {

	  case ENTER:
	     if (menu_frame[num].elementFunction)
	     {
		(menu_frame[num].elementFunction)(menu_frame[num].screen,
						  menu_frame[num].elementValues[menu_frame[num].choice],
						  menu_frame[num].elementStrings[menu_frame[num].choice]);
	     }
	     else
		return (menu_frame[num].elementValues[menu_frame[num].choice]);
	     break;

	  case ESC:
	     if (menu_frame[num].warnFunction)
	     {
		register LONG retCode;

		retCode = (menu_frame[num].warnFunction)(menu_frame[num].screen);
		if (retCode)
		   return retCode;
		else
		   break;
	     }
	     else
		return -1;

	  case PG_UP:
	     for (i=0; i < menu_frame[num].windowSize - 1; i++)
	     {
		menu_frame[num].choice--;
		menu_frame[num].index--;

		if (menu_frame[num].index < 0)
		{
		   menu_frame[num].index = 0;
		   if (menu_frame[num].choice >= 0)
		   {
		      if (menu_frame[num].top)
			 menu_frame[num].top--;
		      menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		      scroll_menu(num, 0);
		   }
		}

		if (menu_frame[num].choice < 0)
		   menu_frame[num].choice = 0;

		sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
		PutVidStringToLength(menu_frame[num].screen,
				     buf,
				     y,
				     x + menu_frame[num].index,
				     menu_frame[num].fill_color | menu_frame[num].text_color,
				     width);
	     }
	     break;

	  case PG_DOWN:
	     for (i=0; i < menu_frame[num].windowSize - 1; i++)
	     {
		menu_frame[num].choice++;
		menu_frame[num].index++;

		if (menu_frame[num].index >= menu_frame[num].elementCount)
		   menu_frame[num].index--;

		if (menu_frame[num].index >= menu_frame[num].windowSize)
		{
		   menu_frame[num].index--;
		   if (menu_frame[num].choice < menu_frame[num].elementCount)
		   {
		      menu_frame[num].top++;
		      menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		      scroll_menu(num, 1);
		   }
		}

		if (menu_frame[num].choice >= menu_frame[num].elementCount)
		   menu_frame[num].choice--;

		sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
		PutVidStringToLength(menu_frame[num].screen,
				     buf,
				     y,
				     x + menu_frame[num].index,
				     menu_frame[num].fill_color | menu_frame[num].text_color,
				     width);
	     }
	     break;

	  case UP_ARROW:
	     menu_frame[num].choice--;
	     menu_frame[num].index--;

	     if (menu_frame[num].index < 0)
	     {
		menu_frame[num].index = 0;
		if (menu_frame[num].choice >= 0)
		{
		   if (menu_frame[num].top)
		      menu_frame[num].top--;
		   menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		   scroll_menu(num, 0);
		}
	     }

	     if (menu_frame[num].choice < 0)
		menu_frame[num].choice = 0;

	     break;

	  case SPACE:
	  case DOWN_ARROW:
	     menu_frame[num].choice++;
	     menu_frame[num].index++;

	     if (menu_frame[num].index >= menu_frame[num].elementCount)
		menu_frame[num].index--;

	     if (menu_frame[num].index >= menu_frame[num].windowSize)
	     {
		menu_frame[num].index--;
		if (menu_frame[num].choice < menu_frame[num].elementCount)
		{
		   menu_frame[num].top++;
		   menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		   scroll_menu(num, 1);
		}
	     }

	     if (menu_frame[num].choice >= menu_frame[num].elementCount)
		menu_frame[num].choice--;

	     break;

	  default:
	     break;
       }

       sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
       PutVidStringToLength(menu_frame[num].screen,
			    buf,
			    y,
			    x + menu_frame[num].index,
			    BLUE | BGWHITE,
			    width);

       if (menu_frame[num].elementCount > menu_frame[num].windowSize && menu_frame[num].top)
       {
	  PutVidChar(menu_frame[num].screen,
		  UP_CHAR,
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
       }
       else
       {
	  PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
       }

       if (menu_frame[num].elementCount > menu_frame[num].windowSize
	  && menu_frame[num].bottom < menu_frame[num].elementCount)
       {
	  PutVidChar(menu_frame[num].screen,
		  DOWN_CHAR,
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
       }
       else
       {
	  PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
       }
    }

}


LONG fill_menu(LONG num, LONG ch, LONG attr)
{
   register int i, j;
   BYTE *v;
   BYTE *t;

   v = menu_frame[num].screen->pVidMem;
   t = v;
   for (i=menu_frame[num].starty; i < menu_frame[num].endy + 1; i++)
   {
      for (j=menu_frame[num].startx; j < menu_frame[num].endx + 1; j++)
      {
	 v = t;
	 v += (j * menu_frame[num].screen->nCols * 2) + i * 2;
	 *v++ = (BYTE) ch;
	 *v = (BYTE) attr;
      }
   }
   return 0;

}

LONG save_menu(LONG num)
{
   register int i, j;
   BYTE *buf_ptr;
   BYTE *v;
   BYTE *t;

   buf_ptr = (BYTE *) &menu_frame[num].p[0];
   v = menu_frame[num].screen->pVidMem;
   for (i=menu_frame[num].starty; i < menu_frame[num].endy + 1; i++)
   {
      for (j=menu_frame[num].startx; j < menu_frame[num].endx + 1; j++)
      {
	 t = (v + (j * menu_frame[num].screen->nCols * 2) + i * 2);
	 *buf_ptr++ = *t++;
	 *buf_ptr++ = *t;
	 *(t - 1) = ' ';  // fill window
      }
   }
   return 0;

}

LONG restore_menu(LONG num)
{
   register int i, j;
   BYTE *buf_ptr;
   BYTE *v;
   BYTE *t;

   buf_ptr = (BYTE *) &menu_frame[num].p[0];
   v = menu_frame[num].screen->pVidMem;
   t = v;
   for (i=menu_frame[num].starty; i < menu_frame[num].endy + 1; i++)
   {
      for (j=menu_frame[num].startx; j < menu_frame[num].endx + 1; j++)
      {
	 v = t;
	 v += (j * menu_frame[num].screen->nCols * 2) + i * 2;
	 *v++ = *buf_ptr++;
	 *v = *buf_ptr++;
      }
      menu_frame[num].active = 0;
   }
   return 0;

}


LONG free_menu(LONG num)
{

   menu_frame[num].curx = 0;
   menu_frame[num].cury = 0;
   restore_menu(num);

//   SetXY(menu_frame[num].screen, menu_frame[num].pcurx, menu_frame[num].pcury);

   if (menu_frame[num].p)
      kfree((void *)menu_frame[num].p);

   if (menu_frame[num].elementStrings)
      kfree((void *)menu_frame[num].elementStrings);

   if (menu_frame[num].elementValues)
      kfree((void *)menu_frame[num].elementValues);

   menu_frame[num].p = 0;
   menu_frame[num].elementStrings = 0;
   menu_frame[num].elementValues = 0;
   menu_frame[num].elementCount = 0;
   menu_frame[num].elementFunction = 0;
   menu_frame[num].warnFunction = 0;
   menu_frame[num].owner = 0;

   return 0;

}

LONG display_menu_header(LONG num)
{

   register int y, len, i;

   y = menu_frame[num].starty;
   len = strlen((BYTE *) &menu_frame[num].header[0]);
   len = (menu_frame[num].endy - y - len) / 2;
   if (len < 0)
      return -1;

   y = y + len;

   // 185 <-
   // 204 ->
   // 205 -

   for (i=0; i < menu_frame[num].endy - menu_frame[num].starty; i++)
   {
      PutVidChar(menu_frame[num].screen,
		 205,
		 menu_frame[num].starty + i,
		 menu_frame[num].startx + 2,
		 menu_frame[num].border_color);
   }

   PutVidChar(menu_frame[num].screen,
	      204,
	      menu_frame[num].starty,
	      menu_frame[num].startx + 2,
	      menu_frame[num].border_color);

   PutVidChar(menu_frame[num].screen,
	      185,
	      menu_frame[num].endy,
	      menu_frame[num].startx + 2,
	      menu_frame[num].border_color);

   PutVidString(menu_frame[num].screen,
		menu_frame[num].header,
		y,
		menu_frame[num].startx + 1,
		menu_frame[num].header_color);

   return 0;


}

LONG draw_menu_border(LONG num)
{

   register int i;
   BYTE *v;
   BYTE *t;

   v = menu_frame[num].screen->pVidMem;
   t = v;

   for (i=menu_frame[num].startx + 1; i < menu_frame[num].endx; i++)
   {
      v += (i * menu_frame[num].screen->nCols * 2) + menu_frame[num].starty * 2;
      *v++ = 186;
      *v = menu_frame[num].border_color;
      v = t;
      v += (i * menu_frame[num].screen->nCols * 2) + menu_frame[num].endy * 2;
      *v++ = 186;
      *v = menu_frame[num].border_color;
      v = t;
   }
   for (i=menu_frame[num].starty + 1; i < menu_frame[num].endy; i++)
   {
      v += (menu_frame[num].startx * menu_frame[num].screen->nCols * 2) + (i * 2);
      *v++ = 205;
      *v = menu_frame[num].border_color;
      v = t;
      v += (menu_frame[num].endx * menu_frame[num].screen->nCols * 2) + (i * 2);
      *v++ = 205;
      *v = menu_frame[num].border_color;
      v = t;
   }

   PutVidChar(menu_frame[num].screen, 201, menu_frame[num].starty, menu_frame[num].startx,
	      menu_frame[num].border_color);
   PutVidChar(menu_frame[num].screen, 200, menu_frame[num].starty, menu_frame[num].endx,
	      menu_frame[num].border_color);
   PutVidChar(menu_frame[num].screen, 187, menu_frame[num].endy, menu_frame[num].startx,
	      menu_frame[num].border_color);
   PutVidChar(menu_frame[num].screen, 188, menu_frame[num].endy, menu_frame[num].endx,
	      menu_frame[num].border_color);

   return 0;


}

void display_menu(LONG num)
{

    register LONG i, x, count, width;
    register BYTE **m;

    if (strlen(menu_frame[num].header))
       x = menu_frame[num].startx + 3;
    else
       x = menu_frame[num].startx + 1;

    m = menu_frame[num].elementStrings;
    count = menu_frame[num].windowSize;
    width = menu_frame[num].endy - menu_frame[num].starty;
    if (width >= 1)
       width -= 1;

    for (i=0; i < count; i++)
    {
       BYTE buf[255];

       if (i < menu_frame[num].elementCount)
	  sprintf(buf, " %c%s", 179, m[i]);
       else
	  sprintf(buf, " %c", 179);

       PutVidStringToLength(menu_frame[num].screen,
			    buf,
			    menu_frame[num].starty + 1,
			    x + i,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);


    }

}

LONG add_item_to_menu(LONG num, BYTE *item, LONG value)
{

    if (menu_frame[num].owner && menu_frame[num].elementStrings
       && menu_frame[num].elementCount < 2048)
    {
       menu_frame[num].elementStrings[menu_frame[num].elementCount] = item;
       menu_frame[num].elementValues[menu_frame[num].elementCount++] = value;
       return 0;
    }
    return -1;

}

LONG activate_menu(LONG num)
{

   register LONG choice, len;
   register LONG i, retCode;

   GetXY(menu_frame[num].screen, (LONG *)&menu_frame[num].pcurx, (LONG *)&menu_frame[num].pcury);

   len = 0;
   for (i=0; i < menu_frame[num].elementCount; i++)
   {
      if (strlen(menu_frame[num].elementStrings[i]) > len)
	 len = strlen(menu_frame[num].elementStrings[i]);
   }

   if (menu_frame[num].header)
   {
      if (strlen(menu_frame[num].header) > len)
	 len = strlen(menu_frame[num].header);
   }

   menu_frame[num].endy = len + 3 + menu_frame[num].starty;

   if (strlen(menu_frame[num].header))
   {
      if (menu_frame[num].windowSize)
	 menu_frame[num].endx = menu_frame[num].windowSize + 3 \
				       + menu_frame[num].startx;
      else
	 menu_frame[num].endx = menu_frame[num].elementCount + 3 \
				       + menu_frame[num].startx;
   }
   else
   {
      if (menu_frame[num].windowSize)
	 menu_frame[num].endx = menu_frame[num].windowSize + 1 \
				       + menu_frame[num].startx;
      else
	 menu_frame[num].endx = menu_frame[num].elementCount + 1 \
				       + menu_frame[num].startx;

   }

   if (menu_frame[num].endx + 1 > menu_frame[num].screen->nLines - 1 ||
       menu_frame[num].endy + 1 > menu_frame[num].screen->nCols - 1)
   {
      return -1;
   }

   if (!menu_frame[num].active)
   {
      menu_frame[num].active = TRUE;
      save_menu(num);
      fill_menu(num, ' ', menu_frame[num].fill_color);
   }

   if (menu_frame[num].border)
   {
      draw_menu_border(num);
      display_menu_header(num);
   }

   display_menu(num);

   retCode = get_resp(num);

   restore_menu(num);

   return retCode;

}

LONG make_menu(SCREEN *screen,
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
	       LONG (*warnFunction)(SCREEN *))
{

   register int i;
   register LONG num;
   BYTE *p;

   for (num=1; num < MAX_MENU; num++)
   {
      if (!menu_frame[num].owner)
	 break;
   }

   if (!num || num > MAX_MENU)
      return 0;

   if (startx > screen->nLines - 1 || startx < 0 ||
       starty > screen->nCols - 2 || starty < 0)
      return 0;

   menu_frame[num].p = kmalloc(8192);
   if (!menu_frame[num].p)
      return 0;
   SetData((LONG *)menu_frame[num].p, 0, 8192);

   menu_frame[num].elementStrings = kmalloc(8192);
   if (!menu_frame[num].elementStrings)
   {
      if (menu_frame[num].p)
	 kfree((void *) menu_frame[num].p);
      return 0;
   }
   SetDataB((LONG *)menu_frame[num].elementStrings, 0, 8192);

   menu_frame[num].elementValues = kmalloc(8192);
   if (!menu_frame[num].elementValues)
   {
      if (menu_frame[num].elementStrings)
	 kfree((void *) menu_frame[num].elementStrings);
      if (menu_frame[num].p)
	 kfree((void *) menu_frame[num].p);
      return 0;
   }
   SetData((LONG *)menu_frame[num].elementValues, 0, 8192);

   for (i=0; i < 80; i++)
   {
      if (!header[i])
	 break;
      menu_frame[num].header[i] = header[i];
   }
   menu_frame[num].header[i] = 0x00;   // null terminate string

   menu_frame[num].startx = startx;
   menu_frame[num].endx = startx + 1;
   menu_frame[num].starty = starty;
   menu_frame[num].endy = starty + 1;
   menu_frame[num].screen = screen;
   menu_frame[num].border = border;
   menu_frame[num].num = num;
   menu_frame[num].active = 0;
   menu_frame[num].curx = 0;
   menu_frame[num].cury = 0;
   menu_frame[num].header_color = BRITEWHITE;
   menu_frame[num].border_color = BRITEWHITE;
   menu_frame[num].fill_color = BRITEWHITE;
   menu_frame[num].text_color = BRITEWHITE;
   menu_frame[num].windowSize = windowSize;
   menu_frame[num].elementFunction = elementFunction;
   menu_frame[num].elementCount = 0;
   menu_frame[num].warnFunction = warnFunction;
   if (hcolor)
      menu_frame[num].header_color = hcolor;
   if (bcolor)
      menu_frame[num].border_color = bcolor;
   if (fcolor)
      menu_frame[num].fill_color = fcolor;
   if (tcolor)
      menu_frame[num].text_color = tcolor;

   menu_frame[num].owner = 1;

   return num;


}

LONG menu_write_string(LONG num, BYTE *p, LONG x, LONG y, LONG attr)
{

   if (!menu_frame[num].active)
      return -1;

   PutVidString(menu_frame[num].screen,
		p,
		menu_frame[num].starty + 1 + y,
		menu_frame[num].startx + 1 + x,
		attr);

   return 0;

}

void scroll_portal(LONG num, LONG up)
{

    register LONG x, y;

    if (strlen(menu_frame[num].header))
       x = menu_frame[num].startx + 3;
    else
       x = menu_frame[num].startx + 1;
    y = menu_frame[num].starty + 1;

    ScrollDisplay(menu_frame[num].screen,
		  y,
		  x,
		  menu_frame[num].endy - y,
		  menu_frame[num].endx - x,
		  up);
    return;

}

LONG get_portal_resp(LONG num)
{

    BYTE buf[255];
    register long key, x, y, width;
    register LONG i;

    menu_frame[num].choice = 0;
    menu_frame[num].index = 0;

    if (strlen(menu_frame[num].header))
       x = menu_frame[num].startx + 3;
    else
       x = menu_frame[num].startx + 1;

    y = menu_frame[num].starty + 1;
    width = menu_frame[num].endy - menu_frame[num].starty;
    if (width >= 1)
       width -= 1;
    menu_frame[num].top = 0;
    menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;

    sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
    PutVidStringToLength(menu_frame[num].screen,
			 buf,
			 y,
			 x,
			 BLUE | BGWHITE,
			 width);

    if (menu_frame[num].elementCount > menu_frame[num].windowSize && menu_frame[num].top)
    {
       PutVidChar(menu_frame[num].screen,
		  UP_CHAR,
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
    }
    else
    {
       PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize
       && menu_frame[num].bottom < menu_frame[num].elementCount)
    {
       PutVidChar(menu_frame[num].screen,
		  DOWN_CHAR,
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
    }
    else
    {
       PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
    }

    menu_frame[num].selected = 1;

    for (;;)
    {
       if (debuggerActive)
	  key = KeyboardPollProcedure();
       else
	  key = ReadKeyboard(menu_frame[num].screen);

       if (menu_frame[num].keyboardMask)
	  continue;

       sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
       PutVidStringToLength(menu_frame[num].screen,
			    buf,
			    y,
			    x + menu_frame[num].index,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);


       switch (key & 0x00FFFFFF)
       {

	  case ESC:
	     if (menu_frame[num].warnFunction)
	     {
		register LONG retCode;

		retCode = (menu_frame[num].warnFunction)(menu_frame[num].screen);
		if (retCode)
		{
		   menu_frame[num].selected = 0;
		   return retCode;
		}
		else
		   break;
	     }
	     else
	     {
		menu_frame[num].selected = 0;
		return -1;
	     };

	  case PG_UP:
	     for (i=0; i < menu_frame[num].windowSize - 1; i++)
	     {
		menu_frame[num].choice--;
		menu_frame[num].index--;

		if (menu_frame[num].index < 0)
		{
		   menu_frame[num].index = 0;
		   if (menu_frame[num].choice >= 0)
		   {
		      if (menu_frame[num].top)
			 menu_frame[num].top--;
		      menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		      scroll_portal(num, 0);
		   }
		}

		if (menu_frame[num].choice < 0)
		   menu_frame[num].choice = 0;

		sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
		PutVidStringToLength(menu_frame[num].screen,
				     buf,
				     y,
				     x + menu_frame[num].index,
				     menu_frame[num].fill_color | menu_frame[num].text_color,
				     width);
	     }
	     break;

	  case PG_DOWN:
	     for (i=0; i < menu_frame[num].windowSize - 1; i++)
	     {
		menu_frame[num].choice++;
		menu_frame[num].index++;

		if (menu_frame[num].index >= menu_frame[num].elementCount)
		   menu_frame[num].index--;

		if (menu_frame[num].index >= menu_frame[num].windowSize)
		{
		   menu_frame[num].index--;
		   if (menu_frame[num].choice < menu_frame[num].elementCount)
		   {
		      menu_frame[num].top++;
		      menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		      scroll_portal(num, 1);
		   }
		}

		if (menu_frame[num].choice >= menu_frame[num].elementCount)
		   menu_frame[num].choice--;

		sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
		PutVidStringToLength(menu_frame[num].screen,
				     buf,
				     y,
				     x + menu_frame[num].index,
				     menu_frame[num].fill_color | menu_frame[num].text_color,
				     width);
	     }
	     break;

	  case UP_ARROW:
	     menu_frame[num].choice--;
	     menu_frame[num].index--;

	     if (menu_frame[num].index < 0)
	     {
		menu_frame[num].index = 0;
		if (menu_frame[num].choice >= 0)
		{
		   if (menu_frame[num].top)
		      menu_frame[num].top--;
		   menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		   scroll_menu(num, 0);
		}
	     }

	     if (menu_frame[num].choice < 0)
		menu_frame[num].choice = 0;

	     break;

	  case SPACE:
	  case DOWN_ARROW:
	     menu_frame[num].choice++;
	     menu_frame[num].index++;

	     if (menu_frame[num].index >= menu_frame[num].elementCount)
		menu_frame[num].index--;

	     if (menu_frame[num].index >= menu_frame[num].windowSize)
	     {
		menu_frame[num].index--;
		if (menu_frame[num].choice < menu_frame[num].elementCount)
		{
		   menu_frame[num].top++;
		   menu_frame[num].bottom = menu_frame[num].top + menu_frame[num].windowSize;
		   scroll_menu(num, 1);
		}
	     }

	     if (menu_frame[num].choice >= menu_frame[num].elementCount)
		menu_frame[num].choice--;

	     break;

	  case ENTER:
	  default:
	     if (menu_frame[num].keyboardHandler)
	     {
		register LONG retCode;

		retCode = (menu_frame[num].keyboardHandler)(menu_frame[num].screen,
							    key & 0xFF);
		if (retCode)
		   return (retCode);
	     }
	     break;
       }

       sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].choice]);
       PutVidStringToLength(menu_frame[num].screen,
			    buf,
			    y,
			    x + menu_frame[num].index,
			    BLUE | BGWHITE,
			    width);

       if (menu_frame[num].elementCount > menu_frame[num].windowSize && menu_frame[num].top)
       {
	  PutVidChar(menu_frame[num].screen,
		  UP_CHAR,
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
       }
       else
       {
	  PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
       }

       if (menu_frame[num].elementCount > menu_frame[num].windowSize
	  && menu_frame[num].bottom < menu_frame[num].elementCount)
       {
	  PutVidChar(menu_frame[num].screen,
		  DOWN_CHAR,
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
       }
       else
       {
	  PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
       }
    }

}


LONG free_portal(LONG num)
{

   menu_frame[num].curx = 0;
   menu_frame[num].cury = 0;
   menu_frame[num].selected = 0;

   restore_menu(num);

//   SetXY(menu_frame[num].screen, menu_frame[num].pcurx, menu_frame[num].pcury);

   if (menu_frame[num].p)
      kfree((void *)menu_frame[num].p);

   if (menu_frame[num].elementStrings)
      kfree((void *)menu_frame[num].elementStrings);

   if (menu_frame[num].elementValues)
      kfree((void *)menu_frame[num].elementValues);

   if (menu_frame[num].elementStorage)
      kfree((void *)menu_frame[num].elementStorage);

   menu_frame[num].p = 0;
   menu_frame[num].elementStrings = 0;
   menu_frame[num].elementValues = 0;
   menu_frame[num].elementStorage = 0;
   menu_frame[num].elementCount = 0;
   menu_frame[num].elementFunction = 0;
   menu_frame[num].warnFunction = 0;
   menu_frame[num].owner = 0;

   return 0;

}

LONG add_item_to_portal(LONG num, BYTE *item, LONG index)
{

    if (menu_frame[num].elementStrings)
    {
       menu_frame[num].elementStrings[index] = item;
       return 0;
    }
    return -1;

}

LONG display_portal_header(LONG num)
{

   register int y, len, i;

   y = menu_frame[num].starty;
   len = strlen((BYTE *) &menu_frame[num].header[0]);
   len = (menu_frame[num].endy - y - len) / 2;
   if (len < 0)
      return -1;

   y = y + len;

   for (i=0; i < menu_frame[num].endy - menu_frame[num].starty; i++)
   {
      PutVidChar(menu_frame[num].screen,
		 196,
		 menu_frame[num].starty + i,
		 menu_frame[num].startx + 2,
		 menu_frame[num].border_color);
   }

   PutVidChar(menu_frame[num].screen,
	      195,
	      menu_frame[num].starty,
	      menu_frame[num].startx + 2,
	      menu_frame[num].border_color);

   PutVidChar(menu_frame[num].screen,
	      180,
	      menu_frame[num].endy,
	      menu_frame[num].startx + 2,
	      menu_frame[num].border_color);

   PutVidString(menu_frame[num].screen,
		menu_frame[num].header,
		y,
		menu_frame[num].startx + 1,
		menu_frame[num].header_color);

   return 0;


}

LONG draw_portal_border(LONG num)
{

   register int i;
   BYTE *v;
   BYTE *t;

   v = menu_frame[num].screen->pVidMem;
   t = v;

   for (i=menu_frame[num].startx + 1; i < menu_frame[num].endx; i++)
   {
      v += (i * menu_frame[num].screen->nCols * 2) + menu_frame[num].starty * 2;
      *v++ = 179;
      *v = menu_frame[num].border_color;
      v = t;
      v += (i * menu_frame[num].screen->nCols * 2) + menu_frame[num].endy * 2;
      *v++ = 179;
      *v = menu_frame[num].border_color;
      v = t;
   }
   for (i=menu_frame[num].starty + 1; i < menu_frame[num].endy; i++)
   {
      v += (menu_frame[num].startx * menu_frame[num].screen->nCols * 2) + (i * 2);
      *v++ = 196;
      *v = menu_frame[num].border_color;
      v = t;
      v += (menu_frame[num].endx * menu_frame[num].screen->nCols * 2) + (i * 2);
      *v++ = 196;
      *v = menu_frame[num].border_color;
      v = t;
   }

   PutVidChar(menu_frame[num].screen, 218, menu_frame[num].starty, menu_frame[num].startx,
	      menu_frame[num].border_color);
   PutVidChar(menu_frame[num].screen, 192, menu_frame[num].starty, menu_frame[num].endx,
	      menu_frame[num].border_color);
   PutVidChar(menu_frame[num].screen, 191, menu_frame[num].endy, menu_frame[num].startx,
	      menu_frame[num].border_color);
   PutVidChar(menu_frame[num].screen, 217, menu_frame[num].endy, menu_frame[num].endx,
	      menu_frame[num].border_color);

   return 0;


}

void display_portal(LONG num)
{

    register LONG i, x, count, width;
    register BYTE **m;

    if (strlen(menu_frame[num].header))
       x = menu_frame[num].startx + 3;
    else
       x = menu_frame[num].startx + 1;

    m = menu_frame[num].elementStrings;
    count = menu_frame[num].windowSize;
    width = menu_frame[num].endy - menu_frame[num].starty;
    if (width >= 1)
       width -= 1;

    for (i=0; i < count; i++)
    {
       BYTE buf[255];

       if (i < menu_frame[num].elementCount)
	  sprintf(buf, " %c%s", 179, m[i]);
       else
	  sprintf(buf, " %c", 179);

       PutVidStringToLength(menu_frame[num].screen,
			    buf,
			    menu_frame[num].starty + 1,
			    x + i,
			    menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);


    }

}

LONG update_portal(LONG num)
{

    register LONG i, x, y, count, width;
    register BYTE buf[255];

    if (!menu_frame[num].active)
       return -1;

    if (!menu_frame[num].selected)
       return -1;

    if (strlen(menu_frame[num].header))
       x = menu_frame[num].startx + 3;
    else
       x = menu_frame[num].startx + 1;

    y = menu_frame[num].starty + 1;
    width = menu_frame[num].endy - menu_frame[num].starty;
    if (width >= 1)
       width -= 1;

    for (i=0; i < menu_frame[num].windowSize; i++)
    {
       sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].top + i]);
       PutVidStringToLength(menu_frame[num].screen,
			    buf,
			    menu_frame[num].starty + 1,
			    x + i,
			    (x + i == x + menu_frame[num].index)
			    ? BLUE | BGWHITE
			    : menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize && menu_frame[num].top)
    {
	  PutVidChar(menu_frame[num].screen,
		  UP_CHAR,
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
    }
    else
    {
	  PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize
	  && menu_frame[num].bottom < menu_frame[num].elementCount)
    {
	  PutVidChar(menu_frame[num].screen,
		  DOWN_CHAR,
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
    }
    else
    {
	  PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
    }

    return 0;

}

LONG update_static_portal(LONG num)
{

    register LONG i, x, y, count, width;
    register BYTE buf[255];

    if (!menu_frame[num].active)
       return -1;

    if (strlen(menu_frame[num].header))
       x = menu_frame[num].startx + 3;
    else
       x = menu_frame[num].startx + 1;

    y = menu_frame[num].starty + 1;
    width = menu_frame[num].endy - menu_frame[num].starty;
    if (width >= 1)
       width -= 1;

    for (i=0; i < menu_frame[num].windowSize; i++)
    {
       sprintf(buf, " %c%s", 179, menu_frame[num].elementStrings[menu_frame[num].top + i]);
       PutVidStringToLength(menu_frame[num].screen,
			    buf,
			    menu_frame[num].starty + 1,
			    x + i,
			    (x + i == x + menu_frame[num].index)
//			    ? BLUE | BGWHITE
			    ? menu_frame[num].fill_color | menu_frame[num].text_color
			    : menu_frame[num].fill_color | menu_frame[num].text_color,
			    width);
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize && menu_frame[num].top)
    {
	  PutVidChar(menu_frame[num].screen,
		  UP_CHAR,
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
    }
    else
    {
	  PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x,
		  GetVidCharAttr(menu_frame[num].screen, y, x));
    }

    if (menu_frame[num].elementCount > menu_frame[num].windowSize
	  && menu_frame[num].bottom < menu_frame[num].elementCount)
    {
	  PutVidChar(menu_frame[num].screen,
		  DOWN_CHAR,
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
    }
    else
    {
	  PutVidChar(menu_frame[num].screen,
		  ' ',
		  y,
		  x + menu_frame[num].windowSize - 1,
		  GetVidCharAttr(menu_frame[num].screen, y,
				 x + menu_frame[num].windowSize - 1));
    }

    return 0;

}

LONG activate_portal(LONG num)
{

   register LONG choice, len;
   register LONG i, retCode;

   GetXY(menu_frame[num].screen, (LONG *)&menu_frame[num].pcurx, (LONG *)&menu_frame[num].pcury);

   if (!menu_frame[num].active)
   {
      menu_frame[num].active = TRUE;
      save_menu(num);
      fill_menu(num, ' ', menu_frame[num].fill_color);
      if (menu_frame[num].border)
      {
	 draw_portal_border(num);
	 display_portal_header(num);
      }
   }

   display_portal(num);

   retCode = get_portal_resp(num);

   restore_menu(num);

   return retCode;

}

LONG activate_static_portal(LONG num)
{

   register LONG choice, len;
   register LONG i;

   GetXY(menu_frame[num].screen, (LONG *)&menu_frame[num].pcurx, (LONG *)&menu_frame[num].pcury);

   if (!menu_frame[num].active)
   {
      menu_frame[num].active = TRUE;
      save_menu(num);
      fill_menu(num, ' ', menu_frame[num].fill_color);
      if (menu_frame[num].border)
      {
	 draw_portal_border(num);
	 display_portal_header(num);
      }
   }

   display_portal(num);
   return 0;

}

LONG deactivate_static_portal(LONG num)
{
   restore_menu(num);
   return 0;
}

LONG make_portal(SCREEN *screen,
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
		 LONG (*keyboardHandler)(SCREEN *, LONG))
{

   register LONG i;
   register LONG num;
   BYTE *p;

   for (num=1; num < MAX_MENU; num++)
   {
      if (!menu_frame[num].owner)
	 break;
   }

   if (!num || num > MAX_MENU)
      return 0;

   if (startx > screen->nLines - 1 || startx < 0 ||
       starty > screen->nCols - 2 || starty < 0)
      return 0;

   menu_frame[num].p = kmalloc(screen->nLines * screen->nCols * 2);
   if (!menu_frame[num].p)
      return 0;

   menu_frame[num].elementStorage = kmalloc(numberOfLines * screen->nCols);
   if (!menu_frame[num].elementStorage)
   {
      if (menu_frame[num].p)
	 kfree((void *) menu_frame[num].p);
      return 0;
   }

   menu_frame[num].elementStrings = kmalloc(numberOfLines * sizeof(LONG));
   if (!menu_frame[num].elementStrings)
   {
      if (menu_frame[num].p)
	 kfree((void *) menu_frame[num].p);
      if (menu_frame[num].elementStorage)
	 kfree((void *) menu_frame[num].elementStorage);
      return 0;
   }

   menu_frame[num].elementValues = kmalloc(numberOfLines * sizeof(LONG));
   if (!menu_frame[num].elementValues)
   {
      if (menu_frame[num].elementStrings)
	 kfree((void *) menu_frame[num].elementStrings);
      if (menu_frame[num].p)
	 kfree((void *) menu_frame[num].p);
      if (menu_frame[num].elementStorage)
	 kfree((void *) menu_frame[num].elementStorage);
      return 0;
   }

   SetDataB((LONG *) menu_frame[num].elementStorage, 0x20, numberOfLines * screen->nCols);
   for (i=0; i < numberOfLines + 1; i++)
   {
      register BYTE *p = &menu_frame[num].elementStorage[i * screen->nCols];
      add_item_to_portal(num, p, i);
      p[screen->nCols - 1] = '\0';
   }

   for (i=0; i < 80; i++)
   {
      if (!header[i])
	 break;
      menu_frame[num].header[i] = header[i];
   }
   menu_frame[num].header[i] = 0x00;   // null terminate string

   menu_frame[num].startx = startx;
   menu_frame[num].endx = endx;
   menu_frame[num].starty = starty;
   menu_frame[num].endy = endy;
   menu_frame[num].screen = screen;
   menu_frame[num].border = border;
   menu_frame[num].num = num;
   menu_frame[num].active = 0;
   menu_frame[num].curx = 0;
   menu_frame[num].cury = 0;
   menu_frame[num].selected = 0;
   menu_frame[num].header_color = BRITEWHITE;
   menu_frame[num].border_color = BRITEWHITE;
   menu_frame[num].fill_color = BRITEWHITE;
   menu_frame[num].text_color = BRITEWHITE;

   if (keyboardHandler)
      menu_frame[num].keyboardHandler = keyboardHandler;

   if (!menu_frame[num].header)
      menu_frame[num].windowSize = endx - startx - 1;
   else
      menu_frame[num].windowSize = endx - startx - 3;

   menu_frame[num].elementCount = numberOfLines;
   if (hcolor)
      menu_frame[num].header_color = hcolor;
   if (bcolor)
      menu_frame[num].border_color = bcolor;
   if (fcolor)
      menu_frame[num].fill_color = fcolor;
   if (tcolor)
      menu_frame[num].text_color = tcolor;

   menu_frame[num].owner = 1;

   return num;


}

LONG write_portal(LONG num, BYTE *p, int x, int y, LONG attr)
{

   register LONG i;
   register BYTE *v;

   if (!menu_frame[num].owner)
      return -1;

   if (attr) {};

   if (x > menu_frame[num].elementCount)
      return -1;

   if (y > menu_frame[num].screen->nCols || !*p)
      return -1;

   if (menu_frame[num].elementStrings)
   {
      v = menu_frame[num].elementStrings[x];
      for (i=0; i < menu_frame[num].screen->nCols; i++)
      {
	 if (*p && i >= y)
	    *v = *p++;
	 if (*v == '\0')
	    *v = ' ';
	 v++;
      }
      menu_frame[num].elementStrings[x][menu_frame[num].screen->nCols - 1] = '\0';
      return 0;
   }
   return -1;

}

LONG write_portal_cleol(LONG num, BYTE *p, int x, int y, LONG attr)
{

   register LONG i;
   register BYTE *v;

   if (!menu_frame[num].owner)
      return -1;

   if (attr) {};

   if (x > menu_frame[num].elementCount)
      return -1;

   if (y > menu_frame[num].screen->nCols || !*p)
      return -1;

   if (menu_frame[num].elementStrings)
   {
      v = menu_frame[num].elementStrings[x];
      for (i=0; i < menu_frame[num].screen->nCols; i++)
      {
	 if (*p && i >= y)
	    *v = *p++;
	 if (*v == '\0')
	    *v = ' ';
	 v++;
      }
      menu_frame[num].elementStrings[x][menu_frame[num].screen->nCols - 1] = '\0';
      return 0;
   }
   return -1;

}

LONG write_screen_comment_line(SCREEN *screen, BYTE *p, LONG attr)
{

    PutVidStringCleol(screen,
		      p,
		      screen->nLines - 1,
		      attr);
    return 0;

}

LONG disable_portal_input(LONG num)
{
   register LONG retCode = menu_frame[num].keyboardMask;
   menu_frame[num].keyboardMask = TRUE;
   return retCode;
}

LONG enable_portal_input(LONG num)
{
   register LONG retCode = menu_frame[num].keyboardMask;
   menu_frame[num].keyboardMask = 0;
   return retCode;
}



