
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FENodeHelper.hpp"
#include    "../inc/FEMeshLibrary.hpp"
#include    "../inc/FEBuffer.hpp"
#include    "../inc/FEGeometryLibrary.hpp"
#include    "../inc/FENotify.hpp"
#include    "../inc/material/FEMaterialLibrary.hpp"

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
                /// 如果0,说明classId == UUIDOF(FENode);
                /// 在读取node的时候用作判断是否需要 通过creator 创建,否则直接new FENode,提升性能
                uint16  _nodeBase   :1;
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
            _nodeBase       =   flag>>10;
        }
    };

    void    FENode::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const 
    {
        UNUSED(writer,chunk,version,ctx);

        NodeChunkBit    bits(chunk._flags);
        
        size_t  nName   =   _name.size();
        size_t  nCom    =   _coms.size();

        bits._nodeBase  =   classId() == UUIDOF(FENode) ? 1:0;

        if (nName == 0)             bits._hasName   =   0;
        else if (nName < MaxUint8)  bits._hasName   =   1;
        else if (nName < MaxUint16) bits._hasName   =   2;
        else if (nName < MaxUint32) bits._hasName   =   3;

        if (nCom == 0)              bits._hasCom    =   0;
        else if (nCom < MaxUint8)   bits._hasCom    =   1;
        else if (nCom < MaxUint16)  bits._hasCom    =   2;
        else if (nCom < MaxUint32)  bits._hasCom    =   3;

        bits._hasColor      =   (_color == Rgba8(0,0,0,255)) ? 0:1;
        bits._hasScale      =   _transform._scale     ==  float3(1,1,1)  ? 1 : 0;
        bits._hasTrans      =   _transform._position  ==  real3(0,0,0)   ? 1 : 0;
        bits._hasRotate     =   _transform._rotation  ==  quatf(1,0,0,0) ? 1 : 0;
        bits._hasGeometry   =   _mesh       ? 1 : 0;
        bits._hasMaterial   =   _material   ? 1 : 0;

        /// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        /// 必须修改
        /// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        chunk._flags        =   bits._value;

        auto    nChild      =   _childs.size();
        if (nChild == 0)                chunk._hasChild =   0;
        else if (nChild < MaxUint8)     chunk._hasChild =   1;
        else if (nChild < MaxUint16)    chunk._hasChild =   2;
        else if (nChild < MaxUint32)    chunk._hasChild =   3;

        
        if (bits._hasScale )    writer.write(_transform._scale);
        if (bits._hasTrans )    writer.write(_transform._position);
        if (bits._hasRotate )   writer.write(_transform._rotation);
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
        /// 写入长度数据
        switch(chunk._hasChild)
        {
        case 1: writer.write<uint8>(uint8(nChild));  break;
        case 2: writer.write<uint16>(uint16(nChild));break;
        case 3: writer.write<uint32>(uint32(nChild));break;
        }
        for (size_t i = 0; i < nChild; i++)
        {
            _childs[i]->serialize(writer,version,ctx);
        }
    }
    void    FENode::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) 
    {
        UNUSED(reader,chunk,version,ctx);
        NodeChunkBit    bits(chunk._flags);
        OBJId   matId;
        OBJId   geoId;
        if (bits._hasScale )    reader.read(_transform._scale);
        if (bits._hasTrans )    reader.read(_transform._position);
        if (bits._hasRotate )   reader.read(_transform._rotation);
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
        for (uint i = 0 ;i < cnt; ++ i)
        {
            OBJId   objectId ;
            auto    result  =   reader.read(objectId);
            UNUSED(result);
            assert(result == sizeof(objectId));
            auto    object  =   ctx.query(objectId,nullptr,FESerializeCtx::O_Query).first;
            if (object == nullptr)
                continue;
            auto    com     =   object->cast<FEComponent>();
            if ( com != nullptr)
                addComponent(com);
        }

        uint    childs  =   0;
        switch(chunk._hasChild)
        {
        case 1: childs  =   reader.readValue<uint8>();  break;
        case 2: childs  =   reader.readValue<uint16>(); break;
        case 3: childs  =   reader.readValue<uint32>(); break;
        }
        _childs.reserve(childs);
        for (uint i = 0; i < childs; ++i)
        {
            FEChunkInf      chk         =   {};
            size_t          offStart    =   reader.tell();
            reader.read(chk);
            reader.seek(offStart);
            NodeChunkBit    chkBits(chk._flags);
            if (chkBits._nodeBase)
            {
                Node    node    =   new FENode(_ctx);
                node->setParent(this);
                node->deserialize(reader,version,ctx);
                _childs.emplace_back(node);
            }
            else
            {
                Object  result  =   FEObjectHelper::readObject(_ctx,reader,version,ctx);
                if (result == nullptr)
                    continue;
                else
                    _childs.emplace_back(result->as<FENode>());
            }
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
                uint16  _hasBuffer      :2;
            };
            uint16  _value;
        };
        MeshChunkBit(uint16 flags = 0)
        {
            _hasPrimitive   =   flags>>0;
            _hasBuffer      =   flags>>2;
        }
    };
    void    FEMesh::serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const
    {
        (void)ctx;
        (void)version;

        MeshChunkBit    bits(chk._flags);

        size_t  nPri    =   _primitives.size();
        

        if (nPri == 0)              bits._hasPrimitive  =   0;
        else if (nPri < MaxUint8)   bits._hasPrimitive  =   1;
        else if (nPri < MaxUint16)  bits._hasPrimitive  =   1;
        else if (nPri < MaxUint32)  bits._hasPrimitive  =   2;

        size_t  nBuf    =   _buffers.size();
        if (nBuf == 0)              bits._hasBuffer     =   0;
        else if (nBuf < MaxUint8)   bits._hasBuffer     =   1;
        else if (nBuf < MaxUint16)  bits._hasBuffer     =   1;
        else if (nBuf < MaxUint32)  bits._hasBuffer     =   2;

        /// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        /// 必须修改
        /// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        chk._flags          =   bits._value;

        switch(bits._hasPrimitive)
        {
        case 1: writer.write<uint8>(uint8(nPri));   break;
        case 2: writer.write<uint16>(uint16(nPri)); break;
        case 3: writer.write<uint32>(uint32(nPri)); break;
        }

        for(auto var : _primitives)
        {
            var->serialize(writer,version,ctx);
        }

        switch(bits._hasBuffer)
        {
        case 1: writer.write<uint8>(uint8(nBuf));   break;
        case 2: writer.write<uint16>(uint16(nBuf)); break;
        case 3: writer.write<uint32>(uint32(nBuf)); break;
        }
        for (size_t i = 0; i < nBuf; i++)
        {
            writer.write<FEAttribute>(_buffers[i].attr());
            _buffers[i].buffer()->serialize(writer,version,ctx);
        }

    }
    void    FEMesh::deserializeTraits(FEReader& reader,const FEChunkInf& chk,uint version,FESerializeCtx& ctx)
    {
        (void)ctx;
        (void)version;

        MeshChunkBit    bits(chk._flags);

        _buffers.clear();
        _primitives.clear();

        if (bits._hasPrimitive)
        {
            uint    cnt     =   0;
            switch(bits._hasPrimitive)
            {
            case 1: cnt     =   reader.readValue<uint8>(); break;
            case 2: cnt     =   reader.readValue<uint16>();break;
            case 3: cnt     =   reader.readValue<uint32>();break;
            }
            _primitives.reserve(cnt);
            for (uint i = 0; i < cnt; i++)
            {
                auto    ptr =   FEObjectHelper::readObject(_ctx,reader,version,ctx);
                if (ptr == nullptr)
                    continue;
                auto    pri =   ptr->cast<FEPrimitive>();
                if (pri == nullptr) 
                    continue;
                _primitives.push_back(pri);
            }
        }
        if (bits._hasBuffer)
        {
            uint    cnt     =   0;
            switch(bits._hasBuffer)
            {
            case 1: cnt     =   reader.readValue<uint8>(); break;
            case 2: cnt     =   reader.readValue<uint16>();break;
            case 3: cnt     =   reader.readValue<uint32>();break;
            }
            
            _buffers.reserve(cnt);
            for (uint i = 0; i < cnt; i++)
            {
                FEAttribute attr    =   reader.readValue<FEAttribute>();
                auto        ptr     =   FEObjectHelper::readObject(_ctx,reader,version,ctx);
                if (ptr == nullptr)
                    continue;
                auto        obj     =   ptr->cast<FEBuffer>();
                if (obj == nullptr) 
                    continue;
                FEAttribyteBuffer   data(_ctx,obj,attr);
                _buffers.push_back(data);
            }
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

    void    FEMaterialPBR::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const 
    {
        UNUSED(writer,chunk,version,ctx);
        writer.write(_pbr._value._emissive);
        writer.write(_pbr._value._diffuse);
        writer.write(_pbr._value._spacular);
        writer.write(_pbr._value._roughness);
        writer.write(_pbr._value._metallic);
        writer.write(_pbr._value._transmission);
        writer.write(_pbr._value._iri);
        writer.write(_pbr._value._clearcoat);
        writer.write(_pbr._value._volume);
    }
    void    FEMaterialPBR::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) 
    {
        UNUSED(reader,chunk,version,ctx);
        reader.read(_pbr._value._emissive);
        reader.read(_pbr._value._diffuse);
        reader.read(_pbr._value._spacular);
        reader.read(_pbr._value._roughness);
        reader.read(_pbr._value._metallic);
        reader.read(_pbr._value._transmission);
        reader.read(_pbr._value._iri);
        reader.read(_pbr._value._clearcoat);
        reader.read(_pbr._value._volume);

        _pbr.update();
    }
}
