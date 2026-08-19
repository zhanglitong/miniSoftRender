#include    "SWDSet.h"
#include    "SWDevice.h"
#include    "SWDSetLayout.h"
#include    "SWDSetPool.h"
#include    "SWGPUBuffer.h"

namespace   FE
{
    SWDSet::~SWDSet()
    {
    }

    bool    SWDSet::create(const FEDSet::CreateInfo& cInf)
    {
        _cInfo  =   cInf;
        const FEDSetBindings&   layoutBinds =   cInf._layout->cInfo()._bindings;
        _cInfo._binds.resize(layoutBinds.size());
        for (size_t i = 0; i < layoutBinds.size(); i++)
        {
            _cInfo._binds[i]._binding   =   layoutBinds[i]._binding;
            _cInfo._binds[i]._type      =   layoutBinds[i]._descriptorType;
            _cInfo._binds[i]._name      =   layoutBinds[i]._name;
            _cInfo._binds[i]._typeName  =   layoutBinds[i]._typeName;
        }

        auto    pool    =   _cInfo._pool->as<SWDSetPool>();
        pool->alloc();

        return  true;
    }

    bool    SWDSet::update()
    {
        if (!_dirty)
            return  false;
        else
            _dirty  =   false;

        return  true;
    }
}
