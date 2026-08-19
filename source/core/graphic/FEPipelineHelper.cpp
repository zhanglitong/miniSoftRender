
#include    "../inc/FEStringHelper.hpp"
#include    "../inc/FEFileInfor.hpp"
#include    "../inc/graphic/FEPipelineHelper.h"



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

        auto    aUsingDef   =   root->first_attribute("using_defult");
      
        if (xmlShaders == nullptr || xmlBinds == nullptr)
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

        /// 
        XMLNode*    xmlPL   =   root->first_node("pipeline");
        Pipelines   result;
        for ( ; xmlPL ; xmlPL = xmlPL->next_sibling())
        {
            auto    pileline    =   createPipeline(_ctx,device,renderPass,cInfo,xmlPL,prefix);
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
                auto    pileline    =   device.createGPipeline();
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


    Pipeline    FEPipelineHelper::createPipeline(FEContext& ctx,FEDevice& device,RenderPass renderPass,FEPipeline::CreateInfo& cInfo,XMLNode* node,const String& prefix)
    {
        (void)ctx;
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
        else
            name    =   prefix + "/" + nameOfEnum(cInfo._inputAssemblyState._primitive);
        auto    pileline    =   device.createGPipeline();
        pileline->setName(name);
        if(!pileline->create(cInfo))
            return  nullptr;
        else
            return  pileline.get();
    }

    Shader      FEPipelineHelper::createShader(FEContext& ctx,FEDevice& device,const String& path)
    {
        auto    vsData  =   FEBuffer::loadFile(ctx,path.c_str());
        if (vsData == nullptr)
            return  nullptr;
        auto    shader     =   device.createShader();
        if(shader->create({vsData}))
            return  shader;
        else
            return  nullptr;
        
    }
}