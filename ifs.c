

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  IFS.H
*   DESCRIP  :  Installable File System for MANOS v1.0
*   DATE     :  November 23, 1997
*
*
***************************************************************************/

#include "version.h"
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
#include "timer.h"
#include "peexe.h"
#include "malloc.h"
#include "free.h"
#include "ifs.h"
#include "dosfile.h"

LONG DefaultFileSystem = MSDOS_FS;
IFS IFS_TABLE[MAX_FILESYSTEMS];

LONG IFSUnsup(void)
{
   return (LONG) -1;
}

LONG IFSNull(void)
{
   return (LONG) 0;
}

void InitializeIFS(void)
{
   register int i;

   SetData((LONG *) &IFS_TABLE[0], 0, MAX_FILESYSTEMS * sizeof(IFS));

   IFS_TABLE[MSDOS_FS].FSType = MSDOS_FS;
   IFS_TABLE[MSDOS_FS].BindParameters = REAL_MODE_FS;
   IFS_TABLE[MSDOS_FS].AllocatedFS = -1;

   IFS_TABLE[MSDOS_FS].OpenFile = DOSOpen;
   IFS_TABLE[MSDOS_FS].FileSize = (LONG (*)(BYTE *))IFSUnsup;
   IFS_TABLE[MSDOS_FS].FileSizeHandled = DOSFileSize;
   IFS_TABLE[MSDOS_FS].RenameFile = (LONG (*)(BYTE *, BYTE *)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].RenameFileHandled = (LONG (*)(LONG, BYTE *))IFSUnsup;
   IFS_TABLE[MSDOS_FS].DeleteFile = (LONG (*)(BYTE *))DOSDelete;
   IFS_TABLE[MSDOS_FS].DeleteFileHandled = (LONG (*)(LONG))IFSUnsup;
   IFS_TABLE[MSDOS_FS].CreateFile = DOSCreate;
   IFS_TABLE[MSDOS_FS].CloseFile = DOSClose;
   IFS_TABLE[MSDOS_FS].ReadFile = DOSRead;
   IFS_TABLE[MSDOS_FS].WriteFile = DOSWrite;
   IFS_TABLE[MSDOS_FS].FileSeek = DOSLseek;
   IFS_TABLE[MSDOS_FS].FlushCache = (LONG (*)(LONG))DOSFlush;
   IFS_TABLE[MSDOS_FS].DirectOpenFile = DOSOpen;
   IFS_TABLE[MSDOS_FS].DirectRenameFile = (LONG (*)(BYTE *, BYTE *)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].DirectRenameFileHandled = (LONG (*)(LONG, BYTE *))IFSUnsup;
   IFS_TABLE[MSDOS_FS].DirectDeleteFile = (LONG (*)(BYTE *))DOSDelete;
   IFS_TABLE[MSDOS_FS].DirectDeleteFileHandled = (LONG (*)(LONG))IFSUnsup;
   IFS_TABLE[MSDOS_FS].DirectCreateFile = DOSCreate;
   IFS_TABLE[MSDOS_FS].DirectCloseFile = DOSClose;
   IFS_TABLE[MSDOS_FS].DirectReadFile = DOSRead;
   IFS_TABLE[MSDOS_FS].DirectWriteFile = DOSWrite;
   IFS_TABLE[MSDOS_FS].DirectFileSeek = DOSLseek;
   IFS_TABLE[MSDOS_FS].FileLock = (LONG (*)(LONG, LONG, LONG)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].FileUnlock = (LONG (*)(LONG)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].DirectFileLock = (LONG (*)(LONG, LONG, LONG)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].DirectFileUnlock = (LONG (*)(LONG)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].FindFirst = (LONG (*)(BYTE **)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].FindNext = (LONG (*)(BYTE **)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].StartTransaction = (LONG (*)(LONG)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].EndTransaction = (LONG (*)(LONG)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].AbortTransaction = (LONG (*)(LONG)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].CreateDirectory = (LONG (*)(BYTE *Path)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].DeleteDirectory = (LONG (*)(BYTE *Path)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].ChangeDirectory = (LONG (*)(BYTE *Path)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].RenameDirectory = (LONG (*)(BYTE *oldPath, BYTE *newPath)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].MapPathToDirectory = (LONG (*)(BYTE *Path)) IFSUnsup;
   IFS_TABLE[MSDOS_FS].MapDirectoryToPath = (BYTE *(*)(LONG Directory)) IFSNull;
   return;

}

//  default IFS FileSystemCalls

LONG IFSGetDefaultFileSystem(void)
{ return DefaultFileSystem; }

LONG IFSOpenFile(BYTE *name, LONG flags)
{ return (IFS_TABLE[DefaultFileSystem].OpenFile(name, flags));  }

LONG IFSRenameFile(BYTE *cName, BYTE *nName)
{ return (IFS_TABLE[DefaultFileSystem].RenameFile(cName, nName));  }

LONG IFSRenameFileHandled(LONG fd, BYTE *nName)
{ return (IFS_TABLE[DefaultFileSystem].RenameFileHandled(fd, nName));  }

LONG IFSDeleteFile(BYTE *name)
{ return (IFS_TABLE[DefaultFileSystem].DeleteFile(name)); }

LONG IFSDeleteFileHandled(LONG fd)
{ return (IFS_TABLE[DefaultFileSystem].DeleteFileHandled(fd)); }

LONG IFSCreateFile(BYTE *name, LONG flags)
{ return (IFS_TABLE[DefaultFileSystem].CreateFile(name, flags)); }

LONG IFSCloseFile(LONG fd)
{ return (IFS_TABLE[DefaultFileSystem].CloseFile(fd)); }

LONG IFSReadFile(LONG fd, void *buf, LONG size)
{ return (IFS_TABLE[DefaultFileSystem].ReadFile(fd, buf, size)); }

LONG IFSWriteFile(LONG fd, void *buf, LONG size)
{ return (IFS_TABLE[DefaultFileSystem].WriteFile(fd, buf, size));  }

LONG IFSFileSeek(LONG fd, LONG offset, LONG where)
{ return (IFS_TABLE[DefaultFileSystem].FileSeek(fd, offset, where)); }

LONG IFSFlushCache(LONG fd)
{ return (IFS_TABLE[DefaultFileSystem].FlushCache(fd)); }

LONG IFSDirectOpenFile(BYTE *name, LONG flags)
{ return (IFS_TABLE[DefaultFileSystem].DirectOpenFile(name, flags)); }

LONG IFSDirectRenameFile(BYTE *cName, BYTE *nName)
{ return (IFS_TABLE[DefaultFileSystem].DirectRenameFile(cName, nName)); }

LONG IFSDirectRenameFileHandled(LONG fd, BYTE *nName)
{ return (IFS_TABLE[DefaultFileSystem].DirectRenameFileHandled(fd, nName)); }

LONG IFSDirectDeleteFile(BYTE *name)
{ return (IFS_TABLE[DefaultFileSystem].DirectDeleteFile(name)); }

LONG IFSDirectDeleteFileHandled(LONG fd)
{ return (IFS_TABLE[DefaultFileSystem].DirectDeleteFileHandled(fd)); }

LONG IFSDirectCreateFile(BYTE *name, LONG flags)
{ return (IFS_TABLE[DefaultFileSystem].DirectCreateFile(name, flags));  }

LONG IFSDirectCloseFile(LONG fd)
{ return (IFS_TABLE[DefaultFileSystem].DirectCloseFile(fd)); }

LONG IFSDirectReadFile(LONG fd, void *buf, LONG size)
{ return (IFS_TABLE[DefaultFileSystem].DirectReadFile(fd, buf, size));  }

LONG IFSDirectWriteFile(LONG fd, void *buf, LONG size)
{ return (IFS_TABLE[DefaultFileSystem].DirectWriteFile(fd, buf, size));  }

LONG IFSDirectFileSeek(LONG fd, LONG offset, LONG where)
{ return (IFS_TABLE[DefaultFileSystem].DirectFileSeek(fd, offset, where)); }

LONG IFSFileSize(BYTE *name)
{   return (IFS_TABLE[DefaultFileSystem].FileSize(name));  }

LONG IFSFileSizeHandled(LONG fd)
{   return (IFS_TABLE[DefaultFileSystem].FileSizeHandled(fd));  }

LONG IFSFindFirst(BYTE **retBuffer)
{   return (IFS_TABLE[DefaultFileSystem].FindFirst(retBuffer));  }

LONG IFSFindNext(BYTE **retBuffer)
{   return (IFS_TABLE[DefaultFileSystem].FindNext(retBuffer)); }

LONG IFSFileLock(LONG fd, LONG offset, LONG size)
{   return (IFS_TABLE[DefaultFileSystem].FileLock(fd, offset, size));   }

LONG IFSFileUnlock(LONG lid)
{   return (IFS_TABLE[DefaultFileSystem].FileUnlock(lid));   }

LONG IFSDirectFileLock(LONG fd, LONG offset, LONG size)
{   return (IFS_TABLE[DefaultFileSystem].DirectFileLock(fd, offset, size));   }

LONG IFSDirectFileUnlock(LONG lid)
{   return (IFS_TABLE[DefaultFileSystem].DirectFileUnlock(lid));   }

LONG IFSStartTransaction(LONG fd)
{   return (IFS_TABLE[DefaultFileSystem].StartTransaction(fd));   }

LONG IFSEndTransaction(LONG tid)
{   return (IFS_TABLE[DefaultFileSystem].EndTransaction(tid));   }

LONG IFSAbortTransaction(LONG tid)
{   return (IFS_TABLE[DefaultFileSystem].AbortTransaction(tid));   }

LONG IFSCreateDirectory(BYTE *Path)
{   return (IFS_TABLE[DefaultFileSystem].CreateDirectory(Path));   }

LONG IFSDeleteDirectory(BYTE *Path)
{   return (IFS_TABLE[DefaultFileSystem].DeleteDirectory(Path));   }

LONG IFSChangeDirectory(BYTE *Path)
{   return (IFS_TABLE[DefaultFileSystem].ChangeDirectory(Path));   }

LONG IFSRenameDirectory(BYTE *oldPath, BYTE *newPath)
{   return (IFS_TABLE[DefaultFileSystem].RenameDirectory(oldPath, newPath));   }

LONG IFSMapPathToDirectory(BYTE *Path)
{   return (IFS_TABLE[DefaultFileSystem].MapPathToDirectory(Path));   }

BYTE *IFSMapDirectoryToPath(LONG Directory)
{   return (IFS_TABLE[DefaultFileSystem].MapDirectoryToPath(Directory));   }


// virtual file system calls

LONG VFSOpenFile(LONG fsid, BYTE *name, LONG flags)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].OpenFile(name, flags)
	  : (LONG) -1);  }

LONG VFSRenameFile(LONG fsid, BYTE *cName, BYTE *nName)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].RenameFile(cName, nName)
	  : (LONG) -1);  }

LONG VFSRenameFileHandled(LONG fsid, LONG fd, BYTE *nName)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].RenameFileHandled(fd, nName)
	  : (LONG) -1);  }

LONG VFSDeleteFile(LONG fsid, BYTE *name)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DeleteFile(name)
	  : (LONG) -1);  }

LONG VFSDeleteFileHandled(LONG fsid, LONG fd)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DeleteFileHandled(fd)
	  : (LONG) -1);  }

LONG VFSCreateFile(LONG fsid, BYTE *name, LONG flags)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].CreateFile(name, flags)
	  : (LONG) -1);  }

LONG VFSCloseFile(LONG fsid, LONG fd)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].CloseFile(fd)
	  : (LONG) -1);  }

LONG VFSReadFile(LONG fsid, LONG fd, void *buf, LONG size)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].ReadFile(fd, buf, size)
	  : (LONG) -1);  }

LONG VFSWriteFile(LONG fsid, LONG fd, void *buf, LONG size)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].WriteFile(fd, buf, size)
	  : (LONG) -1);  }

LONG VFSFileSeek(LONG fsid, LONG fd, LONG offset, LONG where)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].FileSeek(fd, offset, where)
	  : (LONG) -1);  }

LONG VFSFlushCache(LONG fsid, LONG fd)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].FlushCache(fd)
	  : (LONG) -1);  }

LONG VFSDirectOpenFile(LONG fsid, BYTE *name, LONG flags)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectOpenFile(name, flags)
	  : (LONG) -1);  }

LONG VFSDirectRenameFile(LONG fsid, BYTE *cName, BYTE *nName)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectRenameFile(cName, nName)
	  : (LONG) -1);  }

LONG VFSDirectRenameFileHandled(LONG fsid, LONG fd, BYTE *nName)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectRenameFileHandled(fd, nName)
	  : (LONG) -1);  }

LONG VFSDirectDeleteFile(LONG fsid, BYTE *name)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectDeleteFile(name)
	  : (LONG) -1);  }

LONG VFSDirectDeleteFileHandled(LONG fsid, LONG fd)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectDeleteFileHandled(fd)
	  : (LONG) -1);  }

LONG VFSDirectCreateFile(LONG fsid, BYTE *name, LONG flags)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectCreateFile(name, flags)
	  : (LONG) -1);  }

LONG VFSDirectCloseFile(LONG fsid, LONG fd)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectCloseFile(fd)
	  : (LONG) -1);  }

LONG VFSDirectReadFile(LONG fsid, LONG fd, void *buf, LONG size)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectReadFile(fd, buf, size)
	  : (LONG) -1);  }

LONG VFSDirectWriteFile(LONG fsid, LONG fd, void *buf, LONG size)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectWriteFile(fd, buf, size)
	  : (LONG) -1);  }

LONG VFSDirectFileSeek(LONG fsid, LONG fd, LONG offset, LONG where)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectFileSeek(fd, offset, where)
	  : (LONG) -1);  }

LONG VFSFileSize(LONG fsid, BYTE *name)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].FileSize(name)
	  : (LONG) -1);  }

LONG VFSFileSizeHandled(LONG fsid, LONG fd)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].FileSizeHandled(fd)
	  : (LONG) -1);  }

LONG VFSFindFirst(LONG fsid, BYTE **retBuffer)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].FindFirst(retBuffer)
	  : (LONG) -1);  }

LONG VFSFindNext(LONG fsid, BYTE **retBuffer)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].FindNext(retBuffer)
	  : (LONG) -1);  }

LONG VFSFileLock(LONG fsid, LONG fd, LONG offset, LONG size)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].FileLock(fd, offset, size)
	  : (LONG) -1);  }

LONG VFSFileUnlock(LONG fsid, LONG lid)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].FileUnlock(lid)
	  : (LONG) -1);  }

LONG VFSDirectFileLock(LONG fsid, LONG fd, LONG offset, LONG size)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectFileLock(fd, offset, size)
	  : (LONG) -1);  }

LONG VFSDirectFileUnlock(LONG fsid, LONG lid)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DirectFileUnlock(lid)
	  : (LONG) -1);  }

LONG VFSStartTransaction(LONG fsid, LONG fd)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].StartTransaction(fd)
	  : (LONG) -1);  }

LONG VFSEndTransaction(LONG fsid, LONG tid)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].EndTransaction(tid)
	  : (LONG) -1);  }

LONG VFSAbortTransaction(LONG fsid, LONG tid)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].AbortTransaction(tid)
	  : (LONG) -1);  }

LONG VFSCreateDirectory(LONG fsid, BYTE *Path)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].CreateDirectory(Path)
	  : (LONG) -1);  }

LONG VFSDeleteDirectory(LONG fsid, BYTE *Path)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].DeleteDirectory(Path)
	  : (LONG) -1);  }

LONG VFSChangeDirectory(LONG fsid, BYTE *Path)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].ChangeDirectory(Path)
	  : (LONG) -1);  }

LONG VFSRenameDirectory(LONG fsid, BYTE *oldPath, BYTE *newPath)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].RenameDirectory(oldPath, newPath)
	  : (LONG) -1);  }

LONG VFSMapPathToDirectory(LONG fsid, BYTE *Path)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].MapPathToDirectory(Path)
	  : (LONG) -1);  }

BYTE *VFSMapDirectoryToPath(LONG fsid, LONG Directory)
{ return (fsid < MAX_FILESYSTEMS && IFS_TABLE[fsid].AllocatedFS == (LONG) -1
	  ? IFS_TABLE[fsid].MapDirectoryToPath(Directory)
	  : (LONG) 0);  }

