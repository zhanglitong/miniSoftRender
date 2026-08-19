
#include    "../inc/graphic/FEDevice.h"

namespace   FE
{
    bool    FEDevice::cacheObject(Object object)
    {
        auto    itr =   std::lower_bound(_cacheObject.begin(), _cacheObject.end(), object,[](const Object& left,const Object& right)
            {
                return  left->objectId() < right->objectId();
            });
        if (itr != _cacheObject.end() && (*itr)->objectId() == object->objectId())
            return  false;
        _cacheObject.insert(itr,object);
        return  true;
    }

    Object  FEDevice::queryCache(const FEUuid& id) const
    {
        auto    itr =   std::lower_bound(_cacheObject.begin(), _cacheObject.end(), id,[](const Object& left,const FEUuid& right)
            {
                return  left->objectId() < right;
            });
        if (itr != _cacheObject.end() && (*itr)->objectId() == id)
            return  (*itr);
        else
            return  nullptr;
    }


    void    FEDevice::destroy()
    {
        _pipelineMgr.clear();
        _cacheObject.clear();
        _graphicPool    =   nullptr;
        _computePool    =   nullptr;
        _transferPool   =   nullptr;
    }
}
