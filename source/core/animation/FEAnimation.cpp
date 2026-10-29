
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FENodeHelper.hpp"
#include    "../inc/animation/FEAnimation.hpp"
#include    "../inc/animation/FEAnimationSys.hpp"
#include    "../inc/graphic/FEScene.h"

namespace FE
{
    /// <summary>
    /// 把节点加入动画系统的更新列表,由场景在动画系统 update 完成后显式刷新
    /// </summary>
    static  void    addToUpdateList(FEContext& ctx,Object object)
    {
        if (object == nullptr)
            return;
        auto    scene   =   ctx.scene();
        auto    sys     =   scene ? scene->animationSystem() : nullptr;
        if (sys != nullptr)
            sys->updateList().addObject(object);
    }

    FEAnimation::FEAnimation(FEContext& ctx)  
        :FEComponent(ctx)
    {
        _name       =   "FEAnimation";
        setEnable(true);
        _priority.setPriority(EPriority::EP_Fist);
        _priority.setOrder(0);
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

    void    FEAnimation::beginSetProp()
    {
        /// 采样基准:以节点当前本地变换为底,未被轨道覆盖的通道保持节点当前值
        if (_owner)
        {
            auto    node    =   _owner->cast<FENode>();
            if (node != nullptr)
            {
                /// 首次覆盖前记录节点原始本地变换,动画停止贡献时据此回退
                if (!_hasOriginal)
                {
                    _original       =   node->transform();
                    _hasOriginal    =   true;
                }
                _transform.setPosition(node->localTranslation());
                _transform.setScale(float3(node->localScaling()));
                _transform.setRotation(node->localRotation());
                _bSampled       =   true;
            }
            _owner->beginSetProp();
        }
    }
    bool    FEAnimation::setProperty(int prop,const KFValue& value)
    {
        if (!isTransformProperty(prop))
        {
            if (_owner)
                return  _owner->setProperty(prop,value);
            else
                return  false;
        }
        setTransformProperty(prop,value);
        return  true;
    }

    KFValue FEAnimation::getProperty(int prop) const
    {
        if (!isTransformProperty(prop))
        {
            if (_owner)
                return  _owner->getProperty(prop);
            else
                return  {};
        }
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
            return  {};
        }
    }

    void    FEAnimation::endSetProp(const real& tm,bool bModify) 
    {
        if (_owner)
        {
            _owner->flags().addFlag(FENode::FLAG_PROP_TRANS| FENode::FLAG_PROP_SCALE| FENode::FLAG_PROP_ROT);
            _owner->endSetProp(tm,false);
        }
        /// 把需要更新的对象加入到更新列表,引擎在所有组件更新完成后统一执行 update + fireChanged
        /// 不在 endSetProp 中立即触发,避免与更新列表重复执行 update/fireChanged
        if (bModify)
            addToUpdateList(_ctx,_owner);

        LOG_DBG("_translation = %lf,%lf,%lf",_transform.position().x,_transform.position().y,_transform.position().z);
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
        {
            markApplied(false);
            return  false;
        }
        if (!isValid())
        {
            markApplied(false);
            return  false;
        }
        auto    rng =   range();
        /// 只有一个关键帧，update 流程需要正常走
        /// 典型的操作: 编辑时间线，添加了一个关键帧后，在添加第二个关键帧
        if (rng.x >= 0 && rng.x == rng.y)
        {
        }
        else if (clipTime <= rng.x && _clipTime  == rng.x)
        {
            flags().addFlag(OutOfRangeBit);
            markApplied(false);
            return  false;
        } 
        else if(clipTime >= rng.y && _clipTime == rng.y)
        {
            flags().addFlag(OutOfRangeBit);
            markApplied(false);
            return  false;
        }
        flags().removeFlag(OutOfRangeBit);
        _clipTime   =   std::clamp(clipTime,rng.x,rng.y);
        _clip->update(_clipTime - _offset,_results);
        /// 画对象应用对象 
        _owner->beginSetProp();
        /// 动画对象
        this->beginSetProp();
        bool    bModify     =   false;
        
        for (auto& var: _results)
        {
            if (!var._valid)
                continue;
            if (this->setProperty(var._prop,var._value))
                bModify     =   true;
        }
        this->endSetProp(clipTime,bModify);
        markApplied(true);
        /// 采样结果直接写入节点本地变换,节点不再反向查询动画组件
        applyTransform();
        return  bModify;
    }

    void    FEAnimation::markApplied(bool applied)
    {
        if (applied)
        {
            flags().addFlag(AppliedBit);
            return;
        }
        /// 从未覆盖过节点时无需恢复
        if (!flags().hasFlag(AppliedBit) && !_hasOriginal)
            return;
        restoreLocal();
    }

    void    FEAnimation::applyTransform()
    {
        auto    node    =   _owner ? _owner->cast<FENode>() : nullptr;
        if (node == nullptr)
            return;
        auto*   tf      =   getTransform();
        /// 未产生过采样时保持节点本地变换不变,避免默认变换覆盖节点
        if (tf == nullptr || !_bSampled)
        {
            restoreOriginal();
            return;
        }
        node->setLocalTranslation(tf->position());
        node->setLocalScaling(real3(tf->scale()));
        node->setLocalRotation(quatr(tf->rotation()));
        addToUpdateList(_ctx,_owner);
    }

    void    FEAnimation::restoreIfInactive()
    {
        /// 仍有效时保持当前采样结果,等下一次采样覆盖
        if (getTransform() != nullptr)
            return;
        markApplied(false);
    }

    void    FEAnimation::restoreLocal()
    {
        flags().removeFlag(AppliedBit);
        restoreOriginal();
        if (_owner)
        {
            _owner->flags().addFlag(FENode::FLAG_PROP_TRANS | FENode::FLAG_PROP_SCALE | FENode::FLAG_PROP_ROT);
            addToUpdateList(_ctx,_owner);
        }
    }

    void    FEAnimation::restoreOriginal()
    {
        if (!_hasOriginal)
            return;
        _hasOriginal    =   false;
        auto    node    =   _owner ? _owner->cast<FENode>() : nullptr;
        if (node == nullptr)
            return;
        /// 只恢复动画真正驱动的通道,未驱动通道保持节点当前值(含动画运行期间的编辑)
        bool    bPos    =   false;
        bool    bScl    =   false;
        bool    bRot    =   false;
        if (_clip != nullptr)
        {
            for (auto& track : _clip->tracks())
            {
                if (!track)
                    continue;
                switch(track->propertyIndex())
                {
                case PROP_TRANSFORM_X:
                case PROP_TRANSFORM_Y:
                case PROP_TRANSFORM_Z:
                case PROP_TRANSFORM_XYZ:
                    bPos    =   true;
                    break;
                case PROP_SCALE_X:
                case PROP_SCALE_Y:
                case PROP_SCALE_Z:
                case PROP_SCALE_XYZ:
                    bScl    =   true;
                    break;
                case PROP_ROTATE_X:
                case PROP_ROTATE_Y:
                case PROP_ROTATE_Z:
                case PROP_ROTATE_XYZ:
                case PROP_QUAT:
                    bRot    =   true;
                    break;
                default:
                    break;
                }
            }
        }
        if (bPos)
            node->setLocalTranslation(_original.position());
        if (bScl)
            node->setLocalScaling(real3(_original.scale()));
        if (bRot)
            node->setLocalRotation(quatr(_original.rotation()));
    }

    FETransform*    FEAnimation::getTransform()
    {
        if (!isEnable() || !isValid() || outOfRange())
            return  nullptr;
        return  &_transform;
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
        /// 播放器名仅在非默认值时写入;旧文件该位恒为 0,读取时回落 "default"
        bits._bit2      =   _playbackName != "default" ? 1:0;
        chunk._flags    =   bits._value;

        if (bits._bit0)
            writer.write(_offset);
        if (bits._bit1)
        {
            auto    result  =   writer.write(_clip->objectId());
            assert(result == sizeof(_clip->objectId()));
            UNUSED(result);
        }
        if (bits._bit2)
        {
            auto    result  =   writer.write(_playbackName);
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
        /// 播放器名:默认值不落盘,旧文件无 _bit2 时保持在构造时的默认值
        _playbackName   =   "default";
        if (bits._bit2)
            reader.read(_playbackName);
    }
}
