#include    "SWShader.h"
#include    "SWDevice.h"
#include    "SWDSetLayout.h"

namespace   FE
{
    SWShader::~SWShader()
    {
    }

    bool    SWShader::create(const FEShader::CreateInfo& info)
    {
        _cInfo  =   info;
        return  true;
    }

    DSetLayout  SWShader::createLayoutFromReflect()
    {
        if (_reflectData._bindings.empty())
            return nullptr;

        FEDSetLayout::CreateInfo info;
        info._bindings = _reflectData._bindings;

        auto layout = new SWDSetLayout(_ctx);
        if (!layout->create(info))
        {
            delete layout;
            return nullptr;
        }
        return layout;
    }
}
