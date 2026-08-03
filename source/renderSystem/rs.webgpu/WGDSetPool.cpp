#include    "WGDSetPool.h"

namespace   FE
{
    WGDSetPool::~WGDSetPool()
    {
    }

    bool WGDSetPool::create(const CreateInfo& cInf)
    {
        _cInfo =   cInf;
        return true;
    }

    void WGDSetPool::alloc()
    {
        _count++;
    }

    void WGDSetPool::free()
    {
        if (_count > 0)
            _count--;
    }
}