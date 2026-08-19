#pragma     once

#include    "FEVec3.hpp"
#include    "FEVec4.hpp"
#include    "FEAabb3.hpp"

namespace   FE
{
    template<typename T>
    static  tvec3<T>    inverse(const tvec3<T>& val)
    {
        const auto  xx  =  (val.x != T(0)) ? (T(1)/val.x) : std::numeric_limits<T>::infinity();
        const auto  yy  =  (val.y != T(0)) ? (T(1)/val.y) : std::numeric_limits<T>::infinity();
        const auto  zz  =  (val.z != T(0)) ? (T(1)/val.z) : std::numeric_limits<T>::infinity();
        return      tvec3<T>(xx,yy,zz);
    }

   template<typename T>
    class   tray
    {
        typedef T           value_type;
        typedef tray<T>     type;
    protected:
        tvec3<T>    _origin;
        tvec3<T>    _direction;
        tvec3<T>    _dirInverse;
    public:
        tray() :
            _origin(value_type(0), value_type(0), value_type(0)),
            _direction(value_type(0), value_type(0), value_type(1))
        {
            _dirInverse =   inverse(_direction);
        }
        tray(const tvec3<T>& origin, const tvec3<T>& direction) :
            _origin(origin),
            _direction(direction)
        {
            _dirInverse =   inverse(_direction);
        }
        inline  bool    isValid() const
        {
            return  _origin != _direction;
        }
        /// <summary>
        /// 设置射线的起点
        /// </summary>
        /// <param name="origin"></param>
        inline  void    setOrigin(const tvec3<T>& origin)
        {
            _origin = origin;
        }
        /// <summary>
        /// 返回射线的起点
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  auto&   getOrigin(void) const
        {
            return _origin;
        }
        /// <summary>
        /// 返回射线的起点
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  auto&   origin(void) const
        {
            return _origin;
        }
        /// <summary>
        /// 返回射线的方向
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  auto&   dir(void) const
        {
            return _direction;
        }
        
        /// <summary>
        /// 设置射线的方向
        /// </summary>
        /// <param name="dir"></param>
        inline  void    setDirection(const tvec3<T>& dir)
        {
            _direction  =   dir;
            _dirInverse =   inverse(dir);
        }
        /// <summary>
        /// 返回射线的方向
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        const   auto&   getDirection(void) const
        {
            return _direction;
        }
        /// <summary>
        /// Gets the position of a point t units along the ray.
        /// </summary>
        /// <param name="time"></param>
        /// <returns></returns>
        inline  auto    getPoint(T time) const
        {
            return tvec3<T>(_origin + (_direction * time));
        }
        /// <summary>
        /// 射线做矩阵变换
        /// 典型应用场景: 几何体相交计算
        /// </summary>
        /// <param name="m"></param>
        /// <returns>自己</returns>
        inline  auto&   transform(const tmat4<T>& m) 
        {
            /// 1. 起点（点）：受平移、旋转、缩放影响
            /// 在齐次坐标中，w分量为1
            tvec4<T>    worldOrigin =   tvec4<T>(this->_origin, T(1));
            _origin                 =   (m * worldOrigin);

            /// 2. 方向（向量）：仅受旋转、缩放影响，不受平移影响
            /// 在齐次坐标中，w分量为0
            tvec4<T>    worldDir    =   tvec4<T>(this->_direction, 0.0f);
            _direction              =   (m * worldDir);
            _dirInverse             =   inverse(_direction);
            /// 3. 注意：如果矩阵包含缩放，方向向量的长度会改变，
            /// 许多求交算法要求方向向量必须单位化,这里不做归一化，目的是计算出来的time在新的矩阵上依然可用
            /// _direction.normalize();
            return  *this;
        }
        /// <summary>
        /// 射线做矩阵变换
        /// 典型应用场景: 几何体相交计算
        /// </summary>
        /// <param name="m"></param>
        /// <returns></returns>
        inline  auto    transformed(const tmat4<T>& m) const
        {
            tray<T>     result;

            /// 1. 起点（点）：受平移、旋转、缩放影响
            /// 在齐次坐标中，w分量为1
            tvec4<T>    worldOrigin =   tvec4<T>(this->_origin, T(1));
            result._origin          =   (m * worldOrigin);

            /// 2. 方向（向量）：仅受旋转、缩放影响，不受平移影响
            /// 在齐次坐标中，w分量为0
            tvec4<T> worldDir       =   tvec4<T>(this->_direction, T(0));
            result._direction       =   (m * worldDir);
            result._dirInverse      =   inverse(result._direction);
            /// 3. 注意：如果矩阵包含缩放，方向向量的长度会改变，
            /// 许多求交算法要求方向向量必须单位化,目的是计算出来的time在新的矩阵上依然可用
            /// result._direction.normalize();
            return  result;
        }
        /// <summary>
        /// 测试射线box相交
        /// 如果相交,返回值中的first == true.否则false
        /// </summary>
        /// <param name="box"></param>
        /// <returns>second为射线到点的距离,调用getPoint方法，则返回交点</returns>
        inline  auto    intersects(const taabb3<T>& box) const
        {
            T           lowt    =   0;
            T           t       =   0;
            bool        hit     =   false;
            tvec3<T>    hitpoint;
            tvec3<T>    min     =   box.getMinimum();
            tvec3<T>    max     =   box.getMaximum();

            if (box.contains(_origin))
                return std::pair<bool, T>(true, 0.0f);

            // Check each face in turn, only check closest 3
            // Min x
            if (_origin.x <= min.x && _direction.x > 0)
            {
                t = (min.x - _origin.x) / _direction.x;
                if (t >= 0)
                {
                    // Substitute t back into ray and check bounds and dist
                    hitpoint = _origin + _direction * t;
                    if (hitpoint.y >= min.y &&
                        hitpoint.y <= max.y &&
                        hitpoint.z >= min.z &&
                        hitpoint.z <= max.z &&
                        (!hit || t < lowt))
                    {
                        hit = true;
                        lowt = t;
                    }
                }
            }
            // Max x
            if (_origin.x >= max.x && _direction.x < 0)
            {
                t = (max.x - _origin.x) / _direction.x;
                if (t >= 0)
                {
                    // Substitute t back into ray and check bounds and dist
                    hitpoint = _origin + _direction * t;
                    if (hitpoint.y >= min.y &&
                        hitpoint.y <= max.y &&
                        hitpoint.z >= min.z &&
                        hitpoint.z <= max.z &&
                        (!hit || t < lowt))
                    {
                        hit = true;
                        lowt = t;
                    }
                }
            }
            // Min y
            if (_origin.y <= min.y && _direction.y > 0)
            {
                t = (min.y - _origin.y) / _direction.y;
                if (t >= 0)
                {
                    // Substitute t back into ray and check bounds and dist
                    hitpoint = _origin + _direction * t;
                    if (hitpoint.x >= min.x &&
                        hitpoint.x <= max.x &&
                        hitpoint.z >= min.z &&
                        hitpoint.z <= max.z &&
                        (!hit || t < lowt))
                    {
                        hit = true;
                        lowt = t;
                    }
                }
            }
            // Max y
            if (_origin.y >= max.y && _direction.y < 0)
            {
                t = (max.y - _origin.y) / _direction.y;
                if (t >= 0)
                {
                    // Substitute t back into ray and check bounds and dist
                    hitpoint = _origin + _direction * t;
                    if (hitpoint.x >= min.x &&
                        hitpoint.x <= max.x &&
                        hitpoint.z >= min.z &&
                        hitpoint.z <= max.z &&
                        (!hit || t < lowt))
                    {
                        hit = true;
                        lowt = t;
                    }
                }
            }
            // Min z
            if (_origin.z <= min.z && _direction.z > 0)
            {
                t = (min.z - _origin.z) / _direction.z;
                if (t >= 0)
                {
                    // Substitute t back into ray and check bounds and dist
                    hitpoint = _origin + _direction * t;
                    if (hitpoint.x >= min.x &&
                        hitpoint.x <= max.x &&
                        hitpoint.y >= min.y &&
                        hitpoint.y <= max.y &&
                        (!hit || t < lowt))
                    {
                        hit = true;
                        lowt = t;
                    }
                }
            }
            // Max z
            if (_origin.z >= max.z && _direction.z < 0)
            {
                t = (max.z - _origin.z) / _direction.z;
                if (t >= 0)
                {
                    // Substitute t back into ray and check bounds and dist
                    hitpoint = _origin + _direction * t;
                    if (hitpoint.x >= min.x &&
                        hitpoint.x <= max.x &&
                        hitpoint.y >= min.y &&
                        hitpoint.y <= max.y &&
                        (!hit || t < lowt))
                    {
                        hit = true;
                        lowt = t;
                    }
                }
            }
            return std::pair<bool, T>(hit, lowt);
        }
#if 0
        inline  auto    intersectSphere(const tvec3<T>& center, T radius)  const
        {
            const auto  raydir  =   _direction;
            // Adjust ray origin relative to sphere center
            const auto  rayorig =   _origin - center;
            const T     len     =   FE::length(rayorig) *  FE::length(rayorig);
            // Check origin inside first
            if (len <= radius * radius)
            {
                return std::pair<bool, T>(true, (T)0);
            }

            /// Mmm, quadratics
            /// Build coeffs which can be used with std quadratic solver
            /// ie t = (-b +/- sqrt(b*b + 4ac)) / 2a
            const   T   a   =   FE::dot(raydir, raydir);
            const   T   b   =   2 * FE::dot(rayorig, raydir);
            const   T   c   =   FE::dot(rayorig, rayorig) - radius * radius;
            const   T   d   =   (b*b) - (4 * a * c);
            if (d < 0)
            {
                // No intersection
                return std::pair<bool, T>(false, (T)0);
            }
            else
            {
                /// BTW, if d=0 there is one intersection, if d > 0 there are 2
                /// But we only want the closest one, so that's ok, just use the 
                /// '-' version of the solver
                T   t = (-b - sqrt(d)) / (2 * a);
                if (t < 0)
                    t = (-b + sqrt(d)) / (2 * a);
                return std::pair<bool, T>(true, (T)t);
            }
        }



        inline  bool    intersectEllipsoid(const tellipsoid<T>& ellip,tvec3<T>& result)
        {
            T           a   =   ellip._radius.x;
            T           b   =   ellip._radius.z;
            tvec3<T>    p0  =   _origin;
            tvec3<T>    p1  =   _origin + _direction * ( ellip._radius.x * 10);
            T   x0  = p0.x,     y0 = p0.y,  z0 = p0.z;
            T   x1  = p1.x,     y1 = p1.y,  z1 = p1.z;

            T   cx  =   ellip._center.x;
            T   cy  =   ellip._center.y;
            T   cz  =   ellip._center.z;
            T   m   =   x1 - x0;
            T   n   =   y1 - y0;
            T   p   =   z1 - z0;

            T   A   =   (m*m + n*n) / (a*a) + p*p / (b*b);
            T   B   =   2 * ((m*(x0 - cx) + n*(y0 - cy)) / (a*a) + p*(z0 - cz) / (b*b));
            T   C   =   ((x0 - cx)*(x0 - cx) + (y0 - cy)*(y0 - cy)) / (a*a) + (z0 - cz)*(z0 - cz) / (b*b) - 1;
            T   test=   B*B - (T)4.0*A*C;
            if (test >= (T)0.0)
            {
                T t0 = (-B - sqrt(test)) / ((T)2.0 * A);
                T t1 = (-B + sqrt(test)) / ((T)2.0 * A);
                tvec3<T>    nor(m, n, p);
                // 其实有两个解，根据你的需要选择t0还是t1。
                result  =   nor*t0 + p0;
                return  true;
            }
            else
            {
                result  =   tvec3<T>(0, 0, 0);
                return  false;
            }
        }
#endif 
        /// <summary>
        /// 射线与平面的交点
        /// </summary>
        /// <param name="normal">平面法向量</param>
        /// <param name="pos">平面上的任意一点</param>
        inline  auto    intersectSurface(const tvec3<T>& normal, const tvec3<T>& pos)
        {
            const   T t =   dot(normal, pos - _origin) / dot(normal, _direction);
            if (t >= FLT_EPSILON)
                return std::pair<bool, T>(true, (T)t);
            else
                return std::pair<bool, T>(false,(T)0);
        }
        /// <summary>
        /// 射线与平面的交点
        /// </summary>
        /// <param name="normal">平面法向量</param>
        /// <param name="distance">平面到原点的距离</param>
        inline  auto    intersectSurface(const tvec3<T>& normal, T distance)
        {
            const   auto    pos =   normal * distance;
            const   T       t   =   dot(normal, pos - _origin) / dot(normal, _direction);
            if (t >= FLT_EPSILON)
                return  std::pair<bool, T>(true,    (T)t);
            else
                return  std::pair<bool, T>(false,   (T)0);
        }
        /// <summary>
        /// 测试代码
        /// </summary>
        /// <param name="box"></param>
        /// <returns></returns>
        inline  auto    intersectAabb(const taabb3<T>& box) const
        {
            const T tx1     =    (box._minimum.x - _origin.x) * _dirInverse.x;
            const T tx2     =    (box._maximum.x - _origin.x) * _dirInverse.x;

            T       tmin    =    (std::min)(tx1, tx2);
            T       tmax    =    (std::max)(tx1, tx2);

            // Y 轴交点计算
            const T ty1     =   (box._minimum.y - _origin.y) * _dirInverse.y;
            const T ty2     =   (box._maximum.y - _origin.y) * _dirInverse.y;

            tmin            =   (std::max)(tmin, (std::min)(ty1, ty2));
            tmax            =   (std::min)(tmax, (std::max)(ty1, ty2));

            // Z 轴交点计算
            const T tz1     =   (box._minimum.z - _origin.z) * _dirInverse.z;
            const T tz2     =   (box._maximum.z - _origin.z) * _dirInverse.z;

            tmin            =   (std::max)(tmin, (std::min)(tz1, tz2));
            tmax            =   (std::min)(tmax, (std::max)(tz1, tz2));

            /// 判断逻辑：
            /// 1. tmax >= tmin: 三个轴向的重叠区间存在
            /// 2. tmax > 0: 盒子不在射线后方
            /// 3. tmin < 0: 射线起点在盒子内部:(tmin > 0) ? tmin : tmax,交点使用tmax计算
            if (tmax >= (std::max)(T(0), tmin))
                return  std::pair<bool, T>(true, (tmin > 0) ? tmin : tmax);
            else
                return  std::pair<bool, T>(false, T(0));
        }
        /// <summary>
        /// 计算射线(Ray)上距离空间中给定点(point)最近的一个点
        /// </summary>
        /// <param name="point"></param>
        /// <returns>返回点</returns>
        inline  auto    closestPointToPoint(const tvec3<T>& point) const
        {
            /// 计算从射线的起点_origin 指向目标点 point 的向量
            const   auto    dir     =   point - _origin;
            /// 点积 Dot Product 计算该偏移向量在射线方向 _direction，通常为单位向量上的投影长度
            /// 如果在起点后方 dirDist < 0
            const   auto    dirDist =   FE::dot(dir,_direction);
            if (dirDist < 0)
                return  _origin;
            else
                return  _origin + _direction * dirDist;
        }
        /// <summary>
        /// 根据给定点重新计算方向
        /// </summary>
        /// <param name="v"></param>
        /// <returns></returns>
        inline  auto&   lookAt(const tvec3<T>& v)
        {
            _direction  =   FE::normalize(v - _origin);
            return  *this;
        }
        
        /// <summary>
        /// 计算一条射线（Ray）与一条线段（Segment）之间的最短距离的平方
        /// </summary>
        /// <param name="v0">Segment的端点</param>
        /// <param name="v1">Segment的端点</param>
        /// <param name="optionalPointOnRay">输出射线上距离线段最近的点</param>
        /// <param name="optionalPointOnSegment">输出线段距离射线最近的点</param>
        /// <returns>result.x 距离平方,result.y 射线的参数t,result.z 线段参数 t(以中心点为原点)</returns>
        inline  tvec3<T>    distanceSqToSegment(const tvec3<T>& v0, const tvec3<T>& v1, tvec3<T>* optionalPointOnSegment,tvec3<T>* optionalPointOnRay) const
        {
            /// from http://www.geometrictools.com/GTEngine/Include/Mathematics/GteDistRaySegment.h
            /// It returns the min distance between the ray and the segment
            /// defined by v0 and v1
            /// It can also set two optional targets :
            /// - The closest point on the ray
            /// - The closest point on the segment
            const   auto    segCenter   =   (v0 + v1) * T(0.5);
            const   auto    segDir      =   normalize(v1 - v0);
            const   auto    diff        =   _origin - segCenter;
            
            const   T       segExtent   =   FE::distance(v0,v1) * T(0.5);
            const   T       a01         =   -FE::dot(_direction,segDir);
            const   T       b0          =   FE::dot(diff,_direction);
            const   T       b1          =   -FE::dot(diff,segDir);
            const   T       c           =   FE::length2(diff);
            const   T       det         =   std::abs(T(1) - a01 * a01);

            T       s0              =   T(0);
            T       s1              =   T(0);
            T       sqrDist         =   T(0);
            T       extDet          =   T(0);
            if (det > 0)
            {
                // The ray and segment are not parallel.
                s0      =   a01 * b1 - b0;
                s1      =   a01 * b0 - b1;
                extDet  =   segExtent * det;

                if (s0 >= 0)
                {
                    if (s1 >= -extDet)
                    {
                        if (s1 <= extDet)
                        {
                            // region 0
                            // Minimum at interior points of ray and segment.
                            const T invDet = T(1) / det;
                            s0      *=  invDet;
                            s1      *=  invDet;
                            sqrDist =   s0 * (s0 + a01 * s1 + 2 * b0) + s1 * (a01 * s0 + s1 + 2 * b1) + c;
                        }
                        else
                        {
                            // region 1
                            s1      =   segExtent;
                            s0      =   (std::max)(T(0), -(a01 * s1 + b0));
                            sqrDist =   -s0 * s0 + s1 * (s1 + 2 * b1) + c;
                        }
                    }
                    else
                    {
                        // region 5
                        s1      =   -segExtent;
                        s0      =   (std::max)(T(0), -(a01 * s1 + b0));
                        sqrDist =   -s0 * s0 + s1 * (s1 + 2 * b1) + c;
                    }
                }
                else
                {
                    if (s1 <= -extDet)
                    {
                        // region 4
                        s0      =   (std::max)(T(0), -(-a01 * segExtent + b0));
                        s1      =   (s0 > 0) ? -segExtent : (std::min)((std::max)(-segExtent, -b1), segExtent);
                        sqrDist =   -s0 * s0 + s1 * (s1 + 2 * b1) + c;

                    }
                    else if (s1 <= extDet)
                    {
                        // region 3
                        s0      =   0;
                        s1      =   (std::min)((std::max)(-segExtent, -b1), segExtent);
                        sqrDist =   s1 * (s1 + 2 * b1) + c;

                    } else
                    {
                        // region 2
                        s0      =   (std::max)(T(0), -(a01 * segExtent + b0));
                        s1      =   (s0 > 0) ? segExtent : (std::min)((std::max)(-segExtent, -b1), segExtent);
                        sqrDist =   -s0 * s0 + s1 * (s1 + 2 * b1) + c;
                    }
                }
            }
            else
            {
                // Ray and segment are parallel.
                s1      =   (a01 > 0) ? -segExtent : segExtent;
                s0      =   (std::max)(T(0), -(a01 * s1 + b0));
                sqrDist =   -s0 * s0 + s1 * (s1 + 2 * b1) + c;
            }

            if (optionalPointOnRay)
                *optionalPointOnRay     =   _origin + _direction * s0;

            if (optionalPointOnSegment)
                *optionalPointOnSegment =   segCenter + segDir * s1;
            return tvec3<T>(sqrDist,s0,s1);
        }

        inline  tvec3<T>    distance2(const tvec3<T> segment[2]
                                    , tvec3<T>* pSegmentPoint = nullptr
                                    , tvec3<T>* pRayPoint = nullptr) const
        {
            return  distanceSqToSegment(segment[0],segment[1],pSegmentPoint,pRayPoint);
        }
        /// <summary>
        /// 射线与三角形相交计算
        /// </summary>
        /// <param name="v0"></param>
        /// <param name="v1"></param>
        /// <param name="v2"></param>
        /// <param name="t"></param>
        /// <param name="u"></param>
        /// <param name="v"></param>
        /// <returns></returns>
        inline  bool    intersect(  const tvec3<T>& v0,
                                    const tvec3<T>& v1,
                                    const tvec3<T>& v2,
                                    T* t,
                                    T* u,
                                    T* v) const
        {
            return  intersectTriangle(_origin,_direction,v0,v1,v2,t,u,v);
        }
        /// <summary>
        /// 射线与三角形相交
        /// 如果dir有大小,可以计算线段与三角形的交点 orig:线段起点，线段终点: orig + dir,t >=0 && t<=1 有交点
        /// M?ller-Trumbore 算法。
        /// (S + t dot dir = v0 + u dot edge1 + v dot edge2)
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="orig"></param>
        /// <param name="dir">如果dir有大小，orig认为是起点，终点: orig + dir</param>
        /// <param name="v0">三角形的顶点</param>
        /// <param name="v1">三角形的顶点</param>
        /// <param name="v2">三角形的顶点</param>
        /// <param name="t">线段上的点可表示为: P(t)=S+t(E-S))，其中 (t in [0,1] )</param>
        /// <param name="u">三角形上的点可表示为:T(u,v)=(1-u-v ) * v0 + u * v1 + v * v2)</param>
        /// <param name="v">三角形上的点可表示为:T(u,v)=(1-u-v ) * v0 + u * v1 + v * v2)</param>
        /// <returns>是否相交</returns>
        static  bool    intersectTriangle(  const tvec3<T>& orig,
                                            const tvec3<T>& dir,
                                            const tvec3<T>& v0,
                                            const tvec3<T>& v1,
                                            const tvec3<T>& v2,
                                            T* t,
                                            T* u,
                                            T* v)
        {
            // Find vectors for two edges sharing vert0
            const   auto    edge1 = v1 - v0;
            const   auto    edge2 = v2 - v0;
            // 算法。穆勒-特朗博尔 
            // 线段上的点可表示为：   P(t)=S+t(E-S))，其中 (t in [0,1])
            // 三角形上的点可表示为：  T(u,v)=(1-u-v ) * V0+u * V1+v * V2)
            // 相交条件：?(u >= 0,v >= 0,u+v <= 1)
            // 交点在三角形内 (0 <= t <= 1) （交点在线段上）
            // Begin calculating determinant - also used to calculate U parameter
            const   auto    pvec    =   cross(dir, edge2);
            // If determinant is near zero, ray lies in plane of triangle
            T   det = dot(edge1, pvec);

            tvec3<T>    tvec;
            if (det > 0)
            {
                tvec = orig - v0;
            }
            else
            {
                tvec = v0 - orig;
                det = -det;
            }
            if (det < 1e-9)
                return false;
            // Calculate U parameter and test bounds
            *u = dot(tvec, pvec);
            if (*u < 0.0f || *u > det)
                return false;
            // Prepare to test V parameter
            const   auto    qvec    =   cross(tvec, edge1);
            // Calculate V parameter and test bounds
            *v = dot(dir, qvec);
            if (*v < T(0.0f) || *u + *v > det)
                return false;
            *t = dot(edge2, qvec);
            T   fInvDet = T(1.0) / det;
            *t *= fInvDet;
            *u *= fInvDet;
            *v *= fInvDet;

            return true;
        }
    };


    using   Ray     =   tray<real>;
    using   RayR    =   tray<real>;
    using   RayF    =   tray<float>;

}
