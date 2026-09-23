
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FENodeHelper.hpp"
#include    "../inc/animation/FEAnimation.hpp"
#include    "../inc/graphic/FEScene.h"

namespace FE
{
    FEAnimation::FEAnimation(FEContext& ctx)  
        :FEComponent(ctx,true)
    {
        _name       =   "FEAnimation";
        setEnable(true);
    }
    FEAnimation::FEAnimation(const FEAnimation& other)
        :FEComponent(other)
    {
        _clip       =   other._clip;
        _results    =   other._results;
        _offset     =   other._offset;
        _name       =   other._name;
        setEnable(other.isEnable());
    }
    
    FEAnimation::~FEAnimation() 
    {
    }

    bool    FEAnimation::isTransformProperty(int prop)
    {
        switch(prop)
        {
        case PROP_TRANSFORM_X:
        case PROP_TRANSFORM_Y:
        case PROP_TRANSFORM_Z:
        case PROP_TRANSFORM_XYZ:
        case PROP_SCALE_X:
        case PROP_SCALE_Y:
        case PROP_SCALE_Z:
        case PROP_SCALE_XYZ:
        case PROP_ROTATE_X:
        case PROP_ROTATE_Y:
        case PROP_ROTATE_Z:
        case PROP_ROTATE_XYZ:
        case PROP_QUAT:
            return  true;
        default:
            return  false;
        }
    }

    void    FEAnimation::setTransformProperty(int prop,const KFValue& value)
    {
        switch(prop)
        {
        case PROP_TRANSFORM_X:
            {
                real3   pos =   _transform.position();
                pos.x       =   std::get<real>(value);
                _transform.setPosition(pos);
            }
            break;
        case PROP_TRANSFORM_Y:
            {
                real3   pos =   _transform.position();
                pos.y       =   std::get<real>(value);
                _transform.setPosition(pos);
            }
            break;
        case PROP_TRANSFORM_Z:
            {
                real3   pos =   _transform.position();
                pos.z       =   std::get<real>(value);
                _transform.setPosition(pos);
            }
            break;
        case PROP_TRANSFORM_XYZ:
            _transform.setPosition(std::get<real3>(value));
            break;
        case PROP_SCALE_X:
            {
                float3  scl =   _transform.scale();
                scl.x       =   (float)std::get<real>(value);
                _transform.setScale(scl);
            }
            break;
        case PROP_SCALE_Y:
            {
                float3  scl =   _transform.scale();
                scl.y       =   (float)std::get<real>(value);
                _transform.setScale(scl);
            }
            break;
        case PROP_SCALE_Z:
            {
                float3  scl =   _transform.scale();
                scl.z       =   (float)std::get<real>(value);
                _transform.setScale(scl);
            }
            break;
        case PROP_SCALE_XYZ:
            _transform.setScale(float3(std::get<real3>(value)));
            break;
        case PROP_ROTATE_X:
            {
                float3  euler   =   _transform.euler();
                euler.x         =   (float)std::get<real>(value);
                _transform.setEuler(euler);
            }
            break;
        case PROP_ROTATE_Y:
            {
                float3  euler   =   _transform.euler();
                euler.y         =   (float)std::get<real>(value);
                _transform.setEuler(euler);
            }
            break;
        case PROP_ROTATE_Z:
            {
                float3  euler   =   _transform.euler();
                euler.z         =   (float)std::get<real>(value);
                _transform.setEuler(euler);
            }
            break;
        case PROP_ROTATE_XYZ:
            _transform.setEuler(float3(std::get<real3>(value)));
            break;
        case PROP_QUAT:
            _transform.setRotation(std::get<quatf>(value));
            break;
        default:
            break;
        }
    }

    bool    FEAnimation::applyTrackResult(int prop,const KFValue& value)
    {
        if (isTransformProperty(prop))
        {
            setTransformProperty(prop,value);
            return  true;
        }
        else
        {
            _owner->setProperty(prop,value);
            return  false;
        }
    }

    KFValue FEAnimation::getProperty(int prop) const
    {
        if (isTransformProperty(prop))
        {
            switch(prop)
            {
            case PROP_TRANSFORM_X:
                return  _transform.position().x;
            case PROP_TRANSFORM_Y:
                return  _transform.position().y;
            case PROP_TRANSFORM_Z:
                return  _transform.position().z;
            case PROP_TRANSFORM_XYZ:
                return  _transform.position();
            case PROP_SCALE_X:
                return  (real)_transform.scale().x;
            case PROP_SCALE_Y:
                return  (real)_transform.scale().y;
            case PROP_SCALE_Z:
                return  (real)_transform.scale().z;
            case PROP_SCALE_XYZ:
                return  real3(_transform.scale());
            case PROP_ROTATE_X:
                return  (real)_transform.euler().x;
            case PROP_ROTATE_Y:
                return  (real)_transform.euler().y;
            case PROP_ROTATE_Z:
                return  (real)_transform.euler().z;
            case PROP_ROTATE_XYZ:
                return  real3(_transform.euler());
            case PROP_QUAT:
                return  _transform.rotation();
            default:
                break;
            }
        }
        return  _owner->getProperty(prop);
    }

    size_t  FEAnimation::objectCount() const 
    {
        if (_clip)
            return  _clip->tracks().size();
        else
            return  0;
    }
    bool    FEAnimation::traverseObject(const ObjectVisitor& fun,uint depth,bool recur) const
    {
        UNUSED(depth,recur);
        if (!fun || _clip == nullptr)
            return  false;
        FETrvsCtx trvsCtx(fun);
        for (auto&  var : _clip->tracks())
        {
            if(!fun(*var,*this,trvsCtx,depth))
                return  false;
            if(recur && var->objectCount()!= 0)
                var->traverseObject(fun,depth+1,recur);
        }
        return  true;
    }

    bool    FEAnimation::update(const real& clipTime)
    {
        if (!isEnable())
            return  false;
        if (!isValid())
            return  false;
        auto    rng =   range();
        if (clipTime <= rng.x && _clipTime  == rng.x)
            return  false;
        else if(clipTime >= rng.y && _clipTime == rng.y)
            return  false;

        _clipTime   =   std::clamp(clipTime,rng.x,rng.y);
        _clip->update(_clipTime - _offset,_results);
        _owner->beginSetProp();

        bool    bModify     =   false;
        bool    bTransMod   =   false;

        for (auto& var: _results)
        {
            if (!var._valid)
                continue;
            if (applyTrackResult(var._prop,var._value))
                bTransMod   =   true;
            else
                bModify     =   true;
        }

        /// 变换数据写入动画自身的 _transform,由节点 updateTransform -> appTransform 应用到节点
        /// 标记所有者需要更新变换,确保 update() 触发 updateTransform() 调用组件的 appTransform
        if (bTransMod)
        {
            _owner->flags().addFlag(FENode::FLAG_PROP_TRANS  |
                                    FENode::FLAG_PROP_SCALE  |
                                    FENode::FLAG_PROP_ROT);
            bModify =   true;
        }

        /// 把需要更新的对象加入到更新列表,引擎在所有组件更新完成后统一执行 update + fireChanged
        /// 不在 endSetProp 中立即触发,避免与更新列表重复执行 update/fireChanged
        if (bModify)
            _ctx.scene()->updateList().addObject(_owner);

        _owner->endSetProp(false);
        return  bModify;
    }

    size_t  FEAnimation::queryDepends(ObjectUSet& uSet) const
    {
        auto        count   =   uSet.size();
        AnimClip    clip    =   _clip;
        if (clip)
            uSet.emplace(Object(clip.get()));
        return  uSet.size() - count;
    }
    
    void    FEAnimation::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const
    {
        UNUSED(writer,chunk,version,ctx);
        FEChunkBits bits(chunk._flags);

        bits._bit0      =   _offset == 0.0 ? 0:1;
        bits._bit1      =   _clip ? 1:0;
        chunk._flags    =   bits._value;

        if (bits._bit0)
            writer.write(_offset);
        if (bits._bit1)
        {
            auto    result  =   writer.write(_clip->objectId());
            assert(result == sizeof(_clip->objectId()));
            UNUSED(result);
        }
    }
    void    FEAnimation::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) 
    {
        UNUSED(reader,chunk,version,ctx);
        FEChunkBits bits(chunk._flags);
        if (bits._bit0)
            reader.read(_offset);

        _clip   =   nullptr;
        if (bits._bit1)
        {
            FEUuid  objectId    =   {};
            auto    result      =   reader.read(objectId);
            assert(result == sizeof(objectId));
            if (result == sizeof(objectId)) 
            {
                Object  object  =   ctx.query(objectId,nullptr,FESerializeCtx::O_Query).first;
                _clip           =   object ? object->cast<FEAnimClip>() : nullptr;
            }
        }
    }

    void*   FEAnimation::queryInterface(const char* clsName)
    {
        if (strcmp(clsName,"FETransform") == 0)
            return  &_transform ;
        else
            return  nullptr;
    }
}
