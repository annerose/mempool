 
#include "kmempool.h"

KMemoryPool pool;

void* operator new(size_t size, char* pszFileName)
{
	//char szMsg[256] = {0};
	//sprintf(szMsg, "%d,%s", line, file);
	return pool.Malloc(size, pszFileName);
}


void* operator new[](size_t nSize, char* pszFileName )
{
	//char szMsg[256] = {0}; 
	//sprintf(szMsg, "%d,%s", nLineNum, pszFileName);
	return pool.Malloc(nSize, pszFileName);
}


void FreePool(void* ptr)
{
	pool.Free(ptr);
}

//void operator delete(void* ptr)
//{
//	pool.Free(ptr);
//	//{
//	//	::operator delete(ptr);
//	//}
//}
//
//
//void operator delete[](void* ptr)
//{
//	pool.Free(ptr);
//	//{
//	//	::operator delete(ptr);
//	//}
//}

// 不会调用
void operator delete(void* p, char* pszFileName )
{
	//pool.Free(p);

	printf("operator delete\n");
}

// 不会调用
void operator delete[](void* ptr, char* pszFileName )
{
	//pool.Free(ptr);
	printf("operator delete [] \n");
}