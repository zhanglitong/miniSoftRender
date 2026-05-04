#pragma     once

#include    "FEDefine.h"
#include    "FEMath.hpp"
namespace   FE
{
    template<class T>
    class    TRect
    {
    protected:
        T   _left;
        T   _top;
        T   _right;
        T   _bottom;
    public:
        TRect(const T& l = 0,const T& t = 0,const T& r = 0,const T& b = 0)
        {
            _left       =   l;
            _top        =   t;
            _right      =   r;
            _bottom     =   b;
        }
        inline  T       width() const
        {
            return  _right - _left;
        }
        inline  T       height() const
        {
            return  _bottom - _top;
        }
        /// <summary>
        /// 大小
        /// </summary>
        /// <returns></returns>
        inline  auto    size() const
        {
            return  tvec2<T>(width(),height());
        }
        inline  auto    halfSize() const
        {
            return  tvec2<T>(width()/T(2),height()/T(2));
        }
        /// <summary>
        /// 中心点
        /// </summary>
        /// <returns></returns>
        inline  auto    center() const
        {
            return  tvec2<T>((_right + _left)/T(2), (_bottom + _top)/T(2));
        }
    };

    using   RectI16     =   TRect<int16>;
    using   RectI32     =   TRect<int32>;
    using   RectU16     =   TRect<uint16>;
    using   RectU32     =   TRect<uint32>;

    using   RectF16     =   TRect<float16>;
    using   RectF32     =   TRect<float32>;

}

