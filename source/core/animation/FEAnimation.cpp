
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FENodeHelper.hpp"
#include    "../inc/animation/FEAnimation.hpp"
#include    "../inc/animation/FEAnimationSys.hpp"
#include    "../inc/FEEulerObject.hpp"
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

    bool    FEAnimation::setTransformProperty(int prop,const KFValue& value)
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
            /// 非变换属性:动画本地副本不保存,由调用方决定如何处理
            return  false;
        }
        return  true;
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
        /// 关键帧写入:先修改 owner(节点),变换数据同时保存到动画本地副本,
        /// 保证编辑结果既能立即反映到节点,又能被动画在驱动阶段正确回放;
        /// 非变换属性动画侧不保留,返回 owner 的处理结果
        bool    bOwner      =   _owner ? _owner->setProperty(prop,value) : false;
        bool    bTransform  =   setTransformProperty(prop,value);
        /// 变换属性同步写入 clip:编辑发生在动画激活(播放/时间线定位)时,
        /// 驱动阶段会按关键帧重采样覆盖节点,若只写本地副本,拖拽结果下一帧就被旧关键帧撤销
        if (bTransform && _clip != nullptr)
            writeTransformKey(prop);
        return  bTransform  ||  bOwner;
    }

    void    FEAnimation::writeTransformKey(int prop)
    {
        enum    Family  {  F_NONE,F_POS,F_SCL,F_ROT  };
        static  auto    familyOf    =   [](int idx) -> int
        {
            switch (idx)
            {
            case PROP_TRANSFORM_X:
            case PROP_TRANSFORM_Y:
            case PROP_TRANSFORM_Z:
            case PROP_TRANSFORM_XYZ:    return  F_POS;
            case PROP_SCALE_X:
            case PROP_SCALE_Y:
            case PROP_SCALE_Z:
            case PROP_SCALE_XYZ:        return  F_SCL;
            case PROP_ROTATE_X:
            case PROP_ROTATE_Y:
            case PROP_ROTATE_Z:
            case PROP_ROTATE_XYZ:
            case PROP_QUAT:             return  F_ROT;
            default:                    return  F_NONE;
            }
        };
        const   int     family  =   familyOf(prop);
        if (family == F_NONE)
            return;
        /// 同一通道可能有不同粒度的轨道(位置 XYZ 或 X/Y/Z;旋转欧拉角或四元数),
        /// 必须按轨道自身属性换算成匹配的值类型写入,类型不匹配时轨道会拒绝写关键帧,
        /// 采样随即把编辑结果覆盖回旧值(旋转抓手此前就因此完全失效)
        real3   eulerDeg    =   real3(0);
        if (family == F_ROT)
        {
            /// quatToEuler 返回的就是角度(度),与 FETransform::setEuler 的输入单位一致,
            /// 不能再做一次 RAD2DEG(会把角度放大 57 倍,采样回放出错误姿态)
            eulerDeg    =   quatToEuler(quatr(_transform.rotation()));
        }
        const   real3   pos     =   _transform.position();
        const   real3   scl     =   real3(_transform.scale());
        const   real    tm      =   _clipTime - _offset;
        /// 拷贝一次轨道列表:只读引用上的元素是 const,无法调用轨道自身的写入接口
        KeyFrameTracks  trackList   =   _clip->tracks();
        for (auto& var : trackList)
        {
            if (var == nullptr)
                continue;
            const   int     idx     =   var->propertyIndex();
            if (familyOf(idx) != family)
                continue;
            KFValue val;
            switch (idx)
            {
            case PROP_TRANSFORM_X:      val = pos.x;        break;
            case PROP_TRANSFORM_Y:      val = pos.y;        break;
            case PROP_TRANSFORM_Z:      val = pos.z;        break;
            case PROP_TRANSFORM_XYZ:    val = pos;          break;
            case PROP_SCALE_X:          val = scl.x;        break;
            case PROP_SCALE_Y:          val = scl.y;        break;
            case PROP_SCALE_Z:          val = scl.z;        break;
            case PROP_SCALE_XYZ:        val = scl;          break;
            case PROP_ROTATE_X:         val = eulerDeg.x;   break;
            case PROP_ROTATE_Y:         val = eulerDeg.y;   break;
            case PROP_ROTATE_Z:         val = eulerDeg.z;   break;
            case PROP_ROTATE_XYZ:       val = eulerDeg;     break;
            case PROP_QUAT:
                {
                    /// 四元数轨道双精度/单精度两种,先按双精度写,失败再退化为单精度
                    const   KFValue  qd  =   quatr(_transform.rotation());
                    const   KFValue  qf  =   _transform.rotation();
                    if (!var->updateKeyFrame(tm,qd) && !var->addKeyFrame(tm,qd)
                        && !var->updateKeyFrame(tm,qf))
                        var->addKeyFrame(tm,qf);
                    continue;
                }
            default:
                continue;
            }
            if (!var->updateKeyFrame(tm,val))
                var->addKeyFrame(tm,val);
        }
    }

    KFValue FEAnimation::getProperty(int prop) const
    {
        /// 编辑过程一律从 owner 读取数据;动画本地副本只用于采样与回放
        if (_owner)
            return  _owner->getProperty(prop);
        return  {};
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
        /// 有轨道但没有任何关键帧时 range 是 (-1,-1);只要有一条空轨道参与合并,
        /// 下界就会被拉到 -1。这种"没有有效范围"的情况不做裁剪,按传入时间采样
        const   real2   clipRange   =   _clip != nullptr ? _clip->range() : real2(-1,-1);
        const   bool    hasRange    =   clipRange.y >= real(0);
        if (hasRange)
            rng     =   real2((std::max)(clipRange.x,real(0)),clipRange.y) + real2(_offset,_offset);
        /// 时间严格落在范围之外才回退到动画覆盖前的姿态;
        /// 恰好等于首/末关键帧时间时必须照常采样 —— 否则时间线停在帧 0 时,
        /// 每次拖拽都由这里把节点恢复成原始变换,表现为"有动画的节点无法移动"
        if (hasRange && (clipTime < rng.x || clipTime > rng.y))
        {
            _clipTime   =   (clipTime < rng.x) ? rng.x : rng.y;
            flags().addFlag(OutOfRangeBit);
            markApplied(false);
            return  false;
        }
        flags().removeFlag(OutOfRangeBit);
        _clipTime   =   hasRange ? std::clamp(clipTime,rng.x,rng.y) : clipTime;
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
            /// 驱动过程:变换属性只写入动画本地副本,由 applyTransform 统一驱动 owner(节点);
            /// 其它属性直接作用于 owner
            if (isTransformProperty(var._prop))
            {
                setTransformProperty(var._prop,var._value);
                bModify     =   true;
            }
            /// 非变换属性直接写 owner;不能走 this->setProperty,否则采样结果会被当成编辑写回 clip
            else if (_owner != nullptr && _owner->setProperty(var._prop,var._value))
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
        /// 旋转中心点(本地空间):几何体绕该点旋转时,节点实际本地平移需补偿
        /// T(position)*T(pivot)*R*T(-pivot) 展开后平移为 position + (pivot - R*pivot)
        real3   pos     =   tf->position();
        quatr   rot     =   quatr(tf->rotation());
        real3   pivot   =   _transform.rotationCenter();
        if  (pivot != real3(0))
            pos     =   pos + pivot - rot * pivot;
        node->setLocalTranslation(pos);
        node->setLocalScaling(real3(tf->scale()));
        node->setLocalRotation(rot);
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
