
#include    "../inc/FEImage.h"
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
namespace   FE
{
    FEImage::FEImage(FEContext& ctx,uint32_t w,uint32_t h,FEFormat fmt)
        :FEObject(ctx)
    {
        if (w && h && fmt != FMT_NONE)
        {
            CreateInfo  info;
            info._extent    =   uint3(w,h,1);
            info._format    =   fmt;
            auto    result  =   create(info);
            UNUSED(result);
            assert(result);
        }
    }

    FEImage::FEImage(const FEImage& other)
        :FEObject(other)
    {
        _cInfo  =   other._cInfo;
    }

    FEImage::~FEImage()
    {
        _cInfo._buffers.clear();
    }

    bool    FEImage::create(const FEImage::CreateInfo& info)
    {
        bool    bCheck  =       info._extent.x != 0 
                            &&  info._extent.y != 0 
                            &&  info._extent.z != 0
                            &&  info._levels   != 0
                            &&  info._layers   != 0
                            &&  info._format   != FMT_NONE;
        assert(bCheck);
        if (!bCheck)
            return  false;

        _cInfo  =   info;
        allocMemory(_ctx,_cInfo);
        
        return  bCheck && !_cInfo._buffers.empty();
    }

    bool    FEImage::allocMemory(FEContext& ctx,FEImage::CreateInfo& info)
    {   
        const auto  nByte   =   bytesOfLayer(info._extent,info._levels,info._align,info._format);

        if (!info._buffers.empty() && info._buffers.size() == info._layers)
            return  true;
        info._buffers.resize(info._layers);

        for (uint i = 0; i < info._layers; i++)
        {
            BufferPtr   buffer  =   new FEBuffer(ctx);
            info._buffers[i]    =   buffer;
            FEBuffer::CreateInfo    cInfo;
            cInfo._buffer.resize(nByte);
            buffer->create(cInfo);
        }
        return  !info._buffers.empty(); 
    }

    void    FEImage::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const
    {
        (void)chunk;
        auto&   cInf    =   cInfo();
        writer.write(cInf._extent);
        writer.write(cInf._format);
        writer.write(cInf._layers);
        writer.write(cInf._levels);
        writer.write(cInf._align);
        for (uint16_t i = 0; i < cInf._layers; i++)
        {
            cInf._buffers[i]->serialize(writer,version,ctx);
        }
    }
    void    FEImage::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx)
    {
        (void)chunk;

        auto&   cInf    =   cInfo();
        reader.read(cInf._extent);
        reader.read(cInf._format);
        reader.read(cInf._layers);
        reader.read(cInf._levels);
        reader.read(cInf._align);
        for (uint16_t i = 0; i < cInf._layers; i++)
        {
            auto    object  =   FEObjectHelper::readObject(_ctx,reader,version,ctx);
            assert(object != nullptr);
            if (object == nullptr)
                continue;
            auto    buffer  =   object->as<FEBuffer>();
            if (buffer == nullptr)
                continue;
            cInf._buffers.emplace_back(buffer);
        }
    }
}
