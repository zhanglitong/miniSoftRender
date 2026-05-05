#pragma     once
#include    <string>
#include    "../inc/FEObject.h"
#include    "../inc/FERect.hpp"
#include    "FEStruct.h"

namespace   FE
{

    DEFINE_CLASS_UUID(FEImage,"{CACB6053-A4DB-4479-9BBD-D9D4433263AD}");

    class   FE_API  FEImage:public FEObject
    {
        IMPLEMENT_CLASS_REFLECT(FEImage)
    public:
        bool    create(const FEImageCreateInfo& info);
    protected:
        static  void*   allocMemory(FEImageCreateInfo& info);
        static  size_t  calcBufferSize(const FEImageCreateInfo& info);
        static  size_t  calcSize(uint32_t w,uint32_t h,uint32_t d,FEImageFormat fmt);
    protected:
        FEImageCreateInfo   _cInfo;
    };
}