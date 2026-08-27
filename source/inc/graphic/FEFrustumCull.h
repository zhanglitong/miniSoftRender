#pragma     once

#include    "../FECamera.hpp"
#include    "FEFactoryRender.hpp"

namespace   FE
{   
    class   FEFrustumCull:public FEObject
    {
    public:
        FEFrustumCull(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEFrustumCull(const FEFrustumCull& other)
            :FEObject(other)
        {}
    public:
        /// <summary>
        /// 执行测试函数
        /// </summary>
        /// <param name="camer"></param>
        /// <param name="factorys"></param>
        void    compute(Camera camer,const RFactorys& factorys);
    };
    using   FrustumCull =   SharedPtr<FEFrustumCull>;
}
