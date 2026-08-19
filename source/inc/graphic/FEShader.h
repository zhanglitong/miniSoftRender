#pragma     once
#include    <iostream>
#include    <stdio.h>
#include    <vector>
#include    <memory>

#include    "../FEDefine.h"
#include    "../FELog.hpp"
#include    "../FEObject.h"
#include	"../FESmallVector.h"
#include    "../FEFormat.hpp"
#include    "../FEBuffer.hpp"
#include    "../FEFormatHelper.hpp"

#include    "RSObject.h"
#include    "FEShaderType.hpp"
#include    "FEGraphicEnumsHelper.hpp"

namespace   FE
{

    enum    FEInputRate :uint16_t
    {
        V_INPUT_VERTEX     =   0,
        V_INPUT_INSTANCE   =   1,
    } ;

    struct  FEInputDesc
    {
        uint16_t        binding     =   0;
        uint16_t        location    =   0;
        uint32_t        offset      =   0;
        FEFormat        format      =   FMT_R32G32B32A32_FLOAT;
        FEInputSlot     slot        =   IS_VERTEX_POS;
    };

    using   InputDescs =   std::vector<FEInputDesc>;

    struct  FEInputBindDesc
    {
        uint16_t        binding     =   0;
        FEInputRate     inputRate   =   V_INPUT_VERTEX;
        uint32_t        stride      =   0;
        InputDescs      inputs;
    };

    using   InputBinds =   std::vector<FEInputBindDesc>;

    
    class   FEShader :public RSObject
    {
    public:
        struct  CreateInfo
        {
            Buffer          _buffer;
            ShaderTypes     _shaderType;
        };
    public:
        FEShader(FEContext& ctx)
            :RSObject(ctx)
        {}

        FEShader(const FEShader& other)
            :RSObject(other)
        {
            _cInfo  =   other._cInfo;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        const auto& cInfo() const
        {
            return  _cInfo;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <param name="info"></param>
        /// <returns></returns>
        virtual bool    create(const CreateInfo& info)  =   0;
        
    protected:
        CreateInfo  _cInfo;
    };


    using   Shader      =   SharedPtr<FEShader>;
    using   ShaderPtr   =   SharedPtr<FEShader>;
    using   Shaders     =   std::vector<Shader>;
}


