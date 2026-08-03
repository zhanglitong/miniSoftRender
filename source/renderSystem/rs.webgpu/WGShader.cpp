#include    "WGShader.h"
#include    "WGDevice.h"
#include    "WGDSetLayout.h"
#include    <algorithm>
#include    <cctype>
#include    <string>

namespace   FE
{
    WGShader::~WGShader()
    {
        if (_native)
        {
            wgpuShaderModuleRelease(_native);
            _native =   nullptr;
        }
    }

    bool WGShader::create(const CreateInfo& info)
    {
        _cInfo =   info;
        auto& wgDevice = const_cast<WGDevice&>(static_cast<const WGDevice&>(_ctx.device()));

        WGPUShaderModuleDescriptorSpirV spirvDesc = {};
        spirvDesc.label = { nullptr,0 };

        const auto* source =   reinterpret_cast<const uint32_t*>(info._buffer->data());
        uint32_t sourceSize =   static_cast<uint32_t>(info._buffer->length() / sizeof(uint32_t));

        if (source && sourceSize > 0)
        {
            spirvDesc.source = source;
            spirvDesc.sourceSize = sourceSize;

            _native =   wgpuDeviceCreateShaderModuleSpirV(wgDevice.device(),&spirvDesc);
        }

        if (_native)
        {
            reflectShader(info);
            return true;
        }

        LOG_ERR("WGShader.create failed");
        return false;
    }

    void WGShader::reflectShader(const CreateInfo& info)
    {
        _reflectData._stage =   info._shaderType;

        if (info._shaderType == ST_VERTEX_BIT)
        {
            _reflectData._stageFlags =   ST_VERTEX_BIT;
        }
        else if (info._shaderType == ST_FRAGMENT_BIT)
        {
            _reflectData._stageFlags =   ST_FRAGMENT_BIT;
        }

        if (info._buffer->data() && info._buffer->length() > 0)
        {
            const char* source =   (const char*)info._buffer->data();
            size_t sourceSize =   info._buffer->length();

            std::string sourceStr(source,sourceSize);
            size_t pos =   0;

            while ((pos = sourceStr.find("@group",pos)) != std::string::npos)
            {
                size_t groupStart =   pos + 7;
                size_t groupEnd =   sourceStr.find("@",groupStart);
                if (groupEnd == std::string::npos) break;

                std::string groupSection =   sourceStr.substr(groupStart,groupEnd - groupStart);

                size_t bindingPos =   groupSection.find("@binding");
                if (bindingPos != std::string::npos)
                {
                    size_t numStart =   bindingPos + 8;
                    while (numStart < groupSection.size() && groupSection[numStart] == ' ')
                        numStart++;

                    size_t numEnd =   numStart;
                    while (numEnd < groupSection.size() && isdigit(groupSection[numEnd]))
                        numEnd++;

                    if (numEnd > numStart)
                    {
                        std::string bindingStr =   groupSection.substr(numStart,numEnd - numStart);
                        uint32_t bindingNum =   (uint32_t)std::stoul(bindingStr);

                        ReflectBinding reflectBinding;
                        reflectBinding._binding =   bindingNum;
                        reflectBinding._descriptorType =   DT_UNIFORM_BUFFER;
                        reflectBinding._stageFlags =   _reflectData._stageFlags;
                        reflectBinding._name =   "binding_" + std::to_string(bindingNum);

                        _reflectData._bindings.push_back(reflectBinding);
                    }
                }

                pos =   groupEnd;
            }

            std::sort(_reflectData._bindings.begin(),_reflectData._bindings.end(),
                [](const ReflectBinding& a,const ReflectBinding& b) {
                    return a._binding < b._binding;
                });
        }
    }

    DSetLayout WGShader::createLayoutFromReflect()
    {
        FEDSetLayout::CreateInfo info;
        for (const auto& binding : _reflectData._bindings)
        {
            FEDSetBinding layoutBinding;
            layoutBinding._binding =   binding._binding;
            layoutBinding._descriptorType =   binding._descriptorType;
            layoutBinding._stageFlags =   _reflectData._stageFlags;
            layoutBinding._name =   binding._name;
            info._bindings.push_back(layoutBinding);
        }

        auto layout = new WGDSetLayout(_ctx);
        if (!layout->create(info))
        {
            delete layout;
            return nullptr;
        }
        return layout;
    }
}