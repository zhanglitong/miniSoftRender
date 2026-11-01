/// <summary>
/// FEBottomDock
/// 编辑器底部停靠面板:标签栏位于面板底边(参考 VS 的“错误列表 / 输出”);
/// 面板内容由注册的绘制回调提供,新增窗口只需再 addPanel 一项,无需改动绘制流程。
/// 上边缘可拖动调整整体高度,标签过多时可横向滚动。
/// </summary>

#pragma     once

#include    <functional>
#include    <string>
#include    <vector>

#include    <imgui.h>

namespace   FE
{
    class   FEBottomDock
    {
    public:
        FEBottomDock() = default;
        ~FEBottomDock() = default;
        FEBottomDock(const FEBottomDock&)               =   delete;
        FEBottomDock& operator=(const FEBottomDock&)    =   delete;
    public:
        /// <summary>
        /// 注册一个标签页(按注册顺序显示);id 用于动作分发与定位
        /// </summary>
        /// <param name="id">唯一标识</param>
        /// <param name="title">标签文字</param>
        /// <param name="draw">内容绘制回调(在面板内容区内调用)</param>
        /// <param name="onHide">切走或收起该页时调用,可为空</param>
        void    addPanel(const std::string& id,const char* title,
                         std::function<void()> draw,std::function<void()> onHide = nullptr);
        /// <summary>
        /// 移除全部标签页并收起面板(解绑场景时调用)
        /// </summary>
        void    clearPanels();
        /// <summary>
        /// 显示面板并切换到指定标签页
        /// </summary>
        void    showPanel(const std::string& id);
        /// <summary>
        /// 开关指定标签页:面板已显示且正停在该页时收起,否则显示并切换
        /// </summary>
        void    togglePanel(const std::string& id);
        /// <summary>
        /// 收起整个面板
        /// </summary>
        void    hide();
        bool    isOpen()    const   {   return  _open;  }
        /// <summary>
        /// 面板是否显示且正停在指定页
        /// </summary>
        bool    isActive(const std::string& id) const;
        /// <summary>
        /// 每帧绘制;面板收起或没有标签页时直接返回
        /// </summary>
        void    draw();
    protected:
        struct  Panel
        {
            std::string                 _id;
            std::string                 _title;
            std::function<void()>       _draw;
            std::function<void()>       _onHide;
        };
        /// <summary>
        /// 查找标签页下标,-1 表示不存在
        /// </summary>
        int     indexOf(const std::string& id) const;
        /// <summary>
        /// 切换到指定页并显示面板(同页不触发 onHide)
        /// </summary>
        void    activate(int index);
        /// <summary>
        /// 上边缘拖动调整面板高度
        /// </summary>
        void    drawResize(const ImVec2& winPos,const ImVec2& winSize);
        /// <summary>
        /// 底部标签栏(自绘,支持过多时横向滚动)
        /// </summary>
        void    drawTabBar(const ImVec2& winPos,const ImVec2& winSize);
    protected:
        std::vector<Panel>  _panels;
        int                 _active     =   0;
        bool                _open       =   false;
        /// <summary>
        /// 面板高度(<=0 表示使用默认高度);上边缘可拖动调整
        /// </summary>
        float               _panelH     =   -1.0f;
        /// <summary>
        /// 本帧实际使用的高度,供拖动时取起始值
        /// </summary>
        float               _curPanelH  =   0.0f;
        float               _resizeStartH   =   0.0f;
        float               _resizeStartY   =   0.0f;
        /// <summary>
        /// 标签栏横向滚动偏移
        /// </summary>
        float               _tabScrollX =   0.0f;
    };
}
