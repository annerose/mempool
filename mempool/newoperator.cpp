 
#include "kmempool.h"

KMemoryPool pool;

void* operator new(size_t size, char* file, int line)
{
	char szMsg[256] = {0};
	sprintf(szMsg, "%d,%s", line, file);
	return pool.Malloc(size, szMsg);
}


void* operator new[](size_t nSize, char* pszFileName, int nLineNum)
{
	char szMsg[256] = {0}; 
	sprintf(szMsg, "%d,%s", nLineNum, pszFileName);
	return pool.Malloc(nSize, szMsg);
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
void operator delete(void* p, char* pszFileName, int nLineNum)
{
	pool.Free(p);
}

// 不会调用
void operator delete[](void* ptr, char* pszFileName, int nLineNum)
{
	pool.Free(ptr);
}