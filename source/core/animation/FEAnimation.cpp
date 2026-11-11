
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

    static  bool    applyTransformProperty(FETransform& tf,int prop,const KFValue& value)
    {
        switch(prop)
        {
        case PROP_TRANSFORM_X:
            {
                real3   pos =   tf.position();
                pos.x       =   std::get<real>(value);
                tf.setPosition(pos);
            }
            break;
        case PROP_TRANSFORM_Y:
            {
                real3   pos =   tf.position();
                pos.y       =   std::get<real>(value);
                tf.setPosition(pos);
            }
            break;
        case PROP_TRANSFORM_Z:
            {
                real3   pos =   tf.position();
                pos.z       =   std::get<real>(value);
                tf.setPosition(pos);
            }
            break;
        case PROP_TRANSFORM_XYZ:
            tf.setPosition(std::get<real3>(value));
            break;
        case PROP_SCALE_X:
            {
                float3  scl =   tf.scale();
                scl.x       =   (float)std::get<real>(value);
                tf.setScale(scl);
            }
            break;
        case PROP_SCALE_Y:
            {
                float3  scl =   tf.scale();
                scl.y       =   (float)std::get<real>(value);
                tf.setScale(scl);
            }
            break;
        case PROP_SCALE_Z:
            {
                float3  scl =   tf.scale();
                scl.z       =   (float)std::get<real>(value);
                tf.setScale(scl);
            }
            break;
        case PROP_SCALE_XYZ:
            tf.setScale(float3(std::get<real3>(value)));
            break;
        case PROP_ROTATE_X:
            {
                float3  euler   =   tf.euler();
                euler.x         =   (float)std::get<real>(value);
                tf.setEuler(euler);
            }
            break;
        case PROP_ROTATE_Y:
            {
                float3  euler   =   tf.euler();
                euler.y         =   (float)std::get<real>(value);
                tf.setEuler(euler);
            }
            break;
        case PROP_ROTATE_Z:
            {
                float3  euler   =   tf.euler();
                euler.z         =   (float)std::get<real>(value);
                tf.setEuler(euler);
            }
            break;
        case PROP_ROTATE_XYZ:
            tf.setEuler(float3(std::get<real3>(value)));
            break;
        case PROP_QUAT:
            /// 四元数轨道的值以 quatr 存储,采样结果也是 quatr;
            /// 直接取 quatf 会抛 bad_variant_access,这里按实际类型兼容写入
            if  (std::holds_alternative<quatr>(value))
                tf.setRotation(quatf(std::get<quatr>(value)));
            else if (std::holds_alternative<quatf>(value))
                tf.setRotation(std::get<quatf>(value));
            break;
        default:
            /// 非变换属性:不进入本次采样副本,由调用方决定如何处理
            return  false;
        }
        return  true;
    }

    /// <summary>
    /// 以 owner 当前属性值为底构造本次采样变换:未被轨道覆盖的通道保持当前值
    /// </summary>
    static  void    loadTransformFromOwner(Object owner,FETransform& tf)
    {
        const   auto    pos =   owner->getProperty(PROP_TRANSFORM_XYZ);
        if  (std::holds_alternative<real3>(pos))
            tf.setPosition(std::get<real3>(pos));
        const   auto    scl =   owner->getProperty(PROP_SCALE_XYZ);
        if  (std::holds_alternative<real3>(scl))
            tf.setScale(float3(std::get<real3>(scl)));
        else if (std::holds_alternative<float3>(scl))
            tf.setScale(std::get<float3>(scl));
        const   auto    rot =   owner->getProperty(PROP_QUAT);
        if  (std::holds_alternative<quatf>(rot))
            tf.setRotation(std::get<quatf>(rot));
        /// 采样基准同时带上累计欧拉角账本:没有被旋转轨道覆盖时,
        /// 回放结束不会把已经累计的圈数丢掉
        const   auto    euler   =   owner->getProperty(PROP_ROTATE_XYZ);
        if  (std::holds_alternative<real3>(euler))
            tf.adoptEuler(float3(std::get<real3>(euler)),true);
    }

    void    FEAnimation::beginSetProp()
    {
        if (_owner)
            _owner->beginSetProp();
    }
    bool    FEAnimation::setProperty(int prop,const KFValue& value)
    {
        /// 编辑写值:直接写 owner(任意 FEObject);
        /// 不写关键帧 —— 打帧是独立动作(时间线的“打帧”),编辑不应污染当前帧
        return  _owner ? _owner->setProperty(prop,value) : false;
    }

    KFValue FEAnimation::getProperty(int prop) const
    {
        /// 编辑过程一律从 owner 读取数据
        if (_owner)
            return  _owner->getProperty(prop);
        return  {};
    }

    void    FEAnimation::endSetProp(const real& tm,bool bModify) 
    {
        if (_owner)
            _owner->endSetProp(tm,false);
        /// 把需要更新的对象加入到更新列表,引擎在所有组件更新完成后统一执行 update + fireChanged
        /// 不在 endSetProp 中立即触发,避免与更新列表重复执行 update/fireChanged
        if (bModify)
            addToUpdateList(_ctx,_owner);
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
        if (!isEnable() || !isValid())
            return  false;

        /// 有轨道但没有任何关键帧时 range 是 (-1,-1);只要有一条空轨道参与合并,
        /// 下界就会被拉到 -1。这种"没有有效范围"的情况不做裁剪,按传入时间采样
        const   real2   clipRange   =   _clip->range();
        const   bool    hasRange    =   clipRange.y >= real(0);
        /// 时间落在范围之外时不采样、也不回退:
        /// 保持节点当前姿态(编辑器暂停/编辑依赖该语义),仅打标记供 UI 提示
        if (hasRange)
        {
            const   real2   rng =   real2((std::max)(clipRange.x,real(0)),clipRange.y) + real2(_offset,_offset);
            if (clipTime < rng.x || clipTime > rng.y)
            {
                flags().addFlag(OutOfRangeBit);
                return  false;
            }
            _clipTime   =   std::clamp(clipTime,rng.x,rng.y);
        }
        else
        {
            _clipTime   =   clipTime;
        }
        flags().removeFlag(OutOfRangeBit);
        _clip->update(_clipTime - _offset,_results);

        /// 本次采样副本:只有第一次遇到变换通道时才从 owner 取值,
        /// 只驱动颜色/显隐等非变换属性时不做多余的属性读取
        FETransform tf;
        bool        bSampled    =   false;
        bool        bModify     =   false;

        _owner->beginSetProp();
        for (auto& var: _results)
        {
            if (!var._valid)
                continue;
            /// 变换通道先累积到本次采样副本,最后统一通过属性接口驱动 owner
            if (isTransformProperty(var._prop))
            {
                if (!bSampled)
                {
                    loadTransformFromOwner(_owner,tf);
                    bSampled    =   true;
                }
                applyTransformProperty(tf,var._prop,var._value);
                bModify     =   true;
            }
            /// 非变换属性直接写 owner;不能走 this->setProperty,否则采样结果会被当成编辑写回 clip
            else if (_owner->setProperty(var._prop,var._value))
                bModify     =   true;
        }
        /// 先写回采样结果再收尾:owner 在 endSetProp 里的那次 update 就能拿到新姿态,
        /// 不必先用旧姿态算一遍、再由更新列表重算
        if (bSampled)
            applyTransform(tf);
        this->endSetProp(clipTime,bModify);
        return  bModify;
    }

    void    FEAnimation::analyzeTracks(bool& bRotation,bool& bPosition,const FEKeyFrameTrack*& segTrack) const
    {
        bRotation   =   false;
        bPosition   =   false;
        segTrack    =   nullptr;
        if (_clip == nullptr)
            return;
        /// 段边界优先用整体位置轨道(位置与旋转在同一关键帧上打帧),
        /// 没有整体位置轨道时退化为第一个位置/旋转轨道
        bool    bSegXYZ    =   false;
        for (auto& track : _clip->tracks())
        {
            if (track == nullptr || track->_times == nullptr
                || track->_times->values().empty())
                continue;
            const   int prop    =   track->propertyIndex();
            switch(prop)
            {
            case    PROP_ROTATE_X:
            case    PROP_ROTATE_Y:
            case    PROP_ROTATE_Z:
            case    PROP_ROTATE_XYZ:
            case    PROP_QUAT:
                bRotation   =   true;
                break;
            case    PROP_TRANSFORM_X:
            case    PROP_TRANSFORM_Y:
            case    PROP_TRANSFORM_Z:
            case    PROP_TRANSFORM_XYZ:
                bPosition   =   true;
                break;
            default:
                break;
            }
            if (prop == PROP_TRANSFORM_XYZ)
            {
                segTrack    =   track.get();
                bSegXYZ     =   true;
            }
            else if (!bSegXYZ && segTrack == nullptr)
            {
                switch(prop)
                {
                case    PROP_TRANSFORM_X:
                case    PROP_TRANSFORM_Y:
                case    PROP_TRANSFORM_Z:
                case    PROP_ROTATE_X:
                case    PROP_ROTATE_Y:
                case    PROP_ROTATE_Z:
                case    PROP_ROTATE_XYZ:
                case    PROP_QUAT:
                    segTrack    =   track.get();
                    break;
                default:
                    break;
                }
            }
        }
    }
    bool    FEAnimation::sampleTransformAt(real localTime,FETransform& out)
    {
        if (_clip == nullptr)
            return  false;
        TrackResults    results;
        _clip->update(localTime,results);
        bool    bAny    =   false;
        for (auto& var : results)
        {
            if (!var._valid || !isTransformProperty(var._prop))
                continue;
            applyTransformProperty(out,var._prop,var._value);
            bAny    =   true;
        }
        return  bAny;
    }

    void    FEAnimation::applyTransform(const FETransform& tf)
    {
        if (_owner == nullptr)
            return;
        real3   pos         =   tf.position();
        const   float3  scl =   tf.scale();
        /// 绕轴心回放:拖拽旋转时绕轴心产生的位置变化已烘焙进位置轨道,
        /// 位置轨道线性插值会走弦线;对象支持轴心且位置/旋转都由轨道驱动时,
        /// 用段起点的关键帧姿态与采样旋转重建位置,使回放严格绕轴心走圆弧
        if (_owner->hasRotationPivot())
        {
            bool    bRotation   =   false;
            bool    bPosition   =   false;
            const   FEKeyFrameTrack*    segTrack    =   nullptr;
            analyzeTracks(bRotation,bPosition,segTrack);
            if (bRotation && bPosition && segTrack != nullptr)
            {
                const   auto&   times   =   segTrack->_times->values();
                /// 段起点是本次旋转开始的那个关键帧(不是拖拽发生的时刻);
                /// 落在第一个关键帧之前时以第一个关键帧为起点
                const   auto    itr     =   std::upper_bound(times.begin(),times.end(),_clipTime - _offset);
                const   real    segStart=   itr == times.begin() ? times.front() : *(itr - 1);
                /// 以当前采样结果为底,段起点的采样只覆盖轨道驱动的通道
                FETransform base;
                base.setPosition(pos);
                base.setRotation(tf.rotation());
                if (sampleTransformAt(segStart,base))
                {
                    const   auto    center  =   _owner->getProperty(PROP_ROTATION_CENTER);
                    const   real3   pivot   =   std::holds_alternative<real3>(center)
                                                ? std::get<real3>(center) : real3(0.0);
                    const   quatr   rel     =   quatr(tf.rotation())
                                            *   glm::inverse(quatr(base.rotation()));
                    pos =   pivot + rel * (base.position() - pivot);
                }
            }
        }
        /// 通过属性接口驱动 owner,与其它属性一致;不再要求 owner 是 FENode
        _owner->setProperty(PROP_TRANSFORM_XYZ,pos);
        _owner->setProperty(PROP_SCALE_XYZ,real3(scl));
        /// 采样结果的累计欧拉角账本一并写回(旋转轨道是欧拉轨道时才有效;
        /// 四元数轨道没有圈数概念,发四元数属性)
        if (tf.eulerValid())
            _owner->setProperty(PROP_ROTATE_XYZ,real3(tf.euler()));
        else
            _owner->setProperty(PROP_QUAT,tf.rotation());
        addToUpdateList(_ctx,_owner);
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
}
