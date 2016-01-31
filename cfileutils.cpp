/**
*************************************************************
* @file cfileutils.cpp
* @brief C File tools
*
*
* @author Gaspar Fernández <blakeyed@totaki.com>
* @version
* @date 30 feb 2012
* Changelog:
*   - 20120530 Doc in English
*   - 20120531 Renamed as cfileutils.cpp to build correctly with Makefile
*
*************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>


#include "cfileutils.h"

char *human_size(char *store, long double size)
{
  static char units[10][6]={"bytes","Kb","Mb","Gb","Tb","Pb","Eb","Zb","Yb","Bb"};  
  int i= 0;

  while (size>1024) {
    size = size /1024;
    i++;
  }

  sprintf(store,"%.2Lf%s",size, units[i]);

  return store;
}

long long file_size(char *fileName)
{
  FILE *fich;

  fich=fopen(fileName,"r");
  if (fich==NULL)
    return -1;

  fseek(fich, 0L, SEEK_END);
  size_t res=ftell(fich);

  fclose(fich);

  return res;
}

// return codes
//  0  - file not found
//  -1 - error findinf file
//  1  - file exists
short file_exists(char *filename)
{
  int fd=open(filename, O_RDONLY);
  if (fd==-1)
    {
      if (errno==2)		/* If errno==2 it means file not found */
	return 0;		/* otherwise there is another error at */
      else 			/* reading file, for example path not  */
	return -1;		/* found, no memory, etc */
    }
  close(fd);			/* If we close the file, it exists */
  return 1;
}

short directory_exists(const char *directory)
{
  struct stat sinfo;

  if (stat(directory, &sinfo)<0)
    {
      return (errno==2)?0:-1;			/* There is nothing in that path. errno==2 is file not found if */
    }						/* we get another error we return -1 */
  return (S_ISDIR(sinfo.st_mode))?1:-2;	/* 1 = Directory exists / -2 = It's not a directory */
}

long long human_size_multiplier(const char *human)
{
  static char units[10][3]={"By","Kb","Mb","Gb","Tb","Pb","Eb","Zb","Yb","Bb"};
  short i;
  long long res=1;
  for (i=0; i<10; i++)
    {
      if (strcmp(human, units[i])==0)
	return res;
      res*=1024;
    }
  return 1;			/* If we reach here, there was no match, so we return bytes*/
}

long long human_to_bytes(const char *human)
{
  long double n;
  char unit[3];
  
  sscanf(human, "%Lf%c%c", &n, &unit[0], &unit[1]);

  unit[0]=toupper(unit[0]);	/* Uppercase, please */
  unit[1]=tolower(unit[1]);	/* Lowercase, please */
  unit[2]='\0';
  
  return n * human_size_multiplier(unit);
}

char *getHomeDir()
{
  static char *home = NULL;
  
  if (home==NULL)
    home = getenv("HOME") ;

  if (home==NULL)
    {
      struct passwd *pw = getpwuid(getuid());
      if (pw!=NULL) 
	home = pw->pw_dir;
    }

  return home;
}

// *out MUST be NULL the first time !
char* makePath(char **out, const char *directory, const char *file)
{
  size_t dlen=strlen(directory);
  size_t flen=strlen(file);
  if (*out!=NULL)
    {
      free(*out);
      *out=NULL;
    }

  *out=(char*)malloc(dlen+flen+2);
  if ( (dlen>0) && (directory[dlen-1]!='/') )
    sprintf(*out, "%s/%s", directory, file);
  else
    sprintf(*out, "%s%s", directory, file);


  return *out;
}

// Error codes:
//   -1 can't open origin file
//   -2 can't open destination file
//   -3 reserved
//   -4 write error
//   -5 read errorr
int copyFile(char *origin, char *destination)
{
  int forigin;
  int fdest;
  char buffer[512];

  int result=0;
  size_t bytes;

  forigin=open(origin, O_RDONLY);
  if (forigin<0)
    return -1;

  fdest=open(destination, O_WRONLY | O_CREAT, 0600);
  if (fdest!=-1)
    {
      while ( (bytes=read(forigin, buffer, (size_t)512))>0)
	{
	  if (write(fdest, buffer, bytes)<0)
	    {
	      result=-4;
	      break;
	    }	  
	}
      if (bytes<0)
	  result=-5;
      
      close(fdest);
    }
  else
    result=-2;
  close(forigin);

  return result;
}

// return codes:
//  0 - removed successfully
//  -1- file doesn't exist
//  -2- can't remove file
int removeFile(char *name)
{
  if (file_exists(name)==0)
    {
      return (unlink(name)<0)?-2:0;
    }
  else
    return -1;
}

// return codes:
//  0  - read successfully
//  -1 - file doesn't exist
//  -2 - can't get file size
//  -3 - can't read file
//  -4 - read error
//  -5 - couldn't read whole file
int file_get_contents(char **data, char *fileName, int freeNotNull)
{
  long long fileSize;
  int fd;
  int totalRead;

  if (!file_exists(fileName))
    return -1;
  
  fileSize=file_size(fileName);
  if (fileSize==-1)
    return -2;
  
  fd=open(fileName, O_RDONLY);
  if (fd<0)
    return -3;

  if ( (*data!=NULL) && (freeNotNull) )
    {
      free(*data);
      *data=NULL;
    }
  *data=(char*)malloc(fileSize+1);
  totalRead=read(fd, *data, fileSize);
  close(fd);

  if (totalRead<0)
    return -4;
  else if (totalRead!=fileSize)
    return -5;
    
  (*data)[fileSize]='\0';

  return 0;
}

int createDir (const char *dirName, mode_t mode)
{
  return mkdir (dirName, mode);
}
