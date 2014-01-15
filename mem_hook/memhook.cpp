#include "memhook.h"
#include <map>
#include <string>
#include <utility>
using namespace std;
#if 1
//#define MEM_DEBUG
#ifdef MEM_DEBUG
const unsigned int mem_magic_number = 0xA5E8C7F0;
std::map<unsigned int,int> global_mem_map ;
#endif


#ifdef MEM_DEBUG
size_t ptr_size(void *ptr)
{
	return (*(size_t *)((char*)ptr - sizeof(size_t)));
}
#endif



#if 0
inline void* operator new(size_t size)
{
	
	printf("my new \n");
#ifdef MEM_DEBUG
	void *ptr = calloc(1, size + 8 + sizeof(size_t));
	if (NULL == ptr)
	{
		return NULL;
	}
	//store magic number at before
	memcpy((char*)ptr, &mem_magic_number, 4);
	//store memory size 
	memcpy((char*)ptr + 4, &size, sizeof(size));
	//store magic number at last
	memcpy((char*)ptr + size + 8 + sizeof(size_t) - 4, &mem_magic_number, 4);
	//global_mem_map.insert(pair<unsigned int, int>(/*(unsigned int)((char*)ptr + 4 + sizeof(size_t))*/0x100,(int)size));
	printf("my insert \n");
	//if(global_mem_map.<unsigned int, int>::pointer != NULL)
	{
		global_mem_map.insert(map<unsigned int, int>::value_type ((unsigned int)((char*)ptr + 4 + sizeof(size_t)), (int)size));
	}
	
	printf("my insert end\n");
	//map<unsigned int, int>::value_type ((unsigned int)((char*)ptr + 4 + sizeof(size_t)), (int)size)
	return (void*)((char*)ptr + 4 + sizeof(size_t));
#else
	return malloc(size);
#endif

}
#endif

#if 0
inline void operator delete(void* ptr)
{

	printf("my delete %s %s %d\n",__FILE__,__FUNCTION__,__LINE__,__FUNCSIG__ __FUNCDNAME__);
#ifdef MEM_DEBUG
	map<unsigned int, int>::iterator iter;
	iter = global_mem_map.find((unsigned int)ptr);

	if(iter != global_mem_map.end())
	{
		printf("size = %d\n",iter->second);		
	}
	else
	{
		printf("not find \n");
	}

	void *p = (void*)((char*)ptr - 4 - sizeof(size_t));
	//ziack_assert(*((uint32_t *)p) == ziack_mem_magic_number);
	if(memcmp(p,(char*)mem_magic_number,sizeof(mem_magic_number)) != 0)
	{
		printf("memcheck error start\n");
	}
	size_t size = *((size_t *)((char*)p + 4));
	//ziack_assert(*((uint32_t *)(p + 4 + sizeof(size_t) + size)) == ziack_mem_magic_number);
	if(memcmp(((char*)p + 4 + sizeof(size_t) + size),(char*)mem_magic_number,sizeof(mem_magic_number)) != 0)
	{
		printf("memcheck error end\n");
	}
	global_mem_map.erase(iter);
	free(p);	
	p = NULL;

#else
	free(ptr);
	ptr = NULL;
#endif
}
#endif
#endif