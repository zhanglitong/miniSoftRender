#pragma     once
#include    "FEMaterialPoint.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEMaterialPoint565,"{9F3B2C71-5A4D-4E86-8C21-7B0D54A1E9C2}");

    /// <summary>
    /// RGB565 点材质:顶点为 8 字节紧凑格式(ushort xyz + RGB565)，
    /// 默认使用顶点颜色，点大小按相机距离衰减并钳制在 1~16 像素，适用于 LAS 等大规模点云。
    /// </summary>
    class   FE_API  FEMaterialPoint565 : public FEMaterialPoint
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEMaterialPoint565)
    public:
        FEMaterialPoint565(FEContext& ctx)
            :FEMaterialPoint(ctx,"point565")
        {
            /// 覆盖标记=0(使用顶点颜色)，点的 min/max 为 1/8 像素，低字节非 0 表示点
            /// 常量色置 0:与片段着色器的 inColor + color*color 组合后不影响顶点颜色
            _pushConst._point   =   (0u << 24) | (1u << 16) | (8u << 8) | 0xFFu;
            _pushConst._color   =   0x00000000u;
            update();
        }
        FEMaterialPoint565(const FEMaterialPoint565& other)
            :FEMaterialPoint(other)
        {}
        virtual ~FEMaterialPoint565()
        {}
    };

    using   MaterialPoint565    =   SharedPtr<FEMaterialPoint565>;
}
