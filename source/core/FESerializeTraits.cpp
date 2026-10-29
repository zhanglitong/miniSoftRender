
#include    "../inc/FEReaderHelper.hpp"
#include    "../inc/FEWriterHelper.hpp"
#include    "../inc/FEObjectHelper.hpp"
#include    "../inc/FENodeHelper.hpp"
#include    "../inc/FEMeshLibrary.hpp"
#include    "../inc/FEBuffer.hpp"
#include    "../inc/FEGeometryLibrary.hpp"
#include    "../inc/FENotify.hpp"
#include    "../inc/material/FEMaterialLibrary.hpp"
#include    "../inc/graphic/FETexture.h"
#include    "../inc/FEImage.h"
#include    "graphic/FETextureHelper.hpp"
#include    <cstring>

namespace   FE
{

    void    FEBuffer::serializeTraits(FEWriter& writer,FEChunkInf& ,uint version,FESerializeCtx& ctx) const
    {
        UNUSED(ctx);
        UNUSED(version);
        uint64  length  =   _cInfo._buffer.size();
        writer.write(length);
        writer.write(_cInfo._offset);
        writer.write(_cInfo._range);
        writer.writeBuffer(_cInfo._buffer.data(),length);

    }
    void    FEBuffer::deserializeTraits(FEReader& reader,const FEChunkInf&,uint version,FESerializeCtx& ctx) 
    {
        UNUSED(ctx);
        UNUSED(version);
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
        /// 值为默认时不写入,减少体积
        bits._hasScale      =   _local._scale     ==  float3(1,1,1)  ? 0 : 1;
        bits._hasTrans      =   _local._position  ==  real3(0,0,0)   ? 0 : 1;
        bits._hasRotate     =   _local.rotation() ==  quatf(1,0,0,0) ? 0 : 1;
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

        
        if (bits._hasScale )    writer.write(_local._scale);
        if (bits._hasTrans )    writer.write(_local._position);
        if (bits._hasRotate )   writer.write(_local._rotation);
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
        if (bits._hasScale )    reader.read(_local._scale);
        if (bits._hasTrans )    reader.read(_local._position);
        if (bits._hasRotate )   reader.read(_local._rotation);
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
                /// 是否绑定参数化几何体,1时只写几何体id与属性组合,顶点数据由加载时重建
                uint16  _hasGeometry    :1;
            };
            uint16  _value;
        };
        MeshChunkBit(uint16 flags = 0)
        {
            _hasPrimitive   =   flags>>0;
            _hasBuffer      =   flags>>2;
            _hasGeometry    =   flags>>4;
        }
    };
    void    FEMesh::serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const
    {
        UNUSED(ctx);
        UNUSED(version);

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

        /// 参数化mesh只记录几何体与属性组合,顶点数据加载时重建
        bits._hasGeometry   =   _geometry != nullptr ? 1 : 0;
        if (bits._hasGeometry)
        {
            bits._hasPrimitive  =   0;
            bits._hasBuffer     =   0;
        }

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

        if (bits._hasPrimitive)
        {
            for(auto var : _primitives)
            {
                var->serialize(writer,version,ctx);
            }
        }
        if (bits._hasBuffer)
        {
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
        if (bits._hasGeometry)
        {
            writer.write(_geometry->objectId());
            Attrs   attrs   =   inputs();
            writer.write<uint8>(uint8(attrs.size()));
            for (auto& var : attrs)
            {
                writer.write<FEAttribute>(var);
            }
        }

    }
    void    FEMesh::deserializeTraits(FEReader& reader,const FEChunkInf& chk,uint version,FESerializeCtx& ctx)
    {
        UNUSED(version);

        MeshChunkBit    bits(chk._flags);

        _buffers.clear();
        _primitives.clear();
        _geometry   =   nullptr;

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
        if (bits._hasGeometry)
        {
            FEUuid  uuid;
            reader.read(uuid);
            uint8   attrCnt =   reader.readValue<uint8>();
            Attrs   inputs;
            inputs.reserve(attrCnt);
            for (uint8 i = 0; i < attrCnt; ++ i)
            {
                inputs.push_back(reader.readValue<FEAttribute>());
            }
            auto            result  =   ctx.query(uuid,nullptr,FESerializeCtx::O_Query);
            FEGeometryParam* geo    =   result.first ? result.first->as<FEGeometryParam>() : nullptr;
            Mesh            built   =   geo ? geo->triangular(inputs) : nullptr;
            if (built)
            {
                _primitives =   built->primitives();
                _buffers    =   built->buffers();
                _geometry   =   geo;
                updateAabb();
            }
        }
        if (!_buffers.empty())
        {
            updateAabb();
        }
    }

    namespace
    {
        /// <summary>
        /// 材质贴图扩展魔数:标记后续为"来源对象引用"格式。
        ///   PBR2:只写来源对象 id(采样参数使用默认值)
        ///   PBR3:来源对象 id + 采样参数(mag/min/address) + mip 策略
        /// v1 工程文件此处是 PBRTexParams._flags(uint),不会与这些值相同
        /// </summary>
        constexpr   uint32  kTexSlotMagicV2 =   0x32524250u;   /// "PBR2"
        constexpr   uint32  kTexSlotMagicV3 =   0x33524250u;   /// "PBR3"
        /// <summary>
        /// 写一个贴图槽位:1 表示后面跟随来源对象 id(FEFileBuffer 压缩字节 / FEImage)
        /// 与采样参数;0 表示该槽位使用默认纹理,无需写入任何数据。
        /// mips:0/1..255,1 表示不使用 mipmap,其余为实际级数
        /// </summary>
        void    writeTextureSlot(FEWriter& writer,const TexPtr& tex)
        {
            const   Object  source  =   (tex != nullptr) ? tex->source() : Object();
            writer.write(uint8(source != nullptr ? 1 : 0));
            if (source == nullptr)
                return;
            writer.write(source->objectId());
            const auto& ci  =   tex->cInfo();
            writer.write(uint8(ci._magFilter));
            writer.write(uint8(ci._minFilter));
            writer.write(uint8(ci._addressMode));
            writer.write(uint8(ci._mips > 0 && ci._mips < 256 ? ci._mips : 1));
        }
        /// <summary>
        /// 读取一个贴图槽位:按来源对象 id 查询(依赖已先写入文件)并重建纹理;
        /// 返回 nullptr 表示沿用默认纹理。
        /// hasParams 为 false 时是 PBR2 旧格式(无采样参数),按默认值重建
        /// </summary>
        TexPtr  readTextureSlot(FEReader& reader,FEContext& feCtx,FESerializeCtx& ctx,bool hasParams)
        {
            uint8   present =   0;
            reader.read(present);
            if (present == 0)
                return  nullptr;
            FEUuid  id;
            reader.read(id);

            FEFilter        magFilter   =   FILTER_LINEAR;
            FEFilter        minFilter   =   FILTER_LINEAR;
            FEAddressMode   addressMode =   ADDRESS_REPEAT;
            uint            mips        =   0;
            if (hasParams)
            {
                uint8   mag =   1;
                uint8   min =   1;
                uint8   addr=   0;
                uint8   mip =   0;
                reader.read(mag);
                reader.read(min);
                reader.read(addr);
                reader.read(mip);
                magFilter   =   (mag == uint8(FILTER_NEAREST)) ? FILTER_NEAREST : FILTER_LINEAR;
                minFilter   =   (min == uint8(FILTER_NEAREST)) ? FILTER_NEAREST : FILTER_LINEAR;
                addressMode =   (addr <= uint8(ADDRESS_CLAMP_TO_BORDER)) ? FEAddressMode(addr) : ADDRESS_REPEAT;
                mips        =   mip;
            }

            auto    result  =   ctx.query(id,nullptr,FESerializeCtx::O_Query);
            if (result.first == nullptr)
                return  nullptr;
            return  FETextureHelper::createFromSource(feCtx,Object(result.first),magFilter,minFilter,addressMode,mips);
        }
        /// <summary>
        /// v1 兼容:紧随存在标记的是创建信息 + 完整 mip 链原始像素(无来源对象)。
        /// 按同一布局重建 CPU 图像作为新来源,保证再次保存时不丢贴图
        /// </summary>
        TexPtr  readTextureSlotV1(FEReader& reader,FEContext& feCtx)
        {
            uint8   present =   0;
            reader.read(present);
            if (present == 0)
                return  nullptr;

            uint32          width   =   0;
            uint32          height  =   0;
            FEFormat        format  =   FMT_NONE;
            uint32          mips    =   1;
            uint8           cube    =   0;
            FEFilter        magFilter   =   FILTER_LINEAR;
            FEFilter        minFilter   =   FILTER_LINEAR;
            FEAddressMode   addressMode =   ADDRESS_REPEAT;
            uint64          nByte   =   0;
            reader.read(width);
            reader.read(height);
            reader.read(format);
            reader.read(mips);
            reader.read(cube);
            reader.read(magFilter);
            reader.read(minFilter);
            reader.read(addressMode);
            reader.read(nByte);
            std::vector<uint8>  pixels;
            pixels.resize(size_t(nByte));
            if (nByte > 0)
                reader.readBuffer(pixels.data(),nByte);

            const   uint32  pixelByte   =   FEFormatHelper::sizeOf(format);
            if (pixelByte == 0 || width == 0 || height == 0)
                return  nullptr;

            ImagePtr    image   =   new FEImage(feCtx);
            FEImage::CreateInfo     ci;
            ci._extent  =   uint3(width,height,1);
            ci._format  =   format;
            ci._layers  =   uint16(cube ? 6 : 1);
            ci._levels  =   uint16(mips ? mips : 1);
            ci._align   =   4;
            if (!image->create(ci))
                return  nullptr;

            /// 层主序、层内 mip 顺序,逐 mip 逐行拷贝到按 _align 对齐的图像行
            size_t  offset  =   0;
            for (uint16 l = 0; l < ci._layers; ++l)
            {
                for (uint16 m = 0; m < ci._levels; ++m)
                {
                    const   uint32  lw      =   (width >> m) ? (width >> m) : 1;
                    const   uint32  lh      =   (height >> m) ? (height >> m) : 1;
                    const   size_t  rowByte =   size_t(lw) * pixelByte;
                    auto            plane   =   image->plane(m,l);
                    for (uint32 y = 0; y < lh && offset + rowByte <= pixels.size(); ++y)
                    {
                        memcpy(plane.dataOffset<uint8>(y,0),pixels.data() + offset,rowByte);
                        offset  +=  rowByte;
                    }
                }
            }

            TexPtr  tex =   FETextureHelper::create(feCtx,*image,magFilter,minFilter,addressMode);
            if (tex != nullptr)
                tex->setSource(Object(image.get()));
            return  tex;
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

        /// <summary>
        /// 贴图扩展标记:bit12 清 0 表示后续写入了贴图参数与各槽位像素。
        /// 旧文件该位保持默认 1,读取时直接跳过,保证向后兼容
        /// </summary>
        FEChunkBits bits(chunk._flags);
        bits._bit12     =   0;
        chunk._flags    =   bits._value;

        /// <summary>
        /// 贴图参数(normalScale/AO 强度/自发光强度/KHR 扩展/UV 变换)与管线变体,
        /// 前面写魔数标记"来源对象引用"格式(来源对象由 queryDepends 提供,已先写入文件)
        /// </summary>
        writer.write(kTexSlotMagicV3);
        writer.write(_texParams._value);
        writer.write(uint8(_doubleSided ? 1 : 0));
        writer.write(uint8(_alphaBlend ? 1 : 0));

        /// <summary>
        /// 六个可编辑贴图槽位:baseColor / metallicRoughness / normal / AO / emissive / 虹彩厚度
        /// </summary>
        writeTextureSlot(writer,_baseColorMap);
        writeTextureSlot(writer,_mrMap);
        writeTextureSlot(writer,_normalMap);
        writeTextureSlot(writer,_occlusionMap);
        writeTextureSlot(writer,_emissiveMap);
        writeTextureSlot(writer,_iriThicknessMap);
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

        /// <summary>
        /// 旧工程文件没有贴图扩展:bit12 为 1 时直接返回,只恢复标量参数
        /// </summary>
        const   FEChunkBits bits(chunk._flags);
        if (bits._bit12 != 0)
            return;

        /// <summary>
        /// v2 以魔数开头(其后为参数与来源对象 id);
        /// v1 此处是 PBRTexParams._flags,把已读出的 4 字节当作首字段并补齐其余字节
        /// </summary>
        uint32  magic   =   0;
        reader.read(magic);
        /// v3=来源引用+采样参数;v2=来源引用(默认采样);否则为 v1 内嵌像素
        const   bool    v3      =   (magic == kTexSlotMagicV3);
        const   bool    v2      =   (magic == kTexSlotMagicV2);
        const   bool    refFmt  =   (v3 || v2);
        if (refFmt)
        {
            reader.read(_texParams._value);
        }
        else
        {
            _texParams._value._flags    =   magic;
            reader.readBuffer(reinterpret_cast<uint8*>(&_texParams._value) + sizeof(uint),
                              sizeof(PBRTexParams) - sizeof(uint));
        }

        uint8   sided   =   0;
        uint8   blend   =   0;
        reader.read(sided);
        reader.read(blend);

        if (refFmt)
        {
            setBaseColorMap(readTextureSlot(reader,_ctx,ctx,v3));
            setMetallicRoughnessMap(readTextureSlot(reader,_ctx,ctx,v3));
            setNormalMap(readTextureSlot(reader,_ctx,ctx,v3));
            setOcclusionMap(readTextureSlot(reader,_ctx,ctx,v3));
            setEmissiveMap(readTextureSlot(reader,_ctx,ctx,v3));
            setIriThicknessMap(readTextureSlot(reader,_ctx,ctx,v3));
        }
        else
        {
            setBaseColorMap(readTextureSlotV1(reader,_ctx));
            setMetallicRoughnessMap(readTextureSlotV1(reader,_ctx));
            setNormalMap(readTextureSlotV1(reader,_ctx));
            setOcclusionMap(readTextureSlotV1(reader,_ctx));
            setEmissiveMap(readTextureSlotV1(reader,_ctx));
            setIriThicknessMap(readTextureSlotV1(reader,_ctx));
        }

        _texParams.update();
        if (blend != 0)
            setAlphaBlend(true);
        if (sided != 0)
            setDoubleSided(true);
    }
}
