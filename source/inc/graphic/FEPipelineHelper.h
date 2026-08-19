#pragma     once

#include    "../FEXML.hpp"
#include    "FEPipeline.h"
#include    "FEDevice.h"

namespace   FE
{

    class   FEPipelineHelper :public RSObject
    {
    public:
        static  Pipelines   create(FEContext& ctx,FEDevice& device,RenderPass renderPass,const char* fileName);
        static  Pipelines   create(FEContext& ctx,FEDevice& device,RenderPass renderPass,XMLNode* node,const String& prefix);
    protected:
        static  Pipeline    createPipeline(FEContext& ctx,FEDevice& device,RenderPass renderPass,FEPipeline::CreateInfo& cInfo,XMLNode* node,const String& prefix);

        static  Shader      createShader(FEContext& ctx,FEDevice& device,const String& path);


    };
}
