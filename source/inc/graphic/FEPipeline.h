#pragma     once

#include    "RSObject.h"
#include    "FEShader.h"
#include    "FEDSet.h"
#include    "FEDSetPool.h"
#include    "FEDSetLayout.h"
#include    "FERenderPass.h"
#include    "FEGraphicEnums.h"


namespace   FE
{

    class   FEPipeline :public RSObject
    {
    public:
        struct  InputAssemblyState
        {
            EPrimitive  _primitive  =   PRI_TRIANGLES;
        };
        struct  CreateInfo
        {
            RenderPass          _renderPass;
            Shaders             _shaders;
            InputBinds          _binds;
            DynamicStates       _dynamicStates;
            InputAssemblyState  _inputAssemblyState;
            /// <summary>
            /// 内部填充，不需要外部赋予
            /// </summary>
            ShaderTypes         _pushConstantStage;
            bool                _pushConstant;
        };
    public:
        FEPipeline(FEContext& ctx)
            :RSObject(ctx)
        {}
        FEPipeline(const FEPipeline& other)
            :RSObject(other)
        {}
        const String&   name() const
        {
            return  _name;
        }
        inline  void    setName(const String& name)
        {
            _name   =   name;
        }
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool    create(const CreateInfo& info)  =   0;
        virtual PLType  type()  const =   0;
        virtual Handle  nativeLayout()  const =   0;
        virtual DSets   createDSets()    =   0;
    protected:
        CreateInfo  _cInfo;
        String      _name;
    };
    using   Pipeline        =   SharedPtr<FEPipeline>;
    using   Pipelines       =   std::vector<Pipeline>;
}