#pragma     once

#include    "../inc/FEDefine.h"
#include    "../inc/FEObject.h"
#include    "../inc/FEColor.hpp"
#include    "../inc/FERect.hpp"


namespace   FE
{
    class   PixelBuffer
    {
    public:
    };
    template<class T,size_t _W,size_t _H>
    class   TPixelBuffer :public PixelBuffer
    {
    public:
        inline  size_t  width() const
        {
            return  _W;
        }
        inline  size_t  height() const
        {
            return  _H;
        }
        inline  size_t  size() const
        {
            return  _W * _H;
        }
        inline  T*      ptr() const
        {
            return  _data.data();
        }
        inline  T*      ptr()
        {
            return  _data.data();
        }
    public:
        std::array<T,_W * _H> _data;
    };

    using   PB32x32     =   TPixelBuffer<Rgba8,32,32>;

    using   PB32x32s    =   std::vector<PB32x32>;

    class   FEPBuffer
    {
    public:
        static  constexpr   uint16  _W  =   32;
        static  constexpr   uint16  _H  =   32;
    public:
        FEPBuffer(size_t w,size_t h)
        {}
        /// <summary>
        /// 一个快的大小
        /// </summary>
        /// <returns></returns>
        ushort2     cellSize() const
        {
            return  ushort2(_W,_H);
        }
        /// <summary>
        /// 横向与纵向共有多少个块
        /// </summary>
        /// <returns></returns>
        ushort2     gridSize() const
        {
            return  _blocks;
        }
        RectU16     clip() const
        {
            return  _clip;
        }
        RectU16     viewport() const
        {
            return  _viewport;
        }
        auto&       tiles() 
        {
            return  _tiles;
        }
        auto&       tiles() const
        {
            return  _tiles;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="w"></param>
        /// <param name="h"></param>
        /// <returns></returns>
        bool    create(uint16 w,uint16 h)
        {
            _clip       =   RectU16(0,0,w,h);
            _viewport   =   RectU16(0,0,w,h);
            _blocks.x   =   w/_W + (w%_W) ? 1 : 0;
            _blocks.y   =   h/_H + (h%_H) ? 1 : 0;
            _tiles.resize(_blocks.x * _blocks.y);
            return  true;
        }
        void    destroy()
        {}
    protected:
        RectU16     _clip       =   {0,0,1,1};
        RectU16     _viewport   =   {0,0,1,1};
        ushort2     _blocks;
        PB32x32s    _tiles;
    };
}