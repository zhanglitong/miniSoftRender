#pragma     once

#include    <atomic>
#include    <assert.h>
#include    "FEFlags.hpp"
namespace   FE
{
    template<class TFlags>
    class   FEReference
    {
    protected:
        using   AtomicInt   =   std::atomic_int;
        using   AtomicPtr   =   std::atomic<FEReference*>;
        using   FactoryId   =   uint16_t;
    public:
        /// <summary>
        /// 引用计数对象
        /// </summary>
        mutable AtomicInt   _refCnt =   0;
        /// <summary>
        /// 补齐内存使用
        /// 可以被子类重写解释并使用
        /// </summary>
        TFlags              _flags  =   0;
    public:
	    FEReference()
        {}
        /// <summary>
        /// 获取引用计数
        /// </summary>
        /// <returns></returns>
        inline  int     refCnt() const
        {
            return  _refCnt;
        }
        /// <summary>
        /// 增加引用计数
        /// </summary>
        inline  void    incRef() const
        {
            _refCnt.fetch_add(1, std::memory_order_relaxed);
        }
        /// <summary>
        /// 释放数据，因为有内存对象管理回收
        /// </summary>
        inline  void    decRef() const
        {
            if (_refCnt.fetch_sub(1, std::memory_order_acq_rel) == 1) 
            {
                delete this;
            }
        }
    protected:
        virtual ~FEReference()  =   default;
    };
} 
