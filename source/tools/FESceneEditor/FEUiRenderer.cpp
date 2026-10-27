/// <summary>
/// FEUiRenderer 实现:布局计算 + ImGui 控件绘制。
/// </summary>

#include    "FEUiRenderer.hpp"

#include    <imgui.h>

#include    <algorithm>
#include    <cstdio>

namespace
{
    const   ImU32   kPlaceholderFill   =   IM_COL32(255,255,255,12);
    const   ImU32   kPlaceholderLine   =   IM_COL32(255,255,255,45);
    const   ImU32   kPlaceholderText   =   IM_COL32(150,156,168,255);
    const   ImU32   kMutedText         =   IM_COL32(140,146,158,255);
    const   float   kDefaultSpacing    =   2.0f;
}

namespace   FE
{
    void    FEUiRenderer::setHandler(const std::string& cls,FEUiHandler handler)
    {
        if (handler)
            _handlers[cls]  =   std::move(handler);
        else
            _handlers.erase(cls);
    }

    std::string FEUiRenderer::widgetLabel(FEUiWidget* widget) const
    {
        if (!widget->_text.empty())
            return  widget->_text;
        if (!widget->_name.empty())
            return  widget->_name;
        return  widget->_class;
    }

    std::string FEUiRenderer::widgetId(FEUiWidget* widget) const
    {
        if (!widget->_name.empty())
            return  widget->_name;
        char    buffer[32]  =   {};
        std::snprintf(buffer,sizeof(buffer),"%p",static_cast<void*>(widget));
        return  buffer;
    }

    float   FEUiRenderer::widgetHint(FEUiWidget* widget,bool horizontal) const
    {
        if (widget == nullptr)
            return  0.0f;
        const   std::string     label   =   widgetLabel(widget);
        const   ImVec2          textSize=   ImGui::CalcTextSize(label.c_str());
        if (!horizontal)
        {
            if (widget->_class == "QLabel")
                return  textSize.y + 4.0f;
            if (widget->_class == "QCheckBox")
                return  ImGui::GetTextLineHeight() + 4.0f;
            if (widget->_class == "QScrollBar")
                return  12.0f;
            if (widget->_class == "QStatusBar")
                return  22.0f;
            if (widget->_layout != nullptr)
                return  ImGui::GetFrameHeight() + 4.0f;
            return  ImGui::GetFrameHeight();
        }
        if (widget->_class == "QLabel")
            return  textSize.x + 8.0f;
        if (widget->_class == "QScrollBar")
            return  12.0f;
        return  textSize.x + 28.0f;
    }

    void    FEUiRenderer::draw(FEUiDocument& doc)
    {
        FEUiWidget*     root    =   doc.root();
        if (root == nullptr)
            return;
        /// <summary>
        /// 只渲染中央控件区,状态栏单独贴底(QDockWidget/菜单栏/工具栏首版不支持)
        /// </summary>
        FEUiWidget*     body    =   doc.find("centralwidget");
        if (body == nullptr)
            body    =   root;
        FEUiWidget*     status  =   doc.findClass("QStatusBar");
        const   ImVec2  pos     =   ImGui::GetCursorScreenPos();
        const   ImVec2  avail   =   ImGui::GetContentRegionAvail();
        const   float   statusH =   (status != nullptr) ? 22.0f : 0.0f;
        const   FEUiRect    rect    =   {   pos.x,pos.y,avail.x,avail.y - statusH  };
        if (rect._w > 0.0f && rect._h > 0.0f)
            drawWidget(body,rect);
        if (status != nullptr && statusH > 0.0f)
        {
            const   FEUiRect    statusRect  =   {   pos.x,pos.y + avail.y - statusH,avail.x,statusH  };
            drawWidget(status,statusRect);
        }
        /// <summary>
        /// 绝对定位绘制后恢复光标并占位,保证窗口内容尺寸正确
        /// </summary>
        ImGui::SetCursorScreenPos(pos);
        ImGui::Dummy(avail);
    }

    void    FEUiRenderer::drawWidget(FEUiWidget* widget,const FEUiRect& rect)
    {
        if (widget == nullptr || rect._w <= 0.0f || rect._h <= 0.0f)
            return;
        const   bool    faded   =   !widget->_enabled;
        if (faded)
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha,ImGui::GetStyle().Alpha * 0.5f);

        if (widget->_class == "QGroupBox")
        {
            drawGroupBox(widget,rect);
        }
        else if (widget->_layout != nullptr)
        {
            drawLayout(widget,rect);
        }
        else if (widget->_class == "QSplitter" && widget->_children.size() >= 2)
        {
            drawSplitter(widget,rect);
        }
        else if (!widget->_children.empty())
        {
            /// <summary>
            /// 没有布局的容器:子控件纵向等分
            /// </summary>
            const   float   h   =   rect._h / float(widget->_children.size());
            float           y   =   rect._y;
            for (auto* child : widget->_children)
            {
                const   FEUiRect    childRect   =   {   rect._x,y,rect._w,h  };
                drawWidget(child,childRect);
                y   +=  h;
            }
        }
        else
        {
            drawControl(widget,rect);
        }

        if (faded)
            ImGui::PopStyleVar();
    }

    void    FEUiRenderer::drawLayout(FEUiWidget* widget,const FEUiRect& rect)
    {
        drawLayoutNode(widget->_layout,rect);
    }

    void    FEUiRenderer::drawLayoutNode(FEUiLayout* layout,const FEUiRect& rect)
    {
        if (layout == nullptr || layout->_items.empty())
            return;
        if (layout->_class == "QGridLayout")
            layoutGrid(layout,rect);
        else
            layoutLinear(layout,rect,layout->_class == "QHBoxLayout");
    }

    void    FEUiRenderer::layoutLinear(FEUiLayout* layout,const FEUiRect& rect,bool horizontal)
    {
        const   size_t  count   =   layout->_items.size();
        const   float   spacing =   layout->_spacing;
        const   float   gaps    =   spacing * float(count - 1);

        std::vector<float>  sizes(count,0.0f);
        std::vector<float>  weights(count,0.0f);
        float               fixedTotal  =   0.0f;
        for (size_t i = 0; i < count; ++i)
        {
            const   FEUiItem&   item    =   layout->_items[i];
            const   int         weight  =   (item._widget != nullptr)
                                            ? (horizontal ? item._widget->_hStretch : item._widget->_vStretch)
                                            : 0;
            if (weight > 0)
            {
                weights[i]  =   float(weight);
            }
            else
            {
                sizes[i]    =   widgetHint(item._widget,horizontal);
                fixedTotal  +=  sizes[i];
            }
        }

        const   float   available   =   std::max(0.0f,(horizontal ? rect._w : rect._h) - gaps - fixedTotal);
        float           weightTotal =   0.0f;
        for (float weight : weights)
            weightTotal += weight;
        if (weightTotal > 0.0f)
        {
            for (size_t i = 0; i < count; ++i)
            {
                if (weights[i] > 0.0f)
                    sizes[i]    =   available * weights[i] / weightTotal;
            }
        }
        else
        {
            /// <summary>
            /// 全部固定时剩余空间均分
            /// </summary>
            const   float   extra   =   std::max(0.0f,(horizontal ? rect._w : rect._h) - gaps - fixedTotal) / float(count);
            for (size_t i = 0; i < count; ++i)
                sizes[i]    +=  extra;
        }

        float   cursor  =   horizontal ? rect._x : rect._y;
        for (size_t i = 0; i < count; ++i)
        {
            FEUiRect    cell    =   rect;
            if (horizontal)
            {
                cell._x =   cursor;
                cell._w =   sizes[i];
            }
            else
            {
                cell._y =   cursor;
                cell._h =   sizes[i];
            }
            const   FEUiItem&   item    =   layout->_items[i];
            if (item._widget != nullptr)
                drawWidget(item._widget,cell);
            else if (item._layout != nullptr)
                drawLayoutNode(item._layout,cell);
            cursor  +=  sizes[i] + spacing;
        }
    }

    void    FEUiRenderer::layoutGrid(FEUiLayout* layout,const FEUiRect& rect)
    {
        int     rows    =   1;
        int     cols    =   1;
        for (const auto& item : layout->_items)
        {
            rows    =   std::max(rows,item._row + item._rowSpan);
            cols    =   std::max(cols,item._col + item._colSpan);
        }
        std::vector<float>  colWeight(size_t(cols),0.0f);
        std::vector<float>  rowWeight(size_t(rows),0.0f);
        std::vector<float>  colHint(size_t(cols),0.0f);
        std::vector<float>  rowHint(size_t(rows),0.0f);

        for (const auto& item : layout->_items)
        {
            const   FEUiWidget*     widget  =   item._widget;
            const   int             hWeight =   (widget != nullptr) ? widget->_hStretch : 0;
            const   int             vWeight =   (widget != nullptr) ? widget->_vStretch : 0;
            if (hWeight > 0)
                colWeight[size_t(item._col)]    =   std::max(colWeight[size_t(item._col)],float(hWeight));
            if (vWeight > 0)
                rowWeight[size_t(item._row)]    =   std::max(rowWeight[size_t(item._row)],float(vWeight));
            if (hWeight == 0 && item._colSpan == 1)
                colHint[size_t(item._col)]      =   std::max(colHint[size_t(item._col)],widgetHint(item._widget,true));
            if (vWeight == 0 && item._rowSpan == 1)
                rowHint[size_t(item._row)]      =   std::max(rowHint[size_t(item._row)],widgetHint(item._widget,false));
        }

        const   float   spacing =   layout->_spacing;
        auto    buildSizes  =   [](std::vector<float>& weights,std::vector<float>& hints,
                                   float total,float gapTotal)
        {
            const   size_t  count   =   weights.size();
            std::vector<float>  sizes(count,0.0f);
            float   fixedTotal  =   0.0f;
            float   weightTotal =   0.0f;
            for (size_t i = 0; i < count; ++i)
            {
                if (weights[i] > 0.0f)
                    weightTotal +=  weights[i];
                else
                    fixedTotal  +=  hints[i];
            }
            const   float   available   =   std::max(0.0f,total - gapTotal - fixedTotal);
            if (weightTotal > 0.0f)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    sizes[i]    =   (weights[i] > 0.0f)
                                    ? available * weights[i] / weightTotal
                                    : hints[i];
                }
            }
            else
            {
                for (size_t i = 0; i < count; ++i)
                    sizes[i]    =   hints[i] + available / float(count);
            }
            return  sizes;
        };

        const   std::vector<float>  colSizes    =   buildSizes(colWeight,colHint,rect._w,spacing * float(cols - 1));
        const   std::vector<float>  rowSizes    =   buildSizes(rowWeight,rowHint,rect._h,spacing * float(rows - 1));

        std::vector<float>  colPos(size_t(cols),rect._x);
        std::vector<float>  rowPos(size_t(rows),rect._y);
        for (int c = 1; c < cols; ++c)
            colPos[size_t(c)]   =   colPos[size_t(c - 1)] + colSizes[size_t(c - 1)] + spacing;
        for (int r = 1; r < rows; ++r)
            rowPos[size_t(r)]   =   rowPos[size_t(r - 1)] + rowSizes[size_t(r - 1)] + spacing;

        for (const auto& item : layout->_items)
        {
            FEUiRect    cell;
            cell._x =   colPos[size_t(item._col)];
            cell._y =   rowPos[size_t(item._row)];
            cell._w =   0.0f;
            cell._h =   0.0f;
            for (int c = 0; c < item._colSpan && item._col + c < cols; ++c)
                cell._w += colSizes[size_t(item._col + c)] + ((c > 0) ? spacing : 0.0f);
            for (int r = 0; r < item._rowSpan && item._row + r < rows; ++r)
                cell._h += rowSizes[size_t(item._row + r)] + ((r > 0) ? spacing : 0.0f);

            if (item._widget != nullptr)
                drawWidget(item._widget,cell);
            else if (item._layout != nullptr)
                drawLayoutNode(item._layout,cell);
        }
    }

    void    FEUiRenderer::drawSplitter(FEUiWidget* widget,const FEUiRect& rect)
    {
        const   float   handle  =   6.0f;
        float           split   =   std::clamp(widget->_split,0.1f,0.9f);
        float           divider =   rect._x + (rect._w - handle) * split;

        const   FEUiRect    left    =   {   rect._x,rect._y,divider - rect._x,rect._h  };
        const   FEUiRect    right   =   {   divider + handle,rect._y,
                                            rect._x + rect._w - (divider + handle),rect._h   };
        drawWidget(widget->_children[0],left);
        drawWidget(widget->_children[1],right);

        const   std::string id  =   "###split_" + widgetId(widget);
        ImGui::SetCursorScreenPos(ImVec2(divider,rect._y));
        ImGui::InvisibleButton(id.c_str(),ImVec2(handle,rect._h));
        if (ImGui::IsItemActive() && rect._w > handle)
        {
            widget->_split  =   std::clamp(widget->_split + ImGui::GetIO().MouseDelta.x / (rect._w - handle),
                                           0.1f,0.9f);
        }
        ImDrawList*     dl  =   ImGui::GetWindowDrawList();
        dl->AddRectFilled(ImVec2(divider,rect._y),ImVec2(divider + handle,rect._y + rect._h),
                          IM_COL32(255,255,255,18));
        dl->AddLine(ImVec2(divider + handle * 0.5f,rect._y),
                    ImVec2(divider + handle * 0.5f,rect._y + rect._h),
                    IM_COL32(255,255,255,38));
    }

    void    FEUiRenderer::drawGroupBox(FEUiWidget* widget,const FEUiRect& rect)
    {
        ImDrawList*     dl      =   ImGui::GetWindowDrawList();
        const   ImVec2  pos     =   ImVec2(rect._x,rect._y);
        const   ImVec2  end     =   ImVec2(rect._x + rect._w,rect._y + rect._h);
        dl->AddRect(pos,end,IM_COL32(255,255,255,45),3.0f);
        if (!widget->_text.empty())
            dl->AddText(ImVec2(rect._x + 8.0f,rect._y + 4.0f),kMutedText,widget->_text.c_str());

        const   FEUiRect    inner   =   {   rect._x + 8.0f,rect._y + 22.0f,
                                            std::max(0.0f,rect._w - 16.0f),std::max(0.0f,rect._h - 30.0f)  };
        if (widget->_layout != nullptr)
            drawLayoutNode(widget->_layout,inner);
        else
        {
            for (auto* child : widget->_children)
                drawWidget(child,inner);
        }
    }

    void    FEUiRenderer::drawControl(FEUiWidget* widget,const FEUiRect& rect)
    {
        const   std::string     cls     =   widget->_class;
        const   std::string     label   =   widgetLabel(widget);
        const   std::string     id      =   widgetId(widget);
        const   std::string     itemId  =   "###" + id;

        if (cls == "QPushButton" || cls == "QToolButton" || cls == "QRadioButton")
        {
            ImGui::SetCursorScreenPos(ImVec2(rect._x,rect._y));
            ImGui::Button((label + itemId).c_str(),ImVec2(rect._w,rect._h));
        }
        else if (cls == "QCheckBox")
        {
            ImGui::SetCursorScreenPos(ImVec2(rect._x,rect._y + (rect._h - ImGui::GetTextLineHeight()) * 0.5f));
            ImGui::Checkbox((label + itemId).c_str(),&widget->_checked);
        }
        else if (cls == "QComboBox")
        {
            const   std::string     preview =   widget->_options.empty()
                                                ? label
                                                : widget->currentOption();
            ImGui::SetCursorScreenPos(ImVec2(rect._x,rect._y));
            ImGui::PushItemWidth(rect._w);
            if (ImGui::BeginCombo(itemId.c_str(),preview.c_str(),ImGuiComboFlags_HeightSmall))
            {
                for (size_t i = 0; i < widget->_options.size(); ++i)
                {
                    const   bool    selected    =   (int(i) == widget->_currentIndex);
                    if (ImGui::Selectable(widget->_options[i].c_str(),selected))
                        widget->_currentIndex   =   int(i);
                    if (selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
        }
        else if (cls == "QLineEdit")
        {
            ImGui::SetCursorScreenPos(ImVec2(rect._x,rect._y));
            ImGui::PushItemWidth(rect._w);
            ImGui::InputText(itemId.c_str(),widget->_buffer,sizeof(widget->_buffer));
            ImGui::PopItemWidth();
        }
        else if (cls == "QLabel")
        {
            if (label.empty())
                return;
            const   ImVec2  textSize    =   ImGui::CalcTextSize(label.c_str());
            ImGui::SetCursorScreenPos(ImVec2(rect._x,
                                             rect._y + std::max(0.0f,(rect._h - textSize.y) * 0.5f)));
            ImGui::TextUnformatted(label.c_str());
        }
        else if (cls == "QScrollBar")
        {
            ImDrawList*     dl  =   ImGui::GetWindowDrawList();
            dl->AddRectFilled(ImVec2(rect._x,rect._y),ImVec2(rect._x + rect._w,rect._y + rect._h),
                              IM_COL32(255,255,255,16),rect._w * 0.5f);
            dl->AddRectFilled(ImVec2(rect._x,rect._y + rect._h * 0.3f),
                              ImVec2(rect._x + rect._w,rect._y + rect._h * 0.7f),
                              IM_COL32(255,255,255,60),rect._w * 0.5f);
        }
        else if (cls == "QStatusBar")
        {
            ImDrawList*     dl  =   ImGui::GetWindowDrawList();
            dl->AddRectFilled(ImVec2(rect._x,rect._y),ImVec2(rect._x + rect._w,rect._y + rect._h),
                              IM_COL32(0,0,0,60));
            const   std::string     text    =   widget->_text.empty() ? std::string("就绪") : widget->_text;
            dl->AddText(ImVec2(rect._x + 8.0f,rect._y + (rect._h - ImGui::GetTextLineHeight()) * 0.5f),
                        kMutedText,text.c_str());
        }
        else
        {
            drawPlaceholder(widget,rect);
        }
    }

    void    FEUiRenderer::drawPlaceholder(FEUiWidget* widget,const FEUiRect& rect)
    {
        ImDrawList*     dl      =   ImGui::GetWindowDrawList();
        const   ImVec2  pos     =   ImVec2(rect._x,rect._y);
        const   ImVec2  end     =   ImVec2(rect._x + rect._w,rect._y + rect._h);
        dl->AddRectFilled(pos,end,kPlaceholderFill,4.0f);
        dl->AddRect(pos,end,kPlaceholderLine,4.0f);

        const   std::string text    =   widget->_class
                                        + (widget->_name.empty() ? std::string()
                                                                 : (" (" + widget->_name + ")"));
        const   ImVec2  textSize    =   ImGui::CalcTextSize(text.c_str());
        if (textSize.x < rect._w - 8.0f && textSize.y < rect._h)
        {
            dl->AddText(ImVec2(rect._x + (rect._w - textSize.x) * 0.5f,
                               rect._y + (rect._h - textSize.y) * 0.5f),kPlaceholderText,text.c_str());
        }

        const   auto    it  =   _handlers.find(widget->_class);
        if (it != _handlers.end() && it->second)
            it->second(*widget,rect);
    }
}
