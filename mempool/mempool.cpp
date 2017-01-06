// mempool.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "kmempool.h"
#include "newoperator.h"


class CBase 
{
public:
	CBase()
	{
		printf("CBase Constructor\n");
	}

	~CBase()
	{
		printf("CBase Distory\n");
	}

	int m_x;
};

void testMem()
{
	KMemoryPool pool;

	char* pBuf = (char*)pool.Malloc(260,  __FUNCTION__);

	//pool.PrintInfo();
 //	pBuf = (char*)pool.ReMalloc(pBuf, 1260);
	
	pool.Free(pBuf);
	pool.PrintTree();


}

void testnew()
{
	char* p = new char[100] ;
	CBase* pbase = new CBase ;

	//delete[] p;
	//delete pbase;
	//FreePool (p);
	//FreePool (pbase);

}

int _tmain(int argc, _TCHAR* argv[])
{
	//testMem();
	testnew();

	return 0;
}

