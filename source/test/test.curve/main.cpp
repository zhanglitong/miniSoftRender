/// test.curve
/// 样条曲线(移植自 three.js r160)无窗口单元测试:
///   FELineCurve3 / FEQuadraticBezierCurve3 / FECubicBezierCurve3
///   FECatmullRomCurve3 / FECurve 弧长重参数化 / Frenet 标架 / FETubeImpl
/// 返回 0 全部通过, 非 0 存在失败项

#include    <cstdio>
#include    <cmath>
#include    <string>

#include    "../../inc/FEMath.hpp"
#include    "../../inc/geometry/curve/FELineCurve3.hpp"
#include    "../../inc/geometry/curve/FEQuadraticBezierCurve3.hpp"
#include    "../../inc/geometry/curve/FECubicBezierCurve3.hpp"
#include    "../../inc/geometry/curve/FECatmullRomCurve3.hpp"
#include    "../../inc/geometry/detail/FETubeImpl.hpp"

using   namespace   FE;

static  uint    g_pass  =   0;
static  uint    g_fail  =   0;

static  void    check(bool cond, const std::string& name)
{
    if (cond)
    {
        ++g_pass;
        printf("[PASS] %s\n", name.c_str());
    }
    else
    {
        ++g_fail;
        printf("[FAIL] %s\n", name.c_str());
    }
}

static  bool    nearEq(float a, float b, float eps = 1e-4f)
{
    return  std::fabs(a - b) <= eps;
}

static  bool    nearEq(const float3& a, const float3& b, float eps = 1e-4f)
{
    return  nearEq(a.x, b.x, eps) && nearEq(a.y, b.y, eps) && nearEq(a.z, b.z, eps);
}

static  bool    isUnit(const float3& v, float eps = 1e-3f)
{
    return  nearEq(FE::length(v), 1.0f, eps);
}

/// 直线段
static  void    testLineCurve()
{
    FELineCurve3    line(float3(0, 0, 0), float3(2, 0, 0));

    check(nearEq(line.getPoint(0.0f), float3(0, 0, 0)),   "Line.getPoint(0) == v1");
    check(nearEq(line.getPoint(1.0f), float3(2, 0, 0)),   "Line.getPoint(1) == v2");
    check(nearEq(line.getPoint(0.5f), float3(1, 0, 0)),   "Line.getPoint(0.5) 中点");
    check(nearEq(line.getPointAt(0.25f), float3(0.5f, 0, 0)), "Line.getPointAt 弧长参数等价 t");
    check(nearEq(line.getTangent(0.3f), float3(1, 0, 0)), "Line.getTangent 解析切线");
    check(nearEq(line.getLength(), 2.0f),                 "Line.getLength == 2");
}

/// 二次贝塞尔
static  void    testQuadraticBezier()
{
    FEQuadraticBezierCurve3  q(float3(0, 0, 0), float3(1, 2, 0), float3(2, 0, 0));

    check(nearEq(q.getPoint(0.0f), float3(0, 0, 0)), "Quad.getPoint(0) == v0");
    check(nearEq(q.getPoint(1.0f), float3(2, 0, 0)), "Quad.getPoint(1) == v2");
    /// B(0.5) = 0.25 v0 + 0.5 v1 + 0.25 v2 = (1, 1, 0)
    check(nearEq(q.getPoint(0.5f), float3(1, 1, 0)), "Quad.getPoint(0.5) == (1,1,0)");

    /// 端点切线分别指向/背离控制点
    check(nearEq(q.getTangent(0.0f), FE::normalize(float3(1, 2, 0))),
          "Quad.getTangent(0) 指向 v1");
    check(nearEq(q.getTangent(1.0f), FE::normalize(float3(1, -2, 0))),
          "Quad.getTangent(1) 背离 v1");
}

/// 三次贝塞尔
static  void    testCubicBezier()
{
    FECubicBezierCurve3  c(float3(0, 0, 0), float3(0, 2, 0),
                           float3(2, 2, 0), float3(2, 0, 0));

    check(nearEq(c.getPoint(0.0f), float3(0, 0, 0)), "Cubic.getPoint(0) == v0");
    check(nearEq(c.getPoint(1.0f), float3(2, 0, 0)), "Cubic.getPoint(1) == v3");
    /// B(0.5) = 0.125 v0 + 0.375 v1 + 0.375 v2 + 0.125 v3 = (1, 1.5, 0)
    check(nearEq(c.getPoint(0.5f), float3(1, 1.5f, 0)), "Cubic.getPoint(0.5) == (1,1.5,0)");
    check(isUnit(c.getTangent(0.5f)), "Cubic.getTangent 单位长度");
}

/// Catmull-Rom 样条
static  void    testCatmullRom()
{
    float3s pts = {
        float3(0, 0, 0), float3(1, 1, 0),
        float3(2, 0, 0), float3(3, 1, 0),
    };

    /// 均匀参数化: 曲线严格插值控制点
    FECatmullRomCurve3  uniform(pts, false, FECatmullRomType::Uniform);
    check(nearEq(uniform.getPoint(0.0f), pts[0]), "CatmullRom uniform 起点插值");
    check(nearEq(uniform.getPoint(1.0f), pts[3]), "CatmullRom uniform 终点插值");
    check(nearEq(uniform.getPoint(1.0f / 3.0f), pts[1], 1e-3f),
          "CatmullRom uniform 过内部控制点 p1");
    check(nearEq(uniform.getPoint(2.0f / 3.0f), pts[2], 1e-3f),
          "CatmullRom uniform 过内部控制点 p2");

    /// 向心参数化: 端点插值
    FECatmullRomCurve3  centripetal(pts, false, FECatmullRomType::Centripetal);
    check(nearEq(centripetal.getPoint(0.0f), pts[0]), "CatmullRom centripetal 起点插值");
    check(nearEq(centripetal.getPoint(1.0f), pts[3]), "CatmullRom centripetal 终点插值");

    /// 闭合: 首尾点重合
    FECatmullRomCurve3  closed(pts, true, FECatmullRomType::Centripetal);
    check(nearEq(closed.getPoint(0.0f), closed.getPoint(1.0f), 1e-3f),
          "CatmullRom closed 首尾点重合");

    /// getPoints 数量
    auto    sampled =   centripetal.getPoints(10);
    check(sampled.size() == 11, "CatmullRom getPoints(10) 返回 11 个点");

    /// 无尖点: 采样点切线均为有效单位向量
    bool    tangentOk   =   true;
    for (uint i = 0; i <= 20; ++i)
    {
        if (!isUnit(centripetal.getTangent((float)i / 20.0f)))
            tangentOk   =   false;
    }
    check(tangentOk, "CatmullRom centripetal 切线均单位化");
}

/// 弧长重参数化
static  void    testArcLength()
{
    /// 对角直线, 长度 = sqrt(3)*2
    FELineCurve3    diag(float3(0, 0, 0), float3(2, 2, 2));
    check(nearEq(diag.getLength(), std::sqrt(12.0f), 1e-2f),
          "ArcLength 对角线长度 2*sqrt(3)");

    /// getUtoTmapping 端点
    check(nearEq(diag.getUtoTmapping(0.0f), 0.0f), "getUtoTmapping(0) == 0");
    check(nearEq(diag.getUtoTmapping(1.0f), 1.0f), "getUtoTmapping(1) == 1");

    /// getPointAt 等距: 弯曲曲线相邻等距点间距应近似相等
    FECatmullRomCurve3  curve(float3s{
        float3(-1, 0, 0), float3(-0.5f, 1.5f, 0.5f),
        float3(0.5f, -1.0f, -0.5f), float3(1, 0, 0) },
        false, FECatmullRomType::Centripetal);

    /// 等距是指"沿曲线弧长"等距而非弦长等距(曲率大处弦长短)
    /// 细分 800 个弧长等距点, 每 100 段求和得到 8 段弧长, 应近似相等
    const uint  coarse  =   8;
    const uint  fine    =   800;
    auto    pts =   curve.getSpacedPoints(fine);
    float   segLens[8] =   {};
    for (uint k = 0; k < coarse; ++k)
        for (uint i = 0; i < fine / coarse; ++i)
        {
            uint    a   =   k * (fine / coarse) + i;
            segLens[k]  +=  FE::distance(pts[a], pts[a + 1]);
        }
    float   total   =   0.0f, sMax = 0.0f, sMin = 1e9f;
    for (float L : segLens) { total += L; sMax = std::max(sMax, L); sMin = std::min(sMin, L); }
    check(nearEq(total, curve.getLength(), 1e-2f), "getSpacedPoints 弧长总和 == getLength");
    check((sMax - sMin) / sMax < 0.02f, "getSpacedPoints 弧长等距分段(误差<2%)");
}

/// Frenet 标架
static  void    testFrenetFrames()
{
    FEQuadraticBezierCurve3  curve(float3(0, 0, 0), float3(1, 2, 1), float3(2, 0, 0));
    const uint  seg =   32;
    FEFrenetFrames  f   =   curve.computeFrenetFrames(seg, false);

    check(f.tangents.size() == seg + 1 && f.normals.size() == seg + 1 &&
          f.binormals.size() == seg + 1, "Frenet 标架数组长度 = segments+1");

    bool    orthonormal =   true;
    for (uint i = 0; i <= seg; ++i)
    {
        const float3&   T   =   f.tangents[i];
        const float3&   N   =   f.normals[i];
        const float3&   B   =   f.binormals[i];
        if (!isUnit(T) || !isUnit(N) || !isUnit(B))
            orthonormal =   false;
        if (!nearEq(FE::dot(T, N), 0.0f, 1e-2f))
            orthonormal =   false;
        if (!nearEq(B, FE::cross(T, N), 2e-2f))
            orthonormal =   false;
    }
    check(orthonormal, "Frenet 标架逐点正交归一(T⊥N, B=T×N)");

    /// 闭合曲线首尾法线一致
    FECatmullRomCurve3  closed(float3s{
        float3(1, 0, 0), float3(0, 1, 0),
        float3(-1, 0, 0), float3(0, -1, 0) },
        true, FECatmullRomType::Centripetal);
    FEFrenetFrames  cf  =   closed.computeFrenetFrames(48, true);
    check(nearEq(cf.normals[0], cf.normals[48], 5e-2f),
          "Frenet closed 首尾法线一致");
}

/// 管道网格生成
static  void    testTubeBuild()
{
    FEParamTube param;
    param.type              =   FECurveType::CubicBezier;
    param.points            =   {
        float3(0, 0, 0), float3(0, 2, 1),
        float3(2, 2, -1), float3(2, 0, 0),
    };
    param.radius            =   0.2f;
    param.tubularSegments   =   16;
    param.radialSegments    =   8;
    param.closed            =   false;

    FETubeImpl  data;
    FETubeImpl::build<true, true>(param, data);

    const uint  ringSize    =   param.radialSegments + 1;
    const uint  ringCount   =   param.tubularSegments + 1;

    check(data.vertices.size() == ringCount * ringSize * 3,
          "Tube 顶点数 = (tubular+1)*(radial+1)*3");
    check(data.normals.size()  == ringCount * ringSize * 3,
          "Tube 法线数与顶点一致");
    check(data.uvs.size()      == ringCount * ringSize * 2,
          "Tube UV 数 = 顶点数*2");
    check(data.indices.size()  == param.tubularSegments * param.radialSegments * 6,
          "Tube 索引数 = tubular*radial*6");

    /// 每个截面环顶点到曲线点距离 == 半径, 且法线单位化
    auto    curve   =   FETubeImpl::createCurve(param);
    bool    radiusOk    =   true;
    bool    normalOk    =   true;
    for (uint i = 0; i <= param.tubularSegments && radiusOk; ++i)
    {
        float3  center  =   curve->getPointAt((float)i / param.tubularSegments);
        for (uint j = 0; j <= param.radialSegments; ++j)
        {
            uint    k   =   i * ringSize + j;
            float3  v(data.vertices[k * 3], data.vertices[k * 3 + 1], data.vertices[k * 3 + 2]);
            float3  n(data.normals[k * 3], data.normals[k * 3 + 1], data.normals[k * 3 + 2]);
            if (!nearEq(FE::distance(v, center), param.radius, 2e-3f))
                radiusOk    =   false;
            if (!isUnit(n, 2e-3f))
                normalOk    =   false;
        }
    }
    check(radiusOk, "Tube 截面顶点到曲线距离恒为半径");
    check(normalOk, "Tube 法线全部单位化");

    /// 索引不越界
    uint    vCount  =   ringCount * ringSize;
    bool    rangeOk =   true;
    for (uint idx : data.indices)
        if (idx >= vCount) rangeOk = false;
    check(rangeOk, "Tube 索引全部在顶点范围内");

    /// 非法参数(点数不足)安全返回空网格
    FEParamTube bad;
    bad.type    =   FECurveType::CubicBezier;
    bad.points  =   { float3(0, 0, 0) };
    FETubeImpl  empty;
    FETubeImpl::build<true, true>(bad, empty);
    check(empty.vertices.empty() && empty.indices.empty(),
          "Tube 控制点不足时安全返回空数据");
}

int     main(int, char**)
{
    printf("==== curve unit tests ====\n");
    testLineCurve();
    testQuadraticBezier();
    testCubicBezier();
    testCatmullRom();
    testArcLength();
    testFrenetFrames();
    testTubeBuild();

    printf("==== summary: %u passed, %u failed ====\n", g_pass, g_fail);
    return  g_fail == 0 ? 0 : 1;
}
