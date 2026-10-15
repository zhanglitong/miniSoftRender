#pragma once
#include    "RSObject.h"
#include    "../FEFormat.hpp"
#include    "FEGraphicEnums.h"

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
        /// 创建 GPU 资源(image + view + sampler)
        /// </summary>
        virtual bool        create(const CreateInfo& info)  =   0;
        /// <summary>
        /// 上传像素数据(RAW 像素,按 format 紧密排布,行尾无 padding)
        /// </summary>
        virtual bool        update(const void* data,uint64 size)    =   0;
    protected:
        CreateInfo  _cInfo;
    };

    using   TexPtr  =   SharedPtr<FETexture>;
    using   TexPtrs =   std::vector<TexPtr>;
}
