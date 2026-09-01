
#include    "../inc/FEStringHelper.hpp"
#include    "../inc/FEFileInfor.hpp"
#include    "../inc/graphic/FEPipelineHelper.h"
#include    "../inc/mesh/FEPrimitive.hpp"
namespace   FE
{
    Pipelines   FEPipelineHelper::create(FEContext& ctx,FEDevice& device,RenderPass renderPass,const char* fileName)
    {
        auto        buffer  =   FEBuffer::loadFile(ctx,fileName); 
        if (buffer == nullptr)
            return  {};
        String      data;
        String      prefix  =   FEFileInfor::fileStem(fileName);

        data.resize(buffer->length() + 1);
        memcpy(data.data(),buffer->data(),buffer->length());

        std::shared_ptr<XMLDoc> doc =   std::make_shared<XMLDoc>();
        try
        {
            doc->parse<0>(data.data());
            XMLNode*    root    = doc->first_node("root");
            if (root == nullptr)
                return  {};
            else
                return  create(ctx,device,renderPass,root,prefix);
        }
        catch (...)
        {
            return  {};
        }
    }
    Pipelines   FEPipelineHelper::create(FEContext& _ctx,FEDevice& device,RenderPass renderPass,XMLNode* root,const String& prefix)
    {
        auto    xmlShaders  =   root->first_node("shaders");
        auto    xmlBinds    =   root->first_node("bindings");
        auto    xmlStates   =   root->first_node("dynamicStates");
        auto    attrType    =   root->first_attribute("type");

        auto    plType      =   attrType ? attrType->value() : "graphic";

        auto    aUsingDef   =   root->first_attribute("using_defult");
      
        if (xmlShaders == nullptr)
            return  {};
        FEPipeline::CreateInfo  cInfo;
        cInfo._renderPass   =   renderPass;

        if (xmlStates)
        {
            for (auto xmlState = xmlStates->first_node(); xmlState ; xmlState = xmlState->next_sibling())
            {
                cInfo._dynamicStates.push_back(stateFromName(xmlState->name()));
            }
        }
        if (cInfo._dynamicStates.empty())
        {
            cInfo._dynamicStates.push_back(VIEWPORT);
            cInfo._dynamicStates.push_back(SCISSOR);
        }

        auto    xmlShader   =   xmlShaders->first_node("shader");
        for ( ; xmlShader ; xmlShader = xmlShader->next_sibling())
        {
            auto    aPath   =   xmlShader->first_attribute("url");
            if (aPath == nullptr)
                continue;
            String  absPath =   _ctx.resourcePath();
            if (aPath->value()[0] == ':')
                absPath +=  (aPath->value() + 1);
            else
                absPath =   aPath->value();
            auto    shader  =   createShader(_ctx,device,absPath);
            if (shader == nullptr)
            {
                LOG_ERR("createShader(%s) return nullptr!",absPath.c_str());
                continue;
            }
            else
            {
                cInfo._shaders.emplace_back(shader);
            }
        }
        if (xmlBinds)
        {
            auto    xmlBind     =   xmlBinds->first_node("binding");
            for (; xmlBind ; xmlBind = xmlBind->next_sibling())
            {
                FEInputBindDesc desc    =   {};
                XMLAttr*        pBind   =   xmlBind->first_attribute("binding");
                XMLAttr*        pRate   =   xmlBind->first_attribute("inputRate");
                desc.binding            =   uint16(pBind ? atoi(pBind->value()) : 0);
                desc.inputRate          =   FEInputRate(pRate ? atoi(pRate->value()) : 0);
                desc.stride             =   0;
                auto            xmlInput=   xmlBind->first_node("input");
                for ( ; xmlInput; xmlInput = xmlInput->next_sibling() )
                {
                    FEInputDesc input   =   {};

                    XMLAttr*    aLoc    =   xmlInput->first_attribute("location");
                    XMLAttr*    aSlot   =   xmlInput->first_attribute("slots");
                    XMLAttr*    aFormat =   xmlInput->first_attribute("formats");
                    XMLAttr*    aOffset =   xmlInput->first_attribute("offsets");

                    input.binding       =   desc.binding;
                    input.location      =   uint16(aLoc ? atoi(aLoc->value()) : 0);

                    String      valSlot =   (aSlot   ? aSlot->value()   : "");
                    String      valFmt  =   (aFormat ? aFormat->value() : "");
                    String      valOff  =   (aOffset ? aOffset->value() : "");

                    Strings     slots   =   FEStringHelper::split(valSlot);
                    Strings     fmts    =   FEStringHelper::split(valFmt);
                    Strings     offsets =   FEStringHelper::split(valOff);
                    assert(slots.size() > 0 && fmts.size() > 0);
                    if (slots.size() == 1)
                    {
                        input.slot      =   FEInputSlotHelper::enumFromName(slots.front().c_str());
                        input.format    =   FEFormatHelper::formatFromName(fmts.front().c_str());
                        if (!offsets.empty())
                            input.offset    =   atoi(offsets.front().c_str());
                        else
                            input.offset    =   desc.stride;
                        desc.inputs.emplace_back(input);
                        desc.stride         +=  FEFormatHelper::sizeOf(input.format);
                    }
                    else if(!slots.empty())
                    {
                        auto    baseLoc =   input.location;

                        for (size_t i = 0 ;i < slots.size(); ++ i )
                        {
                            input.slot          =   FEInputSlotHelper::enumFromName(slots[i].c_str());
                            input.format        =   FEFormatHelper::formatFromName(fmts[i].c_str());
                            input.location      =   baseLoc + uint(i);
                            /// 
                            if (!offsets.empty())
                                input.offset    =   atoi(offsets[i].c_str());
                            else
                                input.offset    =   desc.stride;
                            desc.inputs.emplace_back(input);
                            desc.stride         +=  FEFormatHelper::sizeOf(input.format);
                        }
                    }
                }
                cInfo._binds.emplace_back(desc);
            }
        }
        
        /// 
        XMLNode*    xmlPL   =   root->first_node("pipeline");
        Pipelines   result;
        for ( ; xmlPL ; xmlPL = xmlPL->next_sibling())
        {
            auto    pileline    =   createPipeline(_ctx,device,renderPass,cInfo,xmlPL,prefix,plType);
            if(pileline == nullptr)
            {
                LOG_ERR("createPipeline(%s) return nullptr!",prefix.c_str());
                continue;
            }
               
            result.emplace_back(pileline.get());
        }
        
        if (aUsingDef && atoi(aUsingDef->value()))
        {
            for (uint8_t i = 0; i < EPrimitive::PRI_MAX; ++i)
            {
                switch(i)
                {
                case PRI_POINTS        :    cInfo._inputAssemblyState._primitive    =   PRI_POINTS;         break;
                case PRI_LINES         :    cInfo._inputAssemblyState._primitive    =   PRI_LINES;          break;
                case PRI_LINE_STRIP    :    cInfo._inputAssemblyState._primitive    =   PRI_LINE_STRIP;     break;
                case PRI_TRIANGLES     :    cInfo._inputAssemblyState._primitive    =   PRI_TRIANGLES;      break;
                case PRI_TRIANGLE_STRIP:    cInfo._inputAssemblyState._primitive    =   PRI_TRIANGLE_STRIP; break;
                case PRI_TRIANGLE_FAN  :    cInfo._inputAssemblyState._primitive    =   PRI_TRIANGLE_FAN;   break;
                }

                String  name        =   prefix + "/" + nameOfEnum(cInfo._inputAssemblyState._primitive);
                /// 
                auto    itr         =   std::find_if(result.begin(),result.end(),[&](Pipeline pl)
                {
                    return  pl->name() == name;
                });
                if (itr != result.end())
                    continue;
                auto    pileline    =   device.createPipeline(plType);
                pileline->setName(name);
                if(!pileline->create(cInfo))
                {   
                    LOG_ERR("pileline->create(%s) return nullptr!",name.c_str());
                    continue;
                }
                result.emplace_back(pileline.get());
            }
        }
        return  result;
    }


    Pipeline    FEPipelineHelper::createPipeline(FEContext& ctx,FEDevice& device,RenderPass renderPass,FEPipeline::CreateInfo& cInfo,XMLNode* node,const String& prefix,const char* plType)
    {
        (void)renderPass;
        auto    xmlStates   =   node->first_node("dynamicStates");
        if (xmlStates)
        {
            for (auto xmlState = xmlStates->first_node(); xmlState ; xmlState = xmlState->next_sibling())
            {
                auto    state   =   stateFromName(xmlState->name());
                auto    itr     =   std::find(cInfo._dynamicStates.begin(),cInfo._dynamicStates.end(),state);
                if (itr == cInfo._dynamicStates.end())
                    cInfo._dynamicStates.push_back(state);
            }
        }
        auto    xmlIAState  =   node->first_node("inputAssemblyState");
        auto    aName       =   node->first_attribute("name");
        if (xmlIAState)
        {
            for (auto xmlState = xmlIAState->first_node(); xmlState ; xmlState = xmlState->next_sibling())
            {
                if (_stricmp(xmlState->name(),"primitive") == 0)
                {
                    cInfo._inputAssemblyState._primitive    =   primitiveFromName(xmlState->value());
                }
            }
        }
        String  name;
        if (aName != nullptr)
            name    =   aName->value();
        else if(xmlIAState)
            name    =   prefix + "/" + nameOfEnum(cInfo._inputAssemblyState._primitive);
        
        Pipeline    pileline    =   device.createPipeline(plType);
        if (pileline == nullptr)
        {
            ctx.log().error("createPipeline(%s) device.createPipeline(%s) return nullptr!",name.c_str(),plType);
            return  nullptr;
        }
        pileline->setName(name);
        if(!pileline->create(cInfo))
            return  nullptr;
        else
            return  pileline.get();
       
       
    }

    Shader      FEPipelineHelper::createShader(FEContext& ctx,FEDevice& device,const String& path)
    {
        Buffer  vsData;
        /// 当后端支持 WGSL(WebGPU)时,优先查找 .wgls 文件作为 shader 源
        /// SPIR-V 在 cache/ 目录,文件名为 shader.<stage>.spv (如 shader.comp.spv)
        /// WGSL 查找顺序:
        ///   1) shader.<stage>.wgls  (stage-preserved,vert/frag 各自独立文件,单 main 入口)
        ///   2) shader.wgls          (stripped,vert+frag+compute 合一,用于 computeCull 等)
        if (device.supportWGSLShaders())
        {
            /// 构造 source/ 路径(cache/ -> source/)。
            /// 注意 /cache/(7字符) -> /source/(8字符) 长度变化,
            /// 因此 spvPos 必须在 sourcePath 上重新计算,否则会偏移 1 字符,
            /// 导致 stage-preserved 路径被破坏(如 "shader.frag.wgls" 变成 "shader.fra.wglsv")。
            String  sourcePath =   path;
            auto    cachePos    =   sourcePath.find("/cache/");
            if (cachePos != String::npos)
                sourcePath.replace(cachePos,7,"/source/");

            auto    spvPos  =   sourcePath.rfind(".spv");
            if (spvPos != String::npos)
            {
                /// 1) 先尝试 stage-preserved: shader.<stage>.spv -> shader.<stage>.wgls
                ///    将末尾的 ".spv" 直接替换为 ".wgls",保留中间的 stage 后缀
                String  stageWgls =   sourcePath;
                stageWgls.replace(spvPos,4,".wgls");
                ctx.log().infor("createShader: try stage-preserved WGSL: %s (spvPos=%zu cachePos=%d)",
                                stageWgls.c_str(),spvPos,(int)cachePos);
                vsData  =   FEBuffer::loadFile(ctx,stageWgls.c_str());
                if (vsData)
                    ctx.log().infor("createShader: prefer WGSL %s over SPIR-V",stageWgls.c_str());

                /// 2) 回退到 stripped: shader.<stage>.spv -> shader.wgls
                ///    (用于 vert+frag 合一的旧式 .wgls 文件)
                if (vsData == nullptr)
                {
                    String  strippedWgls =   sourcePath;
                    auto    fnameStart =   strippedWgls.rfind('/');
                    if (fnameStart == String::npos)
                        fnameStart = 0;
                    else
                        fnameStart += 1;
                    auto    dotPos   =   strippedWgls.find('.',fnameStart);
                    if (dotPos != String::npos)
                        strippedWgls.resize(dotPos + 1);
                    else
                        strippedWgls.resize(spvPos);
                    strippedWgls.append("wgls");
                    vsData  =   FEBuffer::loadFile(ctx,strippedWgls.c_str());
                    if (vsData)
                        ctx.log().infor("createShader: prefer WGSL %s over SPIR-V",strippedWgls.c_str());
                }
            }
        }
        if (vsData == nullptr)
            vsData  =   FEBuffer::loadFile(ctx,path.c_str());
        if (vsData == nullptr)
            return  nullptr;
        auto    shader     =   device.createShader();
        if(shader->create({vsData}))
            return  shader;
        else
            return  nullptr;

    }
}
