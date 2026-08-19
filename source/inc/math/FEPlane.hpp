#pragma     once

#include    "glm/glm.hpp"
#include    "FEVec3.hpp"
#include    "FEMat4.hpp"
#include    "FEAabb3.hpp"

namespace   FE
{   
    template<class T>
    class   tplane
    {
    public:
        /// <summary>
        /// The "positive side" of the plane is the half space to which the
        /// plane normal points. The "negative side" is the other half
        /// space. The flag "no side" indicates the plane itself.
        /// </summary>
        enum    Side : uint32_t
        {
            NO_SIDE         ,
            POSITIVE_SIDE   ,
            NEGATIVE_SIDE   ,
            BOTH_SIDE       ,
        };
    public:
        using       Result  =   std::pair<bool,tvec3<T>>;
    public:
        tvec3<T>    _normal;
        T           _distance;
    public:
        tplane()
        {
            _normal     =   tvec3<T>(0, 0, 0);
            _distance   =   0.0f;
        }
        tplane(const tplane& right)
        {
            _normal     =   right._normal;
            _distance   =   right._distance;
        }
        /// <summary>
        /// Construct a plane through a normal, and a distance to move the plane along the normal.
        /// </summary>
        /// <param name="rkNormal"></param>
        /// <param name="fConstant"></param>
        tplane(const tvec3<T>& rkNormal, T fConstant)
        {
            _normal     =   rkNormal;
            _distance   =   -fConstant;
        }
        /// <summary>
        /// Construct a plane using the 4 constants directly
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="z"></param>
        /// <param name="o"></param>
        tplane(T x, T y, T z, T o)
        {
            _normal     =   tvec3<T>(x, y, z);
            T invLen    =   T(1.0) / length(_normal);
            _normal     *=  invLen;
            _distance   =   o * invLen;
        }
        tplane(const tvec3<T>& rkNormal, const tvec3<T>& rkPoint)
        {
            redefine(rkNormal, rkPoint);
        }
        tplane(const tvec3<T>& rkPoint0, const tvec3<T>& rkPoint1, const tvec3<T>& rkPoint2)
        {
            redefine(rkPoint0, rkPoint1, rkPoint2);
        }
        /// <summary>
        /// 到点的距离
        /// </summary>
        /// <param name="pos"></param>
        /// <returns></returns>
        inline  T       distance(const tvec3<T> &pos) const
        {
            return  dot(_normal, pos) + _distance;
        }
        /// <summary>
        /// 面的法线
        /// </summary>
        /// <returns></returns>
        inline  auto    normal() const
        {
            return  _normal;
        }
        inline  Side    getSide(const tvec3<T>& rkPoint) const
        {
            T fDistance = distance(rkPoint);

            if (fDistance < (T)(0.0))
                return tplane::NEGATIVE_SIDE;

            if (fDistance > (T)(0.0))
                return tplane::POSITIVE_SIDE;

            return tplane::NO_SIDE;
        }
        /// <summary>
        /// 移动面偏移量
        /// </summary>
        inline  void    translate(const tvec3<T> &off) 
        {
            _distance   = _distance - dot(_normal,off);
        }
        inline  Side    getSide(const tvec3<T>& centre, const tvec3<T>& halfSize) const
        {
            // Calculate the distance between box centre and the plane
            float dist = distance(centre);

            // Calculate the maximise allows absolute distance for
            // the distance between box centre and plane
            // float maxAbsDist = _normal.absDot(halfSize);
            float maxAbsDist = dot(_normal, halfSize);

            if (dist < -maxAbsDist)
                return tplane::NEGATIVE_SIDE;

            if (dist > +maxAbsDist)
                return tplane::POSITIVE_SIDE;

            return tplane::BOTH_SIDE;
        }
        /// <summary>
        /// 根据给定的三个点定义个面
        /// </summary>
        inline  void    redefine(const tvec3<T>& pt0, const tvec3<T>& pt1, const tvec3<T>& pt2)
        {
            const auto  kEdge1  =   pt1 - pt0;
            const auto  kEdge2  =   pt2 - pt0;
            _normal             =   cross(kEdge1, kEdge2);
            _normal             =   normalize(_normal);
            _distance           =   -dot(_normal, pt0);
        }
        /// <summary>
        /// Redefine this plane based on a normal and a point
        /// </summary>
        /// <param name="vNormal"></param>
        /// <param name="vPoint"></param>
        inline  void    redefine(const tvec3<T>& vNormal, const tvec3<T>& vPoint)
        {
            _normal     =   vNormal;
            _distance   =   -dot(vNormal, vPoint);
        }
        /// <summary>
        /// 获取点到面上投影
        /// 算法:点到直线的距离 distance(p); distance(p) * _normal,类似 t * dir;
        /// 从p - t * dir，得到平面上的点。
        /// </summary>
        inline  auto    project(const tvec3<T>& p) const
        {
            return p - _normal * distance(p);
        }
        /// <summary>
        /// 平面与线段相交测试
        /// </summary>
        /// <param name="p0">线段的两个端点</param>
        /// <param name="p1">线段的两个端点</param>
        /// <returns>std::pair<bool,tvec3<T>>,是否相交，以及交点</returns>
        inline  auto    intersectLine(const tvec3<T>& p0,const tvec3<T>& p1) const
        {
            const   auto    direction   =   p1 - p0;
            const   auto    denominator =   _normal.dot(direction);

            /// line is coplanar, return origin
            if (denominator == 0)
            {
                return  std::pair<bool,tvec3<T>>(true,p1);
            }
            const auto  t   =   -(p1.dot(this->_normal) + this->_distance) / denominator;

            if (t < 0 || t > 1)
                return  std::pair<bool,tvec3<T>>(false,{});
            else
                return  p0 + direction * t;
        }
        /// <summary>
        /// 检测线段与平面相交速度快，不计算交点
        /// </summary>
        /// <param name="vStart"></param>
        /// <param name="vEnd"></param>
        /// <returns></returns>
        inline  bool    intersectsLine(const tvec3<T>& vStart,const tvec3<T>& vEnd) const
        {
            const auto signS    =   distance(vStart);
            const auto signE    =   distance(vEnd);

            return (signS < 0 && signE > 0) || (signE < 0 && signS > 0);
        }
        /// <summary>
        /// 平面与球体相交
        /// </summary>
        /// <param name="vCenter">球体中心点</param>
        /// <param name="radius">球体半径</param>
        /// <returns>true/false</returns>
        inline  bool    intersectsSphere(const tvec3<T>& vCenter,const T& radius) const
        {
            return  (std::abs)(distance(vCenter)) <= radius;
        }
        /// <summary>
        /// 平面与包围盒相交计算
        /// </summary>
        /// <param name="aabb"></param>
        /// <returns></returns>
        inline  bool    intersectsAABB(const taabb3<T>& aabb)
        {
            /// 计算AABB中心
            const auto  center  =   aabb.center();
            /// 计算AABB半长
            /// const auto  extent  =   aabb.getHalfSize();
            /// 与下面等价，性能更好
            const auto  extent  =   aabb._maximum - center;
            /// extent : e;
            /// 根据几何投影原理,一个 AABB 在任意向量 N 上的最大投影半径 r 等于
            /// 如果(N.x) 是正的,就选 + e.x
            /// 如果(N.x) 是负的,就选 - e.x
            /// 这等价于对每一项取绝对值：
            /// r   =   e.x·|N.x| + e.y·|N.y| + e.z·|N.z|
            /// 计算投影半径
            const auto  radius  =   extent.x * (std::abs)(_normal.x) +
                extent.y * (std::abs)(_normal.y) +
                extent.z * (std::abs)(_normal.z);
            /// 计算距离
            const auto  dist    =   distance(center);
            return  (std::abs)(dist) <= radius;
        }
        /// <summary>
        ///  Normalises the plane.
        ///  This method normalises the plane's normal and the length scale of d
        ///  is as well.
        ///  @note
        ///  This function will not crash for zero-sized vectors, but there
        ///  will be no changes made to their components.
        ///  @returns The previous length of the plane's normal.
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  float   normalise(void)
        {
            float fLength = _normal.length();

            // Will also work for zero-sized vectors, but will change nothing
            if (fLength > 1e-08f)
            {
                float fInvLength = 1.0f / fLength;
                _normal *= fInvLength;
                _distance *= fInvLength;
            }
            return fLength;
        }
        inline  bool    operator==(const tplane& right) const
        {
            return (right._distance == _distance && right._normal == _normal);
        }
        inline  bool    operator!=(const tplane& right) const
        {
            return (right._distance != _distance && right._normal != _normal);
        }
        /// <summary>
        /// 计算镜面反射矩阵
        /// </summary>
        inline  auto    reflectionMatrix()
        {
#if 0
            /// row major order
            return tmat4<T>(
                -2 * p._normal.x * p._normal.x + 1, -2 * p._normal.x * p._normal.y,     -2 * p._normal.x * p._normal.z,     -2 * p._normal.x * p._distance, 
                -2 * p._normal.y * p._normal.x,     -2 * p._normal.y * p._normal.y + 1, -2 * p._normal.y * p._normal.z,     -2 * p._normal.y * p._distance, 
                -2 * p._normal.z * p._normal.x,     -2 * p._normal.z * p._normal.y,     -2 * p._normal.z * p._normal.z + 1, -2 * p._normal.z * p._distance,
                0.0f,                               0,                                  0.0f,                               1.0f);
#endif
            /// col major order
            return tmat4<T>(
                -2 * _normal.x * _normal.x + 1, -2 * _normal.y * _normal.x,     -2 * _normal.z * _normal.x,     0, 
                -2 * _normal.x * _normal.y,     -2 * _normal.y * _normal.y + 1, -2 * _normal.z * _normal.y,     0, 
                -2 * _normal.x * _normal.z,     -2 * _normal.y * _normal.z,     -2 * _normal.z * _normal.z + 1, 0,
                -2 * _normal.x * _distance,     -2 * _normal.y * _distance,     -2 * _normal.z * _distance,     1);
        }
    public:
        /// <summary>
        /// 求三个面的交点
        /// 给定三个平面，其法线分别为 n1,n2,n3，距离原点的偏移量为d1,d2,d3
        /// 平面方程为 n· P + d=0 )，它们的交点 P 可以通过以下公式计算
        ///  P = (-d1 * (n2 x n3) - d2(n3 x n1) - d3(n1 x n2))/(n1·(n2 x n3))
        /// </summary>
        /// <param name="p1"></param>
        /// <param name="p2"></param>
        /// <param name="p3"></param>
        /// <returns></returns>
        static  Result  intersectPlanes(const tplane<T>& p1,const tplane<T>& p2,const tplane<T>& p3)
        {
            const auto& n1  =   p1._normal;
            const auto& n2  =   p2._normal;
            const auto& n3  =   p3._normal;

            const auto  c12 =   cross(n1, n2);
            const auto  c23 =   cross(n2, n3);
            const auto  c31 =   cross(n3, n1);

            const auto  det =   dot(n1, c23);

            // 如果 det 趋近于 0，说明平面平行或共线，但在合法视锥体中不会发生
            if (std::abs(det) < T(1e-8f))
                return {false,tvec3<T>(0)};
            T       detInv  =   T(1.0f)/det;
            auto    temp    =   (c23 * -p1._distance) + (c31 * -p2._distance) + (c12 * -p3._distance);
                    temp    *=  detInv;
            return  {true,temp};
        }
    };

    using   PlaneR  =   tplane<real>;
    using   PlaneF  =   tplane<float>;
}
