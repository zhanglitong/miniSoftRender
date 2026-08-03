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
            FEFormat    _depthFmt   =   FMT_D32_UNORM;
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