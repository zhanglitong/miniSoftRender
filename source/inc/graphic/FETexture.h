#pragma once
#include    <utility>
#include    <vector>
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
            /// <summary>
            /// mip 级数(>1 时 update 需按 mip 逐级提供数据,采样器 maxLod=mips-1)
            /// </summary>
            uint8_t         _mips       =   1;
            /// <summary>
            /// 是否为 cubemap(6 面;update 数据按 面主序、面内 mip 顺序 紧密排布)
            /// </summary>
            bool            _cube       =   false;
            /// <summary>
            /// 是否作为渲染目标(color attachment + sampled 双用途,
            /// 如 transmission 场景折射纹理;此时无需 update 上传像素)
            /// </summary>
            bool            _renderTarget = false;
            /// <summary>
            /// 参数比较(含影响图像分配与采样器的全部字段,不含像素数据):
            /// create 重复调用时据此判断是否需要重建 GPU 资源
            /// </summary>
            bool    operator==(const CreateInfo& other) const
            {
                return  _width          ==  other._width
                    &&  _height         ==  other._height
                    &&  _format         ==  other._format
                    &&  _magFilter      ==  other._magFilter
                    &&  _minFilter      ==  other._minFilter
                    &&  _addressMode    ==  other._addressMode
                    &&  _mips           ==  other._mips
                    &&  _cube           ==  other._cube
                    &&  _renderTarget   ==  other._renderTarget;
            }
            bool    operator!=(const CreateInfo& other) const
            {
                return  !(*this == other);
            }
        };
    public:
        FETexture(FEContext& ctx)
            :RSObject(ctx)
        {}
        FETexture(const FETexture& other)
            :RSObject(other)
        {
            _cInfo  =   other._cInfo;
            _source =   other._source;
        }
        /// <summary>
        /// 设置纹理的来源对象(持有引用,可为 nullptr):
        /// FEFileBuffer(原始压缩文件字节)或 FEImage(CPU 图像)。
        /// 保存工程时只序列化该来源,打开时再解码/上传,避免在纹理里长期克隆一份像素。
        /// </summary>
        inline  void    setSource(const Object& source)
        {
            _source =   source;
        }
        /// <summary>
        /// 纹理来源对象;nullptr 表示该纹理没有可序列化的来源
        /// (如 1x1 默认纹理、渲染目标、程序生成的纹理)
        /// </summary>
        inline  Object  source() const
        {
            return  _source;
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
        inline  auto    format() const
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
        /// 创建 GPU 资源(image + view + sampler),可重复调用:
        /// 1. 已创建且参数一致:直接复用,不重建(像素内容保持不变,新数据用 update 上传)
        /// 2. 参数变化或上次未创建完整:先 destroy 旧资源,再按新参数重建
        /// </summary>
        virtual bool    create(const CreateInfo& info)  =   0;
        /// <summary>
        /// 上传像素数据(RAW 像素,按 format 紧密排布,行尾无 padding)
        /// </summary>
        virtual bool    update(const void* data,uint64 size)    =   0;
        /// <summary>
        /// 销毁释放 GPU 资源(image + view + sampler)
        /// </summary>
        virtual void    destroy() = 0;
        /// <summary>
        /// 底层 GPU 图像(用于创建指定 mip 范围的 imageView 等高级操作)
        /// </summary>
        virtual GImage  image() {   return  nullptr;    }
        /// <summary>
        /// 图像视图(render target 用法下作为 color attachment 挂到 RenderInfo._colors)
        /// </summary>
        virtual GImgView    imageView() {   return  nullptr;    }
    protected:
        CreateInfo  _cInfo;
        /// <summary>
        /// 纹理来源对象(FEFileBuffer / FEImage);保存工程时序列化它并据此重建纹理
        /// </summary>
        Object      _source;
    };

    using   TexPtr  =   SharedPtr<FETexture>;
    using   TexPtrs =   std::vector<TexPtr>;
}
