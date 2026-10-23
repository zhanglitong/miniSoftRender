#pragma     once

#include    "node/FENode.hpp"
#include    <algorithm>

namespace   FE
{
    class   FENodeHelper
    {
    public:
        /// <summary>
        /// 修改节点组件的优先级,并重新排序节点组件列表
        /// 直接通过 component->priority() 修改会破坏 _coms 的排序,
        /// 导致组件更新顺序与优先级不一致
        /// </summary>
        /// <param name="node">组件所属节点</param>
        /// <param name="com">节点上的组件</param>
        /// <param name="priority">新的优先级</param>
        /// <param name="order">同优先级内的排序值,越小越靠前</param>
        /// <returns>true 修改成功,组件不属于该节点或参数无效时返回 false</returns>
        static  bool    setComponentPriority(FENode* node,Component com,EPriority priority,int16_t order)
        {
            if (node == nullptr || com == nullptr)
                return  false;

            auto&   coms    =   node->_coms;
            auto    itr     =   std::find(coms.begin(),coms.end(),com);
            if (itr == coms.end())
                return  false;

            com->priority().setPriority(priority);
            com->priority().setOrder(order);
            /// 稳定排序,保证同优先级同 order 的组件保持原有相对顺序
            std::stable_sort(coms.begin(),coms.end(),FEComponent::compare);
            return  true;
        }
    };
}
