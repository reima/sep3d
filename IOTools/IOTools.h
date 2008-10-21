#ifndef __TUM3D_IOTOOLS_H__
#define __TUM3D_IOTOOLS_H__

#include <tchar.h>
#include <atlstr.h>
#include <atlimage.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <share.h>
#include <TCHAR.h>
#include <stack>
#include "crc32.h"

#ifdef MAXPATH
#undef MAXPATH
#endif
#define MAXPATH 1024

using namespace std;

// TODO: change everything to FileName struct.
// TODO: change FileName struct's backend to vector<TCHAR>

namespace IOTools {

	struct FileName {
		FileName(const TCHAR n[MAXPATH]=L"")	{	_stprintf_s(name,MAXPATH,n); }
		FileName(const FileName& other)			{ operator=(other); }
		inline size_t size(void) const			{ return _tcslen(name); }
		inline operator TCHAR*(void)			{ return &name[0]; }	
		inline bool operator==(const FileName& other) const { return (StrCmp(name,other.name)==0); }
		inline bool operator<(const FileName& other) const  { return (StrCmp(name,other.name)<0);  }
		inline bool operator>(const FileName& other) const  { return (StrCmp(name,other.name)>0);  }
		inline void operator=(const FileName& other)		{ memcpy(name,other.name,sizeof(TCHAR)*MAXPATH); }
		TCHAR name[MAXPATH];
	};

	extern bool				SaveImage(LPCTSTR filename, size_t stWidth, size_t stHeight, const unsigned char* bgrData, bool bFlip = false);
	extern unsigned char*	LoadImage(LPCTSTR filename, size_t &stWidth, size_t &stHeight);
	extern bool				SaveBlock(const char* name, const unsigned char *Block, size_t bytes);
	extern bool				LoadBlock(const char* name, unsigned char* &Block, size_t& bytes);
	extern bool				exists(const TCHAR* name);
	extern int				openWrite(const TCHAR* name);
	extern int				openRead(const TCHAR *name);
	extern int				openRW(const TCHAR *name);
	extern size_t			ScanForSubDirs(stack<FileName>& sToDo,const TCHAR* path, const TCHAR* mask);
	extern bool				getCRC32(const TCHAR* name, DWORD& CRC,const unsigned char* sideinfo=NULL, size_t size=0);
	// scans a given path recursivels and in DEPTH-FIRST-ORDER. Hence, subfolders are guaranteed to show up
	// in the stack prior to their parents.
	extern size_t			ScanFolder(stack< pair<FileName,FileName> >& sToDo, const TCHAR* path, const TCHAR* mask,bool bFilterDoubles=true);
	// ScanFolder returns a stack of pairs <path,name>.
	extern bool				mkdir(const TCHAR* name);
	extern bool				deltree(const TCHAR* path,bool bDeleteRoot=true);	
	// deletes everything in the path. bDeleteRoot determines if the topmost directory is deleted as well.
}

#endif
