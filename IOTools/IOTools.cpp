#include "IOTools.h"
#include<map>
//#include<assert.h>

namespace IOTools {

	bool mkdir(const TCHAR *name) { return CreateDirectory(name,NULL)!=0; }
	
	bool deltree(const TCHAR* path, bool bDeleteRoot) {
		stack<FileName> sToDo;
		sToDo.push(FileName(path));
		ScanForSubDirs(sToDo,path,L"*");
		while(sToDo.size()>0) {
			TCHAR currentPath[MAXPATH];
			_stprintf_s(currentPath,MAXPATH,sToDo.top().name);
			sToDo.pop();
			
			// scan current folder for files and delete them on sight
			TCHAR tmp[MAXPATH];
			_stprintf_s(tmp,MAXPATH,L"%s\\*",currentPath);
			WIN32_FIND_DATA file;
			HANDLE hSearch = NULL;
			bool bFirst=true;
			bool bDone=false;
			do {
				if (!bFirst) {
					if (!FindNextFile(hSearch,&file)) break;
				}
				else {
					hSearch = FindFirstFile(tmp,&file);
					if (hSearch==INVALID_HANDLE_VALUE) bDone=true;
					bFirst=false;
				}
				if (!bDone) {
					if ((file.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0) {
						if ((file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==0) {
							TCHAR fname[MAXPATH];
							_stprintf_s(fname,MAXPATH,L"%s\\%s",currentPath,file.cFileName);
							if (DeleteFile(fname)==0) printf("Trouble deleting %s\n",fname);
							//_tprintf(L"delete %s\n",fname);
						}
					}
				}
			} while(!bDone);
			FindClose(hSearch);
			if (sToDo.size()>0 || bDeleteRoot) { 
				//_tprintf(L"delete %s\n",currentPath); }
				if(RemoveDirectory(currentPath)==0) printf("Trouble deleting %s\n"); 
			}
		}
		return true;
	}
  
	size_t ScanForSubDirs(stack<FileName>& sToDo,const TCHAR* path, const TCHAR* mask) {
		TCHAR tmp[MAXPATH];
		_stprintf_s(tmp,MAXPATH,L"%s\\%s",path,mask);
		WIN32_FIND_DATA file;
		HANDLE hSearch = NULL;
		bool bFirst=true;
		bool bDone=false;
		do {
			if (!bFirst) {
				if (!FindNextFile(hSearch,&file)) break;
			}
			else {
				hSearch = FindFirstFile(tmp,&file);
				if (hSearch==INVALID_HANDLE_VALUE) bDone=true;
				bFirst=false;
			}
			if (!bDone) {
				if ((file.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0) {
					if ((file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=0) {
						size_t stLen=_tcslen(file.cFileName);
						bool bDots=(file.cFileName[0]=='.' && file.cFileName[1]=='\0');
						if (!bDots && stLen>1) bDots=(file.cFileName[0]=='.' && file.cFileName[1]=='.' && file.cFileName[2]=='\0');
						if (!bDots) {
							TCHAR folder[MAXPATH];
							_stprintf_s(folder,MAXPATH,L"%s\\%s",path,file.cFileName);
							sToDo.push(FileName(folder));
							ScanForSubDirs(sToDo,folder,mask);
						}
					}
				}
			}
		} while(!bDone);
		FindClose(hSearch);
		return sToDo.size();
	}
	

	size_t ScanFolder(stack<pair<FileName,FileName> >& sToDo, const TCHAR* path, const TCHAR* mask,bool bFilterDoubles) {
		map<FileName,bool> _map;
	
		// First pass only scans for directories
		stack<FileName> dirs;
		dirs.push(FileName(path));
		size_t numSubDirs=ScanForSubDirs(dirs,path,L"*");
		
		// Second pass scans for all files in the subdirectories that match the mask
		while (dirs.size()>0) {
			TCHAR tmp[MAXPATH];
			TCHAR thisPath[MAXPATH];
			_stprintf_s(thisPath,MAXPATH,dirs.top().name);
			_stprintf_s(tmp,MAXPATH,L"%s\\%s",thisPath,mask);
			dirs.pop();
			WIN32_FIND_DATA file;
			HANDLE hSearch = NULL;
			bool bFirst=true;
			bool bDone=false;
			do {
				if (bFirst) {
					hSearch = FindFirstFile(tmp,&file);
					if (hSearch==INVALID_HANDLE_VALUE) bDone=true;
					bFirst=false;
				}
				else if (!FindNextFile(hSearch,&file)) bDone=true;
		
				if (!bDone) {
					if ((file.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0) {
						if ((file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0) {
							if (bFilterDoubles) {
								map<FileName,bool>::iterator it=_map.find(FileName(file.cFileName));
								if (it==_map.end()) {
									_map[FileName(file.cFileName)]=true;
									sToDo.push(pair<FileName,FileName>(thisPath,FileName(file.cFileName)));
								}
							}
							else {
								sToDo.push(pair<FileName,FileName>(thisPath,FileName(file.cFileName)));
							}
						}
					}
				}
				else {
					bDone=true;
					FindClose(hSearch);
				}
			} while(!bDone);
		}
		_map.clear();
		return sToDo.size();
	}


	bool SaveImage(LPCTSTR filename, size_t stWidth, size_t stHeight, const unsigned char* bgrData, bool bFlip) {
		if (!filename || !bgrData) return false;
		CImage *img=new CImage();
		if (FALSE==img->Create(int(stWidth),int(stHeight),24,0)) {
			img->Destroy();
			delete img;
			return false;
		}
		if (!bFlip) for (int j=0; j<int(stHeight); j++) memcpy(img->GetPixelAddress(0,int(stHeight)-1-j),&bgrData[3*j*stWidth],3*stWidth);
		else for (int j=0; j<int(stHeight); j++) memcpy(img->GetPixelAddress(0,j),&bgrData[3*j*stWidth],3*stWidth);
		if (S_OK!=img->Save(filename)) {
			img->Destroy();
			delete img;
			return false;
		}
		else {
			img->Destroy();
			delete img;
			return true;
		}
	}

	unsigned char* LoadImage(LPCTSTR filename, size_t &stWidth, size_t &stHeight) {
		if (!filename) return NULL;
		CImage *img=new CImage();
		if (S_OK!=img->Load(filename)) {
			img->Destroy();
			delete img;
			return NULL;
		}
		stWidth        = size_t(abs(img->GetWidth()));
		stHeight       = size_t(abs(img->GetHeight()));
		bool bBottomUp = (img->GetPitch() < 0);
		if (img->GetBPP()!=24) {
			img->Destroy();
			delete img;
			return NULL;
		}
		unsigned char *ucBuffer=new unsigned char[3*stWidth*stHeight];
		int a=(bBottomUp ? int(stHeight)-1 : 0);
		int b=(bBottomUp ? -1 : 1);
		for (int j=0; j<int(stHeight); j++) memcpy(&ucBuffer[3*j*stWidth],img->GetPixelAddress(0,a+b*j),3*stWidth);
		img->Destroy();
		delete img;
		return ucBuffer;
	}

	bool SaveBlock(const char* name, const unsigned char *Block, size_t bytes) {
		if (!Block) return false;
		FILE *fptr=NULL;
		errno_t err=fopen_s(&fptr,name,"wb");
		if (err) return false;
		fwrite(Block,1,bytes,fptr);
		fclose(fptr);
		return true;
	}

	bool LoadBlock(const char* name, unsigned char* &Block, size_t& bytes) {
		FILE *fptr=NULL;
		errno_t err=fopen_s(&fptr,name,"rb");
		if (err) return false;
		fseek(fptr,0,SEEK_END);
		bytes=ftell(fptr);
		fseek(fptr,0,SEEK_SET);
		Block=new unsigned char[bytes];
		fread(Block,1,bytes,fptr);
		fclose(fptr);
		return true;
	}

	bool exists(const TCHAR* name) {
		FILE* fptr;
		errno_t error =_tfopen_s(&fptr,name,L"r");
		if (!error) {
			fclose(fptr);
			return true;
		}
		else return false;
	}

	int openWrite(const TCHAR* name) {
		if (exists(name)) _tremove(name);
		int hFile=-1;
		errno_t error =_tsopen_s(&hFile,name,
			_O_BINARY | _O_WRONLY | _O_CREAT | _O_NOINHERIT | _O_SEQUENTIAL,	// open flags
			_SH_DENYRW,				// share flags
			_S_IREAD | _S_IWRITE);	// permission flag
		if (error) { 
			_tprintf(L"Error creating file %s.\n",name);
			return -1;
		}
		else return hFile;
	}

	int openRead(const TCHAR *name) {
		int hFile=-1;
		errno_t error =_tsopen_s(&hFile,name,
			_O_BINARY | _O_RDONLY | _O_NOINHERIT | _O_SEQUENTIAL,	// open flags
			_SH_DENYRW,				// share flags
			_S_IREAD | _S_IWRITE);	// permission flag
		if (error) {
			_tprintf(L"Error opening file %s.\n",name);
			return -1;
		}
		else return hFile;
	}

	int openRW(const TCHAR *name) {
		int hFile=-1;
		errno_t error =_tsopen_s(&hFile,name,
			_O_BINARY | _O_RDWR | _O_NOINHERIT | _O_SEQUENTIAL,	// open flags
			_SH_DENYRW,				// share flags
			_S_IREAD | _S_IWRITE);	// permission flag
		if (error) {
			_tprintf(L"Error opening file %s.\n",name);
			return -1;
		}
		else return hFile;
	}

	bool getCRC32(const TCHAR* name, DWORD& CRC,const unsigned char* sideinfo, size_t size) {
		if (!name) return false;
		int hFile=openRead(name);
		if (hFile<0) return false;
		CRC32 *crc=new CRC32();
		const size_t BUFFERSIZE=4096;
		unsigned char *ucBuffer=new unsigned char[BUFFERSIZE];
		__int64 filesize=_lseeki64(hFile,0,SEEK_END);
		_lseeki64(hFile,0,SEEK_SET);
		CRC=0;
		while (filesize-BUFFERSIZE>0) {
			int rd=_read(hFile,ucBuffer,BUFFERSIZE);
			filesize-=BUFFERSIZE;
			CRC=crc->get(ucBuffer,BUFFERSIZE,CRC);
		};
		if (filesize>0) {
			int rd=_read(hFile,ucBuffer,(unsigned int)(filesize));
			CRC=crc->get(ucBuffer,(unsigned int)(filesize),CRC);
		}
		// Add sideinfo
		if (sideinfo && size) CRC=crc->get(sideinfo,size,CRC);
		_close(hFile);
		delete[] ucBuffer;
		delete crc;
		return true;
	}
}