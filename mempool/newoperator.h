////////////////////////////////////////////////////////////////////////////////////////////
/// @file   newoperator.h
/// @purpose:  重定义new和delete，使之使用内存池. 作用范围：include 该头文件的代码
///
/// 
/// @version 1.0
/// @date  2017/01/10
/// 
/// modify history:
/// [modified sequence] [modification date] [modifier] [changes]
////////////////////////////////////////////////////////////////////////////////////////////
#ifndef NEWOPERATOR_H
#define NEWOPERATOR_H


// 重载的operator new的参数个数任意，但第一个参数必须是size_t类型的，返回值必须是void* 
void* operator new(size_t nSize, char* pszFileName);
void* operator new[](size_t nSize, char* pszFileName);

// 操作符 delete 无法使用其他参数....
//void operator delete(void *ptr, size_t nSize);
//void operator delete[](void *ptr, size_t nSize);

// 为了避免编译时出现warning C4291(没有与operator new(unsigned int,const char *,const unsigned int) 匹配的delete)，又重载了带参数的delete
// 尽管没什么用
// 直接调用operator delete 不会调用析构函数
void operator delete(void *ptr, char* pszFileName);
void operator delete[](void *ptr, char* pszFileName);

// #### 尽量不要重载单个参数的 operator delete, 会影响全局
// 释放内存池的空间
void FreePool(void* ptr);

// 数字宏转字符串宏
#define KXSTR(s) KSTR(s)
#define KSTR(s) #s
 


//#define POOL_NEW  new ( __FILE__   , __LINE__)

// 文件名和行号合成一个字符串
#define new  new ( __FILE__  "," KXSTR(__LINE__) )

// FAQ: Is there a placement delete?  https://isocpp.org/wiki/faq/dtors   
// 采用模板实现析构函数和内存池释放的结合
template<class T> void PoolDestroy(T* p)
{
	if (p) 
	{
		// 必须手动调用析构
		p->~T();        // explicit destructor call
		// 再释放内存池
		FreePool(p);
	}
}


// 重定义delete操作，支持不用括号的传值 
struct PoolDelObj
{
	template<class T> 
	void operator+(const T &ptr)
	{ 
		PoolDestroy(ptr);
	}

	// 操作符[]中间必须有index
	//PoolDelObj  operator [] (int i) 
	//{ 
	//	return *this;
	//}

	// 并没有什么用
	//void operator delete [] (void* ptr)
	//{

	//}
};


// 不能用重定义new的方式定义delete
//#define delete  operator delete(p, __FILE__, __LINE__)

// 宏第一个参数不支持[]
//#define delete[](p) operator delete[](p, __FILE__, __LINE__) 

// 重定义delete ok，但是无法重定义 delete[], 所以放弃该方案
//#define delete	PoolDelObj()  +
 
 
//  将malloc/free 用new/delete替换
//#define malloc(s) ((void*)new unsigned char[s])
//#define free(p)   (delete [] (char*)(p));

// 安全删除一个需调用delete释放的对象
#define SAFE_DELETE(pObject) if(pObject) {PoolDestroy (pObject); (pObject) = NULL;}

// 安全删除一组需调用delete[]释放的对象
#define SAFE_DELETE_GROUP(pObject) if(pObject) {PoolDestroy (pObject); (pObject) = NULL;}

// 不允许显示使用delete
#define delete	#error hahahahha

#endif // NEWOPERATOR_H
 

