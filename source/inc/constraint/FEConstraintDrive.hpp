#pragma     once

#include    <algorithm>
#include    <vector>

#include    "FEConstraint.hpp"
#include    "../node/FENode.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEConstraintDrive, "{E7C2A4D1-9B38-4F5A-8D6E-2C1B7A9F0E43}");

    /// <summary>
    /// 驱动约束: 约束所有者(驱动节点)等效父节点,可同时驱动多个被驱动节点(子节点)
    /// 被驱动节点按添加时记录的相对偏移跟随驱动节点的完整 TRS:
    ///     global = driverGlobal * offset
    /// 节点树结构保持不变,不改变真实父子层级
    ///
    /// 典型场景: 一个根节点带动多个挂件/零件一起移动、旋转、缩放
    ///
    /// 用法:
    ///   ConstraintDrive drive = new FEConstraintDrive(ctx);
    ///   driverNode->addComponent(Component(drive.get()));
    ///   drive->addDrivenNode(childA);   /// 添加时记录偏移,添加瞬间不跳变
    ///   drive->addDrivenNode(childB);
    ///   scene->dispatchNodesToSystem({driverNode,childA,childB});
    /// </summary>
    class   FE_API  FEConstraintDrive : public FEConstraint
    {
    public:
        /// <summary>
        /// 被驱动节点及其相对驱动节点的偏移
        /// </summary>
        struct  DrivenNode
        {
            Node        _node;
            FETransform _offset;    ///< 相对驱动节点的本地偏移(添加/重记时计算)
        };
        using   DrivenNodes     =   std::vector<DrivenNode>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEConstraintDrive)
    public:
        FEConstraintDrive(FEContext& ctx)
            :FEConstraint(ctx)
        {
            /// 驱动约束最后执行: 读取驱动节点自身约束求解后的世界变换
            _priority.setPriority(EPriority::EP_Last);
            _priority.setOrder(MaxInt16);
        }
        FEConstraintDrive(const FEConstraintDrive& other)    =   default;
        ~FEConstraintDrive()                                =   default;
    public:
        /// <summary>
        /// 被驱动节点列表
        /// </summary>
        const   DrivenNodes&    drivenNodes() const
        {
            return  _drivenNodes;
        }
        inline  size_t  drivenNodeCount() const
        {
            return  _drivenNodes.size();
        }
        /// <summary>
        /// 添加被驱动节点: 按当前世界姿态记录相对驱动节点的偏移
        /// </summary>
        /// <param name="node">被驱动节点</param>
        /// <returns>是否添加成功(重复节点/空节点/未关联驱动节点时返回 false)</returns>
        bool    addDrivenNode(Node node)
        {
            auto    driver  =   _owner ? _owner->cast<FENode>() : nullptr;
            if (driver == nullptr || node == nullptr || node.get() == driver)
                return  false;
            for (auto& var : _drivenNodes)
            {
                if (var._node == node)
                    return  false;
            }
            DrivenNode  driven  =   {};
            driven._node        =   node;
            driven._offset      =   calcOffset(node.get());
            _drivenNodes.push_back(driven);
            return  true;
        }
        /// <summary>
        /// 移除被驱动节点
        /// </summary>
        /// <param name="node">被驱动节点</param>
        /// <returns>是否移除成功</returns>
        bool    removeDrivenNode(Node node)
        {
            auto    itr =   std::find_if(_drivenNodes.begin(),_drivenNodes.end(),[node](const DrivenNode& var)
            {
                return  var._node == node;
            });
            if (itr == _drivenNodes.end())
                return  false;
            _drivenNodes.erase(itr);
            return  true;
        }
        /// <summary>
        /// 清空所有被驱动节点
        /// </summary>
        void    clearDrivenNodes()
        {
            _drivenNodes.clear();
        }
        /// <summary>
        /// 按节点当前世界姿态重记相对驱动节点的偏移
        /// </summary>
        /// <param name="node">已添加的被驱动节点</param>
        /// <returns>是否重记成功</returns>
        bool    recordOffset(Node node)
        {
            for (auto& var : _drivenNodes)
            {
                if (var._node == node)
                {
                    var._offset  =   calcOffset(node.get());
                    return  true;
                }
            }
            return  false;
        }
        /// <summary>
        /// 驱动节点自身不改变
        /// </summary>
        virtual FETransform solve(const FETransform& currentWorld,const real& time) const override
        {
            UNUSED(time);
            return  currentWorld;
        }
        /// <summary>
        /// 求解所有被驱动节点: 被驱动节点世界变换 = 驱动节点世界变换 * 记录偏移
        /// 驱动节点自身不写回(其本帧世界变换未改变),避免重复反解本地变换引入误差
        /// </summary>
        virtual size_t  solveNodes(Node owner,const FETransform& current,const real& time,SolvedNodes& results) const override
        {
            UNUSED(owner);
            UNUSED(time);
            size_t  count   =   0;
            for (auto& var : _drivenNodes)
            {
                if (var._node == nullptr)
                    continue;
                results.push_back({var._node,current.compose(var._offset)});
                ++count;
            }
            return  count;
        }
        /// <summary>
        /// 解除关联时释放被驱动节点引用
        /// </summary>
        virtual void    detach() override
        {
            _drivenNodes.clear();
            FEConstraint::detach();
        }
    protected:
        /// <summary>
        /// 计算节点相对驱动节点的本地偏移:
        ///     offset = inverse(driverGlobal) * nodeGlobal
        /// </summary>
        /// <param name="node">被驱动节点</param>
        /// <returns>相对偏移</returns>
        FETransform     calcOffset(FENode* node) const
        {
            FETransform offset;
            auto    driver  =   _owner ? _owner->cast<FENode>() : nullptr;
            if (driver == nullptr || node == nullptr)
                return  offset;
            mat4r   localMat    =   FE::inverse(driver->globalTransform()) * node->globalTransform();
            return  FETransform::fromMatrix(localMat);
        }
    protected:
        DrivenNodes     _drivenNodes;
    };

    using   ConstraintDrive     =   SharedPtr<FEConstraintDrive>;
}
