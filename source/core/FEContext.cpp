
#include    "../inc/FEContext.hpp"
#include    "../inc/FEDateTime.hpp"
#include    "../inc/FESetting.hpp"
#include    "../inc/geometry/FEGeometry.hpp"
#include    "../inc/geometry/FEGeometryParam.hpp"
#include    "../inc/geometry/FEGeometryBox.hpp"
#include    "../inc/FEStreamReader.hpp"

#include    "../inc/mesh/FEAttribute.hpp"
#include    "../inc/mesh/FEPrimitive.hpp"
#include    "../inc/mesh/FEDrawArray.hpp"
#include    "../inc/mesh/FEDrawElementUint8.hpp"
#include    "../inc/mesh/FEDrawElementUint16.hpp"
#include    "../inc/mesh/FEDrawElementUint32.hpp"
#include    "../inc/mesh/FEMesh.hpp"
#include    "../inc/mesh/FEMeshBuffer.hpp"

#include    "../inc/node/FENode.hpp"
#include    "../inc/FEEntryList.hpp"

#include    "../inc/FEFileFormatLibrary.hpp"

#include    "../inc/graphic/FEScene.h"

#include    "FEImageHelper.hpp"

namespace   FE
{
    void    systemInitializeCreator(FEContext& ctx)
    {
        (void)ctx;
        DEFINE_CLASS_CREATOR(FEEntryList);
        DEFINE_CLASS_CREATOR(FEPrimitive);
        DEFINE_CLASS_CREATOR(FEDrawArray);
        DEFINE_CLASS_CREATOR(FEDrawElementUint8);
        DEFINE_CLASS_CREATOR(FEDrawElementUint16);
        DEFINE_CLASS_CREATOR(FEDrawElementUint32);
        DEFINE_CLASS_CREATOR(FEMesh);

        DEFINE_CLASS_CREATOR(FEGeometry);
        DEFINE_CLASS_CREATOR(FEGeometryBox);

        DEFINE_CLASS_CREATOR(FEImage);
        DEFINE_CLASS_CREATOR(FEBuffer);
        DEFINE_CLASS_CREATOR(FENode);


        DEFINE_CLASS_CREATOR(FEFormatFepjReader);
        DEFINE_CLASS_CREATOR(FEFormatFepjWriter);

        DEFINE_CLASS_CREATOR(FEFormatGLTFReader);

    }
    void    systemInitializeReader(FEContext& ctx)
    {
        (void)ctx;

        DEFINE_FORMAT_READER(FEFormatFepjReader);
        DEFINE_FORMAT_READER(FEFormatGLTFReader);
    }
    void    systemInitializeWriter(FEContext& ctx)
    {
        (void)ctx;
        DEFINE_FORMAT_WRITER(FEFormatFepjWriter);
    }
    void    systemInitialize(FEContext& ctx)
    {
        (void)ctx;
        systemInitializeCreator(ctx);
        systemInitializeReader(ctx);
        systemInitializeWriter(ctx);
    }
    void    systemRegCreator(const FEUuid& id,const FECreator& creator)
    {
        assert(FEContext::creators().isExist(id) == nullptr);

        FEContext::creators().add(id,creator);
    }
    void    systemRegReader(const FEFileFormat& fmt)
    {
        assert(FEContext::readers().isExist(fmt.toString()) == nullptr);
        FEContext::readers().add(fmt.toString(),fmt);
    }
    void    systemRegWriter(const FEFileFormat& fmt)
    {
        assert(FEContext::writers().isExist(fmt.toString()) == nullptr);
        FEContext::writers().add(fmt.toString(),fmt);
    }

    FECreators& FEContext::creators()
    {
        static  FECreators    sInstance;
        return  sInstance;
    }
    FFReader&   FEContext::readers()
    {
        static  FFReader    sInstance;
        return  sInstance;
    }
    FFWriter&   FEContext::writers()
    {   
        static  FFWriter  sInstance;
        return  sInstance;
    }

    FEContext::FEContext()
    {
        _log    =   new FELog(*this,"log.txt");
        systemInitialize(*this);
    }
    FEContext::~FEContext()
    {
        _log    =   nullptr;
    }
}