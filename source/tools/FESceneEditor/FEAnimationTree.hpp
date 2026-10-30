/// <summary>
/// FEAnimationTree
/// 时间线左侧的动画树:按"播放器 → 节点 → 动画 → 轨道"四级结构显示已登记到动画系统的动画,
/// 支持播放器/节点/动画折叠与搜索。数据来源为 FEAnimationSys(即"添加到播放器"后的结果),
/// 行布局(行高/滚动偏移)由时间线传入,保证与时间线画布逐行对齐。
/// </summary>

#pragma     once

#include    "FEAnimItem.hpp"
#include    "FEUiMetrics.hpp"

#include    "../../../inc/animation/FEPlayback.hpp"
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
        /// 折叠状态表
        /// </summary>
        using   AnimationCollapseMap=   std::map<Animation,bool>;
        using   NodeCollapseMap     =   std::map<const FENode*,bool>;
        using   PlaybackCollapseMap =   std::map<const FEPlayback*,bool>;
        /// <summary>
        /// 一个播放器及其登记的动画(播放器名见 Playback::name())
        /// </summary>
        struct  PlaybackEntry
        {
            Playback        _playback;
            Animations      _anims;
        };
        using   PlaybackEntries     =   std::vector<PlaybackEntry>;
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
            /// <summary>
            /// 行高(不含行间距)
            /// </summary>
            float           _rowH       =   24.0f;
            /// <summary>
            /// 行步进(行高 + 行间距),与模型树一致
            /// </summary>
            float           _rowStep    =   28.0f;
            /// <summary>
            /// 鼠标悬停行,-1 表示无
            /// </summary>
            int             _hoverRow   =   -1;
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
        const   AnimItems&              rows() const        {   return  _rows;    }
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
        /// 名称列点击:播放器/节点/动画行切换折叠(箭头区域使用)
        /// </summary>
        /// <returns>是否命中可折叠行</returns>
        bool    onClickNameColumn(int row);
        /// <summary>
        /// 动画树选中集合(与模型树各自独立,不使用 FLAG_SELECTED)
        /// </summary>
        bool    isSelected(const Object& object) const;
        void    selectSingle(const Object& object);
        void    toggleSelect(const Object& object);
        void    selectRangeTo(const Object& object);
        void    clearSelection();
        void    addToSelection(const Object& object);
        int     rowIndexOf(const Object& object) const;
        const   Objects&                selection() const    {   return  _selection;   }
        /// <summary>
        /// 行所属的动画:动画行返回自身,轨道行返回其上方最近的动画行(行列表按"动画 → 轨道"连续排列)
        /// </summary>
        /// <param name="row">行号</param>
        /// <returns>所属动画;节点/播放器行返回空</returns>
        Animation   rowAnim(int row) const;
    protected:
        /// <summary>
        /// 节点行是否已折叠
        /// </summary>
        bool    isNodeCollapsed(const Node& node) const;
        /// <summary>
        /// 播放器行是否已折叠
        /// </summary>
        bool    isPlaybackCollapsed(const Playback& playback) const;
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
        /// 已登记到动画系统的播放器(按名字排序)及其动画
        /// </summary>
        PlaybackEntries             _playbackEntries;
        /// <summary>
        /// 已登记到动画系统的动画(各播放器的动画汇总)
        /// </summary>
        Animations                  _anims;
        /// <summary>
        /// 行列表
        /// </summary>
        AnimItems                   _rows;
        /// <summary>
        /// 动画树选中集合(行对象指针;主选中项用于 Shift 范围锚点)
        /// </summary>
        Objects                     _selection;
        Object                      _selected;
        Object                      _anchor;
        /// <summary>
        /// 播放器折叠状态
        /// </summary>
        PlaybackCollapseMap         _collapsedPlaybacks;
        /// <summary>
        /// 默认播放器是否已补建:只在绑定新场景后补建一次,删除后不再自动重建
        /// </summary>
        bool                        _defaultEnsured     =   false;
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
        char                        _searchBuf[ui::kSearchBufSize]  =   {   0   };
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
