
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FENodeHelper.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../inc/animation/FEAction.hpp"

namespace FE
{
    FEAction::FEAction(FEContext& ctx)  
        :FEObject(ctx)
        ,FEObjectsTemplate<Animation, AnimationLess>(AnimationLessFunc)
    {
        _playMode       =   PT_Loop;
        _timeMode       =   TM_Default;
        _status         =   PS_Stoped;
        _elapseTime     =   0.0;
        _timeScale      =   1;
        _clipTime       =   0;
        _range          =   {};
    }
    FEAction::FEAction(const FEAction& other)
        :FEObject(other)
        ,FEObjectsTemplate<Animation, AnimationLess>(other)
    {
        _playMode       =   other._playMode;
        _timeMode       =   other._timeMode;
        _status         =   other._status;
        _elapseTime     =   other._elapseTime;
        _timeScale      =   other._timeScale;
        _clipTime       =   other._clipTime;
        _range          =   other._range;
    }

    FEAction::~FEAction()
    {
        _objects.clear();
        _cache.clear();
    }

    void    FEAction::update(const real& delta)
    {
        if (_status != PS_Running || _objects.empty() )
            return;
        _elapseTime +=  delta;
        real    deltaTime   =   delta * _timeScale;
        switch(_timeMode)
        {
        case TM_Default:
            _clipTime   +=  deltaTime;
            break;
        case TM_Invert:
            _clipTime   -=  deltaTime;
            break;
        }
        switch (_playMode)
        {
        case PT_Once:
            {
                if (_clipTime > _range.y)
                {
                    _clipTime   =   _range.y;
                    _status     =   PS_Stoped;
                }   
                else if (_clipTime < _range.x)
                {
                    _clipTime   =   _range.x;
                    _status     =   PS_Stoped;
                }
            }
            break;
        case PT_Loop:
            {
                if (_clipTime > _range.y)
                {
                    _clipTime   =   _range.x;
                }
                else if (_clipTime < _range.x)
                {
                    _clipTime   =   _range.y;
                }
            }
            break;
        case PT_PingPong:
            {
                if (_timeMode == TM_Default && _clipTime > _range.y)
                {
                    _clipTime   =   _range.y;
                    _timeMode   =   TM_Invert;
                }
                else if (_timeMode == TM_Invert && _clipTime < _range.x)
                {
                    _clipTime   =   _range.x;
                    _timeMode   =   TM_Default;
                }
            }
            break;
        }
        if (_cache.empty())
        {
            for (auto& var : _objects)
            {
                var->update(_clipTime);
            }
        }
        else
        {
            updateBatch(_clipTime,delta);
        }
    }

    void    FEAction::setClipTime(const real& clipTime)
    {
        _clipTime   =   std::clamp(clipTime,_range.x,_range.y);
        if (_cache.empty())
        {
            for (auto& var : _objects)
            {
                var->update(_clipTime);
            }
        }
        else
        {
            updateBatch(_clipTime,0);
        }
    }

    void    FEAction::updateBatch(const real& clipTime,const real& delta)
    {
        if (_cache.empty())
        {
            buildCache();
        }
        RealsObject             timeLine    =   nullptr;
        FEKeyFrameTrack::KFOff  kfValue     =   {};
        Object                  owner       =   nullptr;
        ObjectUSet              transOwners;    ///< 需要更新变换的所有者
        ObjectUSet              modOwners;      ///< 有任意属性修改的所有者

        for (auto& var : _cache)
        {
            /// 禁用的动画不播放
            if (var._anim && !var._anim->isEnable())
                continue;
            auto    track   =   var._track;
            auto    range   =   track->range() + real2(var._offTime);
            if (clipTime < (range.x - delta) || clipTime > range.y + delta)
                continue;
            if (track->times() != timeLine)
            {
                timeLine    =   track->times();
                kfValue     =   track->calcFrameOffset(clipTime - var._offTime);
            }
            if (owner != var._owner)
            {
                if (owner)
                    owner->endSetProp(false);
                owner   =   var._owner;
                if (owner)
                    owner->beginSetProp();
            }
            FETrackResult   result;
            result._track   =   track;
            result._prop    =   track->propertyIndex();
            result._valid   =   track->update(kfValue,result);
            if (!result._valid)
                continue;
            /// 变换属性写入动画组件自身 _transform,其他属性写入 _owner
            if (var._anim->applyTrackResult(result._prop,result._value))
                transOwners.emplace(var._owner);
            modOwners.emplace(var._owner);
        }
        if (owner)
            owner->endSetProp(false);

        /// 标记变换变更标志,由 updateList 统一触发节点 updateTransform -> appTransform
        for (Object o : transOwners)
        {
            o->flags().addFlag(FENode::FLAG_PROP_TRANS  |
                               FENode::FLAG_PROP_SCALE  |
                               FENode::FLAG_PROP_ROT);
        }
        /// 所有修改过的对象加入更新列表,引擎统一执行 update + fireChanged
        for (auto& o : modOwners)
        {
            _ctx.scene()->updateList().addObject(o);
        }
    }

    void    FEAction::buildCache()
    {
        size_t  cnt =   0;
        for (auto& var : _objects)
        {
            cnt +=  var->clip()->objects().size();
        }
        _cache.reserve(cnt);
        for (auto& var : _objects)
        {
            auto&   tracks  =   var->clip()->objects();
            auto    owner   =   var->owner();
            real    offTime =   var->offset();
            for (auto& track : tracks)
            {
                TrackObject obj =   {track,owner,offTime,var};
                _cache.emplace_back(obj);
            }
        }
        ///  Ȱ   ʱ      
        ///  ڰ   ʱ   ߶       ,    ͬһʱ   .  ͬtimeLine   󶼼     һ    , ڰ  ն       
        std::sort(_cache.begin(),_cache.end(),[](const TrackObject& l,const TrackObject& r)
        {
            if (l._offTime != r._offTime)
                return  l._offTime < r._offTime;
            else
                if(l._track->times().get() != r._track->times().get())
                    return  l._track->times().get() < r._track->times().get();
                else
                    return  l._owner.get() < r._owner.get();
        });
    }   

    void    FEAction::clearCache()
    {
        _cache.clear();
    }

    real2   FEAction::calcRange()
    {
        real2   result(-1,-1);
        if (_objects.empty())
            return  result;
        else
            result  =   _objects.front()->range();

        for (size_t i = 1 ;i < _objects.size(); ++ i)
        {
            auto    rng =   _objects[i]->range();
            result.x    =   (std::min)(result.x,rng.x);
            result.y    =   (std::max)(result.y,rng.y);
        }
        return   result;
    }


    size_t  FEAction::queryDepends(ObjectUSet& uSet) const
    {
        auto    count   =   uSet.size();
        for (auto  var: objects())
        {
            uSet.emplace(var);
        }
        return  uSet.size() - count;
    }

    void    FEAction::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const
    {
        UNUSED(chunk,version,ctx);
        writer.write(_playMode);
        writer.write(_timeMode);
        writer.write(_timeScale);
        
        uint    count   =   (uint)_objects.size();
        writer.write(count);
        for (auto& object: objects())
        {
            writer.write(object->objectId());
        }
    }
    void    FEAction::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx)
    {
        (void)reader;
        (void)chunk;
        (void)version;
        (void)ctx;

        reader.read(_playMode);
        reader.read(_timeMode);
        reader.read(_timeScale);
        uint            count   =   reader.readValue<uint>();
        Animations      objects;
        objects.reserve(count);
        for (uint i = 0; i < count; i++)
        {
            FEUuid  objectId    =   {};
            auto    result      =   reader.read(objectId);
            assert(result == sizeof(objectId));
            if (result != sizeof(objectId)) 
                continue;
            Object  temp    =   ctx.query(objectId,nullptr,FESerializeCtx::O_Query).first;
            auto    object  =   temp ? temp->cast<FEAnimation>() : nullptr;
            assert(object != nullptr);
            if (object == nullptr)
                continue;
            objects.push_back(object);
        }
        setObjects(std::move(objects));
    }
    
}
