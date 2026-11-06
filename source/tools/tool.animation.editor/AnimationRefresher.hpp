#pragma     once
#include    "node/FENode.hpp"
#include    "animation/FEAnimation.hpp"
#include    "animation/FEAnimationSys.hpp"
#include    "graphic/FEScene.h"

namespace   FE
{
    /// <summary>
    /// 动画停止贡献(停用/删除)后刷新 owner 节点
    /// 播放暂停时 FEPlayback::update 提前返回,不会调用 FEAnimation::update,
    /// 节点不会自动重算全局变换;此处主动标记并让节点按当前本地变换重算
    /// </summary>
    /// <param name="anim">动画对象</param>
    inline  void    refreshAnimOwner(FEAnimation* anim)
    {
        if (anim == nullptr)
            return;
        auto    owner   =   anim->owner();
        auto    node    =   owner ? owner->cast<FENode>() : nullptr;
        if (node == nullptr)
            return;

        node->flags().addFlag(FENode::FLAG_PROP_TRANS | FENode::FLAG_PROP_SCALE | FENode::FLAG_PROP_ROT);
        node->makeDirty();
        node->update();

        auto    scene   =   node->ctx().scene();
        auto    sys     =   scene ? scene->animationSystem() : nullptr;
        if (sys != nullptr)
            sys->updateList().addObject(node);
    }
}
