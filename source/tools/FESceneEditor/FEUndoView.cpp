/// <summary>
/// FEUndoView 实现:撤销历史窗口(参考 Qt 的 QUndoView)
/// </summary>

#include    "FEUndoView.hpp"
#include    "FEUiStyle.hpp"
#include    "FEUiMetrics.hpp"
#include    "FEUiStrings.hpp"

#include    <cstdio>
#include    <imgui.h>

namespace
{
    /// <summary>
    /// 撤销窗口默认位置与尺寸
    /// </summary>
    constexpr   float   kWindowX          =   316.0f;
    constexpr   float   kWindowYOffset    =   8.0f;
    constexpr   float   kWindowW          =   320.0f;
    constexpr   float   kWindowH          =   420.0f;
    /// <summary>
    /// 行最小宽度、圆角与文字左边距
    /// </summary>
    constexpr   float   kRowMinWidth      =   1.0f;
    constexpr   float   kRowRounding      =   2.0f;
    constexpr   float   kRowTextOffsetX   =   4.0f;
    /// <summary>
    /// 条目标签缓冲长度
    /// </summary>
    constexpr   int     kLabelBufSize     =   512;
}

namespace   FE
{
    void    FEUndoView::toggle()
    {
        _open   =   !_open;
    }

    void    FEUndoView::setOpen(bool open)
    {
        _open   =   open;
    }

    void    FEUndoView::draw(FEUndoStack& stack,Actions& pending)
    {
        if (!_open)
            return;

        /// <summary>
        /// 首次打开落在模型树右侧,避免遮挡其它面板
        /// </summary>
        ImGui::SetNextWindowPos(ImVec2(kWindowX,FE::ui::kRibbonHeight + kWindowYOffset),
                                ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(kWindowW,kWindowH),ImGuiCond_FirstUseEver);
        if (ImGui::Begin("撤销历史",&_open))
        {
            const   int     count   =   stack.count();
            const   int     index   =   stack.index();
            /// <summary>
            /// 本帧点击的目标游标;-1 表示无点击(合法游标范围是 0..count)
            /// </summary>
            int     target      =   -1;
            /// <summary>
            /// 本帧是否点了"清空":丢弃 redo 分支
            /// </summary>
            bool    clearRedo   =   false;

            /// <summary>
            /// 顶部按钮行:重置(跳到开始) / 清空(删除所有 redo 数据)
            /// </summary>
            const   float   spacing     =   ImGui::GetStyle().ItemSpacing.x;
            const   float   buttonW     =   (ImGui::GetContentRegionAvail().x - spacing) * 0.5f;
            const   float   buttonH     =   ImGui::GetFrameHeight();
            if (ImGui::Button(FE::ui::text::kReset,ImVec2(buttonW,buttonH)))
                target  =   0;
            ImGui::SameLine();
            if (ImGui::Button(FE::ui::text::kClear,ImVec2(buttonW,buttonH)))
                clearRedo   =   true;

            ImGui::TextDisabled("已执行 %d / 共 %d 条",index,count);
            /// <summary>
            /// 提示单独一行,避免与计数挤在一起被窗口宽度截断
            /// </summary>
            ImGui::TextDisabled("单击条目可回滚/重做到该位置");
            ImGui::Separator();

            if (count == 0)
            {
                ImGui::TextDisabled("暂无历史记录");
            }
            else
            {
                const   ImVec4  dim     =   ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                /// <summary>
                /// 行高与模型树一致:高度取 ui::rowHeight(),行之间留 ui::kRowGap,
                /// 文字按 (行高 - 文本行高) / 2 垂直居中
                /// </summary>
                const   float   rowHeight   =   ui::rowHeight();
                const   float   rowGap      =   ui::kRowGap;
                const   float   textOffset  =   (rowHeight - ImGui::GetTextLineHeight()) * 0.5f;
                const   ImU32   dimColor    =   ImGui::GetColorU32(dim);
                ImGui::BeginChild("##undo_list",ImVec2(0.0f,0.0f),false,
                                  ImGuiWindowFlags_HorizontalScrollbar);
                for (int i = 0; i < count; ++i)
                {
                    /// <summary>
                    /// 游标之后的命令已被撤销,用暗色区分;
                    /// 游标之前(含游标)为已执行状态
                    /// </summary>
                    const   bool    undone  =   (i >= index);
                    const   ImVec2  start   =   ImGui::GetCursorScreenPos();
                    float           width   =   ImGui::GetContentRegionAvail().x;
                    if (width < kRowMinWidth)
                        width   =   kRowMinWidth;
                    char    label[kLabelBufSize]  =   {};
                    snprintf(label,sizeof(label),"%3d  %s",i + 1,stack.text(i).c_str());

                    ImGui::PushID(i);
                    ImGui::SetCursorScreenPos(start);
                    const   bool    pressed =   ImGui::InvisibleButton("##undo_row"
                                                                      ,ImVec2(width,rowHeight));
                    const   bool    hovered =   ImGui::IsItemHovered();
                    ImGui::PopID();
                    if (pressed)
                        target  =   i + 1;

                    /// <summary>
                    /// 行底色:选中态落在最后一条已执行的命令上,悬停行用淡色
                    /// </summary>
                    ImDrawList*     dl      =   ImGui::GetWindowDrawList();
                    const   ImVec2  end     =   ImVec2(start.x + width,start.y + rowHeight);
                    if (i == index - 1)
                        dl->AddRectFilled(start,end,ui::kRowSelected,kRowRounding);
                    else if (hovered)
                        dl->AddRectFilled(start,end,ui::kRowHover,kRowRounding);
                    dl->AddText(ImVec2(start.x + kRowTextOffsetX,start.y + textOffset)
                                ,(i == index - 1) ? ui::kTextSelected
                                                  : (undone ? dimColor : ui::kTextColor)
                                ,label);

                    /// <summary>
                    /// 光标移到下一行:行高 + 行间距,与模型树行步进一致
                    /// </summary>
                    ImGui::SetCursorScreenPos(ImVec2(start.x,start.y + rowHeight + rowGap));
                }
                ImGui::EndChild();
            }

            /// <summary>
            /// 清空只丢弃 redo 分支,不触碰场景,可以立即执行
            /// </summary>
            if (clearRedo && stack.canRedo())
                stack.discardRedo();
            /// <summary>
            /// 回滚/重做(含"重置"跳到开始)会增删场景节点,
            /// 与撤销按钮一样必须推迟到渲染趟之外
            /// </summary>
            if (target >= 0 && target != index)
            {
                FEUndoStack*    owner   =   &stack;
                pending.push_back([owner,target]() {    owner->setIndex(target);    });
            }
        }
        ImGui::End();
    }
}
