#pragma     once
#include    "FEGeometryCylinder.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryCone,"{80CD6245-3FC9-4FBC-A390-F8E1507B22E5}");
    /// <summary>
    /// 目标形状	radiusTop	radiusBottom	radialSegments
    /// 标准圆锥	0	1	32 (或更高)
    /// 三棱锥	    0	1	3
    /// 四棱锥     (金字塔)	0	1	4
    /// 五棱锥	    0	1	5
    /// 六棱锥	    0	1	6
    /// </summary>
    class   FEGeometryCone : public FEGeometryCylinder
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryCone)
       
    public:
		/// <summary>
        /// 构造设置输入顶点的属性数据
        /// </summary>
        FEGeometryCone(FEContext& ctx)
            : FEGeometryCylinder(ctx)
        {
            _param.radiusTop    =   0.0f;
        }
        FEGeometryCone(const FEGeometryCone& other)
            : FEGeometryCylinder(other)
        {
            _param.radiusTop    =   other._param.radiusTop;
        }
        virtual ~FEGeometryCone()
        {}
    };
}
