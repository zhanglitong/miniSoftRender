
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FENodeHelper.hpp"
#include    "../inc/FEMeshLibrary.hpp"
#include    "../inc/FEBuffer.hpp"
#include    "../inc/FEGeometryLibrary.hpp"
#include    "../inc/FENotify.hpp"
namespace   FE
{

    void    FEBuffer::serializeTraits(FEWriter& writer,FEChunkInf& ,uint version,FESerializeCtx& ctx) const
    {
        (void)ctx;
        (void)version;
        uint64  length  =   _cInfo._buffer.size();
        writer.write(length);
        writer.write(_cInfo._offset);
        writer.write(_cInfo._range);
        writer.writeBuffer(_cInfo._buffer.data(),length);

    }
    void    FEBuffer::deserializeTraits(FEReader& reader,const FEChunkInf&,uint version,FESerializeCtx& ctx) 
    {
        (void)ctx;
        (void)version;
        uint64  length  =   _cInfo._buffer.size();
        reader.read(length);
        reader.read(_cInfo._offset);
        reader.read(_cInfo._range);
        _cInfo._buffer.resize(length);
        reader.readBuffer(_cInfo._buffer.data(),length);
    }

    struct  NodeChunkBit
    {
        union
        {
            struct 
            {
                /// 是否写颜色 数据 = [0,0,0,255] 不写入
                uint16  _hasColor   :1;
                /// 是否有名字,0 没有 1,uint8,2:uint16,3:uint32,减少长度字段的内存占用
                uint16  _hasName    :2;
                /// 是否有子节点,0 没有 1,uint8,2:uint16,3:uint32,减少长度字段的内存占用
                uint16  _hasCom     :2;
                /// 是否写材质
                uint16  _hasMaterial:1;
                /// 是否写几何体
                uint16  _hasGeometry:1;
                /// 是否写位置 数据 = [0,0,0] 不写入
                uint16  _hasTrans   :1;
                /// 是否写缩放 数据 = [1,1,1] 不写入
                uint16  _hasScale   :1;
                /// 是否写旋转 数据 = [1,0,0,0] 不写入
                uint16  _hasRotate  :1;
            };
            uint16  _value;
        };
        
        
        NodeChunkBit(uint16 flag = 0)
        {
            _hasColor       =   flag>>0;
            _hasName        =   flag>>1;
            _hasCom         =   flag>>3;
            _hasMaterial    =   flag>>5;
            _hasGeometry    =   flag>>6;
            _hasTrans       =   flag>>7;
            _hasScale       =   flag>>8;
            _hasRotate      =   flag>>9;
        }
    };

    void    FENode::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const 
    {
        (void)ctx;
        (void)version;

        NodeChunkBit    bits(chunk._flags);
        
        size_t  nName   =   _name.size();
        size_t  nCom    =   _coms.size();

        if (nName == 0)             bits._hasName   =   0;
        else if (nName < MaxUint8)  bits._hasName   =   1;
        else if (nName < MaxUint16) bits._hasName   =   1;
        else if (nName < MaxUint32) bits._hasName   =   2;

        if (nCom == 0)              bits._hasCom    =   0;
        else if (nCom < MaxUint8)   bits._hasCom    =   1;
        else if (nCom < MaxUint16)  bits._hasCom    =   1;
        else if (nCom < MaxUint32)  bits._hasCom    =   2;

        bits._hasColor      =   (_color == Rgba8(0,0,0,255)) ? 0:1;
        bits._hasScale      =   _scale  ==  float3(1,1,1)  ? 1 : 0;
        bits._hasTrans      =   _trans  ==  real3(0,0,0)   ? 1 : 0;
        bits._hasRotate     =   _rotate ==  quatf(1,0,0,0) ? 1 : 0;
        bits._hasGeometry   =   _mesh ? 1 : 0;
        bits._hasMaterial   =   _material ? 1 : 0;

        /// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        /// 必须修改
        /// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        chunk._flags        =   bits._value;
        
        writer.write(_dataFlag);
        if (bits._hasScale )    writer.write(_scale);
        if (bits._hasTrans )    writer.write(_trans);
        if (bits._hasRotate )   writer.write(_rotate);
        if (bits._hasColor )    writer.write(_color);
        if (_mesh)              writer.write(_mesh->objectId());
        if (_material)          writer.write(_material->objectId());

        /// 写入名称
        switch(bits._hasName)
        {
        case 1: writer.write<uint8>(_name); break;
        case 2: writer.write<uint16>(_name);break;
        case 3: writer.write<uint32>(_name);break;
        }
        /// 写入长度数据
        switch(bits._hasCom)
        {
        case 1: writer.write<uint8>(uint8(nCom));  break;
        case 2: writer.write<uint16>(uint16(nCom));break;
        case 3: writer.write<uint32>(uint32(nCom));break;
        }
        for (auto& var : _coms)
        {
            OBJId   objectId    =   var->objectId();
            writer.write(objectId);
        }
    }
    void    FENode::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) 
    {
        (void)ctx;
        (void)version;

        NodeChunkBit    bits(chunk._flags);

        reader.read(_dataFlag);

        OBJId   matId;
        OBJId   geoId;
        if (bits._hasScale )    reader.read(_scale);
        if (bits._hasTrans )    reader.read(_trans);
        if (bits._hasRotate )   reader.read(_rotate);
        if (bits._hasColor)     reader.read(_color);
       
        /// 读几何体
        if (bits._hasGeometry)
        {
            OBJId   objectId ;
            auto    result  =   reader.read(objectId);
            UNUSED(result);
            assert(result == sizeof(objectId));
            auto    object  =   ctx.query(objectId,nullptr,FESerializeCtx::O_Query).first;
            _mesh           =   object ? object->cast<FEMesh>() : nullptr;
        }
        /// 读材质
        if (bits._hasMaterial)
        {
            OBJId   objectId ;
            auto    result  =   reader.read(objectId);
            UNUSED(result);
            assert(result == sizeof(objectId));
            auto    object  =   ctx.query(objectId,nullptr,FESerializeCtx::O_Query).first;
            _material       =   object ? object->cast<FEMaterial>() : nullptr;
        }
        switch(bits._hasName)
        {
        case 1: reader.read<uint8>(_name); break;
        case 2: reader.read<uint16>(_name);break;
        case 3: reader.read<uint32>(_name);break;
        }
        /// 读取组件
        uint    cnt =   0;
        switch(bits._hasCom)
        {
        case 1: cnt =   reader.readValue<uint8>(); break;
        case 2: cnt =   reader.readValue<uint16>();break;
        case 3: cnt =   reader.readValue<uint32>();break;
        }
        _coms.reserve(cnt);
        for (uint i = 0 ;i < cnt; ++ i)
        {
            OBJId   objectId ;
            auto    result  =   reader.read(objectId);
            UNUSED(result);
            assert(result == sizeof(objectId));
            auto    object  =   ctx.query(objectId,nullptr,FESerializeCtx::O_Query).first;
            if (object == nullptr)
                continue;
            addObject(object);
        }
    }

    struct  MeshChunkBit
    {
        union
        {
            struct 
            {
                /// 是否有Primitive, 0 没有 1,uint8,2:uint16,3:uint32,减少长度字段的内存占用
                uint16  _hasPrimitive   :2;
                /// 是否有meshBuffer,0 没有 1,uint8,2:uint16,3:uint32,减少长度字段的内存占用
                uint16  _hasSubMesh     :2;
            };
            uint16  _value;
        };
        MeshChunkBit(uint16 flags = 0)
        {
            _hasPrimitive   =   flags>>0;
            _hasSubMesh     =   flags>>2;
        }
    };
    void    FEMesh::serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const
    {
        (void)ctx;
        (void)version;

        MeshChunkBit    bits(chk._flags);

        size_t  nBuf    =   _buffers.size();
        

        if (nBuf == 0)              bits._hasSubMesh    =   0;
        else if (nBuf < MaxUint8)   bits._hasSubMesh    =   1;
        else if (nBuf < MaxUint16)  bits._hasSubMesh    =   1;
        else if (nBuf < MaxUint32)  bits._hasSubMesh    =   2;

        bits._hasPrimitive  =   _primitives.empty() ? 0 : 1;
        /// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        /// 必须修改
        /// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        chk._flags          =   bits._value;

        for(auto var : _primitives)
        {
            var->serialize(writer,version,ctx);
        }

        switch(bits._hasSubMesh)
        {
        case 1: writer.write<uint8>(uint8(nBuf));   break;
        case 2: writer.write<uint16>(uint16(nBuf)); break;
        case 3: writer.write<uint32>(uint32(nBuf)); break;
        }

    }
    void    FEMesh::deserializeTraits(FEReader& reader,const FEChunkInf& chk,uint version,FESerializeCtx& ctx)
    {
        (void)ctx;
        (void)version;

        MeshChunkBit    bits(chk._flags);

        uint    nBuf    =   0;
        
        if (bits._hasPrimitive)
        {
            /// auto    ptr =   FEObjectHelper::readObject(_ctx,reader,version,ctx);
            /// if (ptr != nullptr && ptr->cast<FEPrimitive>())
            ///     _primitive  =   ptr->cast<FEPrimitive>();
        }

        switch(bits._hasSubMesh)
        {
        case 1: nBuf    =   reader.readValue<uint8>(); break;
        case 2: nBuf    =   reader.readValue<uint16>();break;
        case 3: nBuf    =   reader.readValue<uint32>();break;
        }

        _buffers.clear();
        _buffers.reserve(nBuf);
        for (uint i = 0 ;i < nBuf; ++ i)
        {
            /// auto    attObject   =   FEObjectHelper::readObject(_ctx,reader,version,ctx);
            /// auto    bufObject   =   FEObjectHelper::readObject(_ctx,reader,version,ctx);
            /// if (attObject == nullptr || bufObject)
            ///     continue;
            /// auto    attr    =   attObject->cast<FEAttribute>();
            /// auto    buf     =   bufObject->cast<FEBuffer>();
            /// if (attr == nullptr || buf == nullptr)
            ///     continue;
            /// FEMeshBuffer    meshBuffer(_ctx,buf,attr);
            /// _buffers.emplace_back(meshBuffer);
        }
        if (!_buffers.empty())
        {
            updateAabb();
        }
    }

    struct  GeometryChunkBit
    {
        union
        {
            struct 
            {
                /// <summary>
                /// 是否是参数化
                /// </summary>
                uint16  _isParam    :   1;
                /// 是否有mesh, 0 没有,1 有
                uint16  _hasMesh    :   1;
                /// 是否有attr, 0 没有 1,uint8,2:uint16,3:uint32,减少长度字段的内存占用
                uint16  _hasAttr    :   2;
            };
            uint16  _value;
        };
        GeometryChunkBit(uint16 flags = 0)
        {
            _isParam    =   flags>>0;
            _hasMesh    =   flags>>1;
            _hasAttr    =   flags>>2;
        }
    };

    void    FEGeometry::serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const 
    {
        (void)ctx;
        (void)version;

        GeometryChunkBit    bits(chk._flags);
        bits._isParam   =   isParam() ? 1 : 0;
        bits._hasMesh   =   _mesh ? 1: 0;
        chk._flags      =   bits._value;

        if (bits._hasMesh && !bits._isParam)
        {
            FEUuid  uuid    =  _mesh->objectId();
            writer.write(uuid);
        }
    }
    void    FEGeometry::deserializeTraits(FEReader& reader,const FEChunkInf& chk ,uint version,FESerializeCtx& ctx)
    {
        (void)version;
        (void)ctx;
        GeometryChunkBit    bits(chk._flags);
        if (bits._hasMesh && !bits._isParam)
        {
            FEUuid  uuid;
            reader.read(uuid);
            auto    result  =   ctx.query(uuid,nullptr,FESerializeCtx::O_Query);
            if (result.first != nullptr)
                _mesh       =   dynamic_cast<FEMesh*>(result.first);
        }
    }

    void    FEGeometryParam::serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const
    {
        (void)ctx;
        (void)version;

        FEGeometry::serializeTraits(writer,chk,version,ctx);
        assert (_mesh != nullptr);
        Attrs   customs;
        Attrs   systems;
        /// 参数化几何体，为了保证可以正确的参数化，需要把属性信息记录下来
        if (_mesh)
        {
            
        }
        /// 写入系统类型的个数
        writer.write<uint8>(uint8(systems.size()));
        /// 写入自定义类型的个数
        writer.write<uint8>(uint8(customs.size()));
        /// 系统类型写入index
        if (!systems.empty())
        {
            
        }
        /// 自定义类型 写objectId
        /// for (auto& var : customs)
        /// {
        /// }
    }

    void    FEGeometryParam::deserializeTraits(FEReader& reader,const FEChunkInf& chk ,uint version,FESerializeCtx& ctx)
    {
        struct  SlotFormat
        {
            uint16      slot;
            FEFormat    format;
        };

        using   SlotFormats =   std::vector<SlotFormat>;

        FEGeometry::deserializeTraits(reader,chk,version,ctx);

        uint8       systemCnt  =   0;
        uint8       customCnt  =   0;
       
        Attrs       inputs;
        Attrs       systems;
        SlotFormats slotFmts;
        /// 读系统类型的个数
        reader.read<uint8>(systemCnt);
        /// 读自定义类型的个数
        reader.read<uint8>(customCnt);
        /// 读系统类型index
        systems.resize(systemCnt);
        inputs.reserve(systemCnt + customCnt);
        
        slotFmts.resize(systemCnt);

        if (systemCnt)
        {
            reader.readBuffer(systems.data(),slotFmts.size() * sizeof(SlotFormat));
        }
        for (auto  index : slotFmts)
        {
           
        }

        /// 自定义类型 写objectId
        for (uint8 i = 0 ;i < customCnt ; ++ i)
        {
            
        }
        if (!inputs.empty())
        {
            _mesh   =   triangular(inputs);
        }
    }
}
