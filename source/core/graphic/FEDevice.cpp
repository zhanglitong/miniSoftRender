
#include    "../inc/graphic/FEDevice.h"
#include    "../inc/graphic/FEConstUuid.h"
#include    <cstring>

namespace   FE
{
    bool    FEDevice::cacheObject(Object object)
    {
        /// 对象缓存是普通 vector,后台线程创建材质时会查/写,需与主线程互斥
        FEMutex::ScopeLock  lock(_gpuMutex);
        auto    itr =   std::lower_bound(_cacheObject.begin(), _cacheObject.end(), object,[](const Object& left,const Object& right)
            {
                return  left->objectId() < right->objectId();
            });
        if (itr != _cacheObject.end() && (*itr)->objectId() == object->objectId())
        {
            /// 同 ID 覆盖:场景折射纹理 resize 重建后需要替换缓存中的旧对象,
            /// 否则 queryCache 永远返回已失效的旧纹理
            *itr    =   object;
            return  true;
        }
        _cacheObject.insert(itr,object);
        return  true;
    }

    Object  FEDevice::queryCache(const FEUuid& id) const
    {
        FEMutex::ScopeLock  lock(_gpuMutex);
        auto    itr =   std::lower_bound(_cacheObject.begin(), _cacheObject.end(), id,[](const Object& left,const FEUuid& right)
        {
            return  left->objectId() < right;
        });
        if (itr != _cacheObject.end() && (*itr)->objectId() == id)
            return  (*itr);
        else
            return  nullptr;
    }

    const   FEUuid& FEDevice::defaultTextureId(EDefaultTex kind)
    {
        static  const   FEUuid  ids[DEFAULT_TEX_MAX]    =
        {
            FEConstUuid::DefaultWhiteTexId,
            FEConstUuid::DefaultNormalTexId,
            FEConstUuid::DefaultBlackTexId,
            FEConstUuid::DefaultBRDFTexId,
            FEConstUuid::DefaultBlackCubeId,
        };
        if (kind < 0 || kind >= DEFAULT_TEX_MAX)
            return  ids[0];
        return  ids[kind];
    }
    TexPtr  FEDevice::defaultTexture(EDefaultTex kind)
    {
        if (kind < 0 || kind >= DEFAULT_TEX_MAX)
            return  nullptr;
        const   FEUuid& id      =   defaultTextureId(kind);
        /// 命中缓存直接返回(创建后不会再变,无锁快查)
        Object  cached  =   queryCache(id);
        if (cached != nullptr)
            return  cached->as<FETexture>();
        /// 慢路径:与后台加载线程/渲染互斥;queryCache 用同一把递归锁,可重入
        FEMutex::ScopeLock  lock(_gpuMutex);
        cached  =   queryCache(id);
        if (cached != nullptr)
            return  cached->as<FETexture>();

        TexPtr  tex =   createTexture();
        if (tex == nullptr)
            return  nullptr;
        tex->setObjectId(id);

        const   bool    cube    =   (kind == DEFAULT_TEX_BLACK_CUBE);
        FETexture::CreateInfo   ci;
        ci._width       =   1;
        ci._height      =   1;
        ci._format      =   FMT_R8G8B8A8_UNORM;
        ci._magFilter   =   FILTER_LINEAR;
        ci._minFilter   =   FILTER_LINEAR;
        ci._addressMode =   cube ? ADDRESS_CLAMP_TO_EDGE : ADDRESS_REPEAT;
        ci._cube        =   cube;
        if (!tex->create(ci))
            return  nullptr;

        uint8   rgba[4] =   {255,255,255,255};
        switch (kind)
        {
        case DEFAULT_TEX_NORMAL:
            rgba[0] =   128;
            rgba[1] =   128;
            rgba[2] =   255;
            rgba[3] =   255;
            break;
        case DEFAULT_TEX_BLACK:
        case DEFAULT_TEX_BLACK_CUBE:
            rgba[0] =   0;
            rgba[1] =   0;
            rgba[2] =   0;
            rgba[3] =   255;
            break;
        case DEFAULT_TEX_BRDF:
            rgba[0] =   255;
            rgba[1] =   0;
            rgba[2] =   0;
            rgba[3] =   255;
            break;
        default:
            break;
        }
        if (!cube)
        {
            tex->update(rgba,sizeof(rgba));
        }
        else
        {
            uint8   faces[24];
            for (int i = 0; i < 6; ++i)
                memcpy(faces + i * 4,rgba,4);
            tex->update(faces,sizeof(faces));
        }
        cacheObject(Object(tex.get()));
        return  tex;
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
