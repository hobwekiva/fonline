#include "stdafx.h"

#include "datfile.h"
#include "common.h"

#include <assert.h>

#define VER_FALLOUT1 0x00000013
#define VER_FALLOUT2 0x10000014

#define ZLIB_BUFF_SIZE 0x10000
#define GZIP_MODE1 0x0178
#define GZIP_MODE2 0xDA78

char *error_types[] = {
   "Cannot open file.",                       // ERR_CANNOT_OPEN_FILE
   "File invalid or truncated.",              // ERR_FILE_TRUNCATED
   "This file not supported.",                // ERR_FILE_NOT_SUPPORTED
   "Not enough memory to allocate buffer.",   // ERR_ALLOC_MEMORY
   "Fallout1 dat files are not supported.",   // ERR_FILE_NOT_SUPPORTED2
};

namespace {

void SwapBytes(void* ptr, size_t size) {
  assert(ptr != NULL);

  char* bytePtr = (char*) ptr;

  for (size_t i = 0; i < size / 2; i++) {
    char temp;
    temp = bytePtr[i];
    bytePtr[i] = bytePtr[size - 1 - i];
    bytePtr[size - 1 - i] = temp;    
  }
}

} // namespace anonymous

//------------------------------------------------------------------------------

DatArchive::DatArchive() {
  ErrorType = 0;

  FileType = 0; //если там 1, то файл считается компрессированым(не всегда).
  RealSize = 0; //Размер файла без декомпрессии
  PackedSize = 0; //Размер сжатого файла
  Offset = 0; //Адрес файла в виде смещения от начала DAT-файла.

  lError = 0;

  hFile = 0; //Handles: (DAT) files

  m_pInBuf = 0;

  FileSizeFromDat = 0;
  TreeSize = 0;
  FilesTotal = 0;

  ptr = 0;
  buff = 0;
  ptr_end = 0;
  //in buff - DATtree, ptr - pointer

  reader = 0; // reader for current file in DAT-archive

}

bool DatArchive::Init(char* fileName) {
  lError = true;
  buff = NULL;
  m_pInBuf = NULL;

  reader = NULL; // Initially empty reader. We don't know its type at this point

  datFileName = fileName;

  hFile = CreateFile(fileName,  //В hFile находится HANDLE на DAT файл
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL,
    NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    ErrorType = ERR_CANNOT_OPEN_FILE;
    return false;
  }

  if (ReadTree() != RES_OK) {
    return false;
  }

  m_pInBuf = (uint8_t*) malloc(ZLIB_BUFF_SIZE);

  if (m_pInBuf == NULL) {
    ErrorType = ERR_ALLOC_MEMORY;
    return false;
  }

  lError = false;
  return true;
}

bool DatArchive::IsLoaded() {
  return hFile != INVALID_HANDLE_VALUE;
}

//------------------------------------------------------------------------------
DatArchive::~DatArchive()
{
   if(hFile != INVALID_HANDLE_VALUE)
      CloseHandle(hFile);
   if(m_pInBuf != NULL)
      free(m_pInBuf);
   if(buff != NULL)
      delete[] buff;

	SAFEDEL(reader);
	fmap.clear();
}
//------------------------------------------------------------------------------
int DatArchive::ReadTree()
{
	DWORD i,F1DirCount;
	bool Fallout1=false;

	
   //Проверка на то, что файл не менее 8 байт
   i = SetFilePointer(hFile, -8, NULL, FILE_END);
   if(i == 0xFFFFFFFF)
       return ERR_FILE_TRUNCATED;
	
   //Чтение информации из DAT файла
   ReadFile(hFile, &TreeSize, 4, &i, NULL);
   ReadFile(hFile, &FileSizeFromDat, 4, &i, NULL);

   i = SetFilePointer(hFile, 0, NULL, FILE_BEGIN); //Added for Fallout1
   ReadFile(hFile, &F1DirCount, 4, &i, NULL); //Added for Fallout1
   SwapBytes(&F1DirCount, 4); //Added for Fallout1
   if(F1DirCount == 0x01 || F1DirCount == 0x33) Fallout1 = true; //Added for Fallout1
   if(GetFileSize(hFile, NULL) != FileSizeFromDat && Fallout1 == false)
      return ERR_FILE_NOT_SUPPORTED;
   if(!Fallout1)
   {
      i = SetFilePointer (hFile, -(TreeSize + 8), NULL, FILE_END);
      ReadFile(hFile, &FilesTotal, 4, &i, NULL);
   }
   else //FALLOUT 1 !!!
	return ERR_FILE_NOT_SUPPORTED2;


   if(buff != NULL)
      delete[] buff;
   if((buff = new uint8_t[TreeSize]) == NULL)
      return ERR_ALLOC_MEMORY;
   ZeroMemory(buff, TreeSize);

   ReadFile(hFile, buff, TreeSize - 4, &i, NULL);
   ptr_end = buff + TreeSize;

   IndexingDAT();
   
   return RES_OK;
}
//------------------------------------------------------------------------------
void DatArchive::ShowError(void)
{
   if(lError)
      MessageBox(NULL, *(error_types + ErrorType), "Error", MB_OK);
   lError = false;
}
//------------------------------------------------------------------------------

void GetPath(char* res, char* src)
{
	int pos=-1;
	for(int i=0;src[i];i++)
		if(src[i]=='\\') pos=i;
	memcpy(res,src,pos+1);
	res[pos+1]=0;
	strlwr(res);
}

void DatArchive::IndexingDAT() {
  find_map::iterator it = fmap.find(datFileName);
  
  if(it != fmap.end()) {
    WriteLog("%s already indexed\n", datFileName.c_str());
    return;
  } else {
    fmap[datFileName] = IndexMap();
  }
  IndexMap& nmap = fmap[datFileName];

  WriteLog("Indexing %s...",datFileName.c_str());
  TICK tc=GetTickCount();
  char path[1024],fname[1024],last_path[1024];
  last_path[0]=0;
  ptr = buff;
  while (true)
  {
    uint32_t fnsz = *(ULONG *)ptr;
    memcpy(fname, ptr + 4, fnsz);
    fname[fnsz] = 0;
    GetPath(path,fname);
    if(path[0] && strcmp(path, last_path))
    {
      char* str=new char[strlen(path)+1];
      strcpy(str,path);
      uint32_t sz=nmap.index.size();
      nmap.index[str]=ptr;
      if(sz==nmap.index.size()) {
        delete[] str;
      } else strcpy(last_path,path);
    }
    if((ptr + fnsz + 17) >= ptr_end)
      break;
    else
      ptr += fnsz + 17;
  }
  ptr = buff;
  WriteLog("for %d ms\n",GetTickCount()-tc);
}


//------------------------------------------------------------------------------
HANDLE DatArchive::DATOpenFile(char* fname)
{
   // ifwe still have old non-closed reader - we kill it
   if(reader) {
      delete reader;
      reader = NULL;
   }

   if(hFile != INVALID_HANDLE_VALUE)
   {
	  if(FindFile(fname))
      {
		  if(!FileType) reader = new CPlainFile (hFile, Offset, RealSize);
			else reader = new C_Z_PackedFile (hFile, Offset, RealSize, PackedSize);
         return hFile;
      }
   }
   return INVALID_HANDLE_VALUE;
}
//------------------------------------------------------------------------------
bool DatArchive::FindFile(char* fname)
{
	
   char str[1024], fnd[1024], path[1024];
   strcpy(str,fname);
   strlwr(str);
   GetPath(path,str);

  index_map& index = fmap[datFileName].index; 

   ptr = index[path];
   if (!ptr) return false;

   int difpos=strlen(str)-5;
   char difchar=str[difpos];

   uint32_t fnsz;
   while (true)
   {
	  fnsz = *(ULONG *)ptr;
      ptr += 4;
	  char fdif=ptr[difpos];
	  if(fdif>=0x41 && fdif<=0x5a) fdif+=0x20;
	  if(difchar==fdif)
	  {
	      memcpy(fnd, ptr, fnsz);
		  fnd[fnsz] = 0;
		  strlwr(fnd);
	      FileType = *(ptr + fnsz);
	      RealSize = *(uint32_t *)(ptr + fnsz+ 1);
	      PackedSize = *(uint32_t *)(ptr + fnsz + 5);
	      Offset = *(uint32_t *)(ptr + fnsz + 9);
	      if(!strcmp(fnd,str))
		  {
	         return true;
		  }
	  }
      if((ptr + fnsz + 13) >= ptr_end)
         break;
      else
         ptr += fnsz + 13;
   }
   return false;
}
//------------------------------------------------------------------------------
bool DatArchive::DATSetFilePointer(LONG lDistanceToMove, uint32_t dwMoveMethod)
{
   if(hFile == INVALID_HANDLE_VALUE) return false;
   reader->seek (lDistanceToMove, dwMoveMethod);
   return true;
}
//------------------------------------------------------------------------------
uint32_t DatArchive::DATGetFileSize(void)
{
   if(hFile == INVALID_HANDLE_VALUE) return 0;
   return RealSize;
}
//------------------------------------------------------------------------------
bool DatArchive::DATReadFile(LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                                                    LPDWORD lpNumberOfBytesRead)
{
   if(hFile == INVALID_HANDLE_VALUE) return false;
   if(!lpBuffer) return false;
   if(!nNumberOfBytesToRead) {
      lpNumberOfBytesRead = 0;
      return true;
   }
   reader->read (lpBuffer, nNumberOfBytesToRead, (long*)lpNumberOfBytesRead);
   return true;
}
//------------------------------------------------------------------------------