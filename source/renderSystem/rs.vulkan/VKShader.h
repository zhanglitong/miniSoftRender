#pragma     once

#include    "graphic/FEShader.h"
#include    "graphic/FEDSetLayout.h"
#include    "FEVulkan.h"

namespace   FE
{
    class   VKShader :public TRSObject<VkShaderModule,FEShader>
    {
    public:
        
        using   PushConstants   =   std::vector<VkPushConstantRange>;
        struct  ReflectData
        {
            FEDSetBindings  _bindings;
            PushConstants   _pushConstants;
            FEShaderType    _stage;
            
        };
    public:
        VKShader(FEContext& ctx)
            :TRSObject<VkShaderModule,FEShader>(ctx)
        {
        }
        VKShader(const VKShader& other)
            :TRSObject<VkShaderModule,FEShader>(other)
        {}

        virtual ~VKShader();

        virtual bool    create(const CreateInfo& info) override;

        const ReflectData&     reflectData() const   
        { 
            return _reflectData;
        }
        DSetLayout             createLayoutFromReflect();
    protected:  
        void    spirvReflect(const VkShaderModuleCreateInfo& info);
        ReflectData    _reflectData;
    };
}
