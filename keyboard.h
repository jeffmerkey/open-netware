

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  KEYBOARD.H
*   DESCRIP  :  Keyboard Code Defines for MANOS v1.0
*   DATE     :  December 23, 1997
*
*
***************************************************************************/

#include "types.h"

//
//  keyboard scan code values after translation
//

#define   F1         0x0F
#define   F2         0x10
#define   F3         0x11
#define   F4         0x12
#define   F5         0x13
#define   F6         0x14
#define   F7         0x15
#define   F8         0x16
#define   F9         0x17
#define   F10        0x18
#define   F11        0x19
#define   F12        0x1A

#define   INS        0x0E
#define   DEL        0x7F
#define   ENTER      0x0D
#define   ESC        0x1B
#define   BKSP       0x08
#define   SPACE      0x20
#define   TAB        0x09

#define   HOME       0x06
#define   END        0x0B
#define   PG_UP      0x05
#define   PG_DOWN    0x0C

#define   UP_ARROW     0x01
#define   DOWN_ARROW   0x02
#define   LEFT_ARROW   0x03
#define   RIGHT_ARROW  0x04

#define   SCROLL_LOCK_MASK       0x00010000
#define   NUM_LOCK_MASK          0x00020000
#define   CAPS_LOCK_MASK         0x00040000
#define   LEFT_CTRL_MASK         0x00000100
#define   RIGHT_CTRL_MASK        0x00000200
#define   LEFT_SHIFT_MASK        0x00000400
#define   RIGHT_SHIFT_MASK       0x00000800
#define   LEFT_ALT_MASK          0x00001000
#define   RIGHT_ALT_MASK         0x00002000
#define   NUMERIC_PAD_MASK       0x01000000

extern LONG KeyboardPollProcedure(void);
extern void ResetKeyboard(void);
extern void KeyboardEnable(void);
extern void KeyboardDisable(void);

