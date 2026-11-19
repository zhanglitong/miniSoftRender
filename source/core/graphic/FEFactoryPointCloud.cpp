#include    "../inc/graphic/FEFactoryPointCloud.hpp"
#include    "../inc/graphic/FEGPUBuffer.h"
#include    "../inc/graphic/FECmdBuffer.h"
#include    "../inc/graphic/FEInstance.h"
#include    "../inc/graphic/FEOITManager.h"
#include    "../inc/FEContext.hpp"

namespace   FE
{
    namespace
    {
        constexpr   uint    instTransSlots  =   IS_INSTANCE_ITRANSLATE | IS_INSTANCE_TRANSLATE
                                            |   IS_INSTANCE_SCALE | IS_INSTANCE_ROTATE;
        constexpr   uint    instStateSlots  =   IS_INSTANCE_FLAG;
        constexpr   uint    instColorSlots  =   IS_INSTANCE_COLOR;
        /// <summary>
        /// 将绘制类型映射为索引类型
        /// </summary>
        FEIndexType indexTypeOf(EDrawType type)
        {
            switch (type)
            {
            case EDrawType::DRAW_ELEMENT_UINT8:     return  FEIndexType::INDEX_UINT8;
            case EDrawType::DRAW_ELEMENT_UINT16:    return  FEIndexType::INDEX_UINT16;
            case EDrawType::DRAW_ELEMENT_UINT32:    return  FEIndexType::INDEX_UINT32;
            default:                                return  FEIndexType::INDEX_UINT32;
            }
        }
    }

    FEFactoryPointCloud::FEFactoryPointCloud(FEContext& ctx)
        :FEFactoryRender(ctx)
    {
        /// 逐节点绑定时无法使用"组内紧凑命令 + 每命令一次间接绘制"的 GPU 裁剪
        /// (裁剪后命令顺序未知,绑定的顶点缓冲无法对应),直接关闭
        setGPUCull(false);
    }

    FEFactoryPointCloud::~FEFactoryPointCloud()
    {}

    void    FEFactoryPointCloud::onDestroy()
    {
        _meshes.clear();
    }

    void    FEFactoryPointCloud::syncMeshes()
    {
        if (_groupNode.empty())
            return;
        Material    mat     =   _groupNode.front()->_mat;
        if (mat == nullptr)
            return;
        auto        pl      =   mat->pipeline(_key._primitive);
        if (pl == nullptr)
            return;
        auto        gpl     =   pl->as<FEGPipeline>();
        if (gpl == nullptr)
            return;
        auto&       binds   =   gpl->cInfo()._binds;

        /// 1. 统计当前仍被引用的 mesh,释放不再被引用的缓冲
        MeshUSet    alive;
        for (auto& grp : _groupNode)
        {
            for (auto& node : grp->_objects)
            {
                auto    mesh    =   node->mesh();
                if (mesh != nullptr)
                    alive.emplace(mesh);
            }
        }
        for (auto itr = _meshes.begin(); itr != _meshes.end(); )
        {
            if (alive.find(itr->first) == alive.end())
                itr =   _meshes.erase(itr);
            else
                ++itr;
        }

        /// 2. 为新增 mesh 建立独立顶点/索引缓冲(只处理该 mesh 自身的顶点)
        for (auto& mesh : alive)
        {
            if (_meshes.find(mesh) != _meshes.end())
                continue;
            MeshUSet    one;
            one.emplace(mesh);

            GpuMesh     gpuMesh;
            for (auto& bind : binds)
            {
                if (bind.inputRate != V_INPUT_VERTEX)
                    continue;
                VBO     cpuVBO  =   buildBuffer(one,bind);
                if (cpuVBO == nullptr)
                    continue;
                VBO     gpuVBO  =   _device.createVBO();
                if (gpuVBO == nullptr)
                    continue;
                gpuVBO->create({cpuVBO->cInfo()._length,DEVICE_DEFAULT_BIT});

                VBind   vBind;
                vBind._binding  =   bind.binding;
                vBind._vbos.emplace_back(gpuVBO);
                gpuMesh._vbinds.emplace_back(vBind);

                VBinds  cpus(1);
                VBinds  gpus(1);
                cpus[0]._binding            =   bind.binding;
                cpus[0]._vbos.emplace_back(cpuVBO);
                gpus[0]._binding            =   bind.binding;
                gpus[0]._vbos.emplace_back(gpuVBO);
                copyVBinds(cpus,gpus);
            }
            /// 非索引绘制没有索引表;索引绘制为该 mesh 单独建 IBO(偏移从 0 开始)
            if (_key._drawType != DRAW_ARRAY)
                gpuMesh._ibo    =   buildVertexIBO(one);
            _meshes.emplace(mesh,gpuMesh);
        }
    }

    void    FEFactoryPointCloud::update(CMDPtr cmd)
    {
        UNUSED(cmd);
        if (!flags().hasFlag(FLAG_VISIBLE))
            return;
        if (_groupNode.empty())
            return;

        bool    needUpdateVBO   =   false;
        bool    needUpdateInst  =   false;
        uint    slots           =   0;
        for (auto& var : _groupNode)
        {
            var->invalidateCullCount();
            auto&   flg =   var->flags();
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
        }
        if (flags().hasFlag(FLAG_LOD_CHANGED))
        {
            flags().removeFlag(FLAG_LOD_CHANGED);
            needUpdateVBO   =   true;
        }
        if (needUpdateVBO)
        {
            /// 与基类的区别:顶点数据按 mesh 增量维护,不做全量交错重建
            syncMeshes();
            _vboInstances   =   buildInstanceVBOs();
            _vboSphere      =   buildSphereVBO();
            buildIndirect(_indirectFull,_indirectClip);
        }
        else if (needUpdateInst)
        {
            updateInstanceLocal(slots);
            updateInstanceBSphere();
        }
    }

    void    FEFactoryPointCloud::render(CMDPtr cmd,const FERenderPassInfo& pass)
    {
        if (cmd == nullptr)
            return;
        if (!flags().hasFlag(FLAG_VISIBLE))
            return;
        if (_groupNode.empty())
            return;
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

        Material    mat     =   _groupNode.front()->_mat;
        if (mat == nullptr)
            return;
        auto        pl      =   mat->pipeline(_key._primitive)->as<FEGPipeline>();
        if (pl == nullptr)
            return;
        cmd->bindPipeline(pl);
        mat->appDynamicState(cmd,_key._primitive);

        /// 实例缓冲所有节点共用,整组绑定一次
        for (auto& var : _vboInstances)
            cmd->bindVBO(var._binding,uint(var._vbos.size()),var._vbos,{});

        bool    oitOut      =   pass._colorAttachmentCount == 0;
        bool    isArray     =   _key._drawType == EDrawType::DRAW_ARRAY;
        auto    groupCat    =   [](FEMaterial* mat) -> uint
        {
            if (mat->isTransmission())
                return  GroupCat::GC_TRANSMISSION;
            if (mat->isTransparent())
                return  GroupCat::GC_BLEND;
            return  GroupCat::GC_OPAQUE;
        };
        auto    drawGroups  =   [&](uint cats)
        {
            if (cats == GroupCat::GC_NONE)
                return;
            uint32  instId  =   0;
            for (auto& grp : _groupNode)
            {
                auto    grpMat  =   grp->_mat;
                bool    draw    =   (grpMat != nullptr) && ((groupCat(grpMat.get()) & cats) != 0);
                if (draw)
                {
                    auto    grpPl   =   oitOut ? grpMat->oitPipeline(_key._primitive)
                                               : grpMat->pipeline(_key._primitive);
                    auto    gpl     =   (grpPl != nullptr) ? grpPl->as<FEGPipeline>() : nullptr;
                    if (gpl == nullptr)
                    {
                        draw    =   false;
                    }
                    else
                    {
                        if (gpl != pl)
                        {
                            pl  =   gpl;
                            cmd->bindPipeline(pl);
                            grpMat->appDynamicState(cmd,_key._primitive);
                        }
                        FECmdBuffer::DSetBind   binds   =   {};
                        binds.dSets         =   grpMat->dsets();
                        if (oitOut)
                        {
                            auto    oitMgr  =   FEOITManager::active();
                            if (oitMgr == nullptr || oitMgr->listDSet() == nullptr)
                                draw    =   false;
                            else
                                binds.dSets.push_back(oitMgr->listDSet());
                        }
                        if (draw && !binds.dSets.empty())
                        {
                            binds.firstSet      =   0;
                            binds.offsetCount   =   0;
                            binds.offsets       =   nullptr;
                            binds.plBindPoint   =   PL_GRAPIC;
                            binds.plLayout      =   pl->nativeLayout();
                            cmd->bindDescriptors(binds);
                        }
                        else
                        {
                            draw    =   false;
                        }
                    }
                }
                const   FEMesh* lastMesh    =   nullptr;
                for (auto& node : grp->_objects)
                {
                    auto    mesh    =   node->mesh();
                    if (draw && mesh != nullptr)
                    {
                        auto    itr =   _meshes.find(mesh);
                        if (itr != _meshes.end() && !itr->second._vbinds.empty())
                        {
                            auto&   gpuMesh =   itr->second;
                            /// 连续节点共用同一 mesh 时顶点/索引绑定保持不变
                            if (lastMesh != mesh.get())
                            {
                                lastMesh    =   mesh.get();
                                for (auto& vb : gpuMesh._vbinds)
                                    cmd->bindVBO(vb._binding,uint(vb._vbos.size()),vb._vbos,{});
                                if (!isArray && gpuMesh._ibo != nullptr)
                                    cmd->bindIBO(gpuMesh._ibo,0,indexTypeOf(_key._drawType));
                            }

                            auto&   pris    =   mesh->primitives();
                            for (uint i = 0; i < (uint)pris.size(); ++ i)
                            {
                                if (pris[i]->primitive() != _key._primitive)
                                    continue;
                                FECmdIndex  ci  =   {};
                                mesh->drawCmd(ci,i,instId);
                                if (isArray)
                                    cmd->draw(ci.firstIndex,ci.count,ci.baseInstance,1);
                                else
                                    cmd->drawIndex(ci.firstIndex,ci.count,ci.baseVertex,ci.baseInstance,1);
                            }
                        }
                    }
                    ++instId;
                }
            }
        };
        drawGroups(pass._catsFirst);
        drawGroups(pass._catsLate);
    }
}
