

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

#include "types.h"

#define MAX_FILESYSTEMS        20

#define MSDOS_FS               0x00000000
#define FASTFAT_FS             0x00000001
#define VNDI_FS                0x00000002

#define MULTITHREAD_FS         0x00000010
#define REAL_MODE_FS           0x00000011

typedef struct _IFS_STRUCTURE
{
   LONG FSType;
   LONG BindParameters;
   LONG AllocatedFS;
   LONG (*OpenFile)(BYTE *, LONG);
   LONG (*RenameFile)(BYTE *, BYTE *);
   LONG (*RenameFileHandled)(LONG, BYTE *);
   LONG (*DeleteFile)(BYTE *);
   LONG (*DeleteFileHandled)(LONG);
   LONG (*CreateFile)(BYTE *, LONG);
   LONG (*CloseFile)(LONG);
   LONG (*ReadFile)(LONG, void *, LONG);
   LONG (*WriteFile)(LONG, void *, LONG);
   LONG (*FileSeek)(LONG, LONG, LONG);
   LONG (*FlushCache)(LONG);
   LONG (*FileSize)(BYTE *);
   LONG (*FileSizeHandled)(LONG);
   LONG (*FindFirst)(BYTE **);
   LONG (*FindNext)(BYTE **);
   LONG (*DirectOpenFile)(BYTE *, LONG);
   LONG (*DirectRenameFile)(BYTE *, BYTE *);
   LONG (*DirectRenameFileHandled)(LONG, BYTE *);
   LONG (*DirectDeleteFile)(BYTE *);
   LONG (*DirectDeleteFileHandled)(LONG);
   LONG (*DirectCreateFile)(BYTE *, LONG);
   LONG (*DirectCloseFile)(LONG);
   LONG (*DirectReadFile)(LONG, void *, LONG);
   LONG (*DirectWriteFile)(LONG, void *, LONG);
   LONG (*DirectFileSeek)(LONG, LONG, LONG);
   LONG (*FileLock)(LONG, LONG, LONG);
   LONG (*FileUnlock)(LONG);
   LONG (*DirectFileLock)(LONG, LONG, LONG);
   LONG (*DirectFileUnlock)(LONG);
   LONG (*StartTransaction)(LONG);
   LONG (*EndTransaction)(LONG);
   LONG (*AbortTransaction)(LONG);
   LONG (*CreateDirectory)(BYTE *Path);
   LONG (*DeleteDirectory)(BYTE *Path);
   LONG (*ChangeDirectory)(BYTE *Path);
   LONG (*RenameDirectory)(BYTE *oldPath, BYTE *newPath);
   LONG (*MapPathToDirectory)(BYTE *Path);
   BYTE *(*MapDirectoryToPath)(LONG Directory);
} IFS;


extern LONG IFSGetDefaultFileSystem(void);
extern LONG IFSOpenFile(BYTE *, LONG);
extern LONG IFSRenameFile(BYTE *, BYTE *);
extern LONG IFSRenameFileHandled(LONG, BYTE *);
extern LONG IFSDeleteFile(BYTE *);
extern LONG IFSDeleteFileHandled(LONG);
extern LONG IFSCreateFile(BYTE *, LONG);
extern LONG IFSCloseFile(LONG);
extern LONG IFSReadFile(LONG, void *, LONG);
extern LONG IFSWriteFile(LONG, void *, LONG);
extern LONG IFSFileSeek(LONG, LONG, LONG);
extern LONG IFSFlushCache(LONG);
extern LONG IFSDirectOpenFile(BYTE *, LONG);
extern LONG IFSDirectRenameFile(BYTE *, BYTE *);
extern LONG IFSDirectRenameFileHandled(LONG, BYTE *);
extern LONG IFSDirectDeleteFile(BYTE *);
extern LONG IFSDirectDeleteFileHandled(LONG);
extern LONG IFSDirectCreateFile(BYTE *, LONG);
extern LONG IFSDirectCloseFile(LONG);
extern LONG IFSDirectReadFile(LONG, void *, LONG);
extern LONG IFSDirectWriteFile(LONG, void *, LONG);
extern LONG IFSDirectFileSeek(LONG, LONG, LONG);
extern LONG IFSFileSize(BYTE *name);
extern LONG IFSFileSizeHandled(LONG fd);
extern LONG IFSFindFirst(BYTE **retBuffer);
extern LONG IFSFindNext(BYTE **retBuffer);
extern LONG IFSFileLock(LONG, LONG, LONG);
extern LONG IFSFileUnlock(LONG);
extern LONG IFSDirectFileLock(LONG, LONG, LONG);
extern LONG IFSDirectFileUnlock(LONG);
extern LONG IFSStartTransaction(LONG);
extern LONG IFSEndTransaction(LONG);
extern LONG IFSAbortTransaction(LONG);
extern LONG IFSCreateDirectory(BYTE *Path);
extern LONG IFSDeleteDirectory(BYTE *Path);
extern LONG IFSChangeDirectory(BYTE *Path);
extern LONG IFSRenameDirectory(BYTE *oldPath, BYTE *newPath);
extern LONG IFSMapPathToDirectory(BYTE *Path);
extern BYTE *IFSMapDirectoryToPath(LONG Directory);

extern LONG VFSOpenFile(LONG fsid, BYTE *name, LONG flags);
extern LONG VFSRenameFile(LONG fsid, BYTE *cName, BYTE *nName);
extern LONG VFSRenameFileHandled(LONG fsid, LONG fd, BYTE *nName);
extern LONG VFSDeleteFile(LONG fsid, BYTE *name);
extern LONG VFSDeleteFileHandled(LONG fsid, LONG fd);
extern LONG VFSCreateFile(LONG fsid, BYTE *name, LONG flags);
extern LONG VFSCloseFile(LONG fsid, LONG fd);
extern LONG VFSReadFile(LONG fsid, LONG fd, void *buf, LONG size);
extern LONG VFSWriteFile(LONG fsid, LONG fd, void *buf, LONG size);
extern LONG VFSFileSeek(LONG fsid, LONG fd, LONG offset, LONG where);
extern LONG VFSFlushCache(LONG fsid, LONG fd);
extern LONG VFSDirectOpenFile(LONG fsid, BYTE *name, LONG flags);
extern LONG VFSDirectRenameFile(LONG fsid, BYTE *cName, BYTE *nName);
extern LONG VFSDirectRenameFileHandled(LONG fsid, LONG fd, BYTE *nName);
extern LONG VFSDirectDeleteFile(LONG fsid, BYTE *name);
extern LONG VFSDirectDeleteFileHandled(LONG fsid, LONG fd);
extern LONG VFSDirectCreateFile(LONG fsid, BYTE *name, LONG flags);
extern LONG VFSDirectCloseFile(LONG fsid, LONG fd);
extern LONG VFSDirectReadFile(LONG fsid, LONG fd, void *buf, LONG size);
extern LONG VFSDirectWriteFile(LONG fsid, LONG fd, void *buf, LONG size);
extern LONG VFSDirectFileSeek(LONG fsid, LONG fd, LONG offset, LONG where);
extern LONG VFSFileSize(LONG fsid, BYTE *name);
extern LONG VFSFileSizeHandled(LONG fsid, LONG fd);
extern LONG VFSFindFirst(LONG fsid, BYTE **retBuffer);
extern LONG VFSFindNext(LONG fsid, BYTE **retBuffer);
extern LONG VFSFileLock(LONG fsid, LONG fd, LONG offset, LONG size);
extern LONG VFSFileUnlock(LONG fsid, LONG lid);
extern LONG VFSDirectFileLock(LONG fsid, LONG fd, LONG offset, LONG size);
extern LONG VFSDirectFileUnlock(LONG fsid, LONG lid);
extern LONG VFSStartTransaction(LONG fsid, LONG fd);
extern LONG VFSEndTransaction(LONG fsid, LONG tid);
extern LONG VFSAbortTransaction(LONG fsid, LONG tid);
extern LONG VFSCreateDirectory(LONG fsid, BYTE *Path);
extern LONG VFSDeleteDirectory(LONG fsid, BYTE *Path);
extern LONG VFSChangeDirectory(LONG fsid, BYTE *Path);
extern LONG VFSRenameDirectory(LONG fsid, BYTE *oldPath, BYTE *newPath);
extern LONG VFSMapPathToDirectory(LONG fsid, BYTE *Path);
extern BYTE *VFSMapDirectoryToPath(LONG fsid, LONG Directory);
