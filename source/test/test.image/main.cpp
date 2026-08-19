
#include    <iostream>
#include    <stdio.h>
#include    <unordered_set>
#include    <assert.h>
#include    "../../inc/FEImage.h"
#include    "../../core/FEImageHelper.hpp"
#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEStreamWriter.hpp"
#include    "../../inc/FEFileFormatHelper.hpp"
using   namespace   FE;
int     main(int argc,char** argv)
{
    FEContext   ctx;
    FEImage     image(ctx);
    FEImage::CreateInfo cInfor= 
    {
        uint3(111,115,1),
        FMT_R8G8B8_UNORM,
        1,
        1,
    };
    image.create(cInfor);
    /// image.fill(float4(0,0,1,1));
    /// image.plane(0,0).fillRect(FE::RectU32(10,10,20,20),float4(0,1,0,1));
    /// image.plane(0,0).fillRect(FE::RectU32(30,30,60,60),float4(0,1,0,1));
    FEImageHelper::saveBmp(image,"d:/x.bmp");

    return  0;
}
