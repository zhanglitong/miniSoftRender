#pragma     once
#include    "FELight.h"
#include    "FELightPoint.h"
#include    "FELightDir.h"

namespace   FE
{
    class   FELightMgr 
        : public FEObject
        , public FEKeyValues<String,Light>
    {
    public:
        FELightMgr(FEContext& ctx)
            :FEObject(ctx)
        {}
        FELightMgr(const FELightMgr& other)
            :FEObject(other)
        {}
        /// <summary>
        /// 获取所有可见灯光对象数量
        /// </summary>
        /// <returns></returns>
        size_t  count() const
        {
            size_t  cnt   =   0;
            for (auto& var : data())
            {
                cnt +=  var.second->flags().hasFlag(FLAG_VISIBLE) ? 1 :0;
            }
            return  cnt;
        }
        /// <summary>
        /// 获取灯光数据实际占用的内存大小
        /// </summary>
        /// <returns></returns>
        size_t  bytes() const
        {
            return  count() * sizeof(LightData);
        }
    };

    using   LightMgr    =   SharedPtr<FELightMgr>;
}