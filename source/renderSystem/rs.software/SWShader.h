#pragma     once

#include    "graphic/FEShader.h"
#include    "graphic/FEDSetLayout.h"

namespace   FE
{
    class   SWShader :public TRSObject<void*,FEShader>
    {
    public:

        using   PushConstants   =   std::vector<uint32_t>;
        struct  ReflectData
        {
            FEDSetBindings  _bindings;
            PushConstants   _pushConstants;
            FEShaderType    _stage;
        };
    public:
        SWShader(FEContext& ctx)
            :TRSObject<void*,FEShader>(ctx)
        {
        }
        SWShader(const SWShader& other)
            :TRSObject<void*,FEShader>(other)
        {}

        virtual ~SWShader();

        virtual bool    create(const CreateInfo& info) override;

        const ReflectData&     reflectData() const
        {
            return _reflectData;
        }
        DSetLayout             createLayoutFromReflect();
    protected:
        ReflectData    _reflectData;
    };
}
