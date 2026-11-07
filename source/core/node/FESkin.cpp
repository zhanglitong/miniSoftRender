#include    "../../inc/node/FESkin.hpp"
#include    "../../inc/stream/FEReader.hpp"
#include    "../../inc/stream/FEWriter.hpp"

namespace   FE
{
    /// ==================== FESkinData:骨骼数据序列化 ====================
    /// 关节是场景节点,由节点树自己保存;这里只记录关节对象 id + inverseBind。
    /// 反序列化时节点树尚未读取,先从上下文查;查不到的 id 记录到 _pendingJoints,
    /// 由 FESkinData::resolvePending 在节点树读完后再回填。
    void    FESkinData::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const
    {
        UNUSED(version,ctx);

        const   size_t  count       =   _bones.size();
        assert(count <= 0xFFFF);
        uint16          jointCount  =   uint16(count);
        FEChunkBits     bits(chunk._flags);
        bits._bit0      =   jointCount > 0 ? 1 : 0;
        chunk._flags    =   bits._value;

        if (bits._bit0)
        {
            writer.write(jointCount);
            for (auto& bone : _bones)
            {
                FEUuid  jointId =   bone.node ? bone.node->objectId() : FEUuid::zero();
                writer.write(jointId);
                writer.write(bone.inverseBind);
            }
        }
        writer.write(_worldRot);
    }
    void    FESkinData::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx)
    {
        UNUSED(version);
        FEChunkBits     bits(chunk._flags);

        _bones.clear();
        _pendingJoints.clear();
        if (bits._bit0)
        {
            uint16  jointCount  =   0;
            reader.read(jointCount);
            _bones.resize(jointCount);
            _pendingJoints.assign(jointCount,FEUuid::zero());
            for (uint16 i = 0;i < jointCount;++i)
            {
                FEUuid  jointId =   FEUuid::zero();
                reader.read(jointId);
                reader.read(_bones[i].inverseBind);

                Object  object  =   (jointId == FEUuid::zero())
                                    ?   nullptr
                                    :   ctx.query(jointId,nullptr,FESerializeCtx::O_Query).first;
                FENode* joint   =   (object == nullptr) ? nullptr : object->cast<FENode>();
                if (joint != nullptr)
                    _bones[i].node  =   Node(joint);
                else
                    _pendingJoints[i]   =   jointId;
            }
        }
        reader.read(_worldRot);
    }
    size_t  FESkinData::resolveJoints(const std::unordered_map<OBJId,FENode*>& nodes)
    {
        const   size_t  count   =   std::min(_bones.size(),_pendingJoints.size());
        size_t          resolved    =   0;
        for (size_t i = 0;i < count;++i)
        {
            if (_pendingJoints[i] == FEUuid::zero())
                continue;
            auto    itr =   nodes.find(_pendingJoints[i]);
            if (itr == nodes.end() || itr->second == nullptr)
                continue;
            _bones[i].node      =   Node(itr->second);
            _pendingJoints[i]   =   FEUuid::zero();
            ++resolved;
        }
        return  resolved;
    }
    static  void    collectNodes(const Node& node,std::unordered_map<OBJId,FENode*>& nodes)
    {
        if (node == nullptr)
            return;
        nodes[node->objectId()] =   const_cast<FENode*>(node.get());
        for (auto& child : node->children())
            collectNodes(child,nodes);
    }
    size_t  FESkinData::resolvePending(const Nodes& roots)
    {
        std::unordered_map<OBJId,FENode*>   nodes;
        for (auto& root : roots)
            collectNodes(root,nodes);
        if (nodes.empty())
            return  0;

        size_t  resolved    =   0;
        for (auto& var : nodes)
        {
            FENode* node    =   var.second;
            if (node == nullptr)
                continue;
            for (FESkin* skin : node->objects<FESkin>())
            {
                if (skin == nullptr || skin->data() == nullptr)
                    continue;
                resolved    +=  skin->data()->resolveJoints(nodes);
            }
        }
        return  resolved;
    }

    /// ==================== FESkin:蒙皮组件序列化 ====================
    size_t  FESkin::queryDepends(ObjectUSet& uSet) const
    {
        auto    count   =   uSet.size();
        if (_data != nullptr)
            uSet.emplace(Object(const_cast<FESkinData*>(_data.get())));
        FEObject::queryDepends(uSet);
        return  uSet.size() - count;
    }
    void    FESkin::serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const
    {
        UNUSED(version,ctx);
        FEChunkBits     bits(chunk._flags);
        bits._bit0      =   _data ? 1 : 0;
        bits._bit1      =   _cpuSkin ? 1 : 0;
        chunk._flags    =   bits._value;

        if (bits._bit0)
        {
            auto    result  =   writer.write(_data->objectId());
            UNUSED(result);
        }
    }
    void    FESkin::deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx)
    {
        UNUSED(version);
        FEChunkBits     bits(chunk._flags);

        _data       =   nullptr;
        _cpuSkin    =   bits._bit1 != 0;
        if (bits._bit0)
        {
            FEUuid  dataId  =   FEUuid::zero();
            reader.read(dataId);
            Object  object  =   ctx.query(dataId,nullptr,FESerializeCtx::O_Query).first;
            _data   =   object ? object->cast<FESkinData>() : nullptr;
        }
        /// 蒙皮矩阵是每帧运行的缓存,加载后由骨骼动画系统重新生成
        _matrices.clear();
    }
}
