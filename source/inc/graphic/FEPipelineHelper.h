#pragma     once

#include    "../FEXML.hpp"
#include    "FEPipeline.h"
#include    "FEDevice.h"

namespace   FE
{

    class   FEPipelineHelper :public RSObject
    {
    public:
        /// <summary>
        /// 从 xml 文件加载管线。sampleCount 为渲染目标采样数(MSAA),
        /// 需与 FEScene 当前渲染目标一致,否则动态渲染验证失败
        /// </summary>
        static  Pipelines   create(FEContext& ctx,FEDevice& device,RenderPass renderPass,const char* fileName,uint sampleCount = 1);
        static  Pipelines   create(FEContext& ctx,FEDevice& device,RenderPass renderPass,XMLNode* node,const String& prefix,uint sampleCount = 1);
    protected:
        static  Pipeline    createPipeline(FEContext& ctx,FEDevice& device,RenderPass renderPass,FEPipeline::CreateInfo& cInfo,XMLNode* node,const String& prefix,const char* plType);

        static  Shader      createShader(FEContext& ctx,FEDevice& device,const String& path);


    };
}
