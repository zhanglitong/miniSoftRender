
#include    "../inc/material/FEMaterial.hpp"
#include    "../inc/graphic/FEGPipeline.h"
#include    "../inc/graphic/FEDevice.h"
#include    "../inc/graphic/FEConstUuid.h"

namespace   FE
{
    bool    FEMaterial::setup(const String& prefix)
    {
        /// 1. 典型的管线建立，正确配置了管线的情况下,一种图元结构建立一种类型的管线
        /// 2. 用户自定义配置了指定类型图元的管线，没有全部覆盖,那么使用该类型填充其他类型
        ///    避免一些错误
        Pipeline    validPL =   nullptr;
        for (uint8 i = 0; i < EPrimitive::PRI_MAX; ++i)
        {
            Pipeline    temp;
            String      plName  =   prefix + String("/") + String(nameOfEnum(EPrimitive(i)));
            if(_ctx.device().pipelines().query(plName,temp))
            {
                validPL         =   temp;
                _pipelines[i]   =   temp;
            }
        }
        if (validPL)
        {
            for (auto& var : _pipelines)
            {
                if (var == nullptr) var =   validPL;
            }
        }
        for (auto& var: _pipelines)
        {
            if (var == nullptr) continue;
            _dsets  =   _pipelines[0]->createDSets();
            break;
        }
        if (_dsets.empty())
            return  false;
        return  !_dsets.empty();
    }
    void    FEMaterial::appDynamicState(CMDPtr cmd,EPrimitive pri)
    {
        cmd->setPrimitiveTopology(pri);
        auto    pl  =   _pipelines[uint(pri)];
        if (pushConstantSize() && pushConstantData() != nullptr && pl)
        {
            cmd->pushConstants(  pl
                                ,pl->cInfo()._pushConstantStage.data()
                                ,0
                                ,pushConstantSize()
                                ,pushConstantData());
        }
    }
    DSet    FEMaterial::query(uint16 set)
    {
        for (auto& var: _dsets)
        {
            if (var->cInfo()._set != set)
                continue;
            else
                return  var;
        }
        return  nullptr;
    }

    bool    FEMaterial::bind(uint16 set,uint16 bind,const Objects& objects,const uint64s& ranges,const uint64s& offsets)
    {
        if (_dsets.empty())
            return  false;
        DSet    dset    =   query(set);
        if (dset == nullptr)
            return  false;
        else
            return  dset->setBinding(bind,objects,ranges,offsets);
    }
    void    FEMaterial::update()
    {
        for (auto& var : _dsets)
        {
            var->update();
        }
    }

    void    FEMaterial::autoAttach()
    {
        for (auto& dset: _dsets)
        {
            auto&   binds   =   dset->cInfo()._binds;
            for (auto& bind: binds)
            {
                switch(bind._type)
                {
                case DT_UNIFORM_BUFFER:
                    {
                        if (bind._name == "_camera")
                        {
                            auto    object  =   _ctx.device().queryCache(FEConstUuid::CameraUBOId);
                            dset->setBinding(bind._binding,{object});
                        }
                    }
                    break;
                case DT_STORAGE_BUFFER:
                    {
                        if (bind._name == "_lights")
                        {
                            auto    object  =   _ctx.device().queryCache(FEConstUuid::LightsId);
                            dset->setBinding(bind._binding,{object});
                        }
                    }
                    break;
                }
            }
        }
    }
}
