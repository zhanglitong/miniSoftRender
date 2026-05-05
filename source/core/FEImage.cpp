
#include    "FEImage.h"

namespace   FE
{

    bool    FEImage::create(const FEImageCreateInfo& info)
    {
        bool    bCheck  =   info._extent.x != 0 
                            && info._extent.y != 0 
                            && info._extent.z != 0
                            && info._levels != 0
                            && info._layers != 0
                            && info._format != FMT_NONE;
        assert(bCheck);
        if (!bCheck)
            return  false;

        _cInfo  =   info;
        allocMemory(_cInfo);
        return  bCheck && _cInfo._buffer != nullptr;
    }

    void*   FEImage::allocMemory(FEImageCreateInfo& info)
    {   
        const auto  nBuffer =   calcBufferSize(info);
        
        if (info._buffer != nullptr)
        {
            if(info._bufferSize >= nBuffer)
            {
                return  info._buffer;
            }
            else
            {
                info._buffer        =   nullptr;
                info._bufferSize    =   0;
                return  nullptr;
            }
        }
        info._buffer        =   new char[nBuffer];
        info._bufferSize    =   nBuffer;
        return  info._buffer; 
    }

    size_t  FEImage::calcBufferSize(const FEImageCreateInfo& info)
    {
        size_t      nSize   =   0;
        uint32_t    mips    =   (std::max<uint32_t>)(1,info._levels);
        uint32_t    layer   =   (std::max<uint32_t>)(1,info._layers);
        for (uint32_t i = 0; i < layer; ++i)
        {
            for (uint32_t i = 0; i < mips; ++i)
            {
                nSize   +=  calcSize( (std::max<uint32_t>)(info._extent.x >> i,1)
                                    , (std::max<uint32_t>)(info._extent.y >> i,1)
                                    , (std::max<uint32_t>)(info._extent.z >> i,1)
                                    , info._format);
            }
        }
        return  nSize;
    }

    size_t  FEImage::calcSize(uint32_t w,uint32_t h,uint32_t d,FEImageFormat fmt)
    {
        switch(fmt)
        {
        case FMT_RGBA8:     return  (w * h * d) * 4 * sizeof(int8_t);;
        case FMT_RGBAI16:   return  (w * h * d) * 4 * sizeof(int16_t);
        case FMT_RGBAI32:   return  (w * h * d) * 4 * sizeof(int32_t);
        case FMT_RGBAU16:   return  (w * h * d) * 4 * sizeof(uint16_t);
        case FMT_RGBAU32:   return  (w * h * d) * 4 * sizeof(uint32_t);
        case FMT_RGBAF16:   return  (w * h * d) * 4 * sizeof(float16);
        case FMT_RGBAF32:   return  (w * h * d) * 4 * sizeof(float32);
        }
        assert(0 != 0);
        return  0;
    }
}