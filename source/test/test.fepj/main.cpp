
#include    <iostream>
#include    <stdio.h>
#include    <unordered_set>
#include    <assert.h>
#include    "../../inc/FEImage.h"
#include    "../../core/FEImageHelper.hpp"
#include    "../../inc/FEContext.hpp"
#include    "../../inc/FEFileFormatHelper.hpp"
#include    "../../inc/FENodeHelper.hpp"
#include    "../../inc/FEProperty.hpp"


using   namespace   FE;
int     main(int argc,char** argv)
{
    FEContext   ctx;
    FE::ImagePtr    image   =   new FE::FEImage(ctx);
    FE::FEImage::CreateInfo cInfor= 
    {
        uint3(111,115,1),
        FMT_R8G8B8_UNORM,
        1,
        1,
    };

    FEProperty  ppp(ctx);
    reals   ppData  =   {1,2,3.1,222,553,666,1234555};
    ppp.setValue("test",1234);
    ppp.setValue("reals",ppData);
    Buffer          buf =   new FEBuffer(ctx);
    buf->cInfo()._buffer.resize(1024);
    uint            vv ;
    FEWriterMemory  mem(ctx,buf);
    FESerializeCtx    xxxx;
    ppp.serialize(mem,vv,xxxx);
    FEReaderMemory   rrr(ctx,buf);
    ppp.deserialize(rrr,vv,xxxx);
    image->create(cInfor);
    //image->fill(float4(0,0,1,1));
    //image->plane(0,0).fillRect(FE::RectU32(10,10,20,20),float4(0,1,0,1));
    //image->plane(0,0).fillRect(FE::RectU32(30,30,60,60),float4(0,1,0,1));
    FEImageHelper::saveBmp(*image,"d:/x.bmp");

    Node    root    =   new FENode(ctx);
    root->addObject(image.get());

    for (size_t i = 0; i < 100; i++)
    {
        auto    child   =   new FENode(ctx);
        /// child->_trans   =   real3(i,i,i);
        root->addChild(child);
    }
    /// 通过系统获取插件
    FEFileFormat    fmt(".fepj","1.0.0.0","FE Buildin Format!");
    fmt._type       =   FEFileFormat::DT_Model;
    fmt._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;
    auto    writer  =   FEFileFormatHelper::queryWriter(ctx,fmt);
    if (writer)
    {
        writer->writeFile({root.get()},"d:/t1.fepj");
    }

    auto    reader  =   FEFileFormatHelper::queryReader(ctx,fmt);
    if (reader)
    {
        auto    objects =   reader->readFiles({"d:/t1.fepj"});
    }
    
    return  0;
}
