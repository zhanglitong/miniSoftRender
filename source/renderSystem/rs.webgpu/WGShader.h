#pragma     once

#include    "graphic/FEShader.h"
#include    "graphic/FEDSetLayout.h"
#include    "graphic/FEGraphicEnums.h"
#include    "wgpu.h"

namespace   FE
{
    class   WGShader :public TRSObject<WGPUShaderModule, FEShader>
    {
    public:
        struct ReflectBinding
        {
            uint32_t    _binding;
            FEDescType  _descriptorType;
            uint32_t    _stageFlags;
            String      _name;
        };
        using ReflectBindings = std::vector<ReflectBinding>;

        struct ReflectData
        {
            ReflectBindings    _bindings;
            FEShaderType        _stage;
            uint32_t            _stageFlags    =   0;
        };
    public:
        WGShader(FEContext& ctx)
            :TRSObject<WGPUShaderModule, FEShader>(ctx)
        {}
        WGShader(const WGShader& other)
            :TRSObject<WGPUShaderModule, FEShader>(other)
        {}

        virtual ~WGShader();

        virtual bool    create(const CreateInfo& info) override;

        const ReflectData&     reflectData() const   
        {
            return _reflectData;
        }
        DSetLayout    createLayoutFromReflect();

    protected:
        void    reflectShader(const CreateInfo& info);
        ReflectData    _reflectData;
    };
}