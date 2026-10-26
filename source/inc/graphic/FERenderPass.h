#pragma     once

#include    "../FEFormat.hpp"
#include    "RSObject.h"
namespace   FE
{
    /// <summary>
    /// 
    /// </summary>
    class   FE_API  FERenderPass :public RSObject
    {
    public:
        struct  CreateInfo
        {
            FEFormat    _colorFmt   =   FMT_R8G8B8A8_UNORM;
            FEFormat    _depthFmt   =   FMT_D32_S8_UNORM;
            /// <summary>
            /// 是否包含 color attachment（OIT Geometry Pass 等无输出场景可置 false）
            /// </summary>
            bool        _withColor  =   true;
            /// <summary>
            /// 是否包含 depth/stencil attachment
            /// </summary>
            bool        _withDepth  =   true;
            /// <summary>
            /// 附件采样数(MSAA),需与管线 _sampleCount 一致
            /// </summary>
            uint        _samples    =   1;
        };
    protected:
        CreateInfo  _cInfo;
    public:
        FERenderPass(FEContext& ctx)
            :RSObject(ctx)
        {}
        FERenderPass(const FERenderPass& other)
            :RSObject(other)
        {}
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool    create(const CreateInfo& cInfo) =   0;
    };
    using   RenderPass      =   SharedPtr<FERenderPass>;
    using   RenderPassPtr   =   SharedPtr<FERenderPass>;
}
