/// <summary>
/// FEUndoView 实现:撤销历史窗口(参考 Qt 的 QUndoView)
/// </summary>

#include    "FEUndoView.hpp"

#include    <cstdio>
#include    <imgui.h>

namespace
{
    /// <summary>
    /// 功能区高度:窗口默认落在其下方,与 FEEditorUi 的 kRibbonH 保持一致
    /// </summary>
    const   float   kRibbonH    =   128.0f;
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
        ImGui::SetNextWindowPos(ImVec2(316.0f,kRibbonH + 8.0f),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(320.0f,420.0f),ImGuiCond_FirstUseEver);
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
            if (ImGui::Button("重置",ImVec2(buttonW,buttonH)))
                target  =   0;
            ImGui::SameLine();
            if (ImGui::Button("清空",ImVec2(buttonW,buttonH)))
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
                ImGui::BeginChild("##undo_list",ImVec2(0.0f,0.0f),false,
                                  ImGuiWindowFlags_HorizontalScrollbar);
                for (int i = 0; i < count; ++i)
                {
                    /// <summary>
                    /// 游标之后的命令已被撤销,用暗色区分;
                    /// 游标之前(含游标)为已执行状态
                    /// </summary>
                    const   bool    undone  =   (i >= index);
                    char    label[512]  =   {};
                    snprintf(label,sizeof(label),"%3d  %s###undo_row_%d"
                             ,i + 1,stack.text(i).c_str(),i);
                    if (undone)
                        ImGui::PushStyleColor(ImGuiCol_Text,dim);
                    /// <summary>
                    /// 选中态落在最后一条已执行的命令上,与 QUndoView 的当前位置一致
                    /// </summary>
                    if (ImGui::Selectable(label,i == index - 1))
                        target  =   i + 1;
                    if (undone)
                        ImGui::PopStyleColor();
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
