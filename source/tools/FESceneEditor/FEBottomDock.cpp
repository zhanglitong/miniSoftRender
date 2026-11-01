#include    "FEBottomDock.hpp"

#include    "FEUiStyle.hpp"

#include    <algorithm>
#include    <cmath>

namespace   FE
{
namespace
{
    /// <summary>
    /// 面板高度下限/顶部留白/默认占比与上边缘拖动条
    /// </summary>
    constexpr   float   kPanelMinH          =   190.0f;
    constexpr   float   kPanelTopMargin     =   80.0f;
    constexpr   float   kPanelDefaultRatio  =   0.34f;
    constexpr   float   kResizeHandleH      =   4.0f;
    constexpr   float   kResizeLineW        =   2.0f;
    constexpr   float   kResizeLineY        =   0.5f;
    /// <summary>
    /// 面板内边距
    /// </summary>
    constexpr   float   kWindowPadX         =   6.0f;
    constexpr   float   kWindowPadY         =   4.0f;
    /// <summary>
    /// 标签栏高度/左右留白/间距与文字留白
    /// </summary>
    constexpr   float   kTabStripH          =   28.0f;
    constexpr   float   kTabStartX          =   6.0f;
    constexpr   float   kTabSpacing         =   2.0f;
    constexpr   float   kTabPadX            =   18.0f;
    constexpr   float   kTabUnderlineH      =   2.0f;
    constexpr   float   kTabScrollStep      =   80.0f;
    /// <summary>
    /// 标签栏底色/选中页底色/分隔线(hover 底色与强调色取 FEUiStyle)
    /// </summary>
    const   ImU32       kTabBarBg           =   IM_COL32(30,33,40,255);
    const   ImU32       kTabActiveBg        =   IM_COL32(41,44,52,255);
    const   ImU32       kTabDivider         =   IM_COL32(255,255,255,26);
    const   ImU32       kTabActiveText      =   IM_COL32(255,255,255,255);
    const   ImU32       kTabHoverText       =   IM_COL32(210,215,225,255);
}

    void    FEBottomDock::addPanel(const std::string& id,const char* title,
                                   std::function<void()> draw,std::function<void()> onHide)
    {
        Panel   panel;
        panel._id       =   id;
        panel._title    =   (title != nullptr) ? title : "";
        panel._draw     =   std::move(draw);
        panel._onHide   =   std::move(onHide);
        _panels.push_back(std::move(panel));
    }

    void    FEBottomDock::clearPanels()
    {
        if (_open)
            hide();
        _panels.clear();
        _active     =   0;
        _tabScrollX =   0.0f;
    }

    int     FEBottomDock::indexOf(const std::string& id) const
    {
        for (size_t i = 0;i < _panels.size();++i)
        {
            if (_panels[i]._id == id)
                return  int(i);
        }
        return  -1;
    }

    void    FEBottomDock::activate(int index)
    {
        if (index < 0 || index >= int(_panels.size()))
            return;
        if (_active != index)
        {
            const   int     old =   _active;
            if (old >= 0 && old < int(_panels.size()) && _panels[size_t(old)]._onHide)
                _panels[size_t(old)]._onHide();
            _active =   index;
        }
        _open   =   true;
    }

    void    FEBottomDock::showPanel(const std::string& id)
    {
        const   int     index   =   indexOf(id);
        if (index >= 0)
            activate(index);
    }

    void    FEBottomDock::togglePanel(const std::string& id)
    {
        const   int     index   =   indexOf(id);
        if (index < 0)
            return;
        if (_open && _active == index)
            hide();
        else
            activate(index);
    }

    void    FEBottomDock::hide()
    {
        if (!_open)
            return;
        if (_active >= 0 && _active < int(_panels.size()) && _panels[size_t(_active)]._onHide)
            _panels[size_t(_active)]._onHide();
        _open   =   false;
    }

    bool    FEBottomDock::isActive(const std::string& id) const
    {
        return  _open && _active == indexOf(id);
    }

    void    FEBottomDock::drawResize(const ImVec2& winPos,const ImVec2& winSize)
    {
        ImGui::SetCursorScreenPos(winPos);
        ImGui::InvisibleButton("##dock_resize_top",ImVec2(winSize.x,kResizeHandleH));
        const   bool    edgeHot =   ImGui::IsItemHovered() || ImGui::IsItemActive();
        if (edgeHot)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        if (ImGui::IsItemClicked(0))
        {
            _resizeStartY   =   ImGui::GetIO().MousePos.y;
            _resizeStartH   =   _curPanelH;
        }
        if (ImGui::IsItemActive())
        {
            const   float   displayH    =   ImGui::GetIO().DisplaySize.y;
            const   float   maxPanelH   =   std::max(kPanelMinH,displayH - kPanelTopMargin);
            _panelH =   std::max(kPanelMinH,
                                std::min(_resizeStartH + (_resizeStartY - ImGui::GetIO().MousePos.y),maxPanelH));
        }
        if (edgeHot)
        {
            ImGui::GetWindowDrawList()->AddLine(ImVec2(winPos.x,winPos.y + kResizeLineY),
                                                ImVec2(winPos.x + winSize.x,winPos.y + kResizeLineY),
                                                ui::kAccent,kResizeLineW);
        }
    }

    void    FEBottomDock::drawTabBar(const ImVec2& winPos,const ImVec2& winSize)
    {
        ImDrawList*     drawList    =   ImGui::GetWindowDrawList();
        const   float   stripY      =   winPos.y + winSize.y - kTabStripH;
        drawList->AddRectFilled(ImVec2(winPos.x,stripY),
                                ImVec2(winPos.x + winSize.x,winPos.y + winSize.y),kTabBarBg);
        drawList->AddLine(ImVec2(winPos.x,stripY),
                          ImVec2(winPos.x + winSize.x,stripY),kTabDivider);

        /// <summary>
        /// 标签总宽度与横向滚动(标签过多时滚轮滚动)
        /// </summary>
        float   totalW  =   kTabStartX;
        for (size_t i = 0;i < _panels.size();++i)
        {
            const   ImVec2  textSize    =   ImGui::CalcTextSize(_panels[i]._title.c_str());
            totalW  +=  textSize.x + kTabPadX * 2.0f;
            if (i > 0)
                totalW  +=  kTabSpacing;
        }
        const   float   maxScroll   =   std::max(0.0f,totalW + kTabStartX - winSize.x);
        if (ImGui::IsMouseHoveringRect(ImVec2(winPos.x,stripY),
                                       ImVec2(winPos.x + winSize.x,winPos.y + winSize.y)))
        {
            const   float   wheel   =   ImGui::GetIO().MouseWheel;
            if (wheel != 0.0f)
                _tabScrollX -=  wheel * kTabScrollStep;
        }
        _tabScrollX =   std::max(0.0f,std::min(_tabScrollX,maxScroll));

        float   x   =   winPos.x + kTabStartX - _tabScrollX;
        for (int i = 0;i < int(_panels.size());++i)
        {
            const   ImVec2  textSize    =   ImGui::CalcTextSize(_panels[size_t(i)]._title.c_str());
            const   float   tabW        =   textSize.x + kTabPadX * 2.0f;
            const   ImVec2  pos         =   ImVec2(x,stripY);
            const   ImVec2  end         =   ImVec2(x + tabW,winPos.y + winSize.y);

            ImGui::PushID(i);
            ImGui::SetCursorScreenPos(pos);
            const   bool    pressed     =   ImGui::InvisibleButton("##dock_tab",ImVec2(tabW,kTabStripH));
            const   bool    hovered     =   ImGui::IsItemHovered();
            ImGui::PopID();

            if (pressed || (hovered && ImGui::IsMouseClicked(0)))
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            /// <summary>
            /// 选中页用面板底色 + 底部强调线;悬停页用极淡底色
            /// </summary>
            if (i == _active)
            {
                drawList->AddRectFilled(pos,end,kTabActiveBg);
                drawList->AddRectFilled(ImVec2(pos.x,end.y - kTabUnderlineH),end,ui::kAccent);
            }
            else if (hovered)
            {
                drawList->AddRectFilled(pos,end,ui::kRowHover);
            }
            const   ImU32   color   =   (i == _active) ? kTabActiveText
                                      : (hovered ? kTabHoverText : ui::kTextMuted);
            drawList->AddText(ImVec2(pos.x + (tabW - textSize.x) * 0.5f,
                                     pos.y + (kTabStripH - textSize.y) * 0.5f),
                              color,_panels[size_t(i)]._title.c_str());

            if (pressed && i != _active)
                activate(i);
            x   +=  tabW + kTabSpacing;
        }
    }

    void    FEBottomDock::draw()
    {
        if (!_open || _panels.empty())
            return;
        if (_active < 0 || _active >= int(_panels.size()))
            _active =   0;

        ImGuiIO&    io          =   ImGui::GetIO();
        const   float   displayW    =   io.DisplaySize.x;
        const   float   displayH    =   io.DisplaySize.y;
        const   float   minPanelH   =   kPanelMinH;
        const   float   maxPanelH   =   std::max(minPanelH,displayH - kPanelTopMargin);
        float   panelH  =   (_panelH > 0.0f) ? _panelH : displayH * kPanelDefaultRatio;
        panelH  =   std::max(minPanelH,std::min(panelH,maxPanelH));
        _curPanelH  =   panelH;

        ImGui::SetNextWindowPos(ImVec2(0.0f,displayH - panelH),ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(displayW,panelH),ImGuiCond_Always);
        const   ImGuiWindowFlags    flags   =   ImGuiWindowFlags_NoTitleBar
                                            |   ImGuiWindowFlags_NoResize
                                            |   ImGuiWindowFlags_NoMove
                                            |   ImGuiWindowFlags_NoSavedSettings
                                            |   ImGuiWindowFlags_NoScrollbar
                                            |   ImGuiWindowFlags_NoScrollWithMouse
                                            |   ImGuiWindowFlags_NoCollapse;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(kWindowPadX,kWindowPadY));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
        ImGui::Begin("##FEBottomDock",nullptr,flags);
        ImGui::PopStyleVar(2);

        const   ImVec2  winPos  =   ImGui::GetWindowPos();
        const   ImVec2  winSize =   ImGui::GetWindowSize();

        /// <summary>
        /// 上边缘拖动区占住顶部内边距,内容子窗口从其后开始,避免互相遮挡
        /// </summary>
        const   ImVec2  contentPos  =   ImGui::GetCursorScreenPos();
        drawResize(winPos,winSize);
        ImGui::SetCursorScreenPos(contentPos);

        /// <summary>
        /// 内容区:高度 = 面板高度减去底部标签栏与顶部内边距
        /// </summary>
        const   float   contentH    =   std::max(1.0f,winSize.y - kTabStripH - kWindowPadY);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(kWindowPadX,kWindowPadY));
        ImGui::BeginChild("##dock_content",ImVec2(0.0f,contentH),false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        if (_panels[size_t(_active)]._draw)
            _panels[size_t(_active)]._draw();
        ImGui::EndChild();
        ImGui::PopStyleVar();

        drawTabBar(winPos,winSize);
        ImGui::End();
    }
}
