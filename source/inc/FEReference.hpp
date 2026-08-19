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
        /// 寮曠敤璁℃暟瀵硅薄
        /// </summary>
        mutable AtomicInt   _refCnt =   0;
        /// <summary>
        /// 琛ラ綈鍐呭瓨浣跨敤
        /// 鍙互琚瓙绫婚噸鍐欒В閲婂苟浣跨敤
        /// </summary>
        TFlags              _flags  =   0;
    public:
	    FEReference()
        {}
        /// <summary>
        /// 鑾峰彇寮曠敤璁℃暟
        /// </summary>
        /// <returns></returns>
        inline  int     refCnt() const
        {
            return  _refCnt;
        }
        /// <summary>
        /// 澧炲姞寮曠敤璁℃暟
        /// </summary>
        inline  void    incRef() const
        {
            _refCnt.fetch_add(1, std::memory_order_relaxed);
        }
        /// <summary>
        /// 閲婃斁鏁版嵁锛屽洜涓烘湁鍐呭瓨瀵硅薄绠＄悊鍥炴敹
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
