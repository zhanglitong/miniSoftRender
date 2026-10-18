#pragma     once

#include    "RSObject.h"
#include    "FEShader.h"
#include    "FEDSet.h"
#include    "FEDSetPool.h"
#include    "FEDSetLayout.h"
#include    "FERenderPass.h"
#include    "FEGraphicEnums.h"
#include    "../mesh/FEPrimitive.hpp"


namespace   FE
{
    class   FE_API  FEPipeline :public RSObject
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

            // ===== Pipeline State Overrides =====
            // 默认值 0 表示用后端硬编码值，非 0 时覆盖
            bool                _enableDepthTest    =   true;   // OIT color pass: false
            bool                _enableDepthWrite   =   true;   // OIT geometry pass: false（透明不互相遮挡）
            bool                _enableColorBlend   =   false;  // OIT color pass: true
            // 预乘 alpha 混合:src factor=ONE / dst=ONE_MINUS_SRC_ALPHA(用于 transmission
            // 玻璃:BRDF 反射色已含菲涅尔权重,需加性叠加而不是再乘一次 alpha)
            bool                _premultipliedAlpha =   false;
            uint32_t            _colorAttachmentCount = 1;      // OIT geometry pass: 0
            int                 _cullModeOverride    =   -1;     // -1=默认BACK, 0=NULL, 1=FRONT, 2=BACK, 3=FRONT_AND_BACK
        };
    public:
        FEPipeline(FEContext& ctx)
            :RSObject(ctx)
        {}
        FEPipeline(const FEPipeline& other)
            :RSObject(other)
        {}
        virtual ~FEPipeline()
        {
        }
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
