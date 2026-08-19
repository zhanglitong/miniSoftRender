#pragma     once
#include    "FEGeometryCylinder.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometryPyramid,"{313FD65B-3AD8-4E4D-AEBC-4F91B15286CA}");
    /// <summary>
    /// 目标形状	radiusTop	radiusBottom	radialSegments
    /// 标准圆锥	0	1	32 (或更高)
    /// 三棱锥	    0	1	3
    /// 四棱锥     (金字塔)	0	1	4
    /// 五棱锥	    0	1	5
    /// 六棱锥	    0	1	6
    /// </summary>
    class   FEGeometryPyramid : public FEGeometryCylinder
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEGeometryPyramid)

    public:
        /// <summary>
        /// 构造设置输入顶点的属性数据
        /// </summary>
        FEGeometryPyramid(FEContext& ctx)
            : FEGeometryCylinder(ctx)
        {
            _param.radiusTop        =   0;
            _param.radiusBottom     =   1;
            _param.height           =   1;
            _param.radialSegments   =   4;
        }
        FEGeometryPyramid(const FEGeometryPyramid& other)
            : FEGeometryCylinder(other)
        {
            _param.radiusTop        =   other._param.radiusTop     ;
            _param.radiusBottom     =   other._param.radiusBottom  ;
            _param.height           =   other._param.height        ;
            _param.radialSegments   =   other._param.radialSegments;
        }
        virtual ~FEGeometryPyramid()
        {}
    };
}

