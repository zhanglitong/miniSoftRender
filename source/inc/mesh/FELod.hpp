#pragma     once

#include    <type_traits>
#include    <functional>
#include    "../FEMath.hpp"
#include    "../FEObject.h"
namespace   FE
{
   
    struct  FELodInf
    {
        uint    firstIndex  =   {};
        uint    indexCount  =   {};
        float   distance    =   {};
    };
    /// <summary>
    /// 一个mesh 对应一个FELod,也可以没有
    /// 目前LOD 最大四级
    /// 默认情况下，按照距离填写，从小到大
    /// 如果不不足4级，后面的重复，例如只有三级别，那么第四级数据用第三级数据填充
    /// </summary>
    class   FELod :public FEObject
    {
    public:
        FELod(FEContext&ctx)
            :FEObject(ctx)
        {}
        FELod(const FELod& other)
            :FEObject(other)
        {
            _lod[0] =   other._lod[0];
            _lod[1] =   other._lod[1];
            _lod[2] =   other._lod[2];
            _lod[3] =   other._lod[3];
        }
    public:
        FELodInf    _lod[4]   =   {};
    };
   using    Lod     =   SharedPtr<FELod>;
}

