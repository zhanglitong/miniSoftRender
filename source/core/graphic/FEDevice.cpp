
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
        /// 先等 GPU 空闲,确保后续销毁管线/命令池/缓存不会触发 "device is busy" 验证报错
        waitIdle();
        _pipelineMgr.clear();
        _cacheObject.clear();
        _graphicPool    =   nullptr;
        _computePool    =   nullptr;
        _transferPool   =   nullptr;
        /// FELightMgr 由侵入式引用计数(FERef)管理,置空即 decRef 自动释放;
        /// 手动 delete 会 double-free(隐式 operator T* 使 delete 可编译,故无告警)
        _lightsMgr      =   nullptr;
    }
}
