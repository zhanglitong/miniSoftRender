
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
