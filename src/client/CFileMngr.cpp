#include "stdafx.h"

#include "CFileMngr.h"
#include "common.h"

#include <stdio.h>
#include <assert.h>

//#include <SimpleLeakDetector/SimpleLeakDetector.hpp>

//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-//

namespace {

// Returns size of a file.
long FileSize(FILE* file) {
  assert(file != NULL);

  long pos = ftell(file);
  fseek(file, 0, SEEK_END);
  long result = ftell(file);
  fseek(file, pos, SEEK_SET);
  return result;
}

// Reads exactly given number of bytes from the file. 
// Returns -1 if there was some IO error.
int ReadExactly(FILE* file, void* buf, size_t size) {
  assert(file != NULL);
  assert(buf != NULL);

  size_t read = 0;
  while (read != size) {
    int err = ::fread(buf, 1, size - read, file);
    if (err <= 0) {
      return false;
    }
    read += err;
  }

  return true;
}

// Loads contents of a file. If the file does not exist or
// there is some other IO error, returns false. 
bool LoadFile(const char* path, void** buf, size_t* size) {
  assert(path != NULL);
  assert(buf != NULL);
  assert(size != NULL);

  FILE* file = ::fopen(path, "rb");
  if (file == NULL) {
    return false;
  }

  long fsize = FileSize(file);
  if (fsize < 0) {
    ::fclose(file);
    return false;
  }

  *size = fsize;

  // Allocate the memory. Note this one additional byte,
  // we will later set it to 0.
  *buf = ::malloc(*size + 1);
  if (*buf == NULL) {
    ::fclose(file);
    return false;
  }

  if (ReadExactly(file, *buf, *size) == false) {
    ::free(*buf);
    ::fclose(file);
    return false;
  }

  // Set the last allocated byte to zero.
  (*(char**)buf)[*size] = 0;

  ::fclose(file);

  return true;
}

}; // anonymous namespace

char pathlst[][50]=
{
	"art\\critters\\",
	"art\\intrface\\",
	"art\\inven\\",
	"art\\items\\",
	"art\\misc\\",
	"art\\scenery\\",
	"art\\skilldex\\",
	"art\\splash\\",
	"art\\tiles\\",
	"art\\walls\\",

	"maps\\",

	"proto\\items\\",
	"proto\\misc\\",
	"proto\\scenery\\",
	"proto\\tiles\\",
	"proto\\walls\\",

	"sound\\music\\",
	"sound\\sfx\\",

	"text\\english\\game\\",
};


int FileManager::Init()
{  
	WriteLog("FileManager Initialization...\n");
	
	master_dat[0]=0;
	
	if(!opt_masterpath[0]) {
		ReportErrorMessage("FileManager Init","Не найден файл %s или в нем нет раздела master_dat",CFG_FILE);
		return 0;
	}
	
	if(opt_masterpath[0]=='.') strcat(master_dat,".");
	else if(opt_masterpath[1]!=':') strcat(master_dat,"..\\");
	
	strcat(master_dat,opt_masterpath.c_str());
	
	if(strstr(master_dat, ".dat")) {
		lpDAT.Init(master_dat);
		
		if(lpDAT.ErrorType == ERR_CANNOT_OPEN_FILE) {
			ReportErrorMessage("FileManager Init>","файл %s не найден",master_dat);
			return 0;
		}
	} else {
		if (master_dat[strlen(master_dat)-1]!='\\') strcat(master_dat,"\\");
	}

	crit_dat[0]=0;
	if(!opt_critterpath[0])
	{
		ReportErrorMessage("FileManager Init","Не найден файл %s или в нем нет раздела critter_dat",CFG_FILE);
		return 0;
	}
	
	if(opt_critterpath[0]=='.') strcat(crit_dat,".");
		else if(opt_critterpath[1]!=':') strcat(crit_dat,"..\\");
	strcat(crit_dat,opt_critterpath.c_str());
	if(strstr(crit_dat,".dat"))
	{
		lpDATcr.Init(crit_dat);		
		if(lpDATcr.ErrorType==ERR_CANNOT_OPEN_FILE)
		{
			ReportErrorMessage("FileManager Init>","файл %s не найден",crit_dat);
			return 0;
		}
	}
	else
	{
		if(crit_dat[strlen(crit_dat)-1]!='\\') strcat(crit_dat,"\\");
	}

	if(!opt_fopath[0])
	{
		ReportErrorMessage("FileManager Init","Не найден файл %s или в нем нет раздела fonline_dat",CFG_FILE);
		return 0;
	}
	strcpy(fo_dat,opt_fopath.c_str());
	if(fo_dat[strlen(fo_dat)-1]!='\\') strcat(fo_dat,"\\");

	WriteLog("FileManager Initialization complete\n");
	initialized=1;
	return 1;
}

void FileManager::Clear()
{
	WriteLog("FileManager Clear...\n");
	UnloadFile();
	WriteLog("FileManager Clear complete\n");
}

void FileManager::UnloadFile() {
	SAFEDELA(buffer);
}

int FileManager::LoadFile(char* fileName, int pathType)
{
  assert(fileName != NULL);
  assert(initialized);

	if(!initialized)
	{
		ReportErrorMessage("FileMngr LoadFile","FileMngr не был иницилазирован до загрузки файла %s",fileName);
		return 0;
	}
	
	UnloadFile();

	char path[1024]="";
	char pfname[1024]="";

	strcpy(pfname,pathlst[pathType]);
	strcat(pfname,fileName);

	strcpy(path,fo_dat);
	strcat(path,pfname);

	//данные fo
  if (::LoadFile(path, (void**) &buffer, (size_t*) &fileSize)) {
    return 1;
  }

	if(pathType==PT_ART_CRITTERS) {
		if (!lpDATcr.IsLoaded()) {
		  WriteLog("Loading from normal FS.\n");
			//попрбуем загрузить из critter_dat если это каталог
			strcpy(path,crit_dat);
			strcat(path,pfname);
	
      if (::LoadFile(path, (void**) &buffer, (size_t*) &fileSize)) {
        return 1;
      } else {
        return 0; //а вот не вышло
      }
		}	

		if(lpDATcr.DATOpenFile(pfname)!=INVALID_HANDLE_VALUE)
		{
			fileSize = lpDATcr.DATGetFileSize();			

			buffer = new uint8_t[fileSize+1];
			DWORD br;
		
			lpDATcr.DATReadFile(buffer,fileSize,&br);

			buffer[fileSize]=0;

			position=0;
			return 1;
		}

	}
	
	if(!lpDAT.IsLoaded())
	{
	  WriteLog("Loading from normal FS.\n");
		//попрбуем загрузить из master_dat если это каталог
		strcpy(path,master_dat);
		strcat(path,pfname);

    if (::LoadFile(path, (void**) &buffer, (size_t*) &fileSize)) {
      return 1;
    } else return 0; //а вот не вышло
	}

	if(lpDAT.DATOpenFile(pfname)!=INVALID_HANDLE_VALUE)
	{	
		fileSize = lpDAT.DATGetFileSize();

		buffer = new uint8_t[fileSize+1];
		DWORD br;
		
		lpDAT.DATReadFile(buffer,fileSize,&br);

		buffer[fileSize]=0;

		position=0;
		return 1;
	}

	WriteLog("FileMngr - Файл %s не найден\n",pfname);//!Cvet
	return 0;
}

void FileManager::SetCurrentPosition(uint32_t pos)
{
	if(pos<fileSize) position=pos;
}

void FileManager::GoForward(uint32_t offs)
{
	if((position+offs)<fileSize) position+=offs;
}


int FileManager::GetStr(char* str,uint32_t len)
{
	if(position>=fileSize) return 0;

	int rpos=position;
	int wpos=0;
	for(;wpos<len && rpos<fileSize;rpos++)
	{
		if(buffer[rpos]==0xD)
		{
			rpos+=2;
			break;
		}

		str[wpos++]=buffer[rpos];
	}
	str[wpos]=0;
	position=rpos;

	return 1;
}

int FileManager::CopyMem(void* ptr, size_t size)
{
	if(position+size>fileSize) return 0;

	memcpy(ptr,buffer+position,size);

	position+=size;

	return 1;
}

uint8_t FileManager::GetByte() //!Cvet
{
	if(position>=fileSize) return 0;
	uint8_t res=0;

	res=buffer[position++];

	return res;
}

uint16_t FileManager::GetWord()
{
	if(position>=fileSize) return 0;
	uint16_t res=0;

	uint8_t *cres=(uint8_t*)&res;
	cres[1]=buffer[position++];
	cres[0]=buffer[position++];

	return res;
}

uint16_t FileManager::GetRWord() //!Cvet
{
	if(position>=fileSize) return 0;
	uint16_t res=0;

	uint8_t *cres=(uint8_t*)&res;
	cres[0]=buffer[position++];
	cres[1]=buffer[position++];

	return res;
}

uint32_t FileManager::GetDWord()
{
	if(position>=fileSize) return 0;
	uint32_t res=0;
	uint8_t *cres=(uint8_t*)&res;
	for(int i=3;i>=0;i--)
	{
		cres[i]=buffer[position++];
	}

	return res;
}

uint32_t FileManager::GetRDWord() //!Cvet
{
	if(position>=fileSize) return 0;
	uint32_t res=0;
	uint8_t *cres=(uint8_t*)&res;
	for(int i=0;i<=3;i++)
	{
		cres[i]=buffer[position++];
	}

	return res;
}

int FileManager::GetFullPath(char* fname, int PathType, char* get_path) //!Cvet полный путь к файлу
{
	get_path[0]=0;
	strcpy(get_path,fo_dat);
	strcat(get_path,pathlst[PathType]);
	strcat(get_path,fname);

	WIN32_FIND_DATA fd;
	HANDLE f=FindFirstFile(get_path,&fd);

	if(f!=INVALID_HANDLE_VALUE) return 1;

	WriteLog("Файл:|%s|\n",get_path);

	return 0;
}