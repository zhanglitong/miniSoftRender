#include    "../../inc/node/FESkin.hpp"
#include    "../../inc/stream/FEReader.hpp"
#include    "../../inc/stream/FEWriter.hpp"

namespace   FE
{
    /// ==================== FESkinData:骨骼数据序列化 ====================
    /// 关节是场景节点,由节点树自己保存;这里只记录关节对象 id + inverseBind。
    /// 反序列化时节点树尚未读取,查不到的 id 登记到 FESerializeCtx,
    /// 由读取器在对象表完整后统一回填。
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
        if (bits._bit0)
        {
            uint16  jointCount  =   0;
            reader.read(jointCount);
            _bones.resize(jointCount);
            for (uint16 i = 0;i < jointCount;++i)
            {
                FEUuid  jointId =   FEUuid::zero();
                reader.read(jointId);
                reader.read(_bones[i].inverseBind);

                if (jointId == FEUuid::zero())
                    continue;
                Object  object  =   ctx.query(jointId,nullptr,FESerializeCtx::O_Query).first;
                FENode* joint   =   (object == nullptr) ? nullptr : object->cast<FENode>();
                if (joint != nullptr)
                {
                    _bones[i].node  =   Node(joint);
                    continue;
                }
                /// 关节节点作为节点树的一部分写在依赖之后,当前一定查不到:
                /// 登记回填函数,由读取器读完所有对象后统一解析
                FESkinData* self    =   this;
                ctx.bindLater(jointId,[self,i](Object target)->bool
                {
                    FENode* node    =   (target == nullptr) ? nullptr : target->cast<FENode>();
                    if (node == nullptr || i >= self->_bones.size())
                        return  false;
                    self->_bones[i].node    =   Node(node);
                    return  true;
                });
            }
        }
        reader.read(_worldRot);
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
