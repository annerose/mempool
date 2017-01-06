#include "KMemPool.h"

/*#define MEMORY_BLOCK_HEAD_SIZE = sieof(MemBlcokHead)*/
const unsigned long MemoryBlockHeadSize = sizeof(MemBlockHead);

// n + 8
#define MEM_BLOCK_SIZE(nDataLength)   (nDataLength + MemoryBlockHeadSize)

// n - 8
#define MEM_BLOCK_DATA_SIZE(nBlockSize)   (nBlockSize - MemoryBlockHeadSize)


// p0 = p1 - 8
#define MEM_BLOCK_HEAD(pData)   (MemBlockHead*)(((char*)pData) - MemoryBlockHeadSize)

// p1 = p0 + 8
#define MEM_BLOCK_DATA(pHead)   (((char*)pHead) + MemoryBlockHeadSize)

#define MEM_STACK_BLOCK_MIN 16

// 最大Block size 1M
#define MEM_MAX_BLOCK_SIZE (1<< 20)
 

KMemPoolStackToken::KMemPoolStackToken(IN int nBlockSize )
{
	m_ulBlockSize = nBlockSize;

	m_nBlockOutSide = 0;
	m_nBlockInSide = 0;

	{
		writeLock  lock(m_rwMutex);  
		m_pFirstSon = NULL;
		m_pNext = NULL;
	}
	

}

KMemPoolStackToken::~KMemPoolStackToken( void )
{
	if(m_nBlockOutSide > 0)
	{
		printf("memory stack: lost %d * %d\n", m_ulBlockSize, m_nBlockOutSide);
	}

	// 写锁，释放右枝和左枝
	{
		writeLock  lock(m_rwMutex); 

		// 递归
		if(m_pFirstSon)
		{
			DestroySon(m_pFirstSon);
		}

		m_pFirstSon = NULL;

		if(m_pNext)
		{
			delete m_pNext;
		}
		m_pNext = NULL;

	}
}

void* KMemPoolStackToken::Malloc(IN int nSize, OUT int& nAllBlockCout, OUT int& nMemoryUse)
{
	void* pRet = NULL;

	MemBlockHead* pNew = NULL;

	// 可以在右枝申请
	if(m_ulBlockSize > MEM_BLOCK_SIZE(nSize))
	{
		// 写锁	
		writeLock  lock(m_rwMutex); 

		if(m_pFirstSon == NULL)
		{
			// 申请新的内存块
			pNew = (MemBlockHead*)malloc(m_ulBlockSize);
			if(pNew)
			{
				m_nBlockOutSide++;
				nMemoryUse += m_ulBlockSize;
				pNew->m_ulBlockSize = m_ulBlockSize;
				pNew->m_pNext = NULL;

				// 返回给应用程序的内存，跳过了head
				pRet = MEM_BLOCK_DATA(pNew);

				nAllBlockCout++;
			}
			else
			{
				// 失败返回NULL, OS没有内存可以分配
				printf("*** malloc size %d error!!! \n", m_nBlockInSide);
			}
				
		}
		else
		{
			// 有空闲内存, 提取第一块栈顶
			pNew = m_pFirstSon;

			m_pFirstSon = pNew->m_pNext;
			pNew->m_pNext = NULL;
			// 返回给应用程序的内存，跳过了head
			pRet = MEM_BLOCK_DATA(pNew);

			m_nBlockOutSide++;
			m_nBlockInSide--;

		}
		 
	}
	else
	{
		// size 过大, 申请左枝
		// 写锁, 需要改变m_pNext
		{
			writeLock  lock(m_rwMutex); 
			if(m_pNext == NULL)
			{
				m_pNext = new KMemPoolStackToken(m_ulBlockSize*2);
			}
		}


		//  读锁, 不改变m_pNext的值
		{
			readLock  lock(m_rwMutex); 
			if(m_pNext)
			{
				// 递归
				pRet = m_pNext->Malloc(nSize, nAllBlockCout, nMemoryUse);
			}
		}
		
	}

	return pRet;
}

bool KMemPoolStackToken::Free(IN void* pPoint,IN bool bCloseFlag )
{
	bool bRet = false;

	MemBlockHead* pOld = MEM_BLOCK_HEAD(pPoint);

	// 该内存块是否归本内存管理
	if(m_ulBlockSize == pOld->m_ulBlockSize)
	{
		// 超限的内存大小，直接释放，不重用
		if(bCloseFlag || m_ulBlockSize >= MEM_MAX_BLOCK_SIZE)
		{
			free(pOld);			
		}
		else
		{
			// 写锁, 类变量值改变, 重用内存块, 把当前释放的内存放在链表的顶部
			{
				writeLock  lock(m_rwMutex); 
				pOld->m_pNext = m_pFirstSon;
				m_pFirstSon = pOld;
			}
		

			m_nBlockInSide++;
		}
		m_nBlockOutSide--;
		bRet = true;
	}
	else
	{

		readLock lock(m_rwMutex); 
		// 读锁，不改变m_pNext的值 左枝节点一定存在
		if(m_pNext)
		{
			// 递归...
			bRet = m_pNext->Free(pPoint, bCloseFlag);
		}

 

	}

	return bRet;

}


// 释放所有子节点
void KMemPoolStackToken::DestroySon(IN MemBlockHead* pSon )
{
	MemBlockHead* pObjNow = pSon;
	MemBlockHead* pObjNext = NULL;

	while (true)
	{
		if(pObjNow == NULL)
		{
			break;
		}

		pObjNext = pObjNow->m_pNext;
		free(pObjNow);
		m_nBlockInSide--;
		pObjNow = pObjNext;
	}

}

void KMemPoolStackToken::PrintStack()
{
	// 打印左枝内容
	if (m_nBlockInSide + m_nBlockOutSide > 0)
	{
		printf("[%ld] stack: all=%d, out=%d, in=%d\n", 
			m_ulBlockSize,
			m_nBlockInSide + m_nBlockOutSide,
			m_nBlockOutSide,
			m_nBlockInSide);

		// 加读锁
		if(m_pNext)
		{
			m_pNext->PrintStack();
		}

	}
}


///////////////////////////////////////////////////////////////////////////
//	 内存池左枝
KMemPoolStack::KMemPoolStack()
{
	m_bCloseFlag = false;
	m_pMaxPoint = 0;
	m_nAllBlockCount = 0;
	m_nMemoryUse = 0;

	// 构造左枝第一个节点,左枝只创建不释放
	m_pHead = new KMemPoolStackToken(MEM_STACK_BLOCK_MIN);

}

KMemPoolStack::~KMemPoolStack()
{
	printf("mem stack: MaxPoint = 0x%p\n", m_pMaxPoint);

	if(m_pHead)
	{
		delete m_pHead;
		m_pHead = NULL;
	}
}

void KMemPoolStack::SetCloseFlag( bool bCloseFlag /*= true*/ )
{
	m_bCloseFlag = bCloseFlag;
}

void* KMemPoolStack::Malloc( int nSize )
{
	void* pRet = NULL;

	if( nSize <= 0)
	{
		printf("*** KMemPoolStack::Malloc ERROR nSize = %d\n", nSize);
		return pRet;
	}

	if(m_pHead)
	{
		pRet = m_pHead->Malloc(nSize, m_nAllBlockCount, m_nMemoryUse);

		if(m_pMaxPoint < (int)pRet)
		{
			m_pMaxPoint =  (int)pRet;
		}
	}
	return pRet;
}

bool KMemPoolStack::Free( void* pPoint )
{
	bool bRet = false;
	if(m_pHead)
	{
		bRet = m_pHead->Free(pPoint, m_bCloseFlag);
	}
	return bRet;
}

void* KMemPoolStack::ReMalloc( void* pPoint, int nNewSize, bool bCopyOldDataFlag /*= true*/ )
{
	void* pRet = NULL;
	MemBlockHead* pOldToken = NULL;
	int nOldLen = 0;
	if(nNewSize <= 0)
	{
		printf("KMemPoolStack::ReMalloc Error nNewSize = %d\n", nNewSize);
		goto Mem_Free_Old;
	}
	 
	pOldToken = MEM_BLOCK_HEAD(pPoint);
	nOldLen = pOldToken->m_ulBlockSize;

	if(MEM_BLOCK_SIZE(nNewSize) <= nOldLen)
	{
		// 无需重新分配
		pRet = pPoint;
		goto Mem_End_Process;
	}
	else
	{
		pRet = m_pHead->Malloc(nNewSize, m_nAllBlockCount, m_nMemoryUse);
		if(pRet && pPoint && bCopyOldDataFlag)
		{
			memcpy(pRet, pPoint, nOldLen);
		}
	}
	 

Mem_Free_Old:
	m_pHead->Free(pPoint, m_bCloseFlag);

Mem_End_Process:
	return pRet;

	 
}

void KMemPoolStack::PrintStack()
{
	if(m_pHead)
	{
		m_pHead->PrintStack();
	}
}

void KMemPoolStack::PrintInfo()
{
	printf("MemPoolStack: block=%d, use=%d KB, biggest=%p\n",
		m_nAllBlockCount,
		m_nMemoryUse << 10,
		m_pMaxPoint);
}



//////////////////////////////////////////////////////////////////////////
//	KMemoryRegister

const unsigned long MemoryRegisterInfoSize = sizeof(MemoryRegisterInfo);

#define MEMORY_REGISTER_MAX 10000

#define CLEAN_CHAR_BUFFER(p)  (*((char*)p) = '\0')

KMemoryRegister::KMemoryRegister()
{
	m_pMaxPoint = NULL;
	m_nUseMax = 0;
	m_nPointCount = 0;

	m_arrMemRegisterInfo = new MemoryRegisterInfo[MEMORY_REGISTER_MAX];
	memset(m_arrMemRegisterInfo, 0, sizeof(MemoryRegisterInfo) * MEMORY_REGISTER_MAX);



}

KMemoryRegister::~KMemoryRegister()
{
	// 加锁
	exclusiveLock lock(m_wMutex);
	
	printf("KMemoryRegister: max register point = 0x%p\n", m_pMaxPoint);

	for(int i = 0; i < m_nUseMax; i++)
	{
		if(m_arrMemRegisterInfo[i].m_pPoint)
		{
			// 只报警，不释放
			printf("*** Memory Lost: [%p] - %s\n", 
				m_arrMemRegisterInfo[i].m_pPoint,
				m_arrMemRegisterInfo[i].m_szInfo);
		}
	}


	delete [] m_arrMemRegisterInfo;
	m_arrMemRegisterInfo = NULL;

}

void KMemoryRegister::Add( void* pPoint, char* szInfo )
{
	// 加锁
	exclusiveLock lock(m_wMutex);

	if(pPoint > m_pMaxPoint)
	{
		m_pMaxPoint = pPoint;
	}

	for(int i = 0; i < m_nUseMax; i++)
	{
		// 空指针,未使用
		if(m_arrMemRegisterInfo[i].m_pPoint == NULL)
		{
			m_nPointCount++;
			RegisterCopy(m_arrMemRegisterInfo + i, pPoint, szInfo);

			// 直接返回
			goto KMemRegister_Add_End_Process;
		}
	}

	// 处理数组越界
	if(m_nUseMax >= MEMORY_REGISTER_MAX)
	{
		printf("*** Error: MemRegister array is full %d. \n", MEMORY_REGISTER_MAX);
		goto KMemRegister_Add_End_Process;
	}

	// 拷贝到队尾
	RegisterCopy(m_arrMemRegisterInfo + m_nUseMax, pPoint, szInfo);

	m_nPointCount++;
	m_nUseMax++;

KMemRegister_Add_End_Process:

	return;
}


void KMemoryRegister::Del( void* pPoint )
{
	// 加锁
	exclusiveLock lock(m_wMutex);

	for (int i = 0; i < m_nUseMax; i++)
	{
		if(pPoint == m_arrMemRegisterInfo[i].m_pPoint)
		{
			m_nPointCount--;
			m_arrMemRegisterInfo[i].m_pPoint = NULL;
			CLEAN_CHAR_BUFFER(m_arrMemRegisterInfo[i].m_szInfo);

			break;
		}
	}
}

void KMemoryRegister::Modify( void* pOld, void* pNew )
{
	// 加锁
	exclusiveLock lock(m_wMutex);

	if(pOld > m_pMaxPoint)
	{
		m_pMaxPoint = pOld;
	}

	int i = 0;
	for(i = 0; i < m_nUseMax; i++)
	{
		if(pOld == m_arrMemRegisterInfo[i].m_pPoint)
		{
			m_arrMemRegisterInfo[i].m_pPoint = pNew;
			break;
		}
	}

	if(i == m_nUseMax)
	{
		printf("*** Error: KMemoryRegister::Modify  can not found pint %p\n", pOld);

	}



}

void KMemoryRegister::PrintInfo( void )
{
	exclusiveLock lock(m_wMutex);
	// 加锁
	printf("KMemoryRegister: %d / %d, biggest=%p\n", m_nPointCount, m_nUseMax + 1, m_pMaxPoint);
}



void KMemoryRegister::RegisterCopy( MemoryRegisterInfo* pDest, void* pPoint, char* szInfo )
{
	pDest->m_pPoint = pPoint;
	memset(pDest->m_szInfo, 0, sizeof(pDest->m_szInfo));

	if(szInfo)
	{
		strncpy(pDest->m_szInfo, szInfo, sizeof(pDest->m_szInfo) - 1);
	}
 
}



//////////////////////////////////////////////////////////////////////////
//	KMemoryPool
KMemoryPool::KMemoryPool( bool bRegister /*= true*/ )
{
	m_pMemPoolStack = new KMemPoolStack;
	m_pRegister = NULL;

	if(bRegister)
	{
		m_pRegister = new KMemoryRegister;
	}

	printf("KMemoryPool Open m_pRegister = %d\n",m_pRegister);
}

KMemoryPool::~KMemoryPool()
{

	if(m_pRegister)
	{
		delete (m_pRegister);
		m_pRegister = NULL;
	}


	if(m_pMemPoolStack)
	{
		delete m_pMemPoolStack;
		m_pMemPoolStack = NULL;
	}

	printf("KMemoryPool Close.\n");
}

void KMemoryPool::SetCloseFlag( bool bClose /*= true*/ )
{
	if(m_pMemPoolStack)
	{
		m_pMemPoolStack->SetCloseFlag(bClose);
	}
}

void* KMemoryPool::Malloc( int nSize, char* szInfo /*= NULL*/ )
{
	void* pRet = NULL;
	if(m_pMemPoolStack)
	{
		pRet = m_pMemPoolStack->Malloc(nSize);
		if(pRet)
		{
			Register(pRet, szInfo);
		}
	}

	return pRet;
}

bool KMemoryPool::Free( void* pBlock )
{
	bool ret = false;
	if(m_pMemPoolStack)
	{
		ret = m_pMemPoolStack->Free(pBlock);
		if(!ret)
		{
			printf("*** KMemoryPool::Free unmanager block %p\n", pBlock);
		}
	}

	UnRegister(pBlock);

	return ret;
}

void* KMemoryPool::ReMalloc( void* pPoint, int nNewSize, bool bCopyOldDataFlag /*= true*/ )
{
	void* pRet = NULL;
	if(m_pMemPoolStack)
	{
		pRet = m_pMemPoolStack->ReMalloc(pPoint, nNewSize, bCopyOldDataFlag);
		if(m_pRegister)
		{
			if(pRet)
			{
				m_pRegister->Modify(pPoint, pRet);
			}
			else
			{
				m_pRegister->Del(pPoint);
			}
		}

		
	}
	return pRet;
}

void KMemoryPool::Register( void* pPoint, char* pszInfo )
{
	if(m_pRegister)
	{
		m_pRegister->Add(pPoint, pszInfo);
	}
}

void KMemoryPool::UnRegister( void* pPoint )
{
	if(m_pRegister)
	{
		m_pRegister->Del(pPoint);
	}
}

void KMemoryPool::PrintTree()
{
	if(m_pMemPoolStack)
	{
		m_pMemPoolStack->PrintStack();
	}
}

void KMemoryPool::PrintInfo()
{
	if(m_pRegister)
	{
		m_pRegister->PrintInfo();
	}

	if(m_pMemPoolStack)
	{
		m_pMemPoolStack->PrintInfo();
	}
}





