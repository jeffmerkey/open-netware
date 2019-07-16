
#include "types.h"

typedef struct _NAME_CROSS_INDEX {
   BYTE *Name;
   LONG Index;
} NAME_CROSS_INDEX;

typedef struct _SOURCE_LINE_INFO {
   struct _SOURCE_LINE_INFO *next;
   struct _SOURCE_LINE_INFO *prior;
   BYTE *ModuleName;
   LONG SourcePresent;
   BYTE *SourceLine;
   LONG Offset;
   LONG LineNumber;
   LONG Segment;
   LONG ModuleIndex;
   LONG *LineTable;
   LONG LineAddress;
   LONG FileIndex;
   LONG SourceIndex;
} SOURCE_LINE_INFO;


