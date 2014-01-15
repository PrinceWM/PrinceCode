// stdafx.cpp : 只包括标准包含文件的源文件
// LiveChannelC2A.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"
#include "myfile.h"

CMyFile::CMyFile()
{
    m_name = NULL;
    m_path = NULL;
#if defined(MOHO_X86)
    m_dir = NULL;
    m_dirent = NULL;
    m_req = NULL;
#elif defined(MOHO_WIN32)
    lf = -1l;
#endif
}

CMyFile::~CMyFile()
{
}
void CMyFile::close()
{
    if(m_name)
    {
        free(m_name);
        m_name = NULL;
    }
    if(m_path)
    {
        free(m_path);
        m_path = NULL;
    }
#if defined(MOHO_X86)
    if(m_dir)
    {
        closedir(m_dir);
        m_dir = NULL;
    }
    m_dirent = NULL;
    if(m_req)
    {
        regfree(m_req);
        delete m_req;
        m_req = NULL;
    }
#elif defined(MOHO_WIN32)
    if(lf != -1l)
    {
        _findclose(lf);
        lf = -1l;
    }
#endif
}

int CMyFile::findFirst(char* path,char*name)
{
    if(!m_name || strcmp(name,m_name))
    {
        close();
    }
    if(!m_path)
    {
        m_path = strdup(path);
    }
#if defined(MOHO_X86)
    if(!m_dir)
    {
        m_dir = opendir(path);
        if(!m_dir)
        {
            return 0;
        }
        m_name = strdup(name);
    }
    if(!m_req)
    {
        m_req = new regex_t;
        if(!m_req)
        {
            return 0;
        }
        if(0 != regcomp(m_req,name,REG_EXTENDED))
        {
            return 0;
        }
    }
    m_dirent = NULL;
    while((m_dirent = readdir(m_dir)) )
    {
        //skip '.' && '..'
        if( !strcmp(m_dirent->d_name, ".") ||
            !strcmp(m_dirent->d_name, ".."))
        {
            continue;
        }

        const size_t nmatch = 1;
        regmatch_t pm;
        if(0 == regexec(m_req,m_dirent->d_name,nmatch,&pm,REG_NOTBOL))
        {
            break;
        }
    }
    if(m_dirent)
    {
        return 1;
    }
    else
    {
        return 0;
    }
#elif defined(MOHO_WIN32)
    if(!m_name)
    {
        int size = strlen(path) + strlen(name);
        m_name = (char*)malloc(size+1);
        memcpy(m_name,path,strlen(path));
        memcpy(m_name + strlen(path),name,strlen(name));
        m_name[size] = '\0';
    }
    lf = _findfirst(m_name,&file);
    if( lf == -1l)
    {
        return 0;
    }
    return 1;
#endif
}

int CMyFile::findNext()
{
#if defined(MOHO_X86)
    if(!m_dir)
    {
        return 0;
    }
    while((m_dirent = readdir(m_dir)) )
    {
        //skip '.' && '..'
        if( !strcmp(m_dirent->d_name, ".") ||
            !strcmp(m_dirent->d_name, ".."))
        {
            continue;
        }
        return 1;
    }
    return 0;
#elif defined(MOHO_WIN32)
    if(_findnext(lf, &file) == 0)
    {
        return 1;
    }
    return 0;
#endif
}

char* CMyFile::getName()
{
#if defined(MOHO_X86)
    if(!m_dirent)
    {
        return NULL;
    }
    return m_dirent->d_name;
#elif defined(MOHO_WIN32)
    return file.name;
#endif
}

