//
//  lock free队列
//  读写并发模式：支持1对n，不支持n对n，既可以1线程读n个线程写并发，或者n个线程读1个线程写并发
//  不可以n个线程读n个线程写并发
//////////////////////////////////////////////////////////////////////

#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#endif

namespace lock_free
{
 
typedef int                 int32;  
typedef unsigned int        uint32;  
 
//原子加
inline uint32 AtomAdd(void * var, const uint32 value)
{
#ifdef WIN32
    return InterlockedExchangeAdd((long *)(var), value); // NOLINT
#else
    return __sync_fetch_and_add((uint32 *)(var), value);  // NOLINT
#endif
}
 
//原子减
inline uint32 AtomDec(void * var, int32 value)
{
    value = value * -1;
#ifdef WIN32
    return InterlockedExchangeAdd((long *)(var), value); // NOLINT
#else
    return __sync_fetch_and_add((uint32 *)(var), value);  // NOLINT
#endif
}  
 
typedef struct QUEUE_NODE
{
    bool IsEmpty;
    void *pObject;
}QUEUE_NODE;
 
class Queue  
{
public:
    Queue( int32 nSize );
    virtual ~Queue();
 
public:
    bool Push( void *pObject );
    void* Pop();
    void Clear();//清除数据
protected:
     
private:
    QUEUE_NODE *m_queue;
    uint32 m_push;
    uint32 m_pop;
    uint32 m_nSize;
    int32 m_nWriteAbleCount;
    int32 m_nReadAbleCount;
};
 
#ifndef NULL
#define NULL 0
#endif
 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
Queue::Queue( int32 nSize )
{
    m_nSize = nSize;
    m_nWriteAbleCount = m_nSize;
    m_nReadAbleCount = 0;
    m_queue = new QUEUE_NODE[m_nSize];
    m_push = 0;
    m_pop = 0;
    Clear();
}
 
Queue::~Queue()
{
    if ( NULL == m_queue ) return;
    delete[]m_queue;
}
 
bool Queue::Push( void *pObject )
{
    if ( 0 >= m_nWriteAbleCount ) return false;//列队已满
    if ( 0 >= AtomDec(&m_nWriteAbleCount,1) ) //已经有足够多的push操作向列队不同位置写入数据
    {
        AtomAdd(&m_nWriteAbleCount, 1);
        return false;
    }
    uint32 pushPos = AtomAdd(&m_push, 1);
    pushPos = pushPos % m_nSize;
    /*
        只有在NPop并发情况下，因Pop无序完成，第一个位置的Pop未完成，后面的Pop就先完成提示有空位
        因为该类只允许1对N，所以必然是单线程Push，所以条件内push控制变量不需要原子操作
     */
    if ( !m_queue[pushPos].IsEmpty ) 
    {
        m_push--;
        m_nWriteAbleCount++;
        return false;
    }
    m_queue[pushPos].pObject = pObject;
    m_queue[pushPos].IsEmpty = false;
    AtomAdd(&m_nReadAbleCount,1);
     
     
    return true;
}
 
void* Queue::Pop()
{
    if ( 0 >= m_nReadAbleCount ) return NULL;//空列队
    if ( 0 >= AtomDec(&m_nReadAbleCount,1)) //已经有足够多的pop操作读取列队不同位置的数据
    {
        AtomAdd(&m_nReadAbleCount, 1);
        return NULL;
    }
    uint32 popPos = AtomAdd(&m_pop, 1);
    popPos = popPos % m_nSize;
    /*
        只有在NPush并发情况下，因Push无序完成，第一个位置的Push未完成，后面的Push就先完成提示有数据
        因为该类只允许1对N，所以必然是单线程Pop，所以条件内Pop控制变量不需要原子操作
     */
    if ( m_queue[popPos].IsEmpty )
    {
        m_nReadAbleCount++;
        m_pop--;
    }
    void *pObject = m_queue[popPos].pObject;
    m_queue[popPos].pObject = NULL;
    m_queue[popPos].IsEmpty = true;
    AtomAdd(&m_nWriteAbleCount,1);
 
    return pObject;
}
 
void Queue::Clear()
{
    if ( NULL == m_queue ) return;
    uint32 i = 0;
    m_nWriteAbleCount = m_nSize;
    m_nReadAbleCount = 0;
    for ( i = 0; i < m_nSize; i++ )
    {
        m_queue[i].IsEmpty = true;
        m_queue[i].pObject = NULL;
    }
}
 
}