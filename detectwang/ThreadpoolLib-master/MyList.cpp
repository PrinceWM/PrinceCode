#include "MyList.h"
#include <cassert>
#include"MyThread.h"
CMyList::CMyList(void)
{
}


CMyList::~CMyList(void)
{
}

bool CMyList::addThread( CMyThread*t )
{
	assert(t);
	if(!t)
		return false;
	m_mutex.Lock();
	m_list.push_back(t);
	m_mutex.Unlock();
	return true;
}

CMyThread* CMyList::FindThread( int port )
{
	m_mutex.Lock();
	std::list<CMyThread*>::iterator iter=m_list.begin();
	for(;iter!=m_list.end();iter++)
	{
		if((*iter)->dataport == port)
		{			
			m_mutex.Unlock();
			return (*iter);
		}
	}
	m_mutex.Unlock();
	return NULL;
}

bool CMyList::removeThread( CMyThread*t )
{
// 	std::list<CThread*>::iterator iter=m_list.begin();
// 	for(iter;iter!=m_list.end();iter++)
// 	{
// 		if(*iter==t)
// 		{
// 			break;
// 		}
// 	}
	assert(t);
	if(!t)
		return false;
	m_mutex.Lock();
	m_list.remove(t);
	m_mutex.Unlock();
	return true;
}

int CMyList::getSize()
{
	m_mutex.Lock();
	int size= m_list.size();
	m_mutex.Unlock();
	return size;
}

bool CMyList::isEmpty()
{
	m_mutex.Lock();
	bool ret= m_list.empty();
	m_mutex.Unlock();
	return ret;
}

bool CMyList::clear()
{
	m_mutex.Lock();
	std::list<CMyThread*>::iterator iter=m_list.begin();
	for(;iter!=m_list.end();iter++)
	{
		delete (*iter);
	}
	m_list.clear();
	m_mutex.Unlock();
	return true;
}
