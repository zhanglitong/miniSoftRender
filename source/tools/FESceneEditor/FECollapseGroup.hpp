/// <summary>
/// FECollapseGroup
/// 可折叠分组控件:标题栏 + 展开/折叠箭头,组内可以放置任意 ImGui 控件。
///   - 单击左侧箭头切换展开状态;
///   - 双击标题栏同样可以折叠/展开;
///   - 展开时组内控件自动缩进(与模型树的缩进一致)。
/// 另提供 valueRow 静态方法,用来绘制 "label + 数值编辑框 + 勾选框" 的行:
/// 勾选时数值可编辑并写回,未勾选时以只读框显示(只展示不修改)。
/// </summary>

#pragma     once

#include    "FEUiStyle.hpp"

#include    <imgui.h>

#include    <string>

namespace   FE
{
    class   FECollapseGroup
    {
    public:
        FECollapseGroup() = default;
        explicit    FECollapseGroup(const std::string& title)   :   _title(title)    {}
    public:
        const   std::string&    title() const           {   return  _title;     }
        void    setTitle(const std::string& title)      {   _title  =   title;  }
        bool    opened() const                          {   return  _open;      }
        void    setOpened(bool open)                    {   _open   =   open;   }
        /// <summary>
        /// 绘制标题栏。返回 true 表示已展开,调用方随后绘制的控件会缩进。
        /// 与 end() 成对调用(返回 false 时也必须调用 end())。
        /// </summary>
        /// <returns>true 展开 / false 折叠</returns>
        bool    begin()
        {
            const   float   headerH =   FE::ui::rowHeight();
            const   ImVec2  start   =   ImGui::GetCursorScreenPos();
            float           width   =   ImGui::GetContentRegionAvail().x;
            if (width < 80.0f)
                width   =   80.0f;

            ImGui::PushID(this);
            ImGui::SetCursorScreenPos(start);
            ImGui::InvisibleButton("##grp_header",ImVec2(width,headerH));

            const   bool    hovered =   ImGui::IsItemHovered();
            const   ImVec2  mouse   =   ImGui::GetIO().MousePos;
            /// <summary>
            /// 箭头热区:标题栏左侧一小段
            /// </summary>
            const   bool    onArrow =   mouse.x < start.x + 26.0f;

            /// <summary>
            /// 双击标题栏切换;箭头区的双击在第一次单击时已经切换过,这里不再重复,
            /// 否则箭头处双击会连续翻转两次而看不出效果
            /// </summary>
            bool    toggle  =   false;
            if (hovered && ImGui::IsMouseDoubleClicked(0))
                toggle  =   !onArrow;
            else if (hovered && ImGui::IsMouseClicked(0) && onArrow)
                toggle  =   true;
            if (toggle)
                _open   =   !_open;

            ImDrawList*     dl  =   ImGui::GetWindowDrawList();
            const   ImU32   bg  =   hovered ?   IM_COL32(255,255,255,20)
                                            :   IM_COL32(255,255,255,10);
            dl->AddRectFilled(start,ImVec2(start.x + width,start.y + headerH),bg,3.0f);
            /// <summary>
            /// 左侧强调条
            /// </summary>
            dl->AddRectFilled(start,ImVec2(start.x + 3.0f,start.y + headerH),FE::ui::kAccent,2.0f);

            /// <summary>
            /// 展开箭头:展开时朝下,折叠时朝右
            /// </summary>
            const   float   cx  =   start.x + 13.0f;
            const   float   cy  =   start.y + headerH * 0.5f;
            const   float   r   =   ImGui::GetFontSize() * 0.26f;
            if (_open)
                dl->AddTriangleFilled(ImVec2(cx - r,cy - r * 0.6f),
                                      ImVec2(cx + r,cy - r * 0.6f),
                                      ImVec2(cx,cy + r * 0.9f),FE::ui::kArrowColor);
            else
                dl->AddTriangleFilled(ImVec2(cx - r * 0.6f,cy - r),
                                      ImVec2(cx - r * 0.6f,cy + r),
                                      ImVec2(cx + r * 0.9f,cy),FE::ui::kArrowColor);

            /// <summary>
            /// 标题文字(垂直居中)
            /// </summary>
            dl->AddText(ImVec2(start.x + 28.0f,cy - ImGui::GetFontSize() * 0.5f),
                        FE::ui::kTextColor,_title.c_str());

            dl->AddLine(ImVec2(start.x,start.y + headerH - 0.5f),
                        ImVec2(start.x + width,start.y + headerH - 0.5f),
                        IM_COL32(255,255,255,22));

            if (_open)
            {
                ImGui::Indent(FE::ui::kIndent);
                _indented   =   true;
            }
            return  _open;
        }
        /// <summary>
        /// 结束分组:恢复缩进、弹出 ID,并在组之间留出间隔
        /// </summary>
        void    end()
        {
            if (_indented)
            {
                ImGui::Unindent(FE::ui::kIndent);
                _indented   =   false;
            }
            ImGui::PopID();
            ImGui::Dummy(ImVec2(0.0f,4.0f));
        }
        /// <summary>
        /// 一行属性:标签 + 数值编辑框 + 勾选框。
        /// 数值始终可编辑(返回 true 表示本次被修改);
        /// 勾选框表示"当前时间该属性是否有关键帧",点击时通过 keyToggled 通知调用方。
        /// </summary>
        /// <param name="label">行标签(同时作为 ImGui 标识,组内必须唯一)</param>
        /// <param name="value">数值(输入输出)</param>
        /// <param name="keyed">当前时间是否有关键帧(输入输出)</param>
        /// <param name="speed">拖动步进</param>
        /// <param name="fmt">数值显示格式</param>
        /// <param name="keyToggled">本次勾选框是否被点击(可为 nullptr)</param>
        /// <returns>true 表示数值被修改</returns>
        static  bool    valueRow(const char* label,float& value,bool& keyed,float speed,
                                 const char* fmt = "%.3f",bool* keyToggled = nullptr)
        {
            const   ImVec2  start   =   ImGui::GetCursorScreenPos();
            const   float   frameH  =   ImGui::GetFrameHeight();
            const   float   avail   =   ImGui::GetContentRegionAvail().x;
            const   float   labelW  =   22.0f;
            const   float   checkW  =   frameH;
            const   float   gap     =   6.0f;
            float           valueW  =   avail - labelW - checkW - gap;
            if (valueW < 48.0f)
                valueW  =   48.0f;

            ImDrawList*     dl      =   ImGui::GetWindowDrawList();
            dl->AddText(ImVec2(start.x + 2.0f,start.y + (frameH - ImGui::GetFontSize()) * 0.5f),
                        FE::ui::kTextColor,label);

            bool    changed =   false;
            ImGui::PushID(label);

            ImGui::SetCursorScreenPos(ImVec2(start.x + labelW,start.y));
            ImGui::PushItemWidth(valueW);
            changed =   ImGui::DragFloat("##value",&value,speed,0.0f,0.0f,fmt);
            ImGui::PopItemWidth();

            ImGui::SetCursorScreenPos(ImVec2(start.x + labelW + valueW + gap,start.y));
            if (ImGui::Checkbox("##keyed",&keyed) && keyToggled != nullptr)
                *keyToggled =   true;
            ImGui::PopID();

            /// <summary>
            /// 光标移到下一行
            /// </summary>
            ImGui::SetCursorScreenPos(ImVec2(start.x,start.y + frameH + 2.0f));
            ImGui::Dummy(ImVec2(0.0f,0.0f));
            return  changed;
        }
    protected:
        /// <summary>
        /// 标题
        /// </summary>
        std::string     _title      =   "分组";
        /// <summary>
        /// 是否展开
        /// </summary>
        bool            _open       =   true;
        /// <summary>
        /// 当前是否处于缩进状态(begin 展开后置位,end 中恢复)
        /// </summary>
        bool            _indented   =   false;
    };
}
