#pragma     once
#include    <algorithm>
#include    <vector>
#include    "../FEComponentSys.hpp"
#include    "../graphic/FEScene.h"
#include    "../graphic/FEShaderDefine.h"
#include    "../node/FESkin.hpp"
#include    "../node/FENode.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FESkinSys, "{8F2C4A61-3B7D-4E52-9C6A-1D5E7B9A0C43}");
    /// <summary>
    /// 骨骼动画系统:管理挂在节点上的 FESkin 组件,每帧在动画/约束之后、
    /// 渲染工厂更新之前完成蒙皮。
    /// 组件由场景在节点入场景时通过 dispatchToSystem 自动登记(FEAnimation 同一套机制),
    /// 系统不再每帧遍历整棵节点树。
    ///   GPU: 重算矩阵并上传 SBO,材质绑定 SB_Skin;
    ///   CPU: skinMesh 写回顶点,节点置 FLAG_PROP_MESH 并进入 _updateList,
    ///        由 updateObjectList 统一 update + fireChanged,触发渲染工厂重建 VBO。
    /// </summary>
    class   FE_API  FESkinSys :public FEComponentSys
    {
        IMPLEMENT_CLASS_REFLECT(FESkinSys)
    public:
        FESkinSys(FEContext& ctx)
            :FEComponentSys(ctx)
        {
            /// 在动画、约束之后执行:蒙皮需要最终的关节世界矩阵
            priority(FEFactory::PT_Update).setOrder(FEFactory::UO_Skin);
        }
        FESkinSys(const FESkinSys& other)
            :FEComponentSys(other)
        {
            priority(FEFactory::PT_Update).setOrder(FEFactory::UO_Skin);
        }
        virtual size_t  addObject(Component com) override
        {
            FESkin*     skin    =   (com == nullptr) ? nullptr : com->cast<FESkin>();
            if (skin == nullptr || contains(skin))
                return  0;
            _skins.emplace_back(skin);
            return  1;
        }
        virtual size_t  addObjects(const Components& coms) override
        {
            size_t  count   =   0;
            for (auto& com : coms)
                count   +=  addObject(com);
            return  count;
        }
        virtual size_t  removeObject(Component com) override
        {
            FESkin*     skin    =   (com == nullptr) ? nullptr : com->cast<FESkin>();
            if (skin == nullptr)
                return  0;
            auto    itr =   std::find_if(_skins.begin(),_skins.end(),
                                         [skin](const Skin& var) { return var.get() == skin; });
            if (itr == _skins.end())
                return  0;
            _skins.erase(itr);
            return  1;
        }
        virtual size_t  removeObjects(const Components& coms) override
        {
            size_t  count   =   0;
            for (auto& com : coms)
                count   +=  removeObject(com);
            return  count;
        }
        virtual void    update(const real& tmDelta) override
        {
            UNUSED(tmDelta);
            /// GPU compute 蒙皮:所有 skin 的 dispatch 合并到一个命令缓冲,
            /// 帧末统一提交;compute 不可用时逐个回退 CPU 矩阵上传
            CMDPtr  computeCmd  =   nullptr;
            for (auto& skin : _skins)
            {
                if (skin == nullptr)
                    continue;
                FENode*     node    =   skin->meshNode();
                if (node == nullptr)
                    continue;
                Mesh    mesh    =   node->mesh();
                if (mesh == nullptr)
                    continue;
                if (skin->cpuSkin())
                {
                    skin->updateMatrices();
                    if (skin->skinMesh(*mesh))
                    {
                        node->updateAabb(false);
                        node->flags().addFlag(FENode::FLAG_PROP_MESH);
                        _updateList.addObject(Object(node));
                    }
                }
                else
                {
                    if (skin->prepareCompute())
                    {
                        if (computeCmd == nullptr)
                        {
                            auto    pool    =   ctx().device().computeCmdPool();
                            if (pool != nullptr)
                            {
                                computeCmd  =   pool->createCmd();
                                computeCmd->begin(true);
                                computeCmd->bindPipeline(skin->computePipeline());
                            }
                        }
                        if (computeCmd != nullptr)
                            skin->dispatchCompute(computeCmd.get());
                        else
                            skin->update();
                    }
                    else if (skin->computeSkipped())
                    {
                        /// 关节世界矩阵与上一帧一致(暂停/静止骨骼),蒙皮结果无需重算
                    }
                    else
                    {
                        /// compute 管线不可用(如后端不支持): 回退 CPU 合成 + 上传
                        skin->update();
                    }
                    Material    mat =   node->material();
                    if (mat != nullptr && skin->buffer() != nullptr)
                    {
                        mat->bind(0,SB_Skin,{skin->buffer().get()});
                        mat->update();
                    }
                }
            }
            if (computeCmd != nullptr)
            {
                computeCmd->end();
                computeCmd->submit(ctx().device().queueCompute());
            }
        }
        virtual void    clear() override
        {
            _skins.clear();
            _updateList.clearObjects();
        }
    protected:
        using   Skins   =   std::vector<Skin>;
        bool    contains(FESkin* skin) const
        {
            for (auto& var : _skins)
            {
                if (var.get() == skin)
                    return  true;
            }
            return  false;
        }
    protected:
        /// 登记的蒙皮组件(由场景分发,节点移除时摘除)
        Skins   _skins;
    };

    using   SkinSys =   SharedPtr<FESkinSys>;
}
