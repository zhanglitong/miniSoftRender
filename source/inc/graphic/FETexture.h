#pragma once
#include    "RSObject.h"
#include    "../FEFormat.hpp"
#include    "FEGraphicEnums.h"
#include    "FEGImage.h"

namespace   FE
{
    /// <summary>
    /// 纹理对象:封装 2D 图像 + 视图 + 采样器,
    /// 作为 combined image sampler 绑定到描述符集
    /// (FEFilter/FEAddressMode 定义于 FEGraphicEnums.h)
    /// </summary>
    class   FE_API  FETexture : public RSObject
    {
    public:
        struct  CreateInfo
        {
            uint            _width      =   0;
            uint            _height     =   0;
            FEFormat        _format     =   FMT_R8G8B8A8_UNORM;
            FEFilter        _magFilter  =   FILTER_LINEAR;
            FEFilter        _minFilter  =   FILTER_LINEAR;
            FEAddressMode   _addressMode =  ADDRESS_REPEAT;
            /// mip 级数(>1 时 update 需按 mip 逐级提供数据,采样器 maxLod=mips-1)
            uint            _mips       =   1;
            /// 是否为 cubemap(6 面;update 数据按 面主序、面内 mip 顺序 紧密排布)
            bool            _cube       =   false;
            /// 是否作为渲染目标(color attachment + sampled 双用途,
            /// 如 transmission 场景折射纹理;此时无需 update 上传像素)
            bool            _renderTarget = false;
        };
    public:
        FETexture(FEContext& ctx)
            :RSObject(ctx)
        {}
        FETexture(const FETexture& other)
            :RSObject(other)
        {
            _cInfo  =   other._cInfo;
        }
        /// <summary>
        /// 获取创建信息
        /// </summary>
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        inline  uint    width() const
        {
            return  _cInfo._width;
        }
        inline  uint    height() const
        {
            return  _cInfo._height;
        }
        inline  FEFormat    format() const
        {
            return  _cInfo._format;
        }
        /// <summary>
        /// mip 级数
        /// </summary>
        inline  uint    mips() const
        {
            return  _cInfo._mips;
        }
        /// <summary>
        /// 是否为 cubemap
        /// </summary>
        inline  bool    isCube() const
        {
            return  _cInfo._cube;
        }
        /// <summary>
        /// 创建 GPU 资源(image + view + sampler)
        /// </summary>
        virtual bool        create(const CreateInfo& info)  =   0;
        /// <summary>
        /// 上传像素数据(RAW 像素,按 format 紧密排布,行尾无 padding)
        /// </summary>
        virtual bool        update(const void* data,uint64 size)    =   0;
        /// <summary>
        /// 图像视图(render target 用法下作为 color attachment 挂到 RenderInfo._colors)
        /// </summary>
        virtual GImgView    imageView() {   return  nullptr;    }
        /// <summary>
        /// 底层 GPU 图像(用于创建指定 mip 范围的 imageView 等高级操作)
        /// </summary>
        virtual GImage      image() {   return  nullptr;    }
    protected:
        CreateInfo  _cInfo;
    };

    using   TexPtr  =   SharedPtr<FETexture>;
    using   TexPtrs =   std::vector<TexPtr>;
}
