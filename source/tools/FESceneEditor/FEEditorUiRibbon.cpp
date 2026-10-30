/// <summary>
/// FEEditorUi 的 Ribbon 功能区绘制(标签页 / 分组 / 按钮 / 滚动箭头 / 关于窗口),
/// 从 FEEditorUi.cpp 拆分而来;版式常量与绘制工具见 FEEditorUiInternal.hpp。
/// </summary>

#include    "FEEditorUi.hpp"
#include    "FEEditorUiInternal.hpp"
#include    "FEUiStrings.hpp"

#include    "../../../inc/graphic/FEScene.h"

#include    <algorithm>
#include    <cmath>
#include    <cstdio>
#include    <cstring>
#include    <string>
#include    <vector>

#include    <imgui.h>

using   namespace   FE::ui;
using   namespace   FE::ui::internal;

namespace   FE
{
    void    FEEditorUi::drawRibbon()
    {
        ImGuiIO&    io      =   ImGui::GetIO();
        const float width   =   io.DisplaySize.x;
        if (width < 1.0f)
            return;

        processShortcuts();
        /// <summary>
        /// 先同步抓手:按钮绘制之前更新勾选态,选区变化也在此推给已显示的抓手
        /// </summary>
        syncTransformTools();

        ImGui::SetNextWindowPos(ImVec2(0.0f,0.0f),ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width,kRibbonHeight),ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0.0f,0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize,0.0f);

        const   ImGuiWindowFlags    flags   =   ImGuiWindowFlags_NoTitleBar         |   ImGuiWindowFlags_NoResize
                                            |   ImGuiWindowFlags_NoMove             |   ImGuiWindowFlags_NoScrollbar
                                            |   ImGuiWindowFlags_NoScrollWithMouse  |   ImGuiWindowFlags_NoCollapse
                                            |   ImGuiWindowFlags_NoSavedSettings    |   ImGuiWindowFlags_NoBringToFrontOnFocus;
        /// <summary>
        /// 本帧被点击的动作:窗口结束后统一分发
        /// </summary>
        const   FEUiMenuItem*   triggered   =   nullptr;
        if (ImGui::Begin("##fesr_ribbon",nullptr,flags))
        {
            ImDrawList*     dl          =   ImGui::GetWindowDrawList();
            const   ImVec2  wp          =   ImGui::GetWindowPos();
            const   float   barBottom   =   kGroupRowY - kTabBarBottomGap;
            /// <summary>
            /// 标签条与功能区两层底色;选中标签与功能区同色,视觉上连成一体
            /// </summary>
            dl->AddRectFilled(wp,ImVec2(wp.x + width,wp.y + barBottom),kTabBarBg);
            dl->AddRectFilled(ImVec2(wp.x,wp.y + barBottom),ImVec2(wp.x + width,wp.y + kRibbonHeight),kRibbonBg);

            /// <summary>
            /// 标签页行:MainWindow.ui 的主菜单即 Ribbon 的 Page
            /// </summary>
            std::vector<const FEUiMenuItem*>    pages;
            if (_uiDoc.menuBar() != nullptr)
            {
                for (const auto* page : _uiDoc.menuBar()->_children)
                {
                    if (page->_type == kUiMenu)
                        pages.push_back(page);
                }
            }
            if (_activeTab < 0 || _activeTab >= int(pages.size()))
                _activeTab  =   0;
            ImGui::SetCursorPos(ImVec2(kTabStartX,kTabRowY));
            for (int i = 0; i < int(pages.size()); ++i)
            {
                if (ribbonTab(pages[size_t(i)]->_text.c_str(),_activeTab == i))
                {
                    _activeTab      =   i;
                    /// <summary>
                    /// 换页后内容整体变化,滚动位置回到最左
                    /// </summary>
                    _ribbonScrollX  =   0.0f;
                }
                ImGui::SameLine(0.0f,kTabSpacing);
            }
            /// <summary>
            /// 标签行右边界:右上角信息从这里开始,避免与标签重叠
            /// </summary>
            const   float   tabRight    =   ImGui::GetCursorPosX();

            /// <summary>
            /// 右上角:当前工程名、状态提示与帧率(右对齐,超出部分裁剪)
            /// </summary>
            {
                const   std::string projectName =   _projectName.empty() ? std::string("未命名工程")
                                                                         : _projectName;
                char    info[kInfoBufSize]  =   {};
                if (_statusText.empty())
                    snprintf(info,sizeof(info),"%s   %.1f FPS",projectName.c_str(),io.Framerate);
                else
                    snprintf(info,sizeof(info),"%s   %.1f FPS   |   %s",projectName.c_str(),io.Framerate,
                             _statusText.c_str());
                const   ImVec2  infoSize    =   ImGui::CalcTextSize(info);
                const   float   infoRight   =   width - kInfoMarginX;
                const   float   infoLeft    =   std::max(tabRight,kInfoMarginX);
                dl->PushClipRect(ImVec2(wp.x + infoLeft,wp.y),ImVec2(wp.x + infoRight,wp.y + barBottom),true);
                ImGui::SetCursorPos(ImVec2(std::max(infoLeft,infoRight - infoSize.x),
                                           kTabRowY + kInfoOffsetY));
                ImGui::TextDisabled("%s",info);
                dl->PopClipRect();
            }

            /// <summary>
            /// 功能区:Page 下的子菜单为分组,分组内的动作为按钮;
            /// Page 下直接挂的动作归入以 Page 标题命名的分组
            /// </summary>
            if (!pages.empty())
            {
                const   FEUiMenuItem*   page    =   pages[size_t(_activeTab)];

                /// <summary>
                /// 先按 .ui 中 addaction 的原始顺序组装分组列表:
                /// Page 下直接挂的动作按出现顺序成组,遇到子菜单则另起一个分组
                /// </summary>
                struct  FERibbonGroup
                {
                    RibbonActions           _actions;
                    /// <summary>
                    /// 分组菜单;为空表示这组来自 Page 下直接挂的动作
                    /// </summary>
                    const   FEUiMenuItem*   _menu   =   nullptr;
                };
                std::vector<FERibbonGroup>  groups;
                RibbonActions   direct;
                auto    flushDirect =   [&]()
                {
                    if (direct.empty())
                        return;
                    FERibbonGroup   group;
                    group._actions  =   direct;
                    groups.push_back(group);
                    direct.clear();
                };
                for (const auto* child : page->_children)
                {
                    if (child->_type == kUiAction)
                    {
                        direct.push_back(child);
                    }
                    else if (child->_type == kUiMenu)
                    {
                        flushDirect();
                        FERibbonGroup   group;
                        group._menu =   child;
                        collectGroupActions(child,group._actions);
                        if (!group._actions.empty())
                            groups.push_back(group);
                    }
                }
                flushDirect();

                /// <summary>
                /// 内容总宽 = 各分组宽度之和(含组间间距)
                /// </summary>
                float   contentW    =   0.0f;
                for (const auto& group : groups)
                    contentW    +=  ribbonMetrics(group._actions)._groupW + kGroupGap;

                /// <summary>
                /// 放不下时左右各留出箭头宽度,可视区间相应收窄
                /// </summary>
                const   bool    overflow    =   (contentW > width - kGroupStartX * 2.0f);
                const   float   viewL       =   overflow ? kArrowW : kGroupStartX;
                const   float   viewR       =   overflow ? (width - kArrowW) : (width - kGroupStartX);
                const   float   maxScroll   =   std::max(0.0f,contentW - (viewR - viewL));
                _ribbonScrollX  =   std::max(0.0f,std::min(_ribbonScrollX,maxScroll));

                /// <summary>
                /// 按钮内容整体左移 scrollX,并裁剪到可视区间
                /// </summary>
                dl->PushClipRect(ImVec2(wp.x + viewL,wp.y + barBottom),
                                 ImVec2(wp.x + viewR,wp.y + kRibbonHeight),true);
                float   x   =   viewL - _ribbonScrollX;
                for (const auto& group : groups)
                {
                    const   char*   title   =   (group._menu != nullptr) ? group._menu->_text.c_str()
                                                                        : page->_text.c_str();
                    x   +=  drawRibbonGroup(group._actions,title,x,_iconFont,&triggered);
                }
                dl->PopClipRect();

                /// <summary>
                /// 放不下时在左右两侧画滚动箭头,点击按 kScrollStep 步进
                /// </summary>
                if (overflow)
                {
                    const   float   arrowY  =   wp.y + kGroupRowY + (kContentH - kArrowW) * 0.5f;
                    if (scrollArrow(ImVec2(wp.x,arrowY),kArrowW,_ribbonScrollX > 0.0f,false))
                        _ribbonScrollX  =   std::max(0.0f,_ribbonScrollX - kScrollStep);
                    if (scrollArrow(ImVec2(wp.x + width - kArrowW,arrowY),kArrowW,
                                    _ribbonScrollX < maxScroll,true))
                        _ribbonScrollX  =   std::min(maxScroll,_ribbonScrollX + kScrollStep);
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(3);

        /// <summary>
        /// Ribbon 按钮动作分发。文件对话框是普通的 ImGui 窗口(非系统模态),
        /// 在帧内处理不会引起消息重入
        /// </summary>
        if (triggered != nullptr)
            onAction(triggered);

        /// <summary>
        /// 关于窗口
        /// </summary>
        if (_aboutOpen)
        {
            /// <summary>
            /// 首次打开落在功能区下方居中,避免盖住标签与按钮
            /// </summary>
            ImGui::SetNextWindowPos(ImVec2((width - kAboutWindowW) * 0.5f,kRibbonHeight + kAboutYOff),
                                    ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(kAboutWindowW,0.0f),ImGuiCond_FirstUseEver);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(kAboutPad,kAboutPad));
            if (ImGui::Begin("关于 FESceneEditor",&_aboutOpen,ImGuiWindowFlags_NoCollapse))
            {
                ImDrawList*     dl      =   ImGui::GetWindowDrawList();
                const   float   availW  =   ImGui::GetContentRegionAvail().x;

                /// <summary>
                /// 品牌区:深色底 + 左侧强调条 + 图标徽章 + 标题/副标题/版本标签
                /// </summary>
                const   ImVec2  heroPos =   ImGui::GetCursorScreenPos();
                const   ImVec2  heroEnd(heroPos.x + availW,heroPos.y + kAboutHeroH);
                dl->AddRectFilled(heroPos,heroEnd,kAboutHeroBg,kAboutRowRounding);
                dl->AddRectFilled(heroPos,ImVec2(heroPos.x + kAboutAccentW,heroEnd.y),
                                  ui::kAccent,kAboutRowRounding);

                const   ImVec2  iconCenter(heroPos.x + kAboutIconCx,
                                           heroPos.y + kAboutHeroH * 0.5f);
                dl->AddCircleFilled(iconCenter,kAboutIconR,IM_COL32(255,255,255,18));
                if (_iconFont != nullptr)
                {
                    ImGui::PushFont(_iconFont);
                    const   ImVec2  glyphSize   =   ImGui::CalcTextSize(ui::kGlyphCube);
                    dl->AddText(ImVec2(iconCenter.x - glyphSize.x * 0.5f,
                                       iconCenter.y - glyphSize.y * 0.5f),
                                ui::kAccent,ui::kGlyphCube);
                    ImGui::PopFont();
                }

                const   float   textX   =   heroPos.x + kAboutTextX;
                if (_titleFont != nullptr)
                    ImGui::PushFont(_titleFont);
                dl->AddText(ImVec2(textX,heroPos.y + kAboutTitleY),ui::kTextColor,"FESceneEditor");
                if (_titleFont != nullptr)
                    ImGui::PopFont();
                dl->AddText(ImVec2(textX,heroPos.y + kAboutSubY),ui::kTextMuted,
                            "miniSoftRender 场景编辑器");

                /// <summary>
                /// 右上角版本标签
                /// </summary>
                const   char*   chip    =   "开发版";
                const   ImVec2  chipSize=   ImGui::CalcTextSize(chip);
                const   ImVec2  chipMin(heroEnd.x - chipSize.x - kAboutChipPadX * 2.0f,
                                        heroPos.y + kAboutChipY);
                const   ImVec2  chipMax(chipMin.x + chipSize.x + kAboutChipPadX * 2.0f,
                                        chipMin.y + kAboutChipH);
                dl->AddRectFilled(chipMin,chipMax,kAboutChipBg,kAboutChipH * 0.5f);
                dl->AddRect(chipMin,chipMax,kAboutChipBorder,kAboutChipH * 0.5f);
                dl->AddText(ImVec2(chipMin.x + kAboutChipPadX,
                                   chipMin.y + (kAboutChipH - chipSize.y) * 0.5f),
                            kAboutChipText,chip);

                ImGui::Dummy(ImVec2(0.0f,kAboutHeroH));

                /// <summary>
                /// 分组标题:左侧短强调条 + 标题文字
                /// </summary>
                auto    section =   [&](const char* title)
                {
                    ImGui::Dummy(ImVec2(0.0f,kAboutSectionGap));
                    const   ImVec2  p   =   ImGui::GetCursorScreenPos();
                    dl->AddRectFilled(p,ImVec2(p.x + kAboutSectionBarW,
                                               p.y + ImGui::GetTextLineHeight()),ui::kAccent);
                    dl->AddText(ImVec2(p.x + kAboutSectionBarW + kAboutRowPadX,p.y),
                                ui::kTextColor,title);
                    ImGui::Dummy(ImVec2(0.0f,ImGui::GetTextLineHeight()));
                };
                /// <summary>
                /// 信息行:左侧标签、右侧取值,交替行用极淡底色区分
                /// </summary>
                auto    infoRow =   [&](const char* label,const char* value,bool alt)
                {
                    const   ImVec2  p       =   ImGui::GetCursorScreenPos();
                    const   float   rowW    =   ImGui::GetContentRegionAvail().x;
                    const   float   lineY   =   p.y + (kAboutRowH - ImGui::GetTextLineHeight()) * 0.5f;
                    if (alt)
                        dl->AddRectFilled(p,ImVec2(p.x + rowW,p.y + kAboutRowH),
                                          kAboutRowAlt,kAboutRowRounding);
                    dl->AddText(ImVec2(p.x + kAboutRowPadX,lineY),ui::kTextMuted,label);
                    const   ImVec2  valueSize   =   ImGui::CalcTextSize(value);
                    dl->AddText(ImVec2(p.x + rowW - valueSize.x - kAboutRowPadX,
                                       p.y + (kAboutRowH - valueSize.y) * 0.5f),
                                ui::kTextColor,value);
                    ImGui::Dummy(ImVec2(0.0f,kAboutRowH));
                };

                char    buffer[64]  =   {};
                section("运行环境");
                infoRow("渲染后端","Vulkan",true);
                std::snprintf(buffer,sizeof(buffer),"Dear ImGui %s",ImGui::GetVersion());
                infoRow("界面框架",buffer,false);
                const   ImVec2  display  =   ImGui::GetIO().DisplaySize;
                std::snprintf(buffer,sizeof(buffer),"%.0f × %.0f",display.x,display.y);
                infoRow("显示分辨率",buffer,true);
                std::snprintf(buffer,sizeof(buffer),"%.1f FPS",ImGui::GetIO().Framerate);
                infoRow("帧率",buffer,false);
                std::snprintf(buffer,sizeof(buffer),"Windows x64 / MSVC %d",_MSC_VER);
                infoRow("构建环境",buffer,true);
                std::snprintf(buffer,sizeof(buffer),"%s %s",__DATE__,__TIME__);
                infoRow("构建时间",buffer,false);

                section("关于");
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
                ImGui::TextWrapped("实时场景编辑器,用于 miniSoftRender 引擎的节点编排、"
                                   "材质与光照调试,以及动画时间线与关键帧编辑。");
                ImGui::PopTextWrapPos();

                /// <summary>
                /// 底部:分隔线 + 版权 + 右对齐关闭按钮
                /// </summary>
                ImGui::Dummy(ImVec2(0.0f,kAboutSectionGap));
                const   ImVec2  footerPos   =   ImGui::GetCursorScreenPos();
                dl->AddLine(footerPos,ImVec2(footerPos.x + availW,footerPos.y),kAboutDivider);
                ImGui::Dummy(ImVec2(0.0f,kAboutFooterGap));
                ImGui::TextDisabled("Copyright (C) miniSoftRender Project");
                const   float   closeW  =   ImGui::CalcTextSize(ui::text::kClose).x + kAboutBtnPadX;
                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - closeW);
                if (ImGui::Button(ui::text::kClose,ImVec2(closeW,0.0f)))
                    _aboutOpen  =   false;
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        drawFileDialog();
        drawModelTree();
        drawProperty();
        drawAxisSetting();
        drawTimeLine();
        /// <summary>
        /// 撤销历史窗口:先把它自己的开关状态回写到动作勾选标志,再绘制
        /// </summary>
        FEUiMenuItem*   undoViewItem    =   findActionItem(_uiDoc.menuBar(),"action_undoView");
        if (undoViewItem != nullptr)
            undoViewItem->_checked  =   _undoView.isOpen();
        _undoView.draw(_undoStack,_pendingActions);
        drawUiPreview();
        drawShortcutSettings();
        /// <summary>
        /// 天空盒开关:设置窗口里改动 render/showSkyBox 后立即同步到场景,
        /// 无需等保存或重新加载
        /// </summary>
        if (_scene != nullptr && _ctx != nullptr)
        {
            const   bool    shown   =   _ctx->setting().get("render").get("showSkyBox").getOr(true);
            if  (shown != _skyShown)
            {
                _skyShown   =   shown;
                _scene->showSky(shown);
            }
        }
        drawSettingViewer();
    }
}
