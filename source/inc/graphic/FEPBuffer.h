#pragma     once

#include    "../FEDefine.h"
#include    "../FEObject.h"
#include    "../FEColor.hpp"
#include    "../FEMath.hpp"

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
        const   T*      ptr() const
        {
            return  _data.data();
        }
        inline  T*      ptr()
        {
            return  _data.data();
        }
        const   T*      rowAt(uint16_t row) const
        {
            return  _data.data()  + row * _W;
        }
        inline  T*      rowAt(uint16_t row)
        {
            return  _data.data()  + row * _W;
        }
        const   T*      dataOffset(uint16_t row,uint16_t col) const
        {
            return  _data.data()  + (row * _W + col );
        }
        inline  T*      dataOffset(uint16_t row ,uint16_t col)
        {
            return  _data.data()  + (row * _W + col );
        }
        inline  auto&   rect() const
        {
            return  _rect;
        }
    public:
        /// <summary>
        /// 在大缓冲区上的坐标范围
        /// </summary>
        RectU16                 _rect;
        std::array<T,_W * _H>   _data;
    };


    static  constexpr   uint16  _TileW  =   32;
    static  constexpr   uint16  _TileH  =   32;

    /// <summary>
    /// 瓦片存储类型必须随颜色类型变化(原硬编码float4导致瓦片元素类型与ColorT不匹配)
    /// </summary>
    template<class T>
    using   PTileBuffer =   TPixelBuffer<T,_TileW,_TileH>;
    using   PB32x32     =   PTileBuffer<float4>;
    using   PB32x32s    =   std::vector<PB32x32>;
    class   FEPBuffer :public FEObject
    {
    public:
        /// <summary>
        /// FEObject没有默认构造,派生类必须显式转发上下文
        /// </summary>
        FEPBuffer(FEContext& ctx)
            :FEObject(ctx)
        {}
    };

    using   PBufferPtr  =   SharedPtr<FEPBuffer>;
    template<class ColorT>
    class   TPBuffer :public FEPBuffer
    {
    public:
        TPBuffer(FEContext& ctx)
            :FEPBuffer(ctx)
        {}
        /// <summary>
        /// 一个快的大小
        /// </summary>
        /// <returns></returns>
        ushort2     cellSize() const
        {
            return  ushort2(_TileW,_TileH);
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
            /// 每个方向向上取整的块数(+优先级高于?:,必须加括号,否则恒为1)
            _blocks.x   =   w/_TileW + ((w%_TileW) ? 1 : 0);
            _blocks.y   =   h/_TileH + ((h%_TileH) ? 1 : 0);
            _tiles.resize(_blocks.x * _blocks.y);
            for (uint16_t y = 0; y < _blocks.y; ++y)
            {
                for (uint16_t x = 0; x < _blocks.x ; ++x)
                {
                    /// 瓦片在大缓冲区上的坐标范围(左,上,右,下),右下角必须是起点加边长
                    RectU16 temp(x * _TileW ,y * _TileH,x * _TileW + _TileW,y * _TileH + _TileH);
                    _tiles[y * _blocks.x + x]._rect  =   _clip.intersection(temp);
                }
            }
            return  true;
        }
        void    destroy()
        {}
    protected:
        RectU16     _clip       =   {0,0,1,1};
        RectU16     _viewport   =   {0,0,1,1};
        ushort2     _blocks;
        std::vector<PTileBuffer<ColorT>>    _tiles;
    };

    using   PBOFloat4   =   TPBuffer<float4>;
}
