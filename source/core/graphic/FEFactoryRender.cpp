
#include    "../inc/graphic/FEFactoryRender.hpp"
#include    "../inc/graphic/FEGPUBuffer.h"
#include    "../inc/graphic/FECmdBuffer.h"
#include    "../inc/graphic/FEInstance.h"
#include    "../inc/graphic/FEScene.h"

namespace   FE
{
    using   GroupNode   =   FEFactoryRender::GroupNode;
    using   VBinds      =   FEFactoryRender::VBinds;

    size_t  GroupNode::addNode(Node object)
    {
        auto    itr =   std::lower_bound(_objects.begin(), _objects.end(), object);
        if (itr != _objects.end() && *itr == object)
            return  0;
        _objects.insert(itr, object);
        _aabb.merge(object->globalAabb());

        object->flags().removeFlag(FENode::FLAG_PROP_TRANS | FENode::FLAG_PROP_SCALE | FENode::FLAG_PROP_ROT);

        flags().addFlag(FLAG_ADD_CHILD);

        return  1;
    }
    size_t  GroupNode::addNodes(Nodes::iterator nBegin, Nodes::iterator eEnd)
    {
        /// 1. 记录原数组大小
        size_t  oldSize =   _objects.size();

        /// 2. 将新元素追加到末尾（会触发扩容，但只扩容一次）
        _objects.insert(_objects.end(), nBegin, eEnd);

        /// 3. 对新追加的部分进行排序（O(M log M)）
        auto    oldEnd  =   _objects.begin() + oldSize;
        std::sort(oldEnd, _objects.end());

        /// 4. 原地归并两个有序段：[begin, old_end) 和 [old_end, end)
        ///    这是关键，直接利用原数组空间，不额外创建新 vector
        std::inplace_merge(_objects.begin(), oldEnd, _objects.end());
        for (auto itr = nBegin; itr != eEnd; ++ itr )
        {
            _aabb.merge((*itr)->globalAabb());
            (*itr)->flags().removeFlag(FENode::FLAG_PROP_TRANS | FENode::FLAG_PROP_SCALE | FENode::FLAG_PROP_ROT);
        }
            
        flags().addFlag(FLAG_ADD_CHILD);

        return  _objects.size() - oldSize;
    }

    size_t  GroupNode::addNodes(Nodes& objects)
    {
        return  addNodes(objects.begin(),objects.end());
    }
    size_t  GroupNode::removeNode(Node object)
    {
        size_t  nOld    =   _objects.size();
        /// 删除逻辑：如果当前值在 deletes 里能找到，就删除(返回 true)
        auto    newEnd  =   std::remove_if(_objects.begin(), _objects.end(), [&](const Node& value) 
        {
            return  object == value;
        });
        _objects.erase(newEnd, _objects.end());

        if (_objects.empty())
            _aabb   =   aabb3dr();
        for (auto& object : _objects)
            _aabb.merge(object->globalAabb());

        flags().addFlag(FLAG_REMOVE_CHILD);

        return  _objects.size() - nOld;
    }
    size_t  GroupNode::removeNodes(Nodes& deletes)
    {
        size_t  nOld    =   _objects.size();
        /// 先排序，后续查找速度快
        std::sort(deletes.begin(),deletes.end());
        /// 删除逻辑：如果当前值在 deletes 里能找到，就删除(返回 true)
        auto    newEnd  =   std::remove_if(_objects.begin(), _objects.end(), [&](const Node& object) 
        {
            return  std::binary_search(deletes.begin(), deletes.end(), object);
        });
        _objects.erase(newEnd, _objects.end());

        if (_objects.empty())
            _aabb   =   aabb3dr();
        for (auto& object : _objects)
            _aabb.merge(object->globalAabb());

        flags().addFlag(FLAG_REMOVE_CHILD);

        return  _objects.size() - nOld;
    }

    size_t  GroupNode::nodePropChanged(Node node)
    {
        auto    itr = std::lower_bound(_objects.begin(), _objects.end(), node);
        if (itr == _objects.end())
        {
            return  0;
        }
        else if((*itr) == node)
        {
            if (node->flags().hasFlag(FENode::FLAG_PROP_TRANS))
                flags().addFlag(FENode::FLAG_PROP_TRANS);
            if (node->flags().hasFlag(FENode::FLAG_PROP_SCALE))
                flags().addFlag(FENode::FLAG_PROP_SCALE);
            if (node->flags().hasFlag(FENode::FLAG_PROP_ROT))
                flags().addFlag(FENode::FLAG_PROP_ROT);
            return  1;
        }
        else
        {
            return  0;
        }
    }

    size_t  GroupNode::nodeMeshChanged(Node node)
    {
        return  0;
    }

    void    GroupNode::resetFlags()
    {
        flags() .removeFlag(FLAG_ADD_CHILD)
                .removeFlag(FLAG_REMOVE_CHILD)
                .removeFlag(FENode::FLAG_PROP_ROT)
                .removeFlag(FENode::FLAG_PROP_TRANS)
                .removeFlag(FENode::FLAG_PROP_SCALE);
    }
    size_t  FEFactoryRender::addNode(Node node)
    {
        uint    cnt =   countNode(node);
        Nodes   results;
        results.reserve(cnt);
        collectNode(node,results);
        return  addNodesImpl(results);
    }
    size_t  FEFactoryRender::addNodes(Nodes& nodes)
    {
        uint    cnt =   0;
        Nodes   results;
        for (auto node : nodes)
        {
            cnt +=  countNode(node);
        }
        nodes.reserve(cnt);
        for (auto node : nodes)
        {
            collectNode(node,results);
        }
        return  addNodesImpl(results);
    }

    size_t  FEFactoryRender::addNodes(Nodes::iterator nBegin,Nodes::iterator nEnd)
    {
        Nodes   results(nBegin,nEnd);
        return  addNodesImpl(results);
    }

    size_t  FEFactoryRender::addNodesImpl(Nodes& nodes)
    {
        size_t  result  =   0;
        /// 按照材质排序
        /// 同一个材质的节点可以按照批次直接加入到对相应的节点组中
        /// 性能比单个插入好很多
        std::sort(nodes.begin(),nodes.end(),[](const Node& left,const Node& right)
        {
            return  left->material().get() < right->material().get();
        });
        GroupNode*  pGroup      =   nullptr;
        Material    mat         =   nullptr;
        auto        itrStart    =   nodes.begin();
        for (auto itr = itrStart; itr != nodes.end(); ++ itr )
        {   
            auto    node    =   *itr;
            assert(node->material() != nullptr);
            if (node->material() != mat)
            {
                /// 说明切换了材质
                /// 则把相同材质的数据加入到一个组中
                if ( mat!= nullptr && pGroup)
                    pGroup->addNodes(itrStart,itr);
                /// 更新游标记录第一个发生变化的位置
                itrStart    =   itr;
                mat         =   node->material();
                pGroup      =   queryOrCreateGroup(mat);
            }
        }
        if (pGroup)
        {
            pGroup->addNodes(itrStart,nodes.end());
        }
        if (result)
            flags().addFlag(FLAG_ADD_CHILD);
        return  result;
    }

    size_t  FEFactoryRender::removeNode(Node node)
    {
        auto    mat     =   node->material();
        if (mat == nullptr)
            return  0;
        Mesh    mesh    =   node->mesh(); 
        if (mesh == nullptr)
            return  0;
        auto    group   =   queryOrCreateGroup(mat);
        return  group->removeNode(node);
    }

    size_t  FEFactoryRender::removeNodes(Nodes& nodes) 
    {
        size_t  result  =   0;
        std::sort(nodes.begin(),nodes.end(),[](const Node& left,const Node& right)
        {
            return  left->material().get() < right->material().get();
        });
        GroupNode*      pGroup  =   nullptr;
        Material    mat     =   nullptr;
        for (auto node: nodes)
        {   
            if (node->material() == nullptr)
                continue;
            if (node->material() != mat)
            {
                mat     =   node->material();
                pGroup  =   queryOrCreateGroup(mat);
            }
            result  +=  pGroup->removeNode(node);
        }
        if (result)
            flags().addFlag(FLAG_REMOVE_CHILD);
        return  result;
    }

    size_t  FEFactoryRender::nodePropChanged(Node node)
    {
        auto    grp =   queryGroup(node->material());
        if (grp == nullptr)
            return  0;
        auto    result  =   grp->nodePropChanged(node);
        if (result)
            flags().addFlag(FLAG_UPDATE);
        return  result;
            
    }

    void    FEFactoryRender::update(CMDPtr cmd)
    {
        updateImpl(cmd);
    }


    void    FEFactoryRender::render(CMDPtr cmd)
    {
        uint        count   =   uint(_vboVertexs.size());

        if (_groupNode.empty())
            return;
        Material    mat     =   _groupNode.front()->_mat; 
        auto        pl      =   mat->pipeline(_key._primitive)->as<FEGPipeline>();
        auto&       binds   =   pl->cInfo()._binds;

        cmd->bindPipeline(pl);

        mat->appDynamicState(cmd,_key._primitive);

        for (auto& var: _vboVertexs)
        {
            cmd->bindVBO(var._binding,uint(var._vbos.size()),var._vbos,{});
        }
        for (auto& var: _vboInstances)
        {
            cmd->bindVBO(var._binding,uint(var._vbos.size()),var._vbos,{});
        }
        switch(_key._drawType)
        {
        case EDrawType::DRAW_ELEMENT_UINT8:
            cmd->bindIBO(_ibo,0,FEIndexType::INDEX_UINT8);
            break;
        case EDrawType::DRAW_ELEMENT_UINT16:
            cmd->bindIBO(_ibo,0,FEIndexType::INDEX_UINT16);
            break;
        case EDrawType::DRAW_ELEMENT_UINT32:
            cmd->bindIBO(_ibo,0,FEIndexType::INDEX_UINT32);
            break;
        }
        for (auto& grp : _groupNode)
        {
            FECmdBuffer::DSetBind   binds   =   {};
            binds.dSets         =   grp->_mat->dsets();
            binds.firstSet      =   0;
            binds.offsetCount   =   0;
            binds.offsets       =   nullptr;
            binds.plBindPoint   =   PL_GRAPIC;
            binds.plLayout      =   pl->nativeLayout();
            cmd->bindDescriptors(binds);
            uint64  offset      =   grp->start() * sizeof(FECmdIndex);
            uint64  indxOff     =   grp->start();
            switch(_key._drawType)
            {
            case EDrawType::DRAW_ARRAY:
                cmd->drawArrayIndirect(_indirect,   offset, grp->count(),sizeof(FECmdIndex));
                break;
            case EDrawType::DRAW_ELEMENT_UINT8:
                cmd->drawIndexedIndirect(_indirect, offset, grp->count(),sizeof(FECmdIndex));
                break;
            case EDrawType::DRAW_ELEMENT_UINT16:
                cmd->drawIndexedIndirect(_indirect, offset, grp->count(),sizeof(FECmdIndex));
                break;
            case EDrawType::DRAW_ELEMENT_UINT32:
                cmd->drawIndexedIndirect(_indirect, offset, grp->count(),sizeof(FECmdIndex));
                break;
            }
        }
    }

    void    FEFactoryRender::destroy()
    {
        _groupNode.clear();
        _vboVertexs.clear();
        _vboInstances.clear();
        _ibo        =   nullptr;
        _indirect   =   nullptr;
    }

    void    FEFactoryRender::updateImpl(CMDPtr cmd)
    {
        if (_groupNode.empty())
            return;
       
        bool    needUpdateVBO   =   false;
        bool    needUpdateIBO   =   false;
        bool    needUpdateInst  =   false;
        bool    needUpdateITO   =   false;
        auto    updateVBO       =   [&]()
        {
            for (auto& var : _groupNode)
            {
                needUpdateVBO   |=  var->flags().hasFlag(FLAG_ADD_CHILD);
                needUpdateVBO   |=  var->flags().hasFlag(FLAG_REMOVE_CHILD);

                needUpdateVBO   |=  needUpdateVBO;

                needUpdateITO   |=  needUpdateVBO;

                needUpdateInst  |=  needUpdateVBO;
                needUpdateInst  |=  var->flags().hasFlag(FENode::FLAG_PROP_ROT);  
                needUpdateInst  |=  var->flags().hasFlag(FENode::FLAG_PROP_TRANS); 
                needUpdateInst  |=  var->flags().hasFlag(FENode::FLAG_PROP_SCALE); 
            }
        };

        updateVBO();

        if (needUpdateVBO || needUpdateIBO)
        {
            MeshUSet    meshSet;
            collectMesh(meshSet,_groupNode);
            _vboVertexs     =   buildVertexVBOs(meshSet);
            _ibo            =   buildVertexIBO(meshSet);
        }
        /// 只是更新instance,优化处理
        if (needUpdateITO && needUpdateVBO && needUpdateInst)
        {
            _vboInstances   =   buildInstanceVBOs();
        }
        else if(needUpdateInst) 
        {
            updateInstanceVBOs();
        }
        if (needUpdateITO)
        {
            _indirect       =   buildIndirect();
        }
        /// 清除标记
        for (auto& var : _groupNode)
        {
            var->flags().removeFlag(FLAG_ADD_CHILD)
                        .removeFlag(FLAG_REMOVE_CHILD)
                        .removeFlag(FENode::FLAG_PROP_ROT)
                        .removeFlag(FENode::FLAG_PROP_TRANS)
                        .removeFlag(FENode::FLAG_PROP_SCALE);
        }
    }
   
    uints   bufferIndexs(Mesh mesh,const InputDescs& inputs)
    {   
        uints   indexs;
        auto&   buffers =   mesh->buffers();
        for (size_t i = 0 ;i < buffers.size(); ++ i)
        {
            for (auto& input : inputs)
            {
                if (input.slot == buffers[i].attr().slot())
                {
                    indexs.push_back(uint(i));
                }
            }
        }
        return  indexs;
    }
    VBinds  FEFactoryRender::buildVertexVBOs(MeshUSet& meshSet)
    {
        if (meshSet.empty())
            return  {};
        Material    mat     =   _groupNode.front()->_mat; 
        auto        pl      =   mat->pipeline(_key._primitive)->as<FEGPipeline>();
        auto&       binds   =   pl->cInfo()._binds;
        VBinds      vboCPUs;
        
        for (auto& bind : binds)
        {
            if (bind.inputRate == V_INPUT_INSTANCE)
                continue;
            auto    indexs  =   bufferIndexs(*meshSet.begin(),bind.inputs);
            VBO     cpuVBO  =   buildBuffer(meshSet,indexs);
            assert(cpuVBO != nullptr);
            if (cpuVBO == nullptr)
                continue;
            VBind   vBind   =   {};
            vBind._vbos.push_back(cpuVBO);
            vBind._binding  =   bind.binding;
            vboCPUs.emplace_back(vBind);
        }
        VBinds      vboGPUs(vboCPUs.size());
        for (size_t i = 0; i < vboCPUs.size(); i++)
        {
            vboGPUs[i]._binding =   vboCPUs[i]._binding;
            for (size_t v = 0; v < vboCPUs[i]._vbos.size(); ++v)
            {
                auto    cpu     =   vboCPUs[i]._vbos[v];
                auto    gpu     =   _device.createVBO();
                gpu->create({cpu->cInfo()._length,DEVICE_DEFAULT_BIT});
                vboGPUs[i]._vbos.emplace_back(gpu);
            }
        }
        /// 更新每一个mesh数据在整个大缓冲区中的偏移
        uint    nCount  =   0;
        for (auto var : meshSet)
        {
            auto&   buffers =   var->buffers();
            auto&   buffer  =   buffers.front();
            
            auto&   pris    =   var->primitives();
            for (auto pri : pris)
            {
                if(pri->type() == _key._drawType && pri->primitive() == _key._primitive)
                {
                    pri->setFactory(this);
                    pri->setVertexOffset(nCount);
                }
            }
            /// 设置mesh 所属工厂
            nCount  +=  buffer.count();
        }
        /// 拷贝数据到显卡
        copyVBinds(vboCPUs,vboGPUs);
        return  vboGPUs;
    }


    IBO     FEFactoryRender::buildVertexIBO(MeshUSet& meshSet)
    {
        uint2   count   =   indexCount(meshSet,_key._primitive);
        if (count.x == 0)
            return  nullptr;
        IBO     cpu     =   _device.createIBO();
        if(!cpu->create({count.y,HOST_VISIBLE_BIT}))
            return  nullptr;
        uint8*  pDst    =   (uint8*)cpu->lock(count.y,0);
        uint    offset  =   0;
        switch (_key._drawType)
        {
        case FE::DRAW_ARRAY:
            break;
        case FE::DRAW_ELEMENT_UINT8:
            for (auto var : meshSet)
            {
                for (auto srcPri: var->primitives())
                {
                    if (srcPri->primitive() != _key._primitive)
                        continue;
                    auto    pri =   srcPri->as<FEDrawElementUint8>();
                    uint32  cnt =   pri->copyIndexTo<uint16>((uint16*)pDst,count.x);
                    pDst        +=  FE::alignUp<uint>(cnt * sizeof(uint16),4);
                    pri->setIndexOffset(offset);
                    offset      +=  cnt;
                }
            }
            break;
        case FE::DRAW_ELEMENT_UINT16:
            for (auto var : meshSet)
            {
                for (auto srcPri: var->primitives())
                {
                    if (srcPri->primitive() != _key._primitive)
                        continue;
                    auto    pri =   srcPri->as<FEDrawElementUint16>();
                    uint32  cnt =   pri->copyIndexTo<uint16>((uint16*)pDst,count.x);
                    pDst        +=  FE::alignUp<uint>(cnt * sizeof(uint16),4);
                    pri->setIndexOffset(offset);
                    offset      +=  cnt;
                }
            }
            break;
        case FE::DRAW_ELEMENT_UINT32:
            for (auto var : meshSet)
            {
                for (auto srcPri: var->primitives())
                {
                    if (srcPri->primitive() != _key._primitive)
                        continue;
                    auto    pri =   srcPri->as<FEDrawElementUint32>();
                    uint32  cnt =   pri->copyIndexTo<uint32>((uint32*)pDst,count.x);
                    pDst        +=  cnt * 4;
                    pri->setIndexOffset(offset);
                    offset      +=  cnt;
                }
            }
            break;
        default:
            break;
        }
        cpu->unlock();

        IBO     gpu     =   _device.createIBO();

        gpu->create({cpu->cInfo()._length, DEVICE_DEFAULT_BIT}); 

        auto    cmdPool =   _device.transferCmdPool();
        assert (cmdPool != nullptr);
        if (cmdPool != nullptr)
        {
            CMDPtr      cmd     =   cmdPool->createCmd();
            cmd->begin(true);
            cmd->copyBuffer(cpu,gpu,cpu->cInfo()._length,0,0);
            cmd->end();
            cmd->submit(_device.queueTransfer());
        }
        return  gpu;
    }

    auto    offfetIndex(const InputDescs& inputs)
    {
        uints   indexs;
        for (auto& input: inputs)
        {
            for (size_t i = 0 ;i < sizeof(FEInstanceHelper::instanceInputs) / sizeof(FEInstanceHelper::instanceInputs[0]); ++ i )
            {
                if (input.slot == FEInstanceHelper::instanceInputs[i].slot)
                    indexs.push_back(uint(i));
            }
        }
        return  indexs;
    };
    
    VBinds  FEFactoryRender::buildInstanceVBOs()
    {
        Material    mat     =   _groupNode.front()->_mat; 
        auto        pl      =   mat->pipeline(_key._primitive)->as<FEGPipeline>();
        auto&       binds   =   pl->cInfo()._binds;
        if (binds.empty())
            return  {};
        /// instance数量
        uint32      count   =   0;
        uint32      idxSize =   0;
        for (auto& var : _groupNode)
        {
            var->setStart(count);
            count   +=  (uint32)var->_objects.size();
        }
        VBinds      vboCPUs;
        for (auto& bind : binds)
        {   
            if (bind.inputRate == V_INPUT_VERTEX)
                continue;
            VBind       vBind       =   {};
            /// 找到需要更新属性的槽索引数组
            uints       indexs  =   offfetIndex(bind.inputs);
            /// 一个元素(顶点结构)的大小
            uint        stride  =   0;
            for (auto index : indexs)
            {
                stride  +=  FEInstanceHelper::instanceInputs[index].bytes;
            }
            uint64      length  =   count * stride;
            auto        vbo     =   _device.createVBO();
            vBind._vbos.emplace_back(vbo);
            vbo->create({length,HOST_VISIBLE_BIT});
            uint8*      pDst    =   (uint8*)vbo->lock(length,0);
            FEInstance  inst    =   {};
            uint8*      pSrc    =   (uint8*)&inst;
            uint        instId  =   0;
            for (auto& var : _groupNode)
            {
                for (auto& node: var->_objects)
                {   
                    inst._color         =   node->color();
                    inst._renderBits    =   node->renderBits().data();
                    inst.setTransform(node->globalTransform());
                    inst.setAabb(node->globalAabb());
                    inst.setInstance(instId++);
                    for (auto index: indexs)
                    {
                        auto    nByte   =   FEInstanceHelper::instanceInputs[index].bytes;
                        memcpy(pDst,pSrc + FEInstanceHelper::offsets[index],nByte);
                        pDst    +=  nByte;
                    }
                }
            }
                
            vbo->unlock();
            vBind._binding  =   bind.binding;
            vboCPUs.emplace_back(vBind);
        }

        VBinds      vboGPUs(vboCPUs.size());
        for (size_t i = 0; i < vboCPUs.size(); i++)
        {
            vboGPUs[i]._binding =   vboCPUs[i]._binding;
            for (size_t v = 0; v < vboCPUs[i]._vbos.size(); ++v)
            {
                auto    cpu     =   vboCPUs[i]._vbos[v];
                auto    gpu     =   _device.createVBO();
                gpu->create({cpu->cInfo()._length,DEVICE_DEFAULT_BIT});
                vboGPUs[i]._vbos.emplace_back(gpu);
            }
        }
        if (vboGPUs.empty())
            return  {};
        copyVBinds(vboCPUs,vboGPUs);
        return  vboGPUs;
    }

    void    FEFactoryRender::updateInstanceVBOs()
    {
        Material    mat     =   _groupNode.front()->_mat; 
        auto        pl      =   mat->pipeline(_key._primitive)->as<FEGPipeline>();
        auto&       binds   =   pl->cInfo()._binds;
        if (binds.empty())
            return;
        /// instance数量
        uint32  count   =   0;
        /// 统计有多少个instance 需要更新
        auto    flags   =   FENode::FLAG_PROP_TRANS | FENode::FLAG_PROP_SCALE | FENode::FLAG_PROP_ROT;
        for (auto& var : _groupNode)
        {
            if (!var->flags().containFlag(flags))
                continue;
            for (auto& node: var->_objects)
                count   +=  node->flags().containFlag(flags) ? 1:0;
        }
        VBinds      vboCPUs;
        for (auto& bind : binds)
        {   
            if (bind.inputRate == V_INPUT_VERTEX)
                continue;
            VBind       vBind       =   {};
            /// 找到需要更新属性的槽索引数组
            uints       indexs  =   offfetIndex(bind.inputs);
            /// 一个元素(顶点结构)的大小
            uint        stride  =   0;
            for (auto index : indexs)
            {
                stride  +=  FEInstanceHelper::instanceInputs[index].bytes;
            }
            uint64      length  =   count * stride;
            auto        vbo     =   _device.createVBO();
            BufferCopys regions;
            regions.reserve(count);
            vBind._vbos.emplace_back(vbo);

            vbo->create({length,HOST_VISIBLE_BIT});
            uint8*      pStart  =   (uint8*)vbo->lock(length,0);
            uint8*      pDst    =   pStart;
            FEInstance  inst    =   {};
            uint8*      pSrc    =   (uint8*)&inst;
            for (auto& var : _groupNode)
            {
                if (!var->flags().containFlag(flags))
                    continue;
                uint    instId  =   var->start();
                for (auto& node: var->_objects)
                {   
                    /// 检测标记
                    /// 使用完成后，需要清除标记
                    /// 节点有可能被其他工厂引用，清除标记会引起错误
                    /// 暂时没有考虑好
                    if (node->flags().containFlag(flags))
                    {
                        inst._color         =   node->color();
                        inst._renderBits    =   node->renderBits().data();

                        inst.setTransform(node->globalTransform());
                        inst.setAabb(node->globalAabb());
                        inst.setInstance(instId);
                        /// 保存要拷贝的目标位置
                        BufferCopy  copy;
                        copy.srcOffset  =   pDst - pStart;
                        copy.dstOffset  =   (instId * stride);
                        copy.size       =   stride;
                        regions.emplace_back(copy);
                        for (auto index: indexs)
                        {
                            auto    nByte   =   FEInstanceHelper::instanceInputs[index].bytes;
                            memcpy(pDst,pSrc + FEInstanceHelper::offsets[index],nByte);
                            pDst    +=  nByte;
                        }

                        node->flags().removeFlags(flags);
                    }
                    ++instId;
                }
            }
            vbo->unlock();
            vBind._binding  =   bind.binding;
            vBind._regions.emplace_back(regions);

            vboCPUs.emplace_back(vBind);
        }
        copyVBindsRegions(vboCPUs,_vboInstances);
    }
  
    ITO     FEFactoryRender::buildIndirect()
    {
        uint32      cmdCount    =   0;
        for (auto& var : _groupNode)
        {
            for (auto& node: var->_objects)
            {
                auto    mesh    =   node->mesh();
                auto&   pris    =   mesh->primitives();
                for (auto& pri : pris)
                {
                    if (pri->primitive() != _key._primitive)
                        continue;
                    else
                        ++cmdCount;
                }
            }
        }
        uint64      length  =   sizeof(FECmdIndex) * cmdCount;

        ITO         cpuBuf  =   _device.createITO(); 
        ITO         gpuBuf  =   _device.createITO(); 
        cpuBuf->create({length, HOST_VISIBLE_BIT});
        gpuBuf->create({length, DEVICE_DEFAULT_BIT}); 

        auto        pCmdIdx =   (FECmdIndex*)cpuBuf->lock(length,0);
        size_t      instId  =   0;
        size_t      cmdIdx  =   0;
        for (auto& var : _groupNode)
        {
            for (auto& node: var->_objects)
            {
                auto    mesh    =   node->mesh();
                auto&   pris    =   mesh->primitives();
                uint    priCnt  =   (uint)pris.size();
                for (uint i = 0 ;i < priCnt; ++ i)
                {
                    if (pris[i]->primitive() != _key._primitive)
                        continue;
                    mesh->drawCmd(pCmdIdx[cmdIdx],i,uint(instId));
                    ++cmdIdx;
                }
                ++instId;
            }
        }
        cpuBuf->unlock();

        auto    cmdPool =   _device.transferCmdPool();
        assert (cmdPool != nullptr);
        if (cmdPool != nullptr)
        {
            CMDPtr      cmd     =   cmdPool->createCmd();
            cmd->begin(true);
            cmd->copyBuffer(cpuBuf,gpuBuf,length,0,0);
            cmd->end();
            cmd->submit(_device.queueTransfer());
        }

        return  gpuBuf;
    }

    GroupNode*  FEFactoryRender::queryOrCreateGroup(Material mat)
    {
        auto    itr = std::lower_bound(_groupNode.begin(), _groupNode.end(), mat,[](const Group& left,const Material& right)
        {
            return  left->_mat.get() < right.get();
        });
        if (itr != _groupNode.end() && (*itr)->_mat == mat)
        {
            return  *(itr);
        }  
        Group    group   =   new GroupNode(_ctx);
        group->setMaterial(mat);
        _groupNode.insert(itr, group);
        return  group;
    }

    VBO     FEFactoryRender::buildBuffer(MeshUSet& meshSet,FEInputSlot slot)
    {
        if (meshSet.empty())
            return  nullptr;
        auto&       mesh    =   *meshSet.begin();
        auto&       buffers =   mesh->buffers();
        /// 输出每一个buffer对应的属性
        size_t      index   =   size_t(-1);
        for (size_t i = 0; i < buffers.size(); i++)
        {
            if (buffers[i].attr().slot() != slot)
                continue;
            index   =   i;
            break;
        }
        if (index == size_t(-1))
            return  nullptr;
        /// 统计缓冲所有mesh属性缓冲区的长度
        size_t      length  =   0;
        for (auto& mesh : meshSet)
        {
            auto&   buffer  =    mesh->buffers()[index];
            length  +=  buffer.length();
        }
        /// 创建host 缓冲区
        VBO     vboCPU  =   _device.createVBO();
        auto    result  =   vboCPU->create({length,HOST_VISIBLE_BIT});
        assert(result);
        if(!result)
            return  nullptr;
        /// 属性数据拷贝到大缓冲区中
        uint8*  pData   =   (uint8*)vboCPU->lock(length,0);
        for (auto& mesh : meshSet)
        {
            auto&   buffer  =    mesh->buffers()[index];
            memcpy(pData,buffer.data(),buffer.length());
            pData   +=  buffer.length();
        }
        vboCPU->unlock();
        return  vboCPU;
    }

    VBO     FEFactoryRender::buildBuffer(MeshUSet& meshSet,const uints& indexs)
    {
        if (meshSet.empty())    
            return  nullptr;
        auto&       mesh    =   *meshSet.begin();
        auto&       buffers =   mesh->buffers();
        /// 统计缓冲所有mesh属性缓冲区的长度
        size_t      length  =   0;
        for (auto& mesh : meshSet)
        {
            for (auto index: indexs)
            {
                auto&   buffer  =    mesh->buffers()[index];
                length  +=  buffer.length();
            }
        }
        /// 创建host 缓冲区
        VBO     vboCPU  =   _device.createVBO();
        auto    result  =   vboCPU->create({length,HOST_VISIBLE_BIT});
        assert(result);
        if(!result)
            return  nullptr;
        /// 属性数据拷贝到大缓冲区中
        uint8*  pData   =   (uint8*)vboCPU->lock(length,0);
        float3* pFloat  =   (float3*)pData;
        for (auto& mesh : meshSet)
        {
            struct  DataStride
            {
                uint8*  source  =   nullptr;
                uint16  stride  =   0;
            };
            auto&   buffers     =   mesh->buffers();
            auto    count       =   buffers.front().count();
                 
            DataStride  dataStride[16]  =   {};
            assert(indexs.size() < sizeof(dataStride)/sizeof(dataStride[0]));
            for (size_t i = 0 ;i < indexs.size(); ++ i)
            {
                dataStride[i].source    =   (uint8*)buffers[i].data();
                dataStride[i].stride    =   buffers[i].attr().stride();
            }
            for (uint c = 0; c < count; ++c)
            {
                for (size_t i = 0 ;i < indexs.size(); ++ i)
                {
                    auto    index   =   indexs[i];
                    auto&   buffer  =   buffers[index];
                    auto    stride  =   dataStride[i].stride;
                    memcpy(pData,dataStride[i].source,stride);
                    pData                   +=  stride;
                    dataStride[i].source    +=  stride;
                }
            }
        }
        vboCPU->unlock();
        return  vboCPU;
    }

    void    FEFactoryRender::copyVBinds(VBinds& vboCPUs,VBinds& vboGPUs)
    {
        /// 拷贝数据到显卡
        auto    cmdPool =   _device.transferCmdPool();
        assert(cmdPool != nullptr);
        if (cmdPool != nullptr)
        {
            CMDPtr      cmd     =   cmdPool->createCmd();
            cmd->begin(true);
            for (size_t i = 0; i < vboCPUs.size(); i++)
            {
                for (size_t v = 0; v < vboCPUs[i]._vbos.size(); ++ v)
                {
                    cmd->copyBuffer(vboCPUs[i]._vbos[v],vboGPUs[i]._vbos[v],vboCPUs[i]._vbos[v]->cInfo()._length,0,0);
                }
            }
            cmd->end();
            cmd->submit(_device.queueTransfer());
        }
    }

    void    FEFactoryRender::copyVBindsRegions(VBinds& vboCPUs,VBinds& vboGPUs)
    {
        /// 拷贝数据到显卡
        auto    cmdPool =   _device.transferCmdPool();
        assert(cmdPool != nullptr);
        if (cmdPool != nullptr)
        {
            CMDPtr      cmd     =   cmdPool->createCmd();
            cmd->begin(true);
            for (size_t i = 0; i < vboCPUs.size(); i++)
            {
                for (size_t v = 0; v < vboCPUs[i]._vbos.size(); ++ v)
                {
                    cmd->copyBuffer( vboCPUs[i]._vbos[v]
                                    ,vboGPUs[i]._vbos[v]
                                    ,vboCPUs[i]._regions[v]);
                }
            }
            cmd->end();
            cmd->submit(_device.queueTransfer());
        }
    }
    
    /// <summary>
    /// 获取所有的index 个数 / 安字节计算大小
    /// </summary>
    /// <param name="meshSet"></param>
    /// <returns></returns>
    uint2    FEFactoryRender::indexCount(MeshUSet& meshSet,EPrimitive srcPri)
    {
        uint2   result  =   {0,0};
        for (auto var : meshSet)
        {
            for (auto& pri: var->primitives())
            {
                if (pri->primitive() != srcPri)
                    continue;
                switch(pri->type())
                {
                case DRAW_ARRAY         :
                    break;
                case DRAW_ELEMENT_UINT8 :
                    result.x    +=  pri->as<FEDrawElementUint8>()->count();
                    result.y    =   result.x * sizeof(uint8);
                    result.y    =   FE::alignUp<uint>(result.y,4);
                    break;
                case DRAW_ELEMENT_UINT16:
                    result.x    +=  pri->as<FEDrawElementUint16>()->count();
                    result.y    =   result.x * sizeof(uint16); 
                    result.y    =   FE::alignUp<uint>(result.y,4);
                    break;
                case DRAW_ELEMENT_UINT32:
                    result.x    +=  pri->as<FEDrawElementUint32>()->count();
                    result.y    =   result.x * sizeof(uint32); 
                    break;
                }
            }
        }
        return  result;
    }

    uint    FEFactoryRender::countNode(const Node&  node)
    {
        uint    result  =   0;
        Mesh    mesh    =   node->mesh(); 
        if (mesh && node->material())
        {
            ++result;
        }
        auto&   childs  =   node->children();
        for (auto object : childs)
        {
            Node    child   =   object->as<FENode>();
            if (node == nullptr)
                continue;
            else
                result  +=  countNode(child);
        }

        return  result;
    }

    void    FEFactoryRender::collectNode(const Node&  node,Nodes& nodes)
    {
        uint    result  =   0;
        Mesh    mesh    =   node->mesh(); 

        if (mesh && node->material())
        {
            nodes.emplace_back(node);
        }
        auto&   childs  =   node->children();
        for (auto object : childs)
        {
            Node    child   =   object->as<FENode>();
            if (child == nullptr)
                continue;
            else
                collectNode(child,nodes);
        }
    }

    uint    FEFactoryRender::collectMesh(MeshUSet& meshSet,const Groups& grps)
    {
        uint    result  =   0;
        for (auto grp : grps)
        {
            for (auto node : grp->_objects)
            {
                Mesh    mesh    =   node->mesh(); 
                if (mesh == nullptr)
                    continue;
                ++result;
                meshSet.emplace(mesh);
            }
        }
        return  result;
    }

    RFactory    queryFactory(FEContext&ctx,FEScene& scene,uint64 key,const Nodes& nodes)
    {
        assert(scene.device() != nullptr);

        if(scene.device() == nullptr)
            return  nullptr;
        FactoryRender   factory =   new FEFactoryRender(ctx,*scene.device());
        factory->setKey(key);
        return  factory;
    }
    RFactorys   FEFactoryRender::addNodesToFactory(FEContext& ctx,FEScene& scene,const Nodes& nodeList)
    {
        /// 1. 排序，按照 pri, drawtype, Attr
        uint    nCount  =   0;
        for (auto node : nodeList)
        {
            nCount  +=  countNode(node);
        }
        if (nCount == 0)
            return  {};
        Nodes   nodes;
        /// 申请空间
        nodes.reserve(nCount);
        for (auto node : nodeList)
        {
            collectNode(node,nodes);
        }
        FactoryRenders  result;
        MeshKeyMap      priGroup;
        MeshKeyCount    priCnts;
        /// 按照key 统计节点
        for (auto& node: nodes)
        {
            auto    mesh    =   node->mesh();
            auto&   pris    =   mesh->primitives();
            for (auto& pri : pris)
            {   
                auto    key     =   mesh->key(pri);
                priCnts[key] ++;
            }
        }
        /// 申请空间
        /// 节点按照key进行分组
        for (auto& var : priCnts)
        {
            priGroup[var.first].reserve(var.second);
        }
        for (auto& node: nodes)
        {
            auto    mesh    =   node->mesh();
            auto&   pris    =   mesh->primitives();
            for (auto& pri : pris)
            {   
                auto    key     =   mesh->key(pri);
                priGroup[key].push_back(node);
            }
        }

        for (auto& var : priGroup)
        {
            auto    factory =   queryFactory(ctx,scene,var.first,var.second);
            if (factory != nullptr)
            {
                factory->setKey(var.first);
                factory->addNodes(var.second) ;
                result.emplace_back(factory);
            }
        }
        return  result;
    }
}