#pragma     once

#include    "../FEFormat.hpp"
#include    "../FEAspect.hpp"
#include    "RSObject.h"
#include    "FEGImageView.h"
namespace   FE
{
    class   FEGImage  :public RSObject
    {
    public:
        struct  CreateInfo
        {
            FEFormat    _format;
            FEAspect    _aspect =   ASPECT_COLOR_BIT;
            uint        _width  =   0;
            uint        _height =   0;
            uint        _depth  =   1;
            uint        _mips   =   1;
            uint        _layer  =   1;
        };
    public:
        FEGImage(FEContext& ctx)
            :RSObject(ctx)
        {}

        FEGImage(const FEGImage& other)
            :RSObject(other)
        {
            _cInfo  =   other._cInfo;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool        create(const CreateInfo& info)    =   0;
        virtual GImgView    createView()    =   0;
    protected:
        CreateInfo    _cInfo;
    };

    using   GImage      =   SharedPtr<FEGImage>;
    using   GImages     =   std::vector<GImage>;
}


