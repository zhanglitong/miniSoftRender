#pragma     once

#include    "../FEDefine.h"
#include    "FEVec2.hpp"

namespace   FE
{
    template<typename T>
    class   TRect
    {
    public:
        /// <summary>
        /// 构造
        /// </summary>
        /// <param name="l">左</param>
        /// <param name="t">上</param>
        /// <param name="r">右</param>
        /// <param name="b">下</param>
        TRect(T left = 0, T top = 0, T right = 0, T bottom = 0)
        {
            _left   =   left;
            _top    =   top;
            _right  =   right;
            _bottom =   bottom;
        }

        /// <summary>
        /// 构造
        /// </summary>
        /// <param name="l">左</param>
        /// <param name="t">上</param>
        /// <param name="r">右</param>
        /// <param name="b">下</param>
        template<class U>
        TRect(U left, U top , U right , U bottom )
        {
            _left       =   T(left);
            _top        =   T(top);
            _right      =   T(right);
            _bottom     =   T(bottom);
        }
        TRect(const TRect& right )
        {
            _left       =   T(right._left);
            _top        =   T(right._top);
            _right      =   T(right._right);
            _bottom     =   T(right._bottom);
        }
        template<class U>
        explicit TRect(const TRect<U>& right )
        {
            _left       =   T(right._left);
            _top        =   T(right._top);
            _right      =   T(right._right);
            _bottom     =   T(right._bottom);
        }
        /// <summary>
        /// 通过tvec2<T> 构造
        /// </summary>
        /// <param name="points"></param>
        TRect(std::initializer_list<tvec2<T>> points)
        {
            if (points.size() == 0)
            {
                _left   =   0;
                _top    =   0;
                _right  =   0;
                _bottom =   0;
            }
            else
            {
                auto it    =   points.begin();
                /// 用第一个点初始化边界
                _left   =   it->x;
                _top    =   it->x;
                _right  =   it->y;
                _bottom =   it->y;
                for (++it; it != points.end(); ++it)
                {
                    if (it->x < _left)  _left       =   it->x;
                    if (it->x > _right) _right      =   it->x;
                    if (it->y < _top)   _top        =   it->y;
                    if (it->y > _bottom) _bottom    =   it->y;
                }
            }
        }
        /// <summary>
        /// null
        /// </summary>
        /// <returns></returns>
        bool    isNull() const
        {
            return  _left   == 0
                && _top    == 0
                && _right  == 0
                && _bottom == 0;
        }
        /// <summary>
        /// 是否有效
        /// </summary>
        /// <returns></returns>
        bool    isValid() const
        {
            return  _left <=  _right && _top <= _bottom;
        }
        auto    minX() const
        {
            return  _left; 
        }
        auto    maxX() const
        {
            return  _right; 
        }
        auto    minY() const
        {
            return  _top; 
        }
        auto    maxY() const
        {
            return  _bottom; 
        }
        auto    minPoint() const
        {
            return  tvec2<T>(_left,_top);
        }
        auto    maxPoint() const
        {
            return  tvec2<T>(_right,_bottom);
        }
        auto    left() const
        {
            return  _left;
        }
        auto    top() const
        {
            return  _top;
        }
        auto    right() const
        {
            return  _right;
        }
        auto    bottom() const
        {
            return  _bottom;
        }
        auto    leftTop() const
        {
            return  tvec2<T>(_left,_top);
        }
        auto    rightBottom() const
        {
            return  tvec2<T>(_right,_bottom);
        }
        void    setLeft(T val) 
        {
            _left   =   val;
        }
        void    setTop(T val) 
        {
            _top    =   val;
        }
        void    setRight(T val) 
        {
            _right  =   val;
        }
        void    setBottom(T val)
        {
            _bottom =   val;
        }
        void    plusLeft(T val) 
        {
            _left   +=  val;
        }
        void    plusTop(T val) 
        {
            _top    +=  val;
        }
        void    plusRight(T val) 
        {
            _right  +=  val;
        }
        void    plusBottom(T val)
        {
            _bottom +=  val;
        }
        /// <summary>
        /// 宽度
        /// </summary>
        /// <returns></returns>
        T       width() const
        {
            return  _right - _left;
        }
        /// <summary>
        /// 高度
        /// </summary>
        /// <returns></returns>
        T       height() const
        {
            return  _bottom - _top;
        }
        /// <summary>
        /// 设置函数
        /// </summary>
        /// <param name="l">左</param>
        /// <param name="t">上</param>
        /// <param name="r">右</param>
        /// <param name="b">下</param>
        auto&   set(T l,T t,T r,T b)
        {
            _left   =   l;
            _top    =   t;
            _right  =   r;
            _bottom =   b;

            assert(_left <= _right);
            assert(_top  <= _bottom);
            return  *this;
        }
        auto&   setLeftTop(T l,T t)
        {
            _left   =   T(l);
            _top    =   T(t);
            return  *this;
        }
        auto&   setRightBottom(T r,T b)
        {
            _right  =   T(r);
            _bottom =   T(b);
            return  *this;
        }
        /// <summary>
        /// 设置大小函数,影响right,bottom
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="w"></param>
        /// <param name="h"></param>
        auto&   setSize(T w,T h)
        {
            _right  =   _left + w;
            _bottom =   _top  + h;
            return  *this;
        }
        auto    size() const
        {
            return  tvec2<T>(_right - _left,_bottom - _top);
        }
        /// <summary>
        /// 设置leftTop，不修改大小
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="x"></param>
        /// <param name="y"></param>
        auto&   setPosition(T l,T t)
        {
            const auto w = _right  - _left;
            const auto h = _bottom - _top;
            _left       =   l;
            _top        =   t;
            _right      =   _left + w;
            _bottom     =   _top  + h;
            return  *this;
        }
        auto&   moveOffset(T xOff,T yOff)
        {
            _left       +=  xOff;
            _top        +=  yOff;
            _right      +=  xOff;
            _bottom     +=  yOff;
            return  *this;
        }
        /// <summary>
        /// 修改中心点,整体移动
        /// </summary>
        /// <typeparam name="U"></typeparam>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns></returns>
        auto&   setCenter(T x,T y)
        {
            const auto vCenter  =   center();
            const auto offsetX  =   x - vCenter.x;
            const auto offsetY  =   y - vCenter.y;

            _left   +=  offsetX;
            _right  +=  offsetX;
            _top    +=  offsetY;
            _bottom +=  offsetY;
            return  *this;
        }
        /// <summary>
        /// 获取中心点
        /// </summary>
        /// <returns></returns>
        auto    center() const
        {
            /// 1. 使用 x1 + (delta / 2) 防止 (x1 + x2) 超过 int 最大值导致溢出
            /// 2. 编译器会自动将 / 2 优化为位移指令，无需手动写 >> 1
            if constexpr(std::is_integral_v<T>)
                return  tvec2<T>(_left + (_right - _left)/2, _top + (_bottom - _top)/2);
            else
                return  tvec2<T>((_left + _right) * 0.5f, (_top + _bottom) * 0.5f);
        }
        /// <summary>
        /// 中心点加半径模式,正方形
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="size"></param>
        auto&   fromCenter(T x, T y, T size)
        {
            if constexpr(std::is_integral_v<T>)
            {
                const auto nHalf    =   size/2;
                _left   =   T(x - nHalf);
                _top    =   T(y - nHalf);
                _right  =   T(x + nHalf);
                _bottom =   T(y + nHalf);
            }
            else
            {
                const auto fHalf    =   T(size * 0.5f);
                _left   =   T(x - fHalf);
                _top    =   T(y - fHalf);
                _right  =   T(x + fHalf);
                _bottom =   T(y + fHalf);
            }

            return  *this;
        }
        /// <summary>
        /// 中心点加双半径模式,长方形
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <param name="sizeX"></param>
        /// <param name="sizeY"></param>
        auto&   fromCenter(T x, T y, T sizeX, T sizeY)
        {
            if constexpr(std::is_integral_v<T>)
            {
                const auto wHalf    =   sizeX/2;
                const auto hHalf    =   sizeY/2;

                _left   =   (x - wHalf);
                _top    =   (y - hHalf);
                _right  =   (x + wHalf);
                _bottom =   (y + hHalf);
            }
            else
            {
                const auto wHalf    =   T(sizeX * 0.5f);
                const auto hHalf    =   T(sizeY * 0.5f);

                _left   =   T(x - wHalf);
                _top    =   T(y - hHalf);
                _right  =   T(x + wHalf);
                _bottom =   T(y + hHalf);
            }

            return  *this;
        }
        /// <summary>
        /// 点在矩形内 x->[left,right],y->[top,bottom]
        /// </summary>
        /// <param name="x"></param>
        /// <param name="y"></param>
        /// <returns></returns>
        bool    ptInRect(T x, T y)
        {
            return  x >= _left && x <= _right && y >= _top && y <= _bottom;
        }
        /// <summary>
        /// 严格不重叠（用于渲染裁剪，边缘接触不算）
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        bool    overlaps(const TRect<T>& other) const
        {
            return !( _right  <= other._left || _left >= other._right || 
                _bottom <= other._top  || _top  >= other._bottom );
        }
        /// <summary>
        /// 接触即相交(用于碰撞检测，边缘接触算相交)
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        bool    touches(const TRect<T>& other) const
        {
            return !( _right  < other._left || _left > other._right || 
                _bottom < other._top  || _top  > other._bottom );
        }
        /// <summary>
        /// 返回交集
        /// </summary>
        /// <param name="other"></param>
        /// <returns></returns>
        auto    intersection(const TRect<T>& other) const
        {
            T   ix1 =   (std::max)(_left,     other._left);
            T   iy1 =   (std::max)(_top,      other._top);
            T   ix2 =   (std::min)(_right,    other._right);
            T   iy2 =   (std::min)(_bottom,   other._bottom);

            // 判断是否有重叠区域
            // 不相交返回“空矩形”
            if (ix1 < ix2 && iy1 < iy2)
                return TRect<T>{ix1, iy1, ix2, iy2};
            else
                return TRect<T>{0, 0, 0, 0};
        }
    public:
        T    _left;
        T    _top;
        T    _right;
        T    _bottom;
    };

    using   RectI16     =   TRect<int16>;
    using   RectI32     =   TRect<int32>;
    using   RectU16     =   TRect<uint16>;
    using   RectU32     =   TRect<uint32>;

    using   RectF16     =   TRect<float16>;
    using   RectF32     =   TRect<float32>;

    template class TRect<float>; 
    template class TRect<real>; 

}

