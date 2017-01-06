#ifndef KMEMPOOL_H
#define KMEMPOOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <boost/thread/shared_mutex.hpp>  
#include <boost/thread/mutex.hpp>  
#include <boost/thread/shared_lock_guard.hpp> 

// boost 1.51 不支持 atomic

#define IN
#define OUT
#define INOUT

// 共享锁
typedef boost::shared_mutex rwmutex;  
typedef boost::shared_lock<rwmutex> readLock;  
typedef boost::unique_lock<rwmutex> writeLock;  

// 互斥锁
typedef boost::mutex wmutex;  
typedef boost::unique_lock<wmutex> exclusiveLock;

// 内存块头部
struct MemBlockHead
{
	unsigned long m_ulBlockSize;
	MemBlockHead* m_pNext;
};



// 基本内存块， 右枝
class KMemPoolStackToken
{
public:
	KMemPoolStackToken(IN int nBlockSize);
	~KMemPoolStackToken(void);

public:

	//************************************
	// @brief  description: 分配空间
	// @detail description:
	// @param :   int nSize	需要分配的空间大小
	// @param :   int & nAllBlockCout	分配后的block
	// @param :   int & nMemoryUse		分配后的实际空间大小
	// @return:   void*	分配后的内存
	// @method:   Malloc
	// @see   :
	// @note  : 
	//************************************
	void* Malloc(IN int nSize, OUT int& nAllBlockCout, OUT int& nMemoryUse);

	//************************************
	// @brief  description: 
	// @detail description:
	// @param :   void * pPoint	需要释放的内存
	// @param :   bool bCloseFlag	释放释放释放内存空间给系统
	// @return:   bool	释放成功
	// @method:   Free
	// @see   :
	// @note  : 
	//************************************
	bool Free(IN void* pPoint, IN  bool bCloseFlag);


	// 打印状态
	void PrintStack();

private:

	// 释放所有子节点
	void DestroySon(IN  MemBlockHead* pSon);

private:

	// 第一个可重用的内存块,链表结构
	MemBlockHead* m_pFirstSon;

	// 指向左枝下一节点
	KMemPoolStackToken* m_pNext;

	// 被对象的内存大小
	unsigned long m_ulBlockSize;

	// 分配出去的内存块,  应该用原子锁 boost::atomic
	int m_nBlockOutSide;

	// 保留空闲的内存块, 应该用原子锁 boost::atomic
	int m_nBlockInSide;

	// shared_mutex
	rwmutex m_rwMutex;


};



// 内存池, 左枝
class KMemPoolStack
{
public:
	KMemPoolStack();
	
	~KMemPoolStack();

	// 重新分配内存空间
	void* ReMalloc(void* pPoint, int nNewSize, bool bCopyOldDataFlag = false);

	void* Malloc(int nSize);

	bool Free(void* pPoint);

	void PrintStack();

	void PrintInfo();

	void SetCloseFlag(bool bCloseFlag = true);

private:

	// 左枝首节点
	KMemPoolStackToken* m_pHead;

	// 统计最大内存指针, 如果该内存一直增长，可能会有内存泄露, 应该用原子锁 boost::atomic
	int m_pMaxPoint;

	// 统计所有在用的内存块, 应该用原子锁 boost::atomic
	int m_nAllBlockCount;

	// 统计内存总字节数, 应该用原子锁 boost::atomic
	int m_nMemoryUse;

	//  直接释放内存标志, 不用内存池
	bool m_bCloseFlag;

};


// 说明文字最大长度 128 -4
#define  MEMORY_INFO_SIZE 124

struct MemoryRegisterInfo
{
	void* m_pPoint;
	char m_szInfo[MEMORY_INFO_SIZE];
};



class KMemoryRegister
{
public:
	KMemoryRegister();
	~KMemoryRegister();

	// 添加指针及文字说明
	void Add(void*  pPoint, char* szInfo);

	void Del(void* pPoint);

	// Remalloc更新指针
	void Modify(void* pOld, void* pNew);

	void PrintInfo(void);

private:

	// 把指针和说明copy到结构体MemoryRegisterInfo
	void RegisterCopy(MemoryRegisterInfo* pDest, void* pPoint, char* szInfo);

private:

	// 数组，需改成堆内存, KMemoryRegister本身需要占用内存
	MemoryRegisterInfo* m_arrMemRegisterInfo;

	// 数组使用单元
	int m_nUseMax;

	// 最大注册指针
	void* m_pMaxPoint;

	// 当前指针数
	int m_nPointCount;

	// 互斥mutex
	wmutex m_wMutex;
};



class KMemoryPool
{
public:
	KMemoryPool(bool bRegister = true);

	~KMemoryPool();

public:

	void Register(void* pPoint, char* pszInfo);
	void UnRegister(void* pPoint);

	void SetCloseFlag(bool bClose = true);

	void* ReMalloc(void* pPoint, int nNewSize, bool bCopyOldDataFlag = true);

	void* Malloc(int nSize, char* szInfo = NULL);

	bool Free(void* pBlcok);

	void PrintTree();

	void PrintInfo();

private:

	// 注册管理对象
	KMemoryRegister* m_pRegister;

	KMemPoolStack* m_pMemPoolStack;


};

#endif // KMEMPOOL_H
 

