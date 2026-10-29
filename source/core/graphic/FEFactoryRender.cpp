
#include    "../inc/graphic/FEFactoryRender.hpp"
#include    "../inc/graphic/FEGPUBuffer.h"
#include    "../inc/graphic/FECmdBuffer.h"
#include    "../inc/graphic/FEInstance.h"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/graphic/FEOITManager.h"
#include    "../inc/material/FEMaterialCull.hpp"
#include    "../inc/FEContext.hpp"
#include    "../inc/FEFormatHelper.hpp"

namespace   FE
{
    /// DEBUG: 帧采样全局标志(由 rs.webgpu 的 WGCmdBuffer.beginRender 每帧维护,
    /// core 端组级打点据此与 WGCMD.bind 采样帧对齐)
    FE_API bool g_dbgFrameSampled   =   false;

    using   GroupNode   =   FEFactoryRender::GroupNode;
    using   VBinds      =   FEFactoryRender::VBinds;

    constexpr   uint    instTransSlots  =   IS_INSTANCE_ITRANSLATE | IS_INSTANCE_TRANSLATE | IS_INSTANCE_SCALE | IS_INSTANCE_ROTATE;
    constexpr   uint    instStateSlots  =   IS_INSTANCE_FLAG;
    constexpr   uint    instColorSlots  =   IS_INSTANCE_COLOR;
    constexpr   uint    instLodSlots    =   IS_INSTANCE_LOD_INDEX;

    uint32  GroupNode::cullCount()        
    {   
        if (_cullMat == nullptr)
            return  _cmdCount;
        /// 同一帧会渲染多趟(不透明趟 + 透明趟),GPU→CPU 回读只做一次
        if (!_cullValid)
        {
            auto    cull    =   _cullMat->as<FEMaterialCull>();
            cull->_cullResult.gpu2cpu();
            _cullCount  =   (std::min)(cull->_cullResult._value.clippedCount,_cmdCount);
            _cullValid  =   true;
        }
        return  _cullCount;
    }

    size_t  GroupNode::addNode(Node object)
    {
        auto    itr =   std::lower_bound(_objects.begin(), _objects.end(), object);
        if (itr != _objects.end() && *itr == object)
            return  0;
        _objects.insert(itr, object);
        _aabb.merge(object->globalAabb());

        object->flags().removeFlag(FENode::FLAG_PROP_TRANS | FENode::FLAG_PROP_SCALE | FENode::FLAG_PROP_ROT);

        flags().addFlag(FENode::FLAG_ADD_CHILD);

        /// 如果有移除/添加, _updates 不可用
        _updates.clear();

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

        /// <summary>
        /// 4. 原地归并两个有序段：[begin, old_end) 和 [old_end, end)
        ///    这是关键，直接利用原数组空间，不额外创建新 vector
        /// </summary>
        std::inplace_merge(_objects.begin(), oldEnd, _objects.end());
        for (auto itr = nBegin; itr != eEnd; ++ itr )
        {
            _aabb.merge((*itr)->globalAabb());
            (*itr)->flags().removeFlag(FENode::FLAG_PROP_TRANS | FENode::FLAG_PROP_SCALE | FENode::FLAG_PROP_ROT);
        }
            
        flags().addFlag(FENode::FLAG_ADD_CHILD);

        /// 如果有移除/添加, _updates 不可用
        _updates.clear();

        return  _objects.size() - oldSize;
    }

    size_t  GroupNode::addNodes(Nodes& objects)
    {
        return  addNodes(objects.begin(),objects.end());
    }
    size_t  GroupNode::removeNode(Node obj)
    {
        size_t  nOld    =   _objects.size();
        /// 删除逻辑：如果当前值在 deletes 里能找到，就删除(返回 true)
        auto    newEnd  =   std::remove_if(_objects.begin(), _objects.end(), [&](const Node& value) 
        {
            return  obj == value;
        });
        _objects.erase(newEnd, _objects.end());

        /// 删除后包围盒必须整体重算:只在空组时重置会让旧盒保留已删除节点
        /// 的包围范围(场景自动适配相机会读到过期 aabb)
        _aabb   =   aabb3dr();
        for (auto& object : _objects)
            _aabb.merge(object->globalAabb());

        flags().addFlag(FENode::FLAG_REMOVE_CHILD);

        /// 如果有移除/添加, _updates 不可用
        _updates.clear();

        return  nOld - _objects.size();
    }
    size_t  GroupNode::removeNodes(Nodes& deletes)
    {
        size_t  nOld    =   _objects.size();
        /// <summary>
        /// 先排序，后续查找速度快
        /// </summary>
        std::sort(deletes.begin(),deletes.end());
        /// 删除逻辑：如果当前值在 deletes 里能找到，就删除(返回 true)
        auto    newEnd  =   std::remove_if(_objects.begin(), _objects.end(), [&](const Node& object) 
        {
            return  std::binary_search(deletes.begin(), deletes.end(), object);
        });
        _objects.erase(newEnd, _objects.end());

        /// 同 removeNode:包围盒整体重算,避免残留已删除节点的范围
        _aabb   =   aabb3dr();
        for (auto& object : _objects)
            _aabb.merge(object->globalAabb());

        flags().addFlag(FENode::FLAG_REMOVE_CHILD);

        /// 如果有移除/添加, _updates 不可用
        _updates.clear();

        return  nOld - _objects.size();
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
            if (node->flags().containFlag(FENode::InstanceProps))
            {
                flags().addFlags(node->flags().data());
                /// 添加到需要跟新的节点
                addUpdateNode(node,std::distance(_objects.begin(),itr));
            }
            /// 需要整体更新，清除局部更新数据
            if (node->flags().hasFlag(FENode::FLAG_PROP_MESH))
            {
                flags().addFlag(FENode::FLAG_PROP_MESH);
                clearUpdates();
            }
            return  1;
        }
        return  0;
    }
    void    GroupNode::resetFlags()
    {
        flags() .removeFlag(FENode::ModifyValue);
    }
    void    GroupNode::clearUpdates()
    {
        for (auto& var : _updates)
            var.node->flags().removeFlags(FENode::ModifyValue);
        _updates.clear();
    }

    void    GroupNode::addUpdateNode(Node node,size_t offset)
    {
        /// 不在此处丢弃更新:一旦 _updates 超过 MAX_LOCAL_UPDATE,
        /// isLocalUpdate() 会返回 false,由 updateInstanceLocal 改走整组更新;
        /// 直接丢弃会让对应节点一直停留在旧实例数据上
        auto    itr =   std::lower_bound(_updates.begin(), _updates.end(), node,[](const GroupNode::NodeUpdate& left,const Node& right)
        {
            return  left.node.get() < right.get();
        });
        if (itr != _updates.end() && (*itr).node == node)
            return;
        /// 必须按序插入,否则 lower_bound 的有序前提被破坏,查重失效
        _updates.insert(itr,{node,uint(offset)});
    }
    uint8*  GroupNode::copyPartial(const uints& indexs,uint stride,BufferCopys& regions,const uint8*pStart,uint8* pDst)
    {
        /// 与 updateInstanceLocal/updateInstanceBSphere 的 count 统计保持一致:
        /// 只有走局部更新路径的组才写入局部暂存区。
        /// 否则整组更新(_updates.size() >= _objects.size())的组也会被写入,
        /// 导致暂存区越界(WebGPU 验证层直接 panic: copy overrun)
        if (!isLocalUpdate())
            return  pDst;
        FEInstance  inst    =   {};
        uint8*      pSrc    =   (uint8*)&inst;
        for (auto& uData: _updates)
        {   
            /// 节点在整个大数组中的偏移量
            uint    instIdx =   start() + uData.offset;
            auto&   node    =   uData.node;

            inst._color         =   node->color();
            inst._renderBits    =   node->renderBits().data();

            inst.setTransform(node->globalFETransform());
            inst.setAabb(node->globalAabb());
            inst.setInstance(instIdx);
            /// 保存要拷贝的目标位置
            BufferCopy  copy;
            copy.srcOffset  =   pDst - pStart;
            copy.dstOffset  =   (instIdx * stride);
            copy.size       =   stride;
            regions.emplace_back(copy);
            for (auto index: indexs)
            {
                auto    nByte   =   FEInstanceHelper::instanceInputs[index].bytes;
                memcpy(pDst,pSrc + FEInstanceHelper::offsets[index],nByte);
                pDst    +=  nByte;
            }
        }
        return  pDst;
    }

    uint8*  GroupNode::copyFull(const uints& indexs,uint startInst,uint8* pDst,uint removeFlagBits)
    {
        FEInstance  inst    =   {};
        uint8*      pSrc    =   (uint8*)&inst;
        for (auto& node: _objects)
        {   
            inst._color         =   node->color();
            inst._renderBits    =   node->renderBits().data();
            inst.setTransform(node->globalFETransform());
            inst.setAabb(node->globalAabb());
            inst.setInstance(startInst ++ );
            for (auto index: indexs)
            {
                auto    nByte   =   FEInstanceHelper::instanceInputs[index].bytes;
                memcpy(pDst,pSrc + FEInstanceHelper::offsets[index],nByte);
                pDst    +=  nByte;
            }
            if (removeFlagBits)
                node->flags().removeFlags(removeFlagBits);
        }
        return  pDst;
    }

    FEFactoryRender::TransPass    FEFactoryRender::s_transPass  =   FEFactoryRender::TransPass::TP_ALL;

    void                FEFactoryRender::setTransPass(TransPass p)
    {
        s_transPass =   p;
    }
    FEFactoryRender::TransPass    FEFactoryRender::transPass()
    {
        return  s_transPass;
    }

    FEFactoryRender::FEFactoryRender(FEContext& ctx)
        :FEFactory(ctx)
        ,_device(ctx.device())
    {
        _gpuCull    =   true;
    }

    FEFactoryRender::FEFactoryRender(const FEFactoryRender& other)
        :FEFactory(other)
        ,_device(other._device)
        ,_key(other._key)
        ,_usages(other._usages)
        ,_gpuCull(other._gpuCull)
        ,_resident(other._resident)
    {
    }

    FEFactoryRender::~FEFactoryRender()
    {
        destroy();
    }

    Materials   FEFactoryRender::getOrCreateCullMaterials(Camera camera)
    {
        Materials   result;
        result.reserve(_groupNode.size());
        for (auto grp : _groupNode)
        {
            if (grp->_cullMat == nullptr)
                grp->_cullMat   =   createCullMaterial(grp,camera); 
            else
                updateCullMaterial(grp,camera);
            if (grp->_cullMat)
                result.push_back(grp->_cullMat);
        }
        return  result;
    }

    bool    FEFactoryRender::supportGPUCull() const
    {
        if (!_gpuCull)
            return  false;
        /// WebGPU 后端的 WGGPUBuffer 用 staging buffer 实现 lock/unlock,
        /// staging 是 CPU 端内存,compute shader 写入 GPU buffer 后 staging 不会更新。
        /// cullCount() -> gpu2cpu() -> lock() 读到的是 staging 旧值(通常为 0),
        /// 导致 clippedCount=0、所有实例被错误剔除、模型不可见。
        /// 在实现正确的 GPU→CPU 回读(mapAsync + copy-to-staging)之前,禁用 GPU 裁剪,
        /// 回退到 cmdCount() 全量绘制(_indirectClip 已由 buildIndirect 填充全部命令)。
        if (_device.supportWGSLShaders())
            return  false;
        else
            return  flagBitsVBO()!= nullptr;
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
        results.reserve(cnt);
        for (auto node : nodes)
        {
            collectNode(node,results);
        }
        return  addNodesImpl(results);
    }

    size_t  FEFactoryRender::addNodesFlat(Nodes::iterator nBegin,Nodes::iterator nEnd)
    {
        Nodes   results(nBegin,nEnd);
        return  addNodesImpl(results);
    }

    size_t  FEFactoryRender::addNodesImpl(Nodes& nodes)
    {
        size_t  result  =   0;
        /// <summary>
        /// 按照材质排序
        /// 同一个材质的节点可以按照批次直接加入到对相应的节点组中
        /// 性能比单个插入好很多
        /// </summary>
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
                    result  +=  pGroup->addNodes(itrStart,itr);
                /// 更新游标记录第一个发生变化的位置
                itrStart    =   itr;
                mat         =   node->material();
                pGroup      =   queryOrCreateGroup(mat);
            }
        }
        if (pGroup)
        {
            result  +=  pGroup->addNodes(itrStart,nodes.end());
        }
        if (result)
            flags().addFlag(FENode::FLAG_ADD_CHILD);
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
        GroupNode*  group   =   queryGroup(mat);
        if (group == nullptr)
            return  0;
        const   size_t  result  =   group->removeNode(node);
        if (result > 0)
            flags().addFlag(FENode::FLAG_REMOVE_CHILD);
        return  result;
    }

    size_t  FEFactoryRender::removeNodes(const Nodes& nodes) 
    {
        /// 排序会改变元素顺序,复制一份,避免修改调用方传入的数据
        Nodes   deletes =   nodes;
        std::sort(deletes.begin(),deletes.end(),[](const Node& left,const Node& right)
        {
            return  left->material().get() < right->material().get();
        });
        return  removeNodesSorted(deletes);
    }

    size_t  FEFactoryRender::removeNodesSorted(const Nodes& nodes)
    {
        size_t  result  =   0;
        GroupNode*      pGroup  =   nullptr;
        Material    mat     =   nullptr;
        for (auto node: nodes)
        {   
            if (node->material() == nullptr)
                continue;
            if (node->material() != mat)
            {
                mat     =   node->material();
                pGroup  =   queryGroup(mat);
            }
            if (pGroup != nullptr)
                result  +=  pGroup->removeNode(node);
        }
        if (result)
            flags().addFlag(FENode::FLAG_REMOVE_CHILD);
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
        if (!flags().hasFlag(FLAG_VISIBLE))
            return;
        updateImpl(cmd);
    }

    bool    FEFactoryRender::isTransparent() const
    {
        for (auto& grp : _groupNode)
        {
            if (grp->_mat && grp->_mat->isTransparent())
                return  true;
        }
        return  false;
    }

    bool    FEFactoryRender::hasTransmissionMaterial() const
    {
        for (auto& grp : _groupNode)
        {
            if (grp->_mat && grp->_mat->isTransmission())
                return  true;
        }
        return  false;
    }

    void    FEFactoryRender::render(CMDPtr cmd)
    {
        /// 不可见工厂不参与绘制(update 已经跳过,render 也必须跳过,
        /// 否则会使用未构建的 VBO/实例缓冲与陈旧绘制计数绘制,导致崩溃)
        if (!flags().hasFlag(FLAG_VISIBLE))
            return;
        if (_groupNode.empty())
            return;
        /// 工厂被摘空(所有组均无节点,如变体二次切换后的旧工厂)时
        /// VBO/IBO 均为空,绑定会空指针崩溃,直接跳过
        {
            bool    hasNodes    =   false;
            for (auto& var : _groupNode)
            {
                if (!var->_objects.empty())
                {
                    hasNodes    =   true;
                    break;
                }
            }
            if (!hasNodes)
                return;
        }
        Material    mat     =   _groupNode.front()->_mat;
        /// OIT 双趟过滤:LIST(几何)趟只画 OIT 材质,MAIN(主)趟只画普通材质
        auto        oitPass =   FEOITManager::pass();
        if (oitPass == FEOITManager::OITPass::LIST && !mat->isOIT())    return;
        if (oitPass == FEOITManager::OITPass::MAIN &&  mat->isOIT())    return;
        auto        pl      =   mat->pipeline(_key._primitive)->as<FEGPipeline>();

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
        bool    gpuCull =   supportGPUCull();
        const   bool    isArray =   _key._drawType == EDrawType::DRAW_ARRAY;
        /// 透明组(alpha 混合/transmission)必须在同工厂所有不透明组之后绘制:
        /// 混合管线不写深度,先画会被后画的不透明几何整体覆盖,表现为"玻璃不透明"。
        /// 两遍循环:pass=0 不透明组,pass=1 透明组。
        auto    drawGroups  =   [&](bool transparentPass)
        {
        /// DEBUG: 组级审计——帧采样帧内逐组记录(标志由 WGCmdBuffer.beginRender 维护),
        /// 捕获空 _dsets / null pipeline 的材质,并把 mat 指针与同帧 WGCMD.bind 按序关联
        bool    dbgGrpSampled   =   g_dbgFrameSampled;
        for (auto& grp : _groupNode)
        {
            const bool  grpTransparent  =   grp->_mat && grp->_mat->isTransparent();
            if (grpTransparent != transparentPass)
                continue;
            /// 同工厂内不同材质可能使用不同管线变体(pbr/pbr2s/pbrBlend),
            /// 切换到该组材质自身的管线与动态状态(混合状态/剔除模式随管线变化)
            auto    grpPl   =   grp->_mat->pipeline(_key._primitive)->as<FEGPipeline>();
            if (grpPl != pl)
            {
                pl  =   grpPl;
                cmd->bindPipeline(pl);
                grp->_mat->appDynamicState(cmd,_key._primitive);
            }
            FECmdBuffer::DSetBind   binds   =   {};
            binds.dSets         =   grp->_mat->dsets();
            binds.firstSet      =   0;
            binds.offsetCount   =   0;
            binds.offsets       =   nullptr;
            binds.plBindPoint   =   PL_GRAPIC;
            binds.plLayout      =   pl->nativeLayout();
            if (dbgGrpSampled)
            {
                LOG_ERR("WGGRP: mat=%p pl=%p ndsets=%zu transp=%d",
                        (void*)grp->_mat.get(),(void*)pl,
                        binds.dSets.size(),(int)transparentPass);
            }
            cmd->bindDescriptors(binds);
            uint64  offset      =   grp->cmdOffset() * sizeof(FECmdIndex);
            uint    nCmd        =   gpuCull ? grp->cullCount() : grp->cmdCount();
            if (nCmd == 0)
                continue;
            if (_device.supportWGSLShaders())
            {
                /// WebGPU: 直接使用 firstInstance 索引实例属性。
                /// 实例 VBO 已在 render() 开头以 offset=0 整体绑定,drawIndex 的
                /// firstInstance 由 wgpuRenderPassEncoderDrawIndexed 原生支持,
                /// 实例范围覆盖校验由 ensureVertexSlots 完成。
                /// 绘制项在构建缓冲时已生成(含本工厂的索引/顶点偏移),
                /// 每帧只做线性绘制,不再遍历 节点×图元 与查偏移表
                for (auto& item : grp->_drawItems)
                {
                    if (isArray)
                        cmd->draw(item._firstIndex,item._count,item._instance,item._primCount);
                    else
                        cmd->drawIndex(item._firstIndex,item._count,item._baseVertex,
                                       item._instance,item._primCount);
                }
                continue;
            }
            if (isArray)
            {
                cmd->drawArrayIndirect(_indirectClip,   offset, nCmd,sizeof(FECmdIndex));
            }
            else
            {
                cmd->drawIndexedIndirect(_indirectClip, offset, nCmd,sizeof(FECmdIndex));
            }
        }
        };
        drawGroups(false);
        /// transmission 场景折射预渲染趟(TP_OPAQUE):只画不透明组,
        /// 透明组从场景纹理中剔除(否则玻璃会出现在自身折射背景里并自折射)
        if (s_transPass == TransPass::TP_ALL)
            drawGroups(true);
    }

    void    FEFactoryRender::bindMaterial(CMDPtr cmd,FEMaterial* mat,EPrimitive pri)
    {
        if  (cmd == nullptr || mat == nullptr)
            return;
        auto    pl  =   mat->pipeline(pri);
        if  (pl == nullptr)
            return;
        FECmdBuffer::DSetBind   binds   =   {};
        binds.dSets         =   mat->dsets();
        binds.firstSet      =   0;
        binds.offsetCount   =   0;
        binds.offsets       =   nullptr;
        binds.plBindPoint   =   PL_GRAPIC;
        binds.plLayout      =   pl->nativeLayout();
        cmd->bindDescriptors(binds);
    }

    void    FEFactoryRender::destroy()
    {
        _groupNode.clear();
        _vboVertexs.clear();
        _vboInstances.clear();
        _ibo            =   nullptr;
        _priOffsets.clear();
        _priVertexOffsets.clear();
        _indirectClip   =   nullptr;
        _indirectFull   =   nullptr;
        _vboSphere      =   nullptr;
        /// 派生类释放自有资源
        onDestroy();
    }
    void    FEFactoryRender::clearFlagBits()
    {
        /// 清除标记
        for (auto& var : _groupNode)
        {
            var->resetFlags();
            var->clearUpdates();
        }
    }

    void    FEFactoryRender::updateImpl(CMDPtr cmd)
    {
        UNUSED(cmd);
        if (_groupNode.empty())
            return;
       
        bool    needUpdateVBO   =   false;
        bool    needUpdateInst  =   false;
        /// 如果需要更新instance,系统中有可能有多个instance 数据;
        /// 例如矩阵，例如状态Instance ，例如颜色Instance,例如lodIndex
        /// 有可能同时更新多个binding
        /// slots 保存了哪些slot需要更新,从而知道更新哪些bingding
        uint    slots           =   0;
        /// 计算更新状态
        for (auto& var : _groupNode)
        {
            /// 裁剪结果按帧失效,render 的多个趟复用同一次 GPU→CPU 回读
            var->invalidateCullCount();

            auto&   flg =   var->flags();
            /// 增删节点或 mesh 变化需要重建顶点/索引/实例/间接命令
            needUpdateVBO   |=  flg.hasFlag(FENode::FLAG_ADD_CHILD)
                            ||  flg.hasFlag(FENode::FLAG_REMOVE_CHILD)
                            ||  flg.hasFlag(FENode::FLAG_PROP_MESH);
            if (flg.hasFlag(FENode::FLAG_PROP_ROT) || flg.hasFlag(FENode::FLAG_PROP_TRANS) || flg.hasFlag(FENode::FLAG_PROP_SCALE))
            {
                needUpdateInst  =   true;
                slots   |=  instTransSlots;
            }
            if (flg.hasFlag(FENode::FLAG_PROP_COLOR))
            {
                needUpdateInst  =   true;
                slots   |=  instColorSlots;
            }
            if (flg.hasFlag(FENode::FLAG_PROP_STATE))
            {
                needUpdateInst  =   true;
                slots   |=  instStateSlots;
            }
            if (flg.hasFlag(FENode::FLAG_PROP_LOD))
            {
                needUpdateInst  =   true;
                slots   |=  instLodSlots;
            }
        }
        if (needUpdateVBO)
        {
            MeshUSet    meshSet;
            collectMesh(meshSet,_groupNode);
            _vboVertexs     =   buildVertexVBOs(meshSet);
            _ibo            =   buildVertexIBO(meshSet);
            _vboInstances   =   buildInstanceVBOs();
            _vboSphere      =   buildSphereVBO();
            /// 间接命令与 WGSL 绘制项都依赖本帧最新的实例布局
            buildIndirect(_indirectFull,_indirectClip);
        }
        /// 只是更新instance,优化处理
        else if(needUpdateInst) 
        {
            updateInstanceLocal(slots);
            updateInstanceBSphere();
        }
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
            /// 交错布局必须按管线声明驱动:
            /// mesh 缺失的声明属性(如 helmet 无 TEXCOORD1)以 0 填充,
            /// 保证大缓冲区记录跨度与管线 stride(xml offsets)一致
            VBO     cpuVBO  =   buildBuffer(meshSet,bind.inputs);
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
                    _priVertexOffsets[pri.get()]    =   nCount;
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
        IndexCounts count   =   indexCount(meshSet,_key._primitive);
        if (count._indices == 0)
            return  nullptr;
        IBO     cpu     =   _device.createIBO();
        if(!cpu->create({count._bytes,HOST_VISIBLE_BIT}))
            return  nullptr;
        uint8*  pDst    =   (uint8*)cpu->lock(count._bytes,0);
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
                    uint32  cnt =   pri->copyIndexTo<uint16>((uint16*)pDst,count._indices);
                    /// 写游标按 4 字节对齐前进,记录的 indexOffset 必须按同一对齐换算,
                    /// 否则奇数 count 图元之后的所有图元索引整体错位(渲染出垃圾三角形)
                    uint    adv =   FE::alignUp<uint>(cnt * sizeof(uint16),4);
                    pDst        +=  adv;
                    pri->setIndexOffset(offset);
                    _priOffsets[srcPri.get()]   =   offset;
                    offset      +=  adv / sizeof(uint16);
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
                    uint32  cnt =   pri->copyIndexTo<uint16>((uint16*)pDst,count._indices);
                    /// 同 uint8 路径:写入位置与 indexOffset 必须按同一对齐规则推进
                    uint    adv =   FE::alignUp<uint>(cnt * sizeof(uint16),4);
                    pDst        +=  adv;
                    pri->setIndexOffset(offset);
                    _priOffsets[srcPri.get()]   =   offset;
                    offset      +=  adv / sizeof(uint16);
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
                    uint32  cnt =   pri->copyIndexTo<uint32>((uint32*)pDst,count._indices);
                    pDst        +=  cnt * 4;
                    pri->setIndexOffset(offset);
                    _priOffsets[srcPri.get()]   =   offset;
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

    inline  auto    offfetIndex(const InputDescs& inputs)
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
    inline  auto    offfetIndex(uint slot)
    {
        uints   indexs;
        for (size_t i = 0 ;i < sizeof(FEInstanceHelper::instanceInputs) / sizeof(FEInstanceHelper::instanceInputs[0]); ++ i )
        {
            if (slot == FEInstanceHelper::instanceInputs[i].slot)
                indexs.push_back(uint(i));
        }
        return  indexs;
    };
    
    /// <summary>
    /// inputs中的slot 是否在slots bits中
    /// </summary>
    /// <param name="slots"></param>
    /// <param name="inputs"></param>
    /// <returns></returns>
    inline  bool    containSlot(uint slots,const InputDescs& inputs)
    {
        for (auto& input : inputs)
        {
            if (slots & input.slot)
                return  true;
        }
        return  false;
    }
    
    VBinds  FEFactoryRender::buildInstanceVBOs()
    {
        Material    mat     =   _groupNode.front()->_mat; 
        auto        pl      =   mat->pipeline(_key._primitive)->as<FEGPipeline>();
        auto&       binds   =   pl->cInfo()._binds;
        if (binds.empty())
            return  {};
        /// instance数量
        uint32      count   =   0;
        for (auto& var : _groupNode)
        {
            var->setStart(count);
            count   +=  (uint32)var->_objects.size();
        }
        /// 工厂可能被摘空(如变体二次切换后旧工厂全部节点被移除),
        /// 0 实例不能创建 0 字节 VBO(vkCreateBuffer 要求 size>0)
        if (count == 0)
            return  {};
        VBinds      vboCPUs;
        uint        flgBits =   FENode::ModifyValue;
        for (auto& bind : binds)
        {   
            if (bind.inputRate == V_INPUT_VERTEX)
                continue;
            VBind       vBind       =   {};
            vBind._slots            =   0;
            for (auto& var: bind.inputs)
            {
                vBind._slots   |=   var.slot;
            }
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
            uint8*      pDst        =   (uint8*)vbo->lock(length,0);
            uint        instOffset  =   0;
            for (auto& var : _groupNode)
            {
                pDst        =   var->copyFull(indexs,instOffset,pDst,flgBits);
                instOffset  +=  (uint)var->_objects.size();
            }
            /// 只需要执行一次
            /// copyFull 中移除节点的标志
            flgBits =   0;

            vbo->unlock();
            vBind._binding  =   bind.binding;
            vboCPUs.emplace_back(vBind);
        }

        VBinds      vboGPUs(vboCPUs.size());
        for (size_t i = 0; i < vboCPUs.size(); i++)
        {
            vboGPUs[i]._binding =   vboCPUs[i]._binding;
            vboGPUs[i]._slots   =   vboCPUs[i]._slots;
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
    VBO     FEFactoryRender::buildSphereVBO()
    {
        /// instance数量
        uint32      count   =   0;
        for (auto& var : _groupNode)
        {
            var->setStart(count);
            count   +=  (uint32)var->_objects.size();
        }
        /// 同 buildInstanceVBOs:摘空的工厂不创建 0 字节 VBO
        if (count == 0)
            return  nullptr;
        VBO     vboCPU  =   _device.createVBO();
        VBO     vboGPU  =   _device.createVBO();
        /// 找到需要更新属性的槽索引数组
        uints       indexs  =   offfetIndex(IS_INSTANCE_BOUNDSPHERE);
        /// 一个元素(顶点结构)的大小
        uint        stride  =   0;
        for (auto index : indexs)
        {
            stride  +=  FEInstanceHelper::instanceInputs[index].bytes;
        }
        uint64      length  =   count * stride;
        vboGPU->create({length,DEVICE_DEFAULT_BIT});
        vboCPU->create({length,HOST_VISIBLE_BIT});
        uint8*      pDst        =   (uint8*)vboCPU->lock(length,0);
        uint        instOffset  =   0;
        for (auto& var : _groupNode)
        {
            pDst        =   var->copyFull(indexs,instOffset,pDst,0);
            instOffset  +=  (uint)var->_objects.size();
        }
        vboCPU->unlock();

        auto    cmdPool =   _device.transferCmdPool();
        assert(cmdPool != nullptr);
        if (cmdPool != nullptr)
        {
            CMDPtr      cmd     =   cmdPool->createCmd();
            cmd->begin(true);
            cmd->copyBuffer(vboCPU,vboGPU,{});
            cmd->end();
            cmd->submit(_device.queueTransfer());
        }
        return  vboGPU;
    }

    bool    FEFactoryRender::calcPendingInstances(uint32& localCnt,uint32& grpCnt,uint32& grpInstCnt)
    {
        localCnt    =   0;
        grpCnt      =   0;
        grpInstCnt  =   0;
        for (auto& var : _groupNode)
        {
            if (!var->flags().containFlag(FENode::InstanceProps))
                continue;
            /// 未超过局部更新上限的组走局部路径,否则整组重写
            if (var->isLocalUpdate())
                localCnt    +=  (uint)var->_updates.size();
            else
            {
                ++grpCnt;
                grpInstCnt  +=  (uint)var->_objects.size();
            }
        }
        return  localCnt != 0 || grpCnt != 0;
    }

    void    FEFactoryRender::fillInstanceUpdates(const uints& indexs,uint stride,uint localCnt,
                                                 uint grpInstCnt,VBO staging,BufferCopys& regions,
                                                 uint removeFlagBits)
    {
        if (staging == nullptr)
            return;
        /// 暂存缓冲可能比本次数据更长(跨帧复用),按实际长度加锁
        uint64      length  =   staging->cInfo()._length;
        uint8*      pStart  =   (uint8*)staging->lock(length,0);
        if (pStart == nullptr)
            return;
        uint8*      pDst    =   pStart;
        if (localCnt != 0)
        {
            for (auto& var : _groupNode)
            {
                if (!var->flags().containFlag(FENode::InstanceProps))
                    continue;
                pDst    =   var->copyPartial(indexs,stride,regions,pStart,pDst);
            }
        }
        if (grpInstCnt != 0)
        {
            /// copyFull 中移除节点的标志,多个绑定只执行一次
            uint    flgBits =   removeFlagBits;
            for (auto& var : _groupNode)
            {
                if (!var->flags().containFlag(FENode::InstanceProps))
                    continue;
                if (var->isLocalUpdate())
                    continue;
                auto        pResult =   var->copyFull(indexs,var->start(),pDst,flgBits);
                BufferCopy  copy;
                copy.srcOffset  =   pDst - pStart;
                pDst            =   pResult;

                copy.dstOffset  =   (var->start() * stride);
                copy.size       =   (uint)(var->_objects.size() * stride);
                regions.emplace_back(copy);
                flgBits =   0;
            }
        }
        staging->unlock();
    }

    VBO     FEFactoryRender::acquireStaging(uint64 length)
    {
        /// 不复用暂存缓冲:实例更新频率很低,缓存只会长期占用显存。
        /// 每次新建,由调用方在其作用域结束时释放
        VBO     vbo =   _device.createVBO();
        vbo->create({length,HOST_VISIBLE_BIT});
        return  vbo;
    }

    void    FEFactoryRender::updateInstanceLocal(uint slots)
    {
        Material    mat     =   _groupNode.front()->_mat;
        auto        pl      =   mat->pipeline(_key._primitive)->as<FEGPipeline>();
        auto&       binds   =   pl->cInfo()._binds;
        if (binds.empty())
            return;
        uint32      localCnt    =   0;
        uint32      grpCnt      =   0;
        uint32      grpInstCnt  =   0;
        if (!calcPendingInstances(localCnt,grpCnt,grpInstCnt))
            return;
        /// 局部更新:只写发生变化的那几条实例
        if (localCnt != 0)
        {
            VBinds      vboCPUs;
            vboCPUs.reserve(binds.size());
            for (auto& bind : binds)
            {
                if (bind.inputRate == V_INPUT_VERTEX)
                    continue;
                /// 如果要 bind,inputs 中 不在 slots 中,说明不需要更新
                if (!containSlot(slots,bind.inputs))
                    continue;
                uints       indexs  =   offfetIndex(bind.inputs);
                uint        stride  =   0;
                for (auto index : indexs)
                    stride  +=  FEInstanceHelper::instanceInputs[index].bytes;
                VBO         vbo     =   acquireStaging(localCnt * stride);
                BufferCopys regions;
                regions.reserve(localCnt);
                fillInstanceUpdates(indexs,stride,localCnt,0,vbo,regions,0);
                vboCPUs.emplace_back();
                auto&   vBind       =   vboCPUs.back();
                vBind._binding      =   bind.binding;
                vBind._vbos.emplace_back(vbo);
                vBind._regions.emplace_back(regions);
            }
            copyVBindsRegions(vboCPUs,_vboInstances);
        }
        /// 整组更新:超过局部更新上限的组整体重写
        if (grpInstCnt != 0)
        {
            VBinds      vboCPUs;
            vboCPUs.reserve(binds.size());
            for (auto& bind : binds)
            {
                if (bind.inputRate == V_INPUT_VERTEX)
                    continue;
                if (!containSlot(slots,bind.inputs))
                    continue;
                uints       indexs  =   offfetIndex(bind.inputs);
                uint        stride  =   0;
                for (auto index : indexs)
                    stride  +=  FEInstanceHelper::instanceInputs[index].bytes;
                VBO         vbo     =   acquireStaging(grpInstCnt * stride);
                BufferCopys regions;
                regions.reserve(grpCnt);
                fillInstanceUpdates(indexs,stride,0,grpInstCnt,vbo,regions,FENode::ModifyValue);
                vboCPUs.emplace_back();
                auto&   vBind       =   vboCPUs.back();
                vBind._binding      =   bind.binding;
                vBind._vbos.emplace_back(vbo);
                vBind._regions.emplace_back(regions);
            }
            copyVBindsRegions(vboCPUs,_vboInstances);
        }
    }

    void    FEFactoryRender::updateInstanceBSphere()
    {
        Material    mat     =   _groupNode.front()->_mat;
        auto        pl      =   mat->pipeline(_key._primitive)->as<FEGPipeline>();
        auto&       binds   =   pl->cInfo()._binds;
        if (binds.empty() || _vboSphere == nullptr)
            return;
        uint32      localCnt    =   0;
        uint32      grpCnt      =   0;
        uint32      grpInstCnt  =   0;
        if (!calcPendingInstances(localCnt,grpCnt,grpInstCnt))
            return;
        uints       indexs  =   offfetIndex(IS_INSTANCE_BOUNDSPHERE);
        uint        stride  =   0;
        for (auto index : indexs)
            stride  +=  FEInstanceHelper::instanceInputs[index].bytes;
        if (localCnt != 0)
        {
            VBO         vbo =   acquireStaging(localCnt * stride);
            BufferCopys regions;
            regions.reserve(localCnt);
            fillInstanceUpdates(indexs,stride,localCnt,0,vbo,regions,0);
            copyBuffer(vbo,_vboSphere,regions);
        }
        if (grpInstCnt != 0)
        {
            VBO         vbo =   acquireStaging(grpInstCnt * stride);
            BufferCopys regions;
            regions.reserve(grpCnt);
            fillInstanceUpdates(indexs,stride,0,grpInstCnt,vbo,regions,0);
            copyBuffer(vbo,_vboSphere,regions);
        }
    }

    ITO     FEFactoryRender::buildIndirect(ITO&  gpuAll,ITO& gpuDraw)
    {
        uint32      cmdOffset   =   0;
        for (auto& var : _groupNode)
        {
            var->setCmdOffset(cmdOffset);
            uint    nCmd    =   0;
            for (auto& node: var->_objects)
            {
                auto    mesh    =   node->mesh();
                auto&   pris    =   mesh->primitives();
                for (auto& pri : pris)
                {
                    if (pri->primitive() != _key._primitive)
                        continue;
                    else
                        ++nCmd;
                }
            }
            var->setCmdCount(nCmd);
            cmdOffset   +=  nCmd;
        }
        uint64      length  =   sizeof(FECmdIndex) * cmdOffset;
        /// 工厂被摘空时无间接命令,合法返回空(渲染按 cmdCount==0 跳过)
        if (length == 0)
            return  nullptr;

        ITO         cpuBuf  =   _device.createITO(); 
        ITO         gpuBuf  =   _device.createITO(); 
        ITO         gpuFull =   _device.createITO(); 
        cpuBuf->create({length, HOST_VISIBLE_BIT});
        gpuBuf->create({length, DEVICE_DEFAULT_BIT}); 
        gpuFull->create({length, DEVICE_DEFAULT_BIT}); 

        auto        pCmdIdx =   (FECmdIndex*)cpuBuf->lock(length,0);
        size_t      instId  =   0;
        size_t      cmdIdx  =   0;
        for (auto& var : _groupNode)
        {
            /// 同步重建 WGSL 路径的绘制项(线性绘制,避免每帧遍历节点与查偏移表)
            var->_drawItems.clear();
            var->_drawItems.reserve(var->cmdCount());
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
                    /// primitive 上的 _indexOffset/_vertexOffset 会被其它工厂的
                    /// IBO/VBO 构建覆盖,WGSL 绘制项必须用本工厂构建时记录的偏移
                    /// (与绑定的 _ibo 及 _vboVertexs 布局一致)
                    auto&   item    =   pCmdIdx[cmdIdx];
                    GroupNode::DrawItem drawItem;
                    drawItem._firstIndex    =   item.firstIndex;
                    drawItem._count         =   item.count;
                    drawItem._baseVertex    =   item.baseVertex;
                    drawItem._primCount     =   item.primCount;
                    drawItem._instance      =   item.baseInstance;
                    auto    itOff   =   _priOffsets.find(pris[i].get());
                    if (itOff != _priOffsets.end())
                        drawItem._firstIndex    =   itOff->second;
                    auto    itVtx   =   _priVertexOffsets.find(pris[i].get());
                    if (itVtx != _priVertexOffsets.end())
                        drawItem._baseVertex    =   itVtx->second;
                    var->_drawItems.emplace_back(drawItem);
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
            cmd->copyBuffer(cpuBuf,gpuBuf,  length,0,0);
            cmd->copyBuffer(cpuBuf,gpuFull, length,0,0);
            
            cmd->end();
            cmd->submit(_device.queueTransfer());
        }

        gpuDraw =   gpuBuf;
        gpuAll  =   gpuFull;
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

    Material    FEFactoryRender::createCullMaterial(Group grp,Camera camera)
    {
        if (grp == nullptr || camera == nullptr)
            return  nullptr;

        FEMaterialCull* pCull       =   new FEMaterialCull(_ctx);
        Material        pMat        =   pCull;
        fillCullParams(pCull,grp,camera);
        return  pMat;
    }
    void    FEFactoryRender::updateCullMaterial(Group grp,Camera camera)
    {
        if (grp == nullptr || grp->_cullMat == nullptr || camera == nullptr)
            return;
        FEMaterialCull* pCull       =   grp->_cullMat->cast<FEMaterialCull>();
        if (pCull == nullptr)
            return;
        fillCullParams(pCull,grp,camera);
    }
    void    FEFactoryRender::fillCullParams(FEMaterialCull* pCull,Group grp,Camera camera)
    {
        if (pCull == nullptr || grp == nullptr || camera == nullptr)
            return;
        auto&           cullParam   =   pCull->_cullParam._value;
        auto&           cullResult  =   pCull->_cullResult._value;
        auto            rightDir    =   camera->getRight();

        memset(&cullResult,0,sizeof(cullResult));

        cullParam._offset           =   grp->cmdOffset();
        cullParam._count            =   grp->cmdCount();
        cullParam._minVisiblePixels =   10;

        cullParam._rightX           =   (float)rightDir.x;
        cullParam._rightY           =   (float)rightDir.y;
        cullParam._rightZ           =   (float)rightDir.z;
        cullParam._viewSizeW        =   camera->viewportWidth<float>();
        cullParam._viewSizeH        =   camera->viewportHeight<float>();
        cullParam._mvp              =   camera->getProject() * camera->getView();
        FrustumR    frustum         =   camera->extract();
        for (size_t i = 0; i < 6; i++)
        {
            cullParam._planes[i].x  =   frustum._planes[i]._normal.x;
            cullParam._planes[i].y  =   frustum._planes[i]._normal.y;
            cullParam._planes[i].z  =   frustum._planes[i]._normal.z;
            cullParam._planes[i].w  =   frustum._planes[i]._distance;
        }
        /// 先上传参数与结果,再绑定缓冲并提交(创建/更新两条路径统一)
        pCull->_cullParam.update();
        pCull->_cullResult.update();

        pCull->bind(0,0,{_vboSphere.get()}   );
        pCull->bind(0,1,{_indirectFull.get()});
        pCull->bind(0,2,{_indirectClip.get()});
        pCull->bind(0,3,{flagBitsVBO().get()});

        pCull->bind(0,4,{pCull->_cullResult._gpu.get()});
        pCull->bind(0,5,{pCull->_cullParam._gpu.get()});
        pCull->update();
    }
    VBO     FEFactoryRender::buildBuffer(MeshUSet& meshSet,FEInputSlot slot)
    {
        if (meshSet.empty())
            return  nullptr;
        auto&       temp    =   *meshSet.begin();
        auto&       buffers =   temp->buffers();
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

    VBO     FEFactoryRender::buildBuffer(MeshUSet& meshSet,const InputDescs& inputs)
    {
        if (meshSet.empty())
            return  nullptr;
        /// <summary>
        /// 交错布局必须按管线声明顺序驱动(与 xml offsets/stride 一致):
        /// 1. 组内各 mesh 属性集合可能不同(如 glTF 部分图元无 TANGENT),按 slot 逐 mesh 匹配;
        /// 2. mesh 缺失的声明属性(如 helmet 无 TEXCOORD1)交错时写 0 填充,
        ///    保证大缓冲区记录跨度与管线声明的 stride 一致,否则顶点属性整体错位。
        /// </summary>
        struct  DataStride
        {
            uint8*  source  =   nullptr;
            uint16  stride  =   0;
        };
        uints   slots;
        std::vector<DataStride> strides;
        strides.reserve(inputs.size());
        for (auto& input : inputs)
        {
            slots.push_back(input.slot);
            strides.push_back({nullptr,uint16(FEFormatHelper::sizeOf(input.format))});
        }
        /// 统计缓冲所有mesh属性缓冲区的长度
        size_t      length  =   0;
        for (auto& mesh : meshSet)
        {
            auto    count   =   mesh->buffers().front().count();
            for (size_t i = 0 ;i < slots.size(); ++ i)
                length  +=  size_t(count) * strides[i].stride;
        }
        /// 创建host 缓冲区
        VBO     vboCPU  =   _device.createVBO();
        auto    result  =   vboCPU->create({length,HOST_VISIBLE_BIT});
        assert(result);
        if(!result)
            return  nullptr;
        /// 属性数据拷贝到大缓冲区中
        uint8*  pData   =   (uint8*)vboCPU->lock(length,0);
        std::vector<uint8>  zeros(64,0);
        for (auto& mesh : meshSet)
        {
            auto&   buffers     =   mesh->buffers();
            auto    count       =   buffers.front().count();

            DataStride  dataStride[16]  =   {};
            assert(slots.size() < sizeof(dataStride)/sizeof(dataStride[0]));
            for (size_t i = 0 ;i < slots.size(); ++ i)
            {
                /// 按 slot 在当前 mesh 中找属性缓冲,找不到则该属性交错时填 0
                for (auto& buffer : buffers)
                {
                    if (buffer.attr().slot() == slots[i])
                    {
                        dataStride[i].source    =   (uint8*)buffer.data();
                        break;
                    }
                }
                dataStride[i].stride    =   strides[i].stride;
            }
            for (uint c = 0; c < count; ++c)
            {
                for (size_t i = 0 ;i < slots.size(); ++ i)
                {
                    auto    stride  =   dataStride[i].stride;
                    if (dataStride[i].source != nullptr)
                    {
                        memcpy(pData,dataStride[i].source,stride);
                        dataStride[i].source    +=  stride;
                    }
                    else
                    {
                        memcpy(pData,zeros.data(),stride);
                    }
                    pData                   +=  stride;
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
            /// 先按 binding 建一次映射,避免每个 CPU 绑定都遍历一遍 GPU 绑定列表
            std::map<uint32,size_t> gpuIndexs;
            for (size_t i = 0; i < vboGPUs.size(); ++ i)
                gpuIndexs.emplace(vboGPUs[i]._binding,i);
            CMDPtr      cmd     =   cmdPool->createCmd();
            cmd->begin(true);
            for (size_t i = 0; i < vboCPUs.size(); i++)
            {
                auto    itr =   gpuIndexs.find(vboCPUs[i]._binding);
                if (itr == gpuIndexs.end())
                    continue;
                auto&   gpu =   vboGPUs[itr->second];
                /// 暂存缓冲/拷贝区域/目标缓冲三者必须一一对应,越界直接跳过
                assert(vboCPUs[i]._regions.size() == vboCPUs[i]._vbos.size());
                assert(gpu._vbos.size() == vboCPUs[i]._vbos.size());
                const   size_t  nVbo    =   (std::min)(vboCPUs[i]._vbos.size(),
                                            (std::min)(vboCPUs[i]._regions.size(),gpu._vbos.size()));
                for (size_t v = 0; v < nVbo; ++ v)
                    cmd->copyBuffer( vboCPUs[i]._vbos[v],gpu._vbos[v],vboCPUs[i]._regions[v]);
            }
            cmd->end();
            cmd->submit(_device.queueTransfer());
        }
    }

    void    FEFactoryRender::copyBuffer(VBO src,VBO dst,const BufferCopys& regions)
    {
        /// 拷贝数据到显卡
        auto    cmdPool =   _device.transferCmdPool();
        assert(cmdPool != nullptr);
        if (cmdPool != nullptr)
        {
            CMDPtr      cmd     =   cmdPool->createCmd();
            cmd->begin(true);
            cmd->copyBuffer( src,dst,regions);
            cmd->end();
            cmd->submit(_device.queueTransfer());
        }
    }
    
    VBO     FEFactoryRender::boundSphereVBO()const
    {
        return  _vboSphere;
    }
    VBO     FEFactoryRender::flagBitsVBO() const
    {
        for (auto& var : _vboInstances)
        {   
            if (var._slots & IS_INSTANCE_FLAG)
                return  var._vbos.front();
        }
        return  nullptr;
    }

    FEFactoryRender::IndexCounts   FEFactoryRender::indexCount(const MeshUSet& meshSet,EPrimitive srcPri)
    {
        IndexCounts result;
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
                    {
                        /// uint8 索引在 buildVertexIBO 中转成 uint16 写入,
                        /// 且每个图元末尾按 4 字节对齐填充,分配必须按"写入布局"逐图元累计
                        uint    cnt =   pri->as<FEDrawElementUint8>()->count();
                        result._indices +=  cnt;
                        result._bytes   +=  FE::alignUp<uint>(cnt * sizeof(uint16),4);
                    }
                    break;
                case DRAW_ELEMENT_UINT16:
                    {
                        uint    cnt =   pri->as<FEDrawElementUint16>()->count();
                        result._indices +=  cnt;
                        result._bytes   +=  FE::alignUp<uint>(cnt * sizeof(uint16),4);
                    }
                    break;
                case DRAW_ELEMENT_UINT32:
                    {
                        uint    cnt =   pri->as<FEDrawElementUint32>()->count();
                        result._indices +=  cnt;
                        result._bytes   +=  cnt * sizeof(uint32);
                    }
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
            if (object == nullptr)
                continue;
            /// 检测错误
            assert(object->parent() != nullptr);
            result  +=  countNode(object);
        }

        return  result;
    }

    void    FEFactoryRender::collectNode(const Node&  node,Nodes& nodes)
    {
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

    /// <summary>
    /// 按 key 查找/创建工厂:reuseExisting 时优先复用场景中同 key 的非常驻工厂,
    /// 避免同一 mesh key 重复建厂(VBO/IBO 重复拷贝、绘制批次被拆散)。
    /// 常驻工厂(天空盒/网格/编辑工具)不复用,否则会把普通节点并进常驻工厂。
    /// </summary>
    static  RFactory    queryOrCreateFactory(FEContext& ctx,FEScene& scene,uint64 key,bool reuseExisting)
    {
        if (scene.device() == nullptr)
            return  nullptr;
        if (reuseExisting)
        {
            auto    exist   =   scene.factoryMgr().findObject(key);
            if (exist != nullptr && !exist->isResident())
                return  exist;
        }
        FactoryRender   factory =   new FEFactoryRender(ctx);
        factory->setKey(key);
        return  factory;
    }
    RFactorys   FEFactoryRender::addNodesToFactory(FEContext& ctx,FEScene& scene,const Nodes& nodeList,
                                                   bool reuseExisting)
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
        /// _pris 按"节点×图元"计数;_nodes 按节点去重计数
        /// (同一节点在同一个 key 下的多个图元只占一个节点槽位)
        MeshKeyCount    priCnts;
        MeshKeyCount    nodeCnts;
        std::vector<uint64> keys;
        /// 按照key 统计节点
        /// 为了避免动态分配内存
        for (auto& node: nodes)
        {
            auto    mesh    =   node->mesh();
            auto&   pris    =   mesh->primitives();
            keys.clear();
            for (auto& pri : pris)
            {   
                auto    key     =   mesh->key(pri).value();
                priCnts[key] ++;
                if (std::find(keys.begin(),keys.end(),key) == keys.end())
                {
                    keys.emplace_back(key);
                    nodeCnts[key] ++;
                }
            }
        }
        /// 申请空间
        /// 节点按照key进行分组
        for (auto& var : priCnts)
        {
            auto    itr     =   nodeCnts.find(var.first);
            uint    nNode   =   (itr != nodeCnts.end()) ? itr->second : var.second;
            priGroup[var.first]._nodes.reserve(nNode);
            priGroup[var.first]._pris.reserve(var.second);
        }
        for (auto& node: nodes)
        {
            auto    mesh    =   node->mesh();
            auto&   pris    =   mesh->primitives();

            for (size_t i = 0;i < pris.size(); ++ i)
            {   
                auto    pri     =   pris[i];  
                auto    key     =   mesh->key(pri).value();
                priGroup[key]._nodes.push_back(node);
                priGroup[key]._pris.push_back(pri);
            }
        }

        for (auto& var : priGroup)
        {
            auto    factory =   queryOrCreateFactory(ctx,scene,var.first,reuseExisting);
            if (factory != nullptr)
            {
                for (auto& pri : var.second._pris)
                {
                    pri->setFactory(factory.get());
                }
                /// _nodes 已由 collectNode 递归收集完成,这里直接平铺加入,
                factory->addNodesFlat(var.second._nodes.begin(),var.second._nodes.end());
                result.emplace_back(factory);
            }
        }
        return  result;
    }

    size_t  FEFactoryRender::removeNodesFromFactory(FEScene& scene,const Nodes& nodeList)
    {
        /// 与添加对应:添加时按 mesh key 分组找/建工厂;删除无需按 key 分组,
        /// 直接交给各工厂的 removeNodesSorted(内部按材质整组摘除,有移除时标记
        /// FLAG_REMOVE_CHILD 触发缓冲重建)
        Nodes   nodes;
        for (auto node : nodeList)
        {
            if (node == nullptr)
                continue;
            collectNode(node,nodes);
        }
        /// 每个工厂都要求入参按材质有序,这里统一排序一次,
        /// 各工厂直接走 removeNodesSorted,避免逐个工厂复制+重排
        std::sort(nodes.begin(),nodes.end(),[](const Node& left,const Node& right)
        {
            return  left->material().get() < right->material().get();
        });
        size_t  removed =   0;
        for (auto factory : scene.factoryMgr().objects())
        {
            if (factory != nullptr)
                removed  += factory->removeNodesSorted(nodes);
        }
        return  removed;
    }
}

