#include    <new> 
#include    <memory> 

#include    "../inc/FEObject.h"
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FEProgress.hpp"
#include    "../inc/FENotify.hpp"

namespace   FE
{
    FEObject::FEObject(FEContext& ctx,bool genId)
        :_ctx(ctx)
    {
        _flags          =   FLAG_VISIBLE | FLAG_UPDATE | FLAG_SERIAL;
        if (genId)  _id =   FEUuid::create();
    }
    FEObject::FEObject(const FEObject& other)
        :_ctx(other._ctx)
    {
        _flags  =   other._flags;
    }

    const char*     FEObject::className() const          
    {                                                  
        return  "FEObject";                           
    }                                                  
    const FEUuid&   FEObject::classId() const
    {                                                  
        return  UUIDOF(FEObject);                    
    }
    const CLSProp&  FEObject::property() const
    {                                                           
        return  CLS_PROPERTY(FEObject);                       
    }                                                           
    CLSProp&        FEObject::property()             
    {                                                           
        return  CLS_PROPERTY(FEObject);                       
    }

    uint64  FEObject::serialize(FEWriter&writer ,uint& vVersion,FESerializeCtx& sCtx) const
    {
        FEChunkInf      infor   =   {};
        uint64          nStart  =   writer.tell();
        FEWriterHelper  helper(writer,[&](FEWriterHelper&)
        {   
            uint64  nCur    =   writer.tell();
            writer.seek(nStart);
            writer.write(infor);
            writer.seek(nCur);
        });
        /// 输出版本号，调用者会用到
        vVersion    =   version();
        /// 检测是否需要写入版本号，如果是默认值，不写入
        infor._hasVersion == (vVersion == FEObject::version()) ? 0 : 1;
        
        writer.write(infor);
        writer.write(classId());
        writer.write(objectId());
        writer.write(flags());
        serializeTraits(writer,infor,vVersion,sCtx);

        uint    cnt =   0;
        
        switch(infor._hasChild)
        {
        case 1: writer.write((uint8_t)cnt);     break;
        case 2: writer.write((uint16_t)cnt);    break;
        case 3: writer.write(cnt);              break;
        }
        
        /// 更新进度
        if (sCtx.query)
            sCtx.query(FEUuid::zero(),&writer,FESerializeCtx::O_UpdateProgress);

        return  helper.size();
    }
    uint64  FEObject::deserialize(FEReader& reader ,uint& version,FESerializeCtx& sCtx) 
    {
        FEReaderHelper  helper(reader);
        FEChunkInf      infor   =   {};
        reader.read(infor);
        reader.skip(sizeof(classId()));

        reader.read(_id);
        reader.read(flags());
        /// 赋值默认版本
        version =   FEObject::version();
        /// 如果有版本号，读取
        if (infor._hasVersion)  reader.read(version);
        /// 子类内容读取
        deserializeTraits(reader,infor,version,sCtx);
        /// 更新进度
        if (sCtx.query)
            sCtx.query(FEUuid::zero(),&reader,FESerializeCtx::O_UpdateProgress);

        uint    childs  =   0;
        switch(infor._hasChild)
        {
        case 1: childs  =   reader.readValue<uint8>();  break;
        case 2: childs  =   reader.readValue<uint16>(); break;
        case 3: childs  =   reader.readValue<uint32>(); break;
        }
        
        return  helper.size();
    }

    void    FEObject::serializeTraits(FEWriter& ,FEChunkInf& ,uint ,FESerializeCtx& ) const
    {
        
    }
    void    FEObject::deserializeTraits(FEReader& ,const FEChunkInf& ,uint ,FESerializeCtx& )
    {}

    Object  FEObject::queryInterface(const CLSId& )
    {
        return  nullptr;
    }
    uint    FEObject::version() const
    {
        return  V1_0_0_0;
    }
    size_t  FEObject::queryDepends(ObjectUSet& uset) const
    {
        (void)uset;
        return  0;
    }

    Object  FEObject::clone() const
    {
        return  new FEObject(*this);
    }
}
