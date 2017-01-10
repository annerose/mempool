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

	virtual ~CBase()
	{
		printf("CBase Distory\n");
	}

	int m_x;
};


class CChild : public CBase
{
public:
	CChild():CBase()
	{
		printf("CChild Constructor\n");
	}

	virtual ~CChild()
	{
		printf("CChild Distory\n");
	}
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
	CBase* pbase = new CChild ;


	//operator delete[] (p, __FILE__, __LINE__);

	//PoolDestroy(pbase);
	//delete [] p;
	SAFE_DELETE_GROUP(p);
	SAFE_DELETE(pbase);

//	operator delete (pbase, __FILE__, __LINE__);
 
	//delete[] (p);
	//delete  pbase ;
	//FreePool (p);
	//FreePool (pbase);

}

int _tmain(int argc, _TCHAR* argv[])
{
	//testMem();
	testnew();

	return 0;
}

