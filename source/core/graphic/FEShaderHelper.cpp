#include    <iostream>
#include    <vector>
#include    "../inc/graphic/FEShaderHelper.h"
#include    "shaderc/shaderc.hpp"
#include    "../inc/graphic/FEDevice.h"
namespace   FE
{
    auto    system2Native(FEShaderType type)
    {
        switch(type)
        {
        case ST_VERTEX_BIT                  :   return  shaderc_vertex_shader           ;
        case ST_TESSELLATION_CONTROL_BIT    :   return  shaderc_tess_control_shader     ;
        case ST_TESSELLATION_EVALUATION_BIT :   return  shaderc_tess_evaluation_shader  ;
        case ST_GEOMETRY_BIT                :   return  shaderc_geometry_shader         ;
        case ST_FRAGMENT_BIT                :   return  shaderc_fragment_shader         ;
        case ST_COMPUTE_BIT                 :   return  shaderc_compute_shader          ;
        case ST_RAYGEN_BIT                  :   return  shaderc_raygen_shader           ;
        case ST_ANY_HIT_BIT                 :   return  shaderc_anyhit_shader           ; 
        case ST_CLOSEST_HIT_BIT             :   return  shaderc_closesthit_shader       ;
        case ST_MISS_BIT                    :   return  shaderc_miss_shader             ;
        case ST_INTERSECTION_BIT            :   return  shaderc_intersection_shader     ;
        case ST_CALLABLE_BIT                :   return  shaderc_callable_shader         ;
        case ST_TASK_BIT                    :   return  shaderc_task_shader             ;
        case ST_MESH_BIT                    :   return  shaderc_mesh_shader             ;
        default                             :   return  shaderc_vertex_shader           ;
        }   
    }
    Shader  FEShaderHelper::buildFromMemory(FEContext& ctx,FEDevice& dev,const char* pSource,FEShaderType type,const char* entryPoint)
    {
        /// 1. 创建编译器实例
        shaderc::Compiler           compiler;
        /// 2. 创建编译选项（可选）
        shaderc::CompileOptions     options;
        auto    kind    =   system2Native(type);
        String  source  =   pSource;
        /// 3. 执行编译
        auto    result  =   compiler.CompileGlslToSpv(source, kind, nullptr, entryPoint, options);

        // 4. 检查编译状态
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) 
        {
            std::cerr << "Shader compilation failed: " << result.GetErrorMessage() << std::endl;
            return {};
        }
        uints   spvCodes    =   {result.cbegin(), result.cend()};
        FEShader::CreateInfo    info;
        info._buffer        =   new FEBuffer(ctx);
        info._buffer->cInfo()._buffer.resize(spvCodes.size() * sizeof(uint));
        memcpy(info._buffer->data(),spvCodes.data(),info._buffer->length());

        auto    shader      =   dev.createShader();
        if (shader->create(info))
            return  shader;
        else
            return  nullptr;
    }


     Shader  FEShaderHelper::buildFromFile(FEContext& ctx,FEDevice& dev,const char* fileName,FEShaderType type,const char* entryPoint)
     {
         auto   buffer  =   FEBuffer::loadFile(ctx,fileName); 
         auto   length  =   buffer->length() + 1;
         String source;
         source.resize(length);
         memcpy(source.data(),buffer->data(),buffer->length());

         return buildFromMemory(ctx,dev,source.c_str(),type,entryPoint);
     }
}


