#pragma     once

#include    "FEMath.hpp"
#include    "math/FERay.hpp"

namespace   FE
{
    /// <summary>
    /// 射线与三角形相交检测(Moller-Trumbore算法)
    /// </summary>
    template<typename T>
    bool    intersectTriangle(const tvec3<T>& rayOrigin, const tvec3<T>& rayDir,
                              const tvec3<T>& v0, const tvec3<T>& v1, const tvec3<T>& v2,
                              T* outT, T* outU, T* outV)
    {
        const T EPSILON = T(1e-8);
        tvec3<T> edge1 = v1 - v0;
        tvec3<T> edge2 = v2 - v0;
        tvec3<T> h = cross(rayDir, edge2);
        T a = dot(edge1, h);
        if (a > -EPSILON && a < EPSILON)
            return false;

        T f = T(1.0) / a;
        tvec3<T> s = rayOrigin - v0;
        T u = f * dot(s, h);
        if (u < T(0.0) || u > T(1.0))
            return false;

        tvec3<T> q = cross(s, edge1);
        T v = f * dot(rayDir, q);
        if (v < T(0.0) || u + v > T(1.0))
            return false;

        T t = f * dot(edge2, q);
        if (t > EPSILON)
        {
            if (outT) *outT = t;
            if (outU) *outU = u;
            if (outV) *outV = v;
            return true;
        }
        return false;
    }
    /// <summary>
    /// 判断点是否在三角形内
    /// </summary>
    template<typename T>
    inline  bool    pointinTriangle(tvec3<T> A, tvec3<T> B, tvec3<T> C, tvec3<T> P)
    {
        tvec3<T> v0 = C - A;
        tvec3<T> v1 = B - A;
        tvec3<T> v2 = P - A;

        T dot00 = dot(v0, v0);
        T dot01 = dot(v0, v1);
        T dot02 = dot(v0, v2);
        T dot11 = dot(v1, v1);
        T dot12 = dot(v1, v2);

        T inverDeno = T(1) / std::fma(dot00 , dot11,-dot01 * dot01);

        T u = std::fma(dot11 , dot02, -dot01 * dot12) * inverDeno;
        if (u < 0 || u > T(1)) // if u out of range, return directly
        {
            return false;
        }
        T v = std::fma(dot00 , dot12 ,-dot01 * dot02) * inverDeno;
        if (v < 0 || v > T(1)) // if v out of range, return directly
        {
            return false;
        }

        return u + v <= T(1);
    }

    /// <summary>
    /// 计算点到线段的最近距离
    /// </summary>
    template<typename T>
    T       closeDistanceOnLine(const tvec3<T>& lineStart, const tvec3<T>& lineEnd, const tvec3<T>& point)
    {
        tvec3<T> ab = lineEnd - lineStart;
        tvec3<T> ap = point - lineStart;
        T abLen2 = dot(ab, ab);
        if (abLen2 < T(1e-12))
            return length(ap);
        T t = dot(ap, ab) / abLen2;
        t = clamp(t, T(0.0), T(1.0));
        tvec3<T> closest = lineStart + ab * t;
        return length(point - closest);
    }

    /// <summary>
    /// 计算点在向量(直线)上的投影点
    /// </summary>
    template<typename T>
    tvec3<T>    closePointOnVector(const tvec3<T>& vecDir, const tvec3<T>& vecStart, const tvec3<T>& point)
    {
        tvec3<T> dir = normalize(vecDir);
        tvec3<T> ap = point - vecStart;
        T t = dot(ap, dir);
        return vecStart + dir * t;
    }

    /// <summary>
    /// 将点投影到平面上
    /// </summary>
    template<typename T>
    tvec3<T>    ptProjToPlane(const tvec3<T>& point, const tvec3<T>& planeNormal, const tvec3<T>& planePoint)
    {
        tvec3<T> n = normalize(planeNormal);
        T d = dot(point - planePoint, n);
        return point - n * d;
    }

    /// <summary>
    /// 计算射线与平面的交点
    /// </summary>
    /// <param name="ray">射线</param>
    /// <param name="planeNormal">平面法线</param>
    /// <param name="planePoint">平面上一点</param>
    /// <param name="outPt">输出交点</param>
    /// <returns>是否相交</returns>
    template<typename T>
    bool    calcRaySurFaceInsPt(const tray<T>& ray, const tvec3<T>& planeNormal, const tvec3<T>& planePoint, tvec3<T>& outPt)
    {
        tvec3<T> n = normalize(planeNormal);
        T denom = dot(n, ray.getDirection());
        if (std::abs(denom) < T(1e-8))
            return false;
        T t = dot(planePoint - ray.getOrigin(), n) / denom;
        if (t < T(0.0))
            return false;
        outPt = ray.getPoint(t);
        return true;
    }
}
