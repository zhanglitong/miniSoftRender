
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FENodeHelper.hpp"
#include    "../inc/animation/FEKeyFrameTrack.hpp"

namespace FE
{
    using   KFOff   =   FEKeyFrameTrack::KFOff;

    FEKeyFrameTrack::FEKeyFrameTrack(FEContext& ctx,PropIndex index)  
        :FEObject(ctx)
    {
        _propIndex  =   index;
        _name       =   "FEKeyFrameTrack";
    }
    FEKeyFrameTrack::FEKeyFrameTrack(const FEKeyFrameTrack& other)
        :FEObject(other)
    {
        _propIndex  =   other._propIndex;
        _times      =   other._times;
        _values     =   other._values;
        _type       =   other._type;
    }

    void    FEKeyFrameTrack::sortKeyFames()
    {
        switch(_values.index())
        {
        case 1:     return  sortImpl(std::get<RealsObject>(_values));
        case 2:     return  sortImpl(std::get<Real2sObject>(_values));
        case 3:     return  sortImpl(std::get<Real3sObject>(_values));
        case 4:     return  sortImpl(std::get<Real4sObject>(_values));
        case 5:     return  sortImpl(std::get<QuatrsObject>(_values));
        case 6:     return  sortImpl(std::get<FloatsObject>(_values));
        case 7:     return  sortImpl(std::get<Float2sObject>(_values));
        case 8:     return  sortImpl(std::get<Float3sObject>(_values));
        case 9:     return  sortImpl(std::get<Float4sObject>(_values));
        case 10:    return  sortImpl(std::get<QuatfsObject>(_values));
        case 11:    return  sortImpl(std::get<BoolsObject>(_values));
        case 12:    return  sortImpl(std::get<RgbaObject>(_values));
        case 13:    return  sortImpl(std::get<AlphaObject>(_values));
        case 14:    return  sortImpl(std::get<IntsObject>(_values));
        case 15:    return  sortImpl(std::get<Int2sObject>(_values));
        case 16:    return  sortImpl(std::get<Int3sObject>(_values));
        case 17:    return  sortImpl(std::get<Int4sObject>(_values));
        case 18:    return  sortImpl(std::get<UintsObject>(_values));
        case 19:    return  sortImpl(std::get<Uint2sObject>(_values));
        case 20:    return  sortImpl(std::get<Uint3sObject>(_values));
        case 21:    return  sortImpl(std::get<Uint4sObject>(_values));
        }
    }
    KFOff   FEKeyFrameTrack::calcFrameOffset(const real& clipTime) const
    {
        KFOff   result;
        result.clipTm   =   clipTime;
        if (!isValid())
            return  result;
        auto    rng     =   range();
        auto&   times   =   _times->values();
        if (clipTime <= rng.x)
        {
            result.index    =   0;
            return  result;
        }
        else if(clipTime >= rng.y)
        {
            result.index    =   times.size() - 1;
            return  result;
        }
        auto    itr =   std::lower_bound(times.begin(), times.end(), clipTime, [](const real& l, const real& tm)
        {
            return l < tm;
        });
            
        result.index    =   std::distance(times.begin(),itr) - 1;

        real    startTime   =   times[result.index + 0];
        real    endTime     =   times[result.index + 1];
        real    offset      =   real(clipTime - startTime) / real(endTime - startTime);
        result.offsetTm     =   std::clamp(offset,0.0,1.0);
        return  result;
    }
     
    bool    FEKeyFrameTrack::update(const real& clipTm,FETrackResult& result)
    {
        if (!isValid())
            return  false;
        switch(_values.index())
        {
        case 1:     return  updateImpl(clipTm,result,std::get<RealsObject>(_values));
        case 2:     return  updateImpl(clipTm,result,std::get<Real2sObject>(_values));
        case 3:     return  updateImpl(clipTm,result,std::get<Real3sObject>(_values));
        case 4:     return  updateImpl(clipTm,result,std::get<Real4sObject>(_values));
        case 5:     return  updateImpl(clipTm,result,std::get<QuatrsObject>(_values));
        case 6:     return  updateImpl(clipTm,result,std::get<FloatsObject>(_values));
        case 7:     return  updateImpl(clipTm,result,std::get<Float2sObject>(_values));
        case 8:     return  updateImpl(clipTm,result,std::get<Float3sObject>(_values));
        case 9:     return  updateImpl(clipTm,result,std::get<Float4sObject>(_values));
        case 10:    return  updateImpl(clipTm,result,std::get<QuatfsObject>(_values));
        case 11:    return  updateImpl(clipTm,result,std::get<BoolsObject>(_values));
        case 12:    return  updateImpl(clipTm,result,std::get<RgbaObject>(_values));
        case 13:    return  updateImpl(clipTm,result,std::get<AlphaObject>(_values));
        default:    return  false;
        }
    }
        
    bool    FEKeyFrameTrack::update(const KFOff& kfOff,FETrackResult& result)
    {
        if (!isValid())
            return  false;
        switch(_values.index())
        {
        case 1:     return  updateImpl(kfOff,result,std::get<RealsObject>(_values));
        case 2:     return  updateImpl(kfOff,result,std::get<Real2sObject>(_values));
        case 3:     return  updateImpl(kfOff,result,std::get<Real3sObject>(_values));
        case 4:     return  updateImpl(kfOff,result,std::get<Real4sObject>(_values));
        case 5:     return  updateImpl(kfOff,result,std::get<QuatrsObject>(_values));
        case 6:     return  updateImpl(kfOff,result,std::get<FloatsObject>(_values));
        case 7:     return  updateImpl(kfOff,result,std::get<Float2sObject>(_values));
        case 8:     return  updateImpl(kfOff,result,std::get<Float3sObject>(_values));
        case 9:     return  updateImpl(kfOff,result,std::get<Float4sObject>(_values));
        case 10:    return  updateImpl(kfOff,result,std::get<QuatfsObject>(_values));
        case 11:    return  updateImpl(kfOff,result,std::get<BoolsObject>(_values));
        case 12:    return  updateImpl(kfOff,result,std::get<RgbaObject>(_values));
        case 13:    return  updateImpl(kfOff,result,std::get<AlphaObject>(_values));
        default:    return  false;
        }
    }

    size_t  FEKeyFrameTrack::queryDepends(ObjectUSet& uSet) const
    {
        assert(_times != nullptr);
        size_t      count   =   uSet.size();
        RealsObject object  =   _times;
        if (object != nullptr)
            uSet.emplace(object);
        return  uSet.size() - count;
        
    }

    void    FEKeyFrameTrack::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const 
    {
        UNUSED(writer, chunk, version, ctx);
        writer.write(_tension);
        writer.write(_propIndex);
        writer.write(_type);
        uint    vIndex = (uint)_values.index();
        writer.write(vIndex);

        FEChunkBits bits(chunk._flags);
        bits._bit0      =   _times ? 1:0;
        chunk._flags    =   bits._value;
        if (bits._bit0)
        {
            writer.write(_times->objectId());
        }

        switch(_values.index())
        {
        case 1:     std::get<RealsObject>(_values)   ->serialize(writer,version,ctx);   break;
        case 2:     std::get<Real2sObject>(_values)  ->serialize(writer,version,ctx);   break;
        case 3:     std::get<Real3sObject>(_values)  ->serialize(writer,version,ctx);   break;
        case 4:     std::get<Real4sObject>(_values)  ->serialize(writer,version,ctx);   break;
        case 5:     std::get<QuatrsObject>(_values)  ->serialize(writer,version,ctx);   break;
        case 6:     std::get<FloatsObject>(_values)  ->serialize(writer,version,ctx);   break;
        case 7:     std::get<Float2sObject>(_values) ->serialize(writer,version,ctx);   break;
        case 8:     std::get<Float3sObject>(_values) ->serialize(writer,version,ctx);   break;
        case 9:     std::get<Float4sObject>(_values) ->serialize(writer,version,ctx);   break;
        case 10:    std::get<QuatfsObject>(_values)  ->serialize(writer,version,ctx);   break;
        case 11:    std::get<BoolsObject>(_values)   ->serialize(writer,version,ctx);   break;
        case 12:    std::get<RgbaObject>(_values)    ->serialize(writer,version,ctx);   break;
        case 13:    std::get<AlphaObject>(_values)   ->serialize(writer,version,ctx);   break;
        }
            
    }
    void    FEKeyFrameTrack::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) 
    {   
        UNUSED(reader, chunk, version, ctx);
        reader.read(_tension);
        reader.read(_propIndex);
        reader.read(_type);
        uint   vIndex = 0;    
        reader.read(vIndex);

        FEChunkBits bits(chunk._flags);
        ///          һ   и     
        ///      Ƿ  б  
        if (bits._bit0)
        {
            OBJId   objectId ;
            auto    result  =   reader.read(objectId);
            UNUSED(result);
            assert(result == sizeof(objectId));
            if(result == sizeof(objectId))
            {
                auto    object  =   ctx.query(objectId,nullptr,FESerializeCtx::O_Query).first;
                _times          =   object ? object->cast<FERealsObject>() : nullptr;
            }
        }

        if (vIndex != 0)
        {
            auto    ptr =   FEObjectHelper::readObject(_ctx,reader,version,ctx);
            if (ptr)
            {
                switch(vIndex)
                {
                case 1:     _values =   RealsObject(ptr->as<FERealsObject>());      break;
                case 2:     _values =   Real2sObject(ptr->as<FEReal2sObject>());    break;
                case 3:     _values =   Real3sObject(ptr->as<FEReal3sObject>());    break;
                case 4:     _values =   Real4sObject(ptr->as<FEReal4sObject>());    break;
                case 5:     _values =   QuatrsObject(ptr->as<FEQuatrsObject>());    break;
                case 6:     _values =   FloatsObject(ptr->as<FEFloatsObject>());    break;
                case 7:     _values =   Float2sObject(ptr->as<FEFloat2sObject>());  break;
                case 8:     _values =   Float3sObject(ptr->as<FEFloat3sObject>());  break;
                case 9:     _values =   Float4sObject(ptr->as<FEFloat4sObject>());  break;
                case 10:    _values =   QuatfsObject(ptr->as<FEQuatfsObject>());    break;
                case 11:    _values =   BoolsObject(ptr->as<FEBoolsObject>());      break;
                case 12:    _values =   RgbaObject(ptr->as<FEColorObject>());       break;
                case 13:    _values =   AlphaObject(ptr->as<FEAlphaObject>());      break;
                }
            }
        }
            
    }
    
}
