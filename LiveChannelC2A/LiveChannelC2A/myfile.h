#ifndef MYFILE_H
#define MYFILE_H

#if defined(MOHO_X86)
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#endif

class CMyFile
{
public:
	CMyFile();
	~CMyFile();
	void close();    
	int findFirst(char* path,char*name);   
	int findNext();   
	char* getName();
private:

	char* m_name;
	char* m_path;
#if defined(MOHO_X86)
	DIR * m_dir;
	struct dirent* m_dirent;
	regex_t *m_req;
#elif defined(MOHO_WIN32)
	_finddata_t file;
	LONG lf;
#endif
};

#endif // MYFILE_H
