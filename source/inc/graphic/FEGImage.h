#pragma     once

#include    "../FEFormat.hpp"
#include    "../FEAspect.hpp"
#include    "../FEImageUsage.hpp"
#include    "../FEImageLayout.hpp"
#include    "../FEImageType.hpp"
#include    "RSObject.h"
#include    "FEGImageView.h"
namespace   FE
{
    
    class   FE_API  FEGImage  :public RSObject
    {
    public:
        struct  CreateInfo
        {
            FEFormat        _format;
            FEAspect        _aspect =   ASPECT_COLOR_BIT;
            FEImageType     _type   =   IT_2D;
            FEImageLayout   _layout =   IL_UNDEFINED;
            ImageUsageBits  _usage  =   USAGE_TRANSFER_SRC | USAGE_TRANSFER_DST | USAGE_SAMPLED;
            uint            _width  =   0;
            uint            _height =   0;
            uint            _depth  =   1;
            uint            _mips   =   1;
            uint            _layer  =   1;
            /// <summary>
            /// 采样数(MSAA):1=单采样,4=4x。多采样图像仅可作为 attachment 使用
            /// </summary>
            uint            _samples    =   1;
        };
    public:
        FEGImage(FEContext& ctx)
            :RSObject(ctx)
        {}

        FEGImage(const FEGImage& other)
            :RSObject(other)
        {
            _cInfo  =   other._cInfo;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }

        void    setLayout(FEImageLayout layout)
        {
            _cInfo._layout =   layout;
        }
        /// <summary>
        /// 包装外部原生图像(如交换链图像)时填写尺寸/格式信息,不做任何资源分配。
        /// 布局保持原值(默认 UNDEFINED),由渲染时的布局切换按需更新
        /// </summary>
        /// <param name="width">宽</param>
        /// <param name="height">高</param>
        /// <param name="format">像素格式</param>
        void    setNativeInfo(uint width,uint height,FEFormat format)
        {
            _cInfo._width   =   width;
            _cInfo._height  =   height;
            _cInfo._depth   =   1;
            _cInfo._mips    =   1;
            _cInfo._layer   =   1;
            _cInfo._format  =   format;
        }
        virtual bool        create(const CreateInfo& info)    =   0;
        virtual void        destroy()       =   0;
        virtual GImgView    createView()    =   0;
        /// <summary>
        /// 创建指定 mip 范围的 imageView(用于 pass1 只渲 mip0,透明管线采样完整链)
        /// 默认实现用 baseMip=0,levelCount=_cInfo._mips(等价无参版本)
        /// </summary>
        virtual GImgView    createView(uint baseMip,uint levelCount)
        {   
            UNUSED(baseMip,levelCount);
            return  createView();  
        }
        virtual bool        resize(const uint3& dims,uint layer,uint mips) 
        {
            if (   dims.x == _cInfo._width 
                && dims.y == _cInfo._height 
                && dims.z == _cInfo._depth)
                return  false;

            CreateInfo  info    =   _cInfo;
            info._width         =   dims.x;
            info._height        =   dims.y;
            info._depth         =   dims.z;
            info._layer         =   layer == 0 ? _cInfo._layer : layer;
            info._mips          =   mips == 0  ? _cInfo._mips : mips;
            /// 销毁当前的资源
            destroy();
            /// 创建新的资源
            bool    result     =   create(info);
            if (result)
                fireNotify();
            return  result;    
        }
    protected:
        CreateInfo    _cInfo;
    };

    using   GImage      =   SharedPtr<FEGImage>;
    using   GImages     =   std::vector<GImage>;
}


