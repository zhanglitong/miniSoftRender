#pragma     once
#include    "../../FEObject.h"
#include    "FEBoxImpl.hpp"

namespace   FE
{
    struct  FEParamEllipsoidEGM
    {
        /// <summary>
        /// 椭球体的大小
        /// </summary>
        float3  _size   =   {1,1,1};
        /// <summary>
        /// 分段数量
        /// </summary>
        uint3   _segs   =   {10,10,10};
    };

    class   FEEllipsoidEGMImpl
    {
    public:
        uints       indices;
        floats      vertices;
        floats      normals;
        floats      uvs;
        uint32_t    numberOfVertices    =   0;
    public:
        template<bool bNormal,bool bUV>
        static  void    build(const FEParamEllipsoidEGM& params,FEEllipsoidEGMImpl& result)
        {
            /// 先生成立方体网格
            FEParamBox  boxParam;
            boxParam._size  =   params._size;
            boxParam._segs  =   params._segs;
            FEBoxImpl   box;
            FEBoxImpl::build<bNormal,bUV>(boxParam,box);

            result.vertices         =   box.vertices;
            result.normals          =   box.normals;
            result.uvs              =   box.uvs;
            result.indices          =   box.indices;
            result.numberOfVertices =   box.numberOfVertices;

            /// 将立方体顶点投影到椭球面上 (Elliptical Grid Mapping)
            const float3  half    =   params._size * 0.5f;
            spherify<bNormal>(result, half);
        }
    protected:
        template<bool bNormal>
        static  void    spherify(FEEllipsoidEGMImpl& result, const float3& half)
        {
            auto&   vertices   =   result.vertices;
            auto&   normals    =   result.normals;

            uint    len =   (uint)(vertices.size() / 3);
            for (uint i = 0; i < len; ++i)
            {
                const auto  idx =   i * 3;
                /// 归一化到单位立方体 [-1, 1]
                const auto  nx  =   vertices[idx + 0] / half.x;
                const auto  ny  =   vertices[idx + 1] / half.y;
                const auto  nz  =   vertices[idx + 2] / half.z;

                /// 应用球面投影公式 (Elliptical Grid Mapping)
                const auto  sx  =   nx * std::sqrt(1.0f - (ny * ny / 2.f) - (nz * nz / 2.f) + (ny * ny * nz * nz / 3.f));
                const auto  sy  =   ny * std::sqrt(1.0f - (nz * nz / 2.f) - (nx * nx / 2.f) + (nz * nz * nx * nx / 3.f));
                const auto  sz  =   nz * std::sqrt(1.0f - (nx * nx / 2.f) - (ny * ny / 2.f) + (nx * nx * ny * ny / 3.f));

                /// 缩放回原始尺寸
                vertices[idx + 0]     =   sx * half.x;
                vertices[idx + 1]     =   sy * half.y;
                vertices[idx + 2]     =   sz * half.z;

                if constexpr (bNormal)
                {
                    /// 椭球面法线 = normalize((x/a², y/b², z/c²))
                    float3  vNor(sx / half.x, sy / half.y, sz / half.z);
                    vNor    =   FE::normalize(vNor);
                    normals[idx + 0]     =   vNor.x;
                    normals[idx + 1]     =   vNor.y;
                    normals[idx + 2]     =   vNor.z;
                }
            }
        }
    };
}
