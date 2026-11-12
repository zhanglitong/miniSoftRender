#pragma     once

#include    <type_traits>
#include    <functional>
#include    "../FEMath.hpp"
#include    "../FEObject.h"
namespace   FE
{
   
    struct  FELodInf
    {
        uint    start       =   {};
        uint    count       =   {};
        float   dist        =   {};
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
        /// <summary>
        /// 校验 LOD 数据是否自洽。total 为该图元的几何总量
        /// (索引绘制取索引数,非索引绘制取顶点数,与命令 count 同单位):
        /// - L0 必须有效(count>0),否则 shader 会把命令的 count 清成 0,物体消失
        /// - 有效级必须连续在前(shader 遇到 count==0 即停止)
        /// - start+count 不能超出 total(否则 GPU 越界读 IBO / 顶点流)
        /// - dist 必须非降序(shader 按"最后一个 dist<=距离"选级,乱序会选错)
        /// </summary>
        bool    valid(uint total) const
        {
            if (total == 0 || _lod[0].count == 0)
                return  false;
            float   lastDist    =   -1.0f;
            bool    tail        =   false;
            for (uint i = 0; i < 4; ++ i)
            {
                const FELodInf& inf =   _lod[i];
                if (inf.count == 0)
                {
                    tail    =   true;
                    continue;
                }
                /// 有效项必须连续在前
                if (tail)
                    return  false;
                if (uint64(inf.start) + inf.count > total)
                    return  false;
                if (inf.dist < lastDist)
                    return  false;
                lastDist    =   inf.dist;
            }
            return  true;
        }
    public:
        FELodInf    _lod[4]   =   {};
    };
   using    Lod     =   SharedPtr<FELod>;
}

