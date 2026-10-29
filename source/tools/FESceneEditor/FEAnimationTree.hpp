/// <summary>
/// FEAnimationTree
/// 时间线左侧的动画树:按"节点 → 动画 → 轨道"三级结构显示已登记到动画系统的动画,
/// 支持节点/动画折叠与搜索。数据来源为 FEAnimationSys(即"添加到动画树"后的结果),
/// 行布局(行高/滚动偏移)由时间线传入,保证与时间线画布逐行对齐。
/// </summary>

#pragma     once

#include    "../../../inc/animation/FEAnimation.hpp"
#include    "../../../inc/animation/FEKeyFrameTrack.hpp"
#include    "../../../inc/node/FENode.hpp"

#include    <imgui.h>

#include    <functional>
#include    <map>
#include    <string>
#include    <vector>

namespace   FE
{
    class   FEAnimationSys;

    class   FEAnimationTree
    {
    public:
        /// <summary>
        /// 行类型:节点 / 动画 / 轨道
        /// </summary>
        enum    class   RowType
        {
            Node,
            Animation,
            Track,
        };
        /// <summary>
        /// 动画树的一行
        /// </summary>
        struct  Row
        {
            RowType         _type   =   RowType::Track;
            Node            _node;
            Animation       _anim;
            KeyFrameTrack   _track;
        };
        /// <summary>
        /// 行列表 / 折叠状态表
        /// </summary>
        using   Rows                =   std::vector<Row>;
        using   AnimationCollapseMap=   std::map<Animation,bool>;
        using   NodeCollapseMap     =   std::map<const FENode*,bool>;
        /// <summary>
        /// 名称列绘制参数(由时间线提供,与画布行布局共享)
        /// </summary>
        struct  Layout
        {
            ImDrawList*     _drawList   =   nullptr;
            /// <summary>
            /// 名称列左右边界
            /// </summary>
            float           _left       =   0.0f;
            float           _right      =   0.0f;
            /// <summary>
            /// 可见行区域上下边界
            /// </summary>
            float           _top        =   0.0f;
            float           _bottom     =   0.0f;
            /// <summary>
            /// 第 0 行顶部屏幕 y(已含滚动偏移)
            /// </summary>
            float           _rowTop0    =   0.0f;
            float           _rowH       =   22.0f;
        };
        /// <summary>
        /// 工具栏图标按钮回调(由时间线提供,复用其图标字体)
        /// </summary>
        using   IconButtonFn    =   std::function<bool(const char*,const char*,const char*,bool)>;
    public:
        FEAnimationTree() = default;
        ~FEAnimationTree() = default;
        FEAnimationTree(const FEAnimationTree&)             =   delete;
        FEAnimationTree& operator=(const FEAnimationTree&)  =   delete;
    public:
        /// <summary>
        /// 清空全部数据与编辑状态
        /// </summary>
        void    reset();
        /// <summary>
        /// 从动画系统重新收集已登记的动画
        /// </summary>
        /// <param name="sys">动画系统(动画树的数据来源)</param>
        void    reload(FEAnimationSys* sys);
        /// <summary>
        /// 重建行列表(节点 + 动画 + 轨道),并按当前搜索条件过滤
        /// </summary>
        void    rebuild();
        const   Animations&             animations() const  {   return  _anims;   }
        Animations&                     animations()        {   return  _anims;   }
        const   Rows&                   rows() const        {   return  _rows;    }
        Animation   activeAnim() const                      {   return  _activeAnim;   }
        void    setActiveAnim(Animation anim)                {   _activeAnim =   anim;  }
        /// <summary>
        /// 动画树上方搜索栏(搜索框 + 大小写匹配 + 全匹配)
        /// </summary>
        /// <param name="iconButton">时间线的图标按钮实现</param>
        /// <param name="groupWidth">搜索栏可用宽度</param>
        void    drawSearchBar(const IconButtonFn& iconButton,float groupWidth);
        /// <summary>
        /// 名称列:底色、折叠三角与名称
        /// </summary>
        void    drawNames(const Layout& layout) const;
        /// <summary>
        /// 名称列点击:节点行切换节点折叠,动画行切换折叠并设为活动动画
        /// </summary>
        /// <returns>是否命中可折叠行</returns>
        bool    onClickNameColumn(int row);
    protected:
        /// <summary>
        /// 节点行是否已折叠
        /// </summary>
        bool    isNodeCollapsed(const Node& node) const;
        /// <summary>
        /// 动画行是否已折叠
        /// </summary>
        bool    isAnimCollapsed(Animation anim) const;
        /// <summary>
        /// 名称是否命中当前搜索条件
        /// </summary>
        bool    nameMatches(const std::string& name) const;
    protected:
        /// <summary>
        /// 已登记到动画系统的动画
        /// </summary>
        Animations                  _anims;
        Rows                        _rows;
        /// <summary>
        /// 动画折叠状态
        /// </summary>
        AnimationCollapseMap        _collapsed;
        /// <summary>
        /// 节点折叠状态
        /// </summary>
        NodeCollapseMap             _collapsedNodes;
        /// <summary>
        /// 当前活动动画(添加关键帧的目标)
        /// </summary>
        Animation                   _activeAnim;
        /// <summary>
        /// 搜索框输入缓冲
        /// </summary>
        char                        _searchBuf[64]  =   {   0   };
        /// <summary>
        /// 已执行的搜索关键字(回车后才生效)
        /// </summary>
        std::string                 _searchQuery;
        /// <summary>
        /// 搜索是否区分大小写
        /// </summary>
        bool                        _searchCase     =   false;
        /// <summary>
        /// 搜索是否全字匹配
        /// </summary>
        bool                        _searchWhole    =   false;
    };
}
