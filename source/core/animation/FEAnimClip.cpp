
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FENodeHelper.hpp"
#include    "../inc/animation/FEAnimClip.hpp"

namespace FE
{
    FEAnimClip::FEAnimClip(FEContext& ctx)  
        :FEObject(ctx)
        ,FEObjectsTemplate<KeyFrameTrack, TrackLess>(TrackLessFunc)
    {
    }
    FEAnimClip::FEAnimClip(const FEAnimClip& other)
        :FEObject(other)
        ,FEObjectsTemplate<KeyFrameTrack, TrackLess>(other)
    {
        _objects     =   other._objects;
    }
    FEAnimClip::~FEAnimClip()
    {
    }

    bool    FEAnimClip::addKeyFrame(const real& time,const KFValue& val)
    {
        bool    bOk =   false;
        for (auto& track : _objects)
        {
            if (track->addKeyFrame(time,val))
                bOk =   true;
        }
        if (bOk)
            flags().addFlag(ClipChanged);
        return  bOk;
    }
    bool    FEAnimClip::insertKeyFrame(size_t idx,const real& time,const KFValue& val)
    {
        bool    bOk =   false;
        for (auto& track : _objects)
        {
            if (track->insertKeyFrame(idx,time,val))
                bOk =   true;
        }
        if (bOk)
            flags().addFlag(ClipChanged);
        return  bOk;
    }
    bool    FEAnimClip::removeKeyFrame(const real& time)
    {
        bool    bOk =   false;
        for (auto& track : _objects)
        {
            if (track->removeKeyFrame(time))
                bOk =   true;
        }
        if (bOk)
            flags().addFlag(ClipChanged);
        return  bOk;
    }
    bool    FEAnimClip::removeKeyFrame(size_t idx)
    {
        bool    bOk =   false;
        for (auto& track : _objects)
        {
            if (track->removeKeyFrame(idx))
                bOk =   true;
        }
        if (bOk)
            flags().addFlag(ClipChanged);
        return  bOk;
    }
    bool    FEAnimClip::updateKeyFrame(const real& time,const KFValue& val)
    {
        bool    bOk =   false;
        for (auto& track : _objects)
        {
            if (track->updateKeyFrame(time,val))
                bOk =   true;
        }
        if (bOk)
            flags().addFlag(ClipChanged);
        return  bOk;
    }
    bool    FEAnimClip::updateKeyFrame(size_t idx,const KFValue& val)
    {
        bool    bOk =   false;
        for (auto& track : _objects)
        {
            if (track->updateKeyFrame(idx,val))
                bOk =   true;
        }
        if (bOk)
            flags().addFlag(ClipChanged);
        return  bOk;
    }
    void    FEAnimClip::simplifyTimeline()
    {
        bool    bChanged    =   false;
        size_t  cnt         =   _objects.size();
        for (size_t i = 0; i < cnt; ++ i)
        {
            auto    trackI  =   _objects[i];
            if (!trackI->_times)
                continue;
            auto&   timesI  =   trackI->_times->values();
            /// 与之前的 track 比较,若 times 相同则共享其 _times
            for (size_t j = 0; j < i; ++ j)
            {
                auto    trackJ  =   _objects[j];
                if (!trackJ->_times)
                    continue;
                auto&   timesJ  =   trackJ->_times->values();
                if (timesI.size() != timesJ.size())
                    continue;
                if (std::equal(timesI.begin(),timesI.end(),timesJ.begin()))
                {
                    trackI->setTimeObject(trackJ->_times);
                    bChanged    =   true;
                    break;
                }
            }
        }
        if (bChanged)
            flags().addFlag(ClipChanged);
    }

    bool    FEAnimClip::isChanged() const
    {
        if (flags().hasFlag(ClipChanged))
            return  true;
        for (auto& track : _objects)
        {
            if (track->isChanged())
                return  true;
        }
        return  false;
    }
    real2   FEAnimClip::range() const
    {
        real2   result(-1,-1);
        if (_objects.empty())
            return  result;
        else
            result  =   _objects.front()->range();
        for (size_t i = 1; i < _objects.size(); ++i)
        {
            auto    tmp =   _objects[i]->range();
            result.x    =   (std::min)(result.x,tmp.x);
            result.y    =   (std::max)(result.y,tmp.y);
        }
        return  result;
    }

    void    FEAnimClip::update(const real& clipTime,TrackResults& results)
    {
        results.resize(_objects.size());
        /// _objects 已经按照times 对象排序，即相同的timeline 会在一起
        RealsObject             timeLine    =   nullptr;
        FEKeyFrameTrack::KFOff  kfValue     =   {};
        for (size_t i = 0 ;i < _objects.size() ; ++ i)
        {
            auto        track   =   _objects[i];
            if (track->times() != timeLine)
            {
                timeLine    =   track->times();
                kfValue     =   track->calcFrameOffset(clipTime);
            }
            results[i]._track   =   track;
            results[i]._prop    =   track->propertyIndex();
            results[i]._valid   =   track->update(kfValue,results[i]);
        }
    }

    size_t  FEAnimClip::queryDepends(ObjectUSet& uSet) const
    {
        auto    count   =   uSet.size();
        for (auto  var: objects())
        {
            uSet.emplace(var);
        }
        return  uSet.size() - count;
    }

    void    FEAnimClip::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const
    {
        UNUSED(chunk,version,ctx);
        uint    nTrack = (uint)_objects.size();
        writer.write(nTrack);
        for (auto& track: objects())
        {
            writer.write(track->objectId());
        }
    }
    void    FEAnimClip::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx)
    {
        (void)reader;
        (void)chunk;
        (void)version;
        (void)ctx;
        uint            nTrack  =   reader.readValue<uint>();
        KeyFrameTracks  tracks;
        tracks.reserve(nTrack);
        for (uint i = 0; i < nTrack; i++)
        {
            FEUuid  objectId    =   {};
            auto    result      =   reader.read(objectId);
            assert(result == sizeof(objectId));
            if (result != sizeof(objectId)) 
                continue;
            Object  object  =   ctx.query(objectId,nullptr,FESerializeCtx::O_Query).first;
            auto    track   =   object ? object->cast<FEKeyFrameTrack>() : nullptr;
            assert(track != nullptr);
            if (track == nullptr)
                continue;
            tracks.push_back(track);
        }
        setObjects(std::move(tracks));
    }
}
