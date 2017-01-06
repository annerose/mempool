#ifndef NEWOPERATOR_H
#define NEWOPERATOR_H

 
// 重载的operator new的参数个数任意，但第一个参数必须是size_t类型的，返回值必须是void* 
void* operator new(size_t nSize, char* pszFileName, int nLineNum);
void* operator new[](size_t nSize, char* pszFileName, int nLineNum);

// 操作符 delete 无法使用其他参数....
//void operator delete(void *ptr, size_t nSize);
//void operator delete[](void *ptr, size_t nSize);

// 为了避免编译时出现warning C4291(没有与operator new(unsigned int,const char *,const unsigned int) 匹配的delete)，又重载了带参数的delete
// 尽管没什么用
void operator delete(void *ptr, char* pszFileName, int nLineNum);
void operator delete[](void *ptr, char* pszFileName, int nLineNum);


void FreePool(void* ptr);

#define POOL_NEW  new (__FILE__, __LINE__)
#define new POOL_NEW

//#define POOL_DELETE delete(__FILE__, __LINE__)
//#define delete POOL_DELETE
 
//#define DEBUG_DELETE    GetMonitorConsoleSinglton()->RecordDeletePosition(__FILE__, __LINE__); delete
//#define POOL_DELETE  delete(__LINE__)
//#define delete POOL_DELETE

//  将malloc/free 用new/delete替换
//#define malloc(s) ((void*)new unsigned char[s])
//#define free(p)   (delete [] (char*)(p));



#endif // NEWOPERATOR_H
 

