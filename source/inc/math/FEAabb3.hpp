#pragma     once
#include    "../FEFlags.hpp"
#include    "FEVec3.hpp"
#include    "FEMat4.hpp"

namespace   FE
{
    template<typename T>
    class   taabb3
    {
    public:
        using   Box3D   =   taabb3<T>;
    public:
        enum    Extent:uint32_t
        {
            EXTENT_NULL,
            EXTENT_FINITE,
            EXTENT_INFINITE
        };
    public:
        tvec3<T>    _minimum;
        tvec3<T>    _maximum;
        Extent      _extent;
    public:
        /// 
        ///    1-----2
        ///   /|    /|
        ///  / |   / |
        /// 5-----4  |
        /// |  0--|--3
        /// | /   | /
        /// |/    |/
        /// 6-----7
        /// 
        typedef enum :uint32_t
        {
            FAR_LEFT_BOTTOM     =   0,
            FAR_LEFT_TOP        =   1,
            FAR_RIGHT_TOP       =   2,
            FAR_RIGHT_BOTTOM    =   3,
            NEAR_RIGHT_BOTTOM   =   7,
            NEAR_LEFT_BOTTOM    =   6,
            NEAR_LEFT_TOP       =   5,
            NEAR_RIGHT_TOP      =   4
        } CornerEnum;

        taabb3()
        {
            _minimum = tvec3<T>(T(-0.5), T(-0.5), T(-0.5));
            _maximum = tvec3<T>(T(0.5), T(0.5), T(0.5));
            _extent = EXTENT_NULL;
        }
        taabb3(const taabb3 & rkBox)
        {
            setExtents(rkBox._minimum, rkBox._maximum);
            _extent = rkBox._extent;
        }

        template<typename U>
        taabb3(const taabb3<U> & rkBox)
        {
            setExtents(rkBox._minimum, rkBox._maximum);
            _extent =   (taabb3<T>::Extent )rkBox._extent;
        }

        taabb3(const tvec3<T>& min, const tvec3<T>& max)
        {
            setExtents(min, max);
        }

        template<typename U>
        taabb3(const tvec3<U>& min, const tvec3<U>& max)
        {
            const tvec3<T>  valTypeMin = min;
            const tvec3<T>  valTypeMax = max;
            setExtents(valTypeMin, valTypeMax);
        }

        taabb3( T mx, T my, T mz,
                T Mx, T My, T Mz)
        {
            setExtents(mx, my, mz, Mx, My, Mz);
        }

        taabb3<T>& operator=(const taabb3<T>& right)
        {
            setExtents(right._minimum, right._maximum);
            _extent = right._extent;
            return *this;
        }

        template<typename U>
        taabb3<T>& operator=(const taabb3<U>& right)
        {
            setExtents(right._minimum, right._maximum);
            _extent = (taabb3<T>::Extent)right._extent;
            return *this;
        }

        const   auto&   getMinimum(void) const
        {
            return _minimum;
        }
        inline  auto&   getMinimum(void)
        {
            return _minimum;
        }
        inline  auto&   setMinimum(const tvec3<T>& mins)
        {
            _minimum = mins;
            return  *this;
        }
        inline  auto&   setMinimum(T x, T y, T z)
        {
            _minimum = tvec3<T>(x, y, z);
            return  *this;
        }
        const   auto&   getMaximum(void) const
        {
            return _maximum;
        }
        inline  auto&   getMaximum(void)
        {
            return _maximum;
        }
        inline  auto&   setMaximum(const tvec3<T>& vec)
        {
            _maximum    =   vec;
            return  *this;
        }
        inline  auto&   setMaximum(T x, T y, T z)
        {
            _maximum.x = x;
            _maximum.y = y;
            _maximum.z = z;
            return  *this;
        }
        inline  auto&   setMaximumX(T x)
        {
            _maximum.x = x;
            return  *this;
        }
        inline  auto&   setMaximumY(T y)
        {
            _maximum.y = y;
            return  *this;
        }
        inline  auto&   setMaximumZ(T z)
        {
            _maximum.z = z;
            return  *this;
        }
        inline  auto&   setExtents(const tvec3<T>& min, const tvec3<T>& max)
        {
            _minimum    =   min;
            _maximum    =   max;
            _extent     =   EXTENT_FINITE;
            return  *this;
        }
        inline  auto&   setExtents( T mx, T my, T mz,T Mx, T My, T Mz)
        {
            _minimum.x  =   mx;
            _minimum.y  =   my;
            _minimum.z  =   mz;

            _maximum.x  =   Mx;
            _maximum.y  =   My;
            _maximum.z  =   Mz;
            _extent     =   EXTENT_FINITE;
            return  *this;
        }
        /// <summary>
        /// 内部做大小比较,外部不管线 point0,point1,的大小
        /// </summary>
        /// <param name="point0"></param>
        /// <param name="point1"></param>
        /// <returns></returns>
        inline  auto&   calcExtents(const tvec3<T>& point0, const tvec3<T>& point1)
        {
            std::tie(_minimum.x,_maximum.x) =   std::minmax(point0.x, point1.x);
            std::tie(_minimum.y,_maximum.y) =   std::minmax(point0.x, point1.x);
            std::tie(_minimum.z,_maximum.z) =   std::minmax(point0.x, point1.x);

            _extent =   EXTENT_FINITE;
            return  *this;
        }
        /// <summary>
        /// 获取八个顶点数据
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="corners"></param>
        /// <returns></returns>
        template<typename U>
        inline  auto&   getAllCorners(tvec3<U> corners[8]) const
        {
            corners[0].x = (U)_minimum.x;
            corners[0].y = (U)_minimum.y;
            corners[0].z = (U)_minimum.z;

            corners[1].x = (U)_minimum.x;
            corners[1].y = (U)_maximum.y;
            corners[1].z = (U)_minimum.z;

            corners[2].x = (U)_maximum.x;
            corners[2].y = (U)_maximum.y;
            corners[2].z = (U)_minimum.z;

            corners[3].x = (U)_maximum.x;
            corners[3].y = (U)_minimum.y;
            corners[3].z = (U)_minimum.z;

            corners[4].x = (U)_maximum.x;
            corners[4].y = (U)_maximum.y;
            corners[4].z = (U)_maximum.z;

            corners[5].x = (U)_minimum.x;
            corners[5].y = (U)_maximum.y;
            corners[5].z = (U)_maximum.z;

            corners[6].x = (U)_minimum.x;
            corners[6].y = (U)_minimum.y;
            corners[6].z = (U)_maximum.z;

            corners[7].x = (U)_maximum.x;
            corners[7].y = (U)_minimum.y;
            corners[7].z = (U)_maximum.z;
            return  *this;
        }
        /// <summary>
        /// 获取八个顶点数据
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="corners"></param>
        /// <returns></returns>
        template<typename U>
        inline  auto&   allCorners(tvec3<U> corners[8]) const
        {
            return  getAllCorners(corners);
        }
        /// <summary>
        /// 获取12条边的数据
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="indices"></param>
        /// <returns></returns>
        template<typename U>
        inline  auto&   lineIndices(U indices[24]) const
        {
            /// uint    indices[]   =
            /// {
            ///     0,1,1,2,2,3,3,0,
            ///     4,5,5,6,6,7,7,4,
            ///     0,6,1,5,2,4,3,7,
            /// }
            /// --- 底面 4 条边 ---
            indices[0]  =   0;
            indices[1]  =   1;

            indices[2]  =   1;
            indices[3]  =   2;

            indices[4]  =   2;
            indices[5]  =   3;

            indices[6]  =   3;
            indices[7]  =   0;

            /// --- 顶面 4 条边 ---
            indices[8]  =   4;
            indices[9]  =   5;

            indices[10] =   5;
            indices[11] =   6;

            indices[12] =   6;
            indices[13] =   7;

            indices[14] =   7;
            indices[15] =   4;

            /// --- 连接底面与顶面的 4 条垂直边 ---
            /// 对应关系取决于你 getAllCorners 的具体映射

            indices[16] =   0;
            indices[17] =   6;

            indices[18] =   1;
            indices[19] =   5;

            indices[20] =   2;
            indices[21] =   4;

            indices[22] =   3;
            indices[23] =   7;

            return  *this;
        }
        /// <summary>
        /// 获取指定点
        /// </summary>
        /// <param name="cornerToGet"></param>
        /// <returns></returns>
        inline  auto    getCorner(CornerEnum cornerToGet) const
        {
            switch (cornerToGet)
            {
            case FAR_LEFT_BOTTOM:
                return _minimum;
            case FAR_LEFT_TOP:
                return tvec3<T>(_minimum.x, _maximum.y, _minimum.z);
            case FAR_RIGHT_TOP:
                return tvec3<T>(_maximum.x, _maximum.y, _minimum.z);
            case FAR_RIGHT_BOTTOM:
                return tvec3<T>(_maximum.x, _minimum.y, _minimum.z);
            case NEAR_RIGHT_BOTTOM:
                return tvec3<T>(_maximum.x, _minimum.y, _maximum.z);
            case NEAR_LEFT_BOTTOM:
                return tvec3<T>(_minimum.x, _minimum.y, _maximum.z);
            case NEAR_LEFT_TOP:
                return tvec3<T>(_minimum.x, _maximum.y, _maximum.z);
            case NEAR_RIGHT_TOP:
                return _maximum;
            default:
                return tvec3<T>();
            }
        }
        /// <summary>
        /// 合并包围盒
        /// </summary>
        /// <param name="right"></param>
        /// <returns></returns>
        inline  auto&   merge(const taabb3<T>& right)
        {
            if ((right._extent == EXTENT_NULL) )
            {
                return  *this;
            }
            else if (_extent == EXTENT_NULL)
            {
                _minimum        =   right._minimum;
                _maximum        =   right._maximum;
                _extent         =   right._extent;
                return  *this;
            }
            /// 无限大也做合作动作
            else if (_extent == EXTENT_INFINITE)
            {
                tvec3<T>    min =   _minimum;
                tvec3<T>    max =   _maximum;
                max =   FE::max(max,right._maximum);
                min =   FE::min(min,right._minimum);
                _minimum        =   min;
                _maximum        =   max;
            }
            else if (right._extent == EXTENT_INFINITE)
            {
                _extent         =   EXTENT_INFINITE;
            }
            else
            {
                tvec3<T>    min =   _minimum;
                tvec3<T>    max =   _maximum;
                max =   FE::max(max,right._maximum);
                min =   FE::min(min,right._minimum);

                setExtents(min, max);
            }
            return  *this;
        }
        /// <summary>
        /// 合并包围盒
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="right"></param>
        /// <returns></returns>
        template<typename U>
        inline  auto&   merge(const taabb3<U>& right)
        {
            if ((right._extent == EXTENT_NULL) || (_extent == EXTENT_INFINITE))
            {
                return  *this;
            }
            else if (right._extent == EXTENT_INFINITE)
            {
                _extent = EXTENT_INFINITE;
            }
            else if (_extent == EXTENT_NULL)
            {
                setExtents(right._minimum, right._maximum);
            }
            else
            {
                //! merge
                tvec3<T> min = _minimum;
                tvec3<T> max = _maximum;
                max.makeCeil(right._maximum);
                min.makeFloor(right._minimum);
                setExtents(min, max);
            }
            return  *this;
        }
        /// <summary>
        /// 合并点
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        inline  auto&   merge(const tvec3<T>& point)
        {
            switch (_extent)
            {
            case EXTENT_NULL: // if null, use this point
                setExtents(point, point);
                return  *this;
            case EXTENT_FINITE:
                _maximum    =   FE::max(_maximum,point);
                _minimum    =   FE::min(_minimum,point);
                return  *this;
            case EXTENT_INFINITE:
                return  *this;
            }
            return  *this;
        }
        /// <summary>
        /// 包围盒应用矩阵
        /// </summary>
        /// <param name="matrix"></param>
        /// <returns></returns>
        inline  auto&   transform(const tmat4<T>& mat)
        {
            tvec3<T>    oldMin  =   _minimum;
            tvec3<T>    oldMax  =   _maximum;
            tvec3<T>    currentCorner;

            /// We sequentially compute the corners in the following order :
            /// 0, 6, 5, 1, 2, 4 ,7 , 3
            /// This sequence allows us to only change one member at a time to get at all corners.
            /// For each one, we transform it using the matrix
            /// Which gives the resulting point and merge the resulting point.

            currentCorner   =   oldMin;
            tvec3<T> vVert  =   mat * tvec4<T>(currentCorner,T(1));
            setExtents(vVert, vVert);

            /// First corner 
            /// min min min
            currentCorner   =   oldMin;
            merge(mat * tvec4<T>(currentCorner,T(1)));

            /// min,min,max
            currentCorner.z =   oldMax.z;
            merge(mat * tvec4<T>(currentCorner,T(1)));

            /// min max max
            currentCorner.y =   oldMax.y;
            merge(mat * tvec4<T>(currentCorner,T(1)));

            /// min max min
            currentCorner.z =   oldMin.z;
            merge(mat * tvec4<T>(currentCorner,T(1)));

            /// max max min
            currentCorner.x =   oldMax.x;
            merge(mat * tvec4<T>(currentCorner,T(1)));

            /// max max max
            currentCorner.z =   oldMax.z;
            merge(mat * tvec4<T>(currentCorner,T(1)));

            /// max min max
            currentCorner.y =   oldMin.y;
            merge(mat * tvec4<T>(currentCorner,T(1)));

            /// max min min
            currentCorner.z =   oldMin.z;
            merge(mat * tvec4<T>(currentCorner,T(1)));
            return  *this;
        }
        template<class U>
        inline  auto&   transform(const tmat4<U>& mat)
        {
            tvec3<U>    oldMin  =   _minimum;
            tvec3<U>    oldMax  =   _maximum;
            tvec3<U>    currentCorner;

            /// We sequentially compute the corners in the following order :
            /// 0, 6, 5, 1, 2, 4 ,7 , 3
            /// This sequence allows us to only change one member at a time to get at all corners.
            /// For each one, we transform it using the matrix
            /// Which gives the resulting point and merge the resulting point.

            currentCorner   =   oldMin;
            tvec3<U> vVert  =   mat * currentCorner;
            setExtents(vVert, vVert);

            /// First corner 
            /// min min min
            currentCorner   =   oldMin;
            merge(mat * currentCorner);

            /// min,min,max
            currentCorner.z =   oldMax.z;
            merge(mat * currentCorner);

            /// min max max
            currentCorner.y =   oldMax.y;
            merge(mat * currentCorner);

            /// min max min
            currentCorner.z =   oldMin.z;
            merge(mat * currentCorner);

            /// max max min
            currentCorner.x =   oldMax.x;
            merge(mat * currentCorner);

            /// max max max
            currentCorner.z =   oldMax.z;
            merge(mat * currentCorner);

            /// max min max
            currentCorner.y =   oldMin.y;
            merge(mat * currentCorner);

            /// max min min
            currentCorner.z =   oldMin.z;
            merge(mat * currentCorner);
            return  *this;
        }
        /// <summary>
        /// 返回新的aabb;
        /// </summary>
        /// <param name="mat"></param>
        /// <returns></returns>
        inline  auto    transformed(const tmat4<T>& mat) const
        {
            taabb3<T>   result  =   *this;
            result.transform(mat);
            return  result;
        }
        template<class U>
        inline  auto    transformed(const tmat4<U>& mat) const
        {
            taabb3<T>   result  =   *this;
            result.transform(mat);
            return  result;
        }
        /// <summary>
        /// 忽略z
        /// </summary>
        /// <param name="b2"></param>
        /// <returns></returns>
        inline  bool    intersectsNoZ(const taabb3& b2) const
        {
            if (_maximum.x < b2._minimum.x)
                return false;
            if (_maximum.y < b2._minimum.y)
                return false;
            if (_minimum.x > b2._maximum.x)
                return false;
            if (_minimum.y > b2._maximum.y)
                return false;
            return true;

        }
        /// <summary>
        /// 检测点是否被包含
        /// </summary>
        /// <param name="pt"></param>
        /// <returns></returns>
        inline  bool    pointIn(const tvec3<T>& pt)
        {
            if (pt.x > _maximum.x || pt.x < _minimum.x)
                return  false;
            if (pt.y > _maximum.y || pt.y < _minimum.y)
                return  false;
            if (pt.z > _maximum.z || pt.z < _minimum.z)
                return  false;
            return  true;

        }
        /// <summary>
        /// 包围盒与包围盒交集
        /// </summary>
        /// <param name="b2"></param>
        /// <returns></returns>
        inline  Box3D   intersection(const taabb3<T>& b2) const
        {
            tvec3<T> intMin = _minimum;
            tvec3<T> intMax = _maximum;

            /// intMin.makeCeil(b2.getMinimum());
            /// intMax.makeFloor(b2.getMaximum());
            /// 
            intMin  =   FE::max (intMin,    b2.getMinimum());
            intMax  =   FE::min (intMax,    b2.getMaximum());

            if (intMin.x < intMax.x &&
                intMin.y < intMax.y &&
                intMin.z < intMax.z)
            {
                return taabb3<T>(intMin, intMax);
            }

            return taabb3<T>();
        }
        /// <summary>
        /// 设置为null
        /// </summary>
        /// <returns></returns>
        inline  auto&   setNull()
        {
            *this = taabb3<T>();
            return  *this;
        }
        /// <summary>
        /// 是否是null
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  bool    isNull(void) const
        {
            return (_extent == EXTENT_NULL);
        }
        /// <summary>
        /// 是否是有限的
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  bool    isFinite(void) const
        {
            return (_extent == EXTENT_FINITE);
        }
        /// <summary>
        /// 设置为无限大
        /// </summary>
        inline  void    setInfinite()
        {
            _extent = EXTENT_INFINITE;
        }
        /// <summary>
        /// 是否是无限大
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  bool    isInfinite(void) const
        {
            return (_extent == EXTENT_INFINITE);
        }
        /// <summary>
        /// 缩放自己
        /// </summary>
        /// <param name="s"></param>
        /// <returns></returns>
        inline  auto&   scale(const tvec3<T>& s)
        {
            tvec3<T> min = _minimum * s;
            tvec3<T> max = _maximum * s;
            setExtents(min, max);
            return  *this;
        }
        /// <summary>
        /// 不改变自己,返回新的包围盒
        /// </summary>
        /// <param name="s"></param>
        /// <returns></returns>
        inline  auto    scaled(const tvec3<T>& s)
        {
            tvec3<T> min = _minimum * s;
            tvec3<T> max = _maximum * s;
            return  taabb3<T>(min,max);
        }
        /// <summary>
        /// 检测两个包围盒是否相交
        /// </summary>
        /// <param name="b2"></param>
        /// <returns></returns>
        inline  bool    intersects(const taabb3<T>& b2) const
        {
            if (_maximum.x < b2._minimum.x)
                return false;
            if (_maximum.y < b2._minimum.y)
                return false;
            if (_maximum.z < b2._minimum.z)
                return false;
            if (_minimum.x > b2._maximum.x)
                return false;
            if (_minimum.y > b2._maximum.y)
                return false;
            if (_minimum.z > b2._maximum.z)
                return false;
            return true;
        }
        /// <summary>
        /// 相交测试
        /// </summary>
        /// <param name="v"></param>
        /// <returns></returns>
        inline  bool    intersects(const tvec3<T>& v) const
        {
            return( v.x >= _minimum.x  &&  v.x <= _maximum.x  &&
                v.y >= _minimum.y  &&  v.y <= _maximum.y  &&
                v.z >= _minimum.z  &&  v.z <= _maximum.z);
        }
        /// <summary>
        /// 相交测试
        /// </summary>
        /// <param name="v"></param>
        /// <returns></returns>
        inline  bool    intersects(const tvec2<T>& v) const
        {
            return( v.x >= _minimum.x  &&  v.x <= _maximum.x  &&
                v.y >= _minimum.y  &&  v.y <= _maximum.y);
        }        
        /// <summary>
        /// 包围盒与三角形相交测试
        /// 分离轴定理
        /// </summary>
        /// <param name="p0">三角形的顶点</param>
        /// <param name="p1">三角形的顶点</param>
        /// <param name="p2">三角形的顶点</param>
        /// <returns>true,false</returns>
        inline  bool    intersects(const tvec3<T>& p0,const tvec3<T>& p1,const tvec3<T>& p2) const
        {
            const   auto    cent    =   center();
            const   auto    half    =   _maximum - cent;
            /// translate triangle to aabb origin
            const   auto    v0      =   p0 - cent;
            const   auto    v1      =   p1 - cent;
            const   auto    v2      =   p2 - cent;
            /// compute edge vectors for triangle
            const   auto    f0      =  v1 - v0;
            const   auto    f1      =  v2 - v1;
            const   auto    f2      =  v0 - v2;
            /// axis_ij = u_i x f_j (u0, u1, u2 = face normals of aabb = x,y,z axes vectors since aabb is axis aligned)
            /// 轴的构造逻辑：axis_ij = e_i x f_j
            /// 例如 X轴(1,0,0) x f0(x,y,z) = (0, -f0.z, f0.y)
            const  tvec3<T> axes[]    =
            {
                {0, -f0.z, f0.y}, {0, -f1.z, f1.y}, {0, -f2.z, f2.y},
                {f0.z, 0, -f0.x}, {f1.z, 0, -f1.x}, {f2.z, 0, -f2.x},
                {-f0.y, f0.x, 0}, {-f1.y, f1.x, 0}, {-f2.y, f2.x, 0}
            };

            if (!satForAxes(axes, 9,v0, v1, v2, half))
                return false;
            /// test 3 face normals from the aabb
            const  tvec3<T> axes1[] =   {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
            if (!satForAxes(axes1,  3,v0, v1, v2, half))
                return false;

            /// finally testing the face normal of the triangle
            /// use already existing triangle edge vectors here
            const  tvec3<T> triangleNormal  =   cross(f0, f1);
            return satForAxes(&triangleNormal,1, v0, v1, v2, half);
        }
        /// <summary>
        /// 得到给定点到包围盒的距离
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        inline  T       distanceTo(const tvec3<T>& point) const
        {
            /// tvec3<T>    clamped =   point.clamped(_minimum,_maximum);
            tvec3<T>    clamped =   FE::clamp(point, _minimum, _maximum);
            return      length(clamped - point);
        }
        /// <summary>
        /// 两个包围盒的距离
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  T       distanceTo(const taabb3<T>& other) const
        {
            const auto  dx  =   (std::max)({T(0.0), _minimum.x - other._maximum.x, other._minimum.x - _maximum.x});
            const auto  dy  =   (std::max)({T(0.0), _minimum.y - other._maximum.y, other._minimum.y - _maximum.y});
            const auto  dz  =   (std::max)({T(0.0), _minimum.z - other._maximum.z, other._minimum.z - _maximum.z});
            return (T)std::sqrt(dx*dx + dy*dy + dz*dz);
        }
        /// <summary>
        /// 得到里包围盒最近的点
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        inline  auto    closedPoint(const tvec3<T>& point) const
        {
            return  FE::clamp(point, _minimum, _maximum);
        }
        /// <summary>
        /// 返回中心点
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  auto    getCenter(void) const
        {
            return tvec3<T>((_maximum.x + _minimum.x) * T(0.5f),
                (_maximum.y + _minimum.y) * T(0.5f),
                (_maximum.z + _minimum.z) * T(0.5f));
        }
        /// <summary>
        /// 返回中心点
        /// </summary>
        /// <returns></returns>
        inline  auto    center() const
        {
            return  getCenter();
        }
        /// <summary>
        /// 计算包围盒三个维度的长宽高
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  auto    getSize(void) const
        {
            return _maximum - _minimum;
        }
        /// <summary>
        /// 计算包围盒三个维度的长宽高的一半
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        inline  auto    getHalfSize(void) const
        {
            return (_maximum - _minimum) * T(0.5);
        }
        /// <summary>
        /// 包含点测试
        /// </summary>
        /// <param name="v"></param>
        /// <returns></returns>
        inline  bool    contains(const tvec3<T>& v) const
        {
            return  _minimum.x <= v.x && v.x <= _maximum.x &&
                _minimum.y <= v.y && v.y <= _maximum.y &&
                _minimum.z <= v.z && v.z <= _maximum.z;
        }
        /// <summary>
        /// 包含测试
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        inline  bool    contains(const taabb3& other) const
        {
            return this->_minimum.x <= other._minimum.x &&
                this->_minimum.y <= other._minimum.y &&
                this->_minimum.z <= other._minimum.z &&
                other._maximum.x <= this->_maximum.x &&
                other._maximum.y <= this->_maximum.y &&
                other._maximum.z <= this->_maximum.z;
        }
        inline  bool    operator== (const taabb3& right) const
        {
            return this->_minimum == right._minimum &&
                this->_maximum == right._maximum;
        }
        inline  bool    operator!= (const taabb3& right) const
        {
            return !(*this == right);
        }
        
        /// <summary>
        /// 计算包围盒 right 是否在当前包围盒的内部(严格内部,不包含相交的情况)
        /// </summary>
        /// <param name="right"></param>
        /// <param name="epsilon"></param>
        /// <returns>在内部返回true, 否则返回false</returns>
        inline  bool    inside(const taabb3& right, T epsilon = std::numeric_limits<T>::epsilon()) const
        {
            if (this == &right || this->isNull() || right.isNull())
            {
                return false;
            }

            if (right._minimum.x - this->_minimum.x <= epsilon)
                return false;
            if (this->_maximum.x - right._maximum.x <= epsilon)
                return false;

            if (right._minimum.y - this->_minimum.y <= epsilon)
                return false;
            if (this->_maximum.y - right._maximum.y <= epsilon)
                return false;

            if (right._minimum.z - this->_minimum.z <= epsilon)
                return false;
            if (this->_maximum.z - right._maximum.z <= epsilon)
                return false;

            return true;
        }
        /// <summary>
        /// 表面积准确
        /// </summary>
        /// <returns></returns>
        inline  T       area() const
        {
            const auto  size    =   getSize();
            return  (size.x * size.y + size.y * size.z + size.z * size.x) * T(2);
        }
        /// <summary>
        /// 在部分计算中只需要比较相对面积，可以少计算一次乘法
        /// </summary>
        /// <returns></returns>
        inline  T       halfArea() const
        {
            const auto  size    =   getSize();
            return  size.x * size.y + size.y * size.z + size.z * size.x;
        }
        /// <summary>
        /// 体积计算
        /// </summary>
        /// <returns></returns>
        inline  T       volume() const
        {
            const auto  size    =   getSize();
            return  size.x * size.y * size.z;
        }
        /// <summary>
        /// 体积计算平方
        /// </summary>
        /// <returns></returns>
        inline  T       volume2() const
        {
            const auto  vol    =   volume();
            return  vol * vol;
        }
        /// <summary>
        /// 返回最长的轴0:x轴,1,y轴,2,z轴
        /// </summary>
        inline  int     longestAxis() const
        {
            const auto  size    =   getSize();
            if (size.x >= size.y && size.x >= size.z)
                return  0;
            else
                return (size.y > size.z) ? 1 : 2;
        }
    public:
        /// <summary>
        /// 分离轴测试
        /// </summary>
        /// <param name="axes"></param>
        /// <param name="nAxes"></param>
        /// <param name="v0"></param>
        /// <param name="v1"></param>
        /// <param name="v2"></param>
        /// <param name="extents"></param>
        /// <returns></returns>
        static  bool    satForAxes(  const tvec3<T>* axes
                                    ,size_t          nAxes
                                    ,const tvec3<T>& v0
                                    ,const tvec3<T>& v1
                                    ,const tvec3<T>& v2
                                    ,const tvec3<T>& extents)
        {
            for (size_t i = 0, j = nAxes - 1; i <= j; ++i )
            {
                const   auto&   testAxis   =   axes[i];
                /// project the aabb onto the separating axis
                const   auto    r   =   extents.x * std::abs(testAxis.x) + extents.y * std::abs(testAxis.y) + extents.z * std::abs(testAxis.z);
                /// project all 3 vertices of the triangle onto the seperating axis
                const   auto    p0  =   FE::dot(v0,testAxis);
                const   auto    p1  =   FE::dot(v1,testAxis);
                const   auto    p2  =   FE::dot(v2,testAxis);
                /// actual test, basically see if either of the most extreme of the triangle points intersects r
                if ((std::max)(-(std::max)(p0, (std::max)(p1, p2)), (std::min)(p0, (std::min)(p1, p2))) > r)
                {
                    /// points of the projected triangle are outside the projected half-length of the aabb
                    /// the axis is separating and we can exit
                    return false;
                }
            }
            return true;
        }
    };

    template class taabb3<float>; 
    template class taabb3<double>; 

    using   aabb3dr =   taabb3<real>;
    using   aabb3df =   taabb3<float>;
}

