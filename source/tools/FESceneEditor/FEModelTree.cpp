/// <summary>
/// FEModelTree 实现:ImGui 绘制场景节点树并处理选择/展开/显隐/拖拽/重命名。
/// </summary>

#include    "FEModelTree.hpp"
#include    "FEEditorUndoCmds.hpp"

#include    "../../../inc/FEUndoStack.hpp"
#include    "../../../inc/graphic/FEScene.h"
#include    "../../../inc/animation/FEAnimationHelper.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../../inc/constraint/FEConstraintTrans.hpp"
#include    "../../../inc/constraint/FEConstraintFollow.hpp"
#include    "../../../inc/constraint/FEConstraintLookAt.hpp"
#include    "../../../inc/constraint/FEConstraintPath.hpp"
#include    "../../../inc/constraint/FEConstraintDrive.hpp"

#include    <algorithm>
#include    <cstdio>
#include    <functional>

namespace
{
    /// <summary>
    /// Font Awesome 字形:节点/对象
    /// </summary>
    const   char*   kGlyphCube     =   u8"\uf1b2";
    /// <summary>
    /// Font Awesome 字形:显示
    /// </summary>
    const   char*   kGlyphEye      =   u8"\uf06e";
    /// <summary>
    /// Font Awesome 字形:隐藏
    /// </summary>
    const   char*   kGlyphEyeOff   =   u8"\uf070";

    const   ImU32   kRowSelected   =   IM_COL32(74,144,226,72);
    const   ImU32   kRowHover      =   IM_COL32(255,255,255,14);
    /// <summary>
    /// 隔行背景色(奇数行叠加的淡白色,偶数行保持窗口底色)
    /// </summary>
    const   ImU32   kRowAlt        =   IM_COL32(255,255,255,10);
    /// <summary>
    /// 选中行的矩形线框颜色
    /// </summary>
    const   ImU32   kRowSelBorder  =   IM_COL32(74,144,226,220);
    /// <summary>
    /// 拖拽嵌入目标时的整行高亮底色
    /// </summary>
    const   ImU32   kDropFill      =   IM_COL32(74,144,226,56);
    const   ImU32   kTextColor     =   IM_COL32(214,219,229,255);
    /// <summary>
    /// 选中行的高亮前景色
    /// </summary>
    const   ImU32   kTextSelected  =   IM_COL32(235,245,255,255);
    const   ImU32   kTextHidden    =   IM_COL32(140,146,158,255);
    const   ImU32   kIconHidden    =   IM_COL32(110,116,128,255);
    const   ImU32   kAccent        =   IM_COL32(74,144,226,255);
    const   ImU32   kArrowColor    =   IM_COL32(150,156,168,255);

    /// <summary>
    /// 行距相对字体大小的倍数:行距 = 字体大小 × 2
    /// </summary>
    const   float   kRowLineScale  =   2.4f;
    /// <summary>
    /// 行与行之间的留白(像素)
    /// </summary>
    const   float   kRowGap        =   4.0f;

    /// <summary>
    /// 从 const 引用取节点指针(FEObject::cast 在 const 上返回 const 指针)
    /// </summary>
    /// <param name="object">对象引用</param>
    /// <returns>节点;不是节点时返回空 Node</returns>
    FE::Node    nodeOf(const FE::Object& object)
    {
        auto*   raw =   const_cast<FE::FEObject*>(object.get());
        return  (raw != nullptr) ? raw->cast<FE::FENode>() : FE::Node();
    }
    /// <summary>
    /// 生成同级唯一名称:未被占用时直接返回,否则追加序号
    /// </summary>
    FE::String uniqueNameUnder(const FE::Nodes& siblings,const FE::String& base)
    {
        auto    used    =   [&siblings](const FE::String& name)
        {
            for (const FE::Node& var : siblings)
            {
                if (var != nullptr && name == var->name())
                    return  true;
            }
            return  false;
        };
        if (!used(base))
            return  base;
        int index   =   1;
        while (true)
        {
            FE::String  name    =   base + " " + std::to_string(index);
            if (!used(name))
                return  name;
            ++index;
        }
    }
    /// <summary>
    /// 动画是否已登记到动画系统(动画树)
    /// </summary>
    bool    animationRegistered(FE::FEAnimationSys* sys,const FE::FEAnimation* anim)
    {
        if (sys == nullptr || anim == nullptr)
            return  false;
        for (auto& var : sys->playbacks())
        {
            for (const FE::Animation& item : var.second->objects())
            {
                if (item.get() == anim)
                    return  true;
            }
        }
        return  false;
    }
}

namespace   FE
{
    FEModelTree::~FEModelTree()
    {
        unbind();
    }

    void    FEModelTree::unbind()
    {
        if (_nodeTree != nullptr)
        {
            _nodeTree->eventsAddNode()      -=  this;
            _nodeTree->eventsRemoveNode()   -=  this;
            _nodeTree->eventsChangedNode()  -=  this;
            _nodeTree->eventsClear()        -=  this;
            _nodeTree   =   nullptr;
        }
        /// <summary>
        /// 同时归还所有节点引用(行列表、选中、右键菜单、拖拽、重命名目标)。
        /// FEEditorUi::detach 在场景销毁(device 释放)前调用本方法;
        /// 若这些引用活到 UI 析构,节点及其材质会晚于 device 销毁,
        /// 材质持有的 pipeline 析构时访问已释放的 device
        /// (表现为 FEVKDevice::logicalDevice 里 this 为空指针崩溃)
        /// </summary>
        _rows.clear();
        /// <summary>
        /// 解绑前清除选中标记,避免同一节点树重新绑定时残留高亮
        /// </summary>
        for (const Object& object : _selection)
        {
            if (!object)
                continue;
            FEObject*   raw =   const_cast<FEObject*>(object.get());
            raw->flags().removeFlag(FLAG_SELECTED);
        }
        _selection.clear();
        _selected       =   Object();
        _anchor         =   Object();
        _context        =   Object();
        _dragSource     =   Object();
        _dropTarget     =   Object();
        _renameTarget   =   Object();
        _dragging       =   false;
        _dropMode       =   0;
        /// <summary>
        /// 解绑时退出隔离,避免重新绑定另一棵树后仍只显示上一棵树残留的隔离节点
        /// </summary>
        _isolate        =   false;
        _isolated.clear();
        _rowsDirty      =   true;
    }

    void    FEModelTree::bindNodeTree(FENodeTree* nodeTree)
    {
        if (_nodeTree == nodeTree)
            return;
        unbind();
        _nodeTree   =   nodeTree;
        if (_nodeTree != nullptr)
        {
            /// 节点树增删/变更/清空时只置脏标记,行列表在下一帧统一重建
            _nodeTree->eventsAddNode()      +=  {this,&FEModelTree::onNodeAdded};
            _nodeTree->eventsRemoveNode()   +=  {this,&FEModelTree::onNodeRemoved};
            _nodeTree->eventsChangedNode()  +=  {this,&FEModelTree::onNodeChanged};
            _nodeTree->eventsClear()        +=  {this,&FEModelTree::onTreeCleared};
        }
        _rowsDirty  =   true;
    }

    void    FEModelTree::rebuildRows()
    {
        _rows.clear();
        if (_nodeTree != nullptr)
        {
            if (_isolate)
            {
                /// <summary>
                /// 隔离状态:模型树只显示被隔离节点及其子树;已从节点树删除的直接剔除,
                /// 全部失效时自动退出隔离,恢复整棵树
                /// </summary>
                for (auto itr = _isolated.begin(); itr != _isolated.end(); )
                {
                    if (itr->get() == nullptr || !isInTree(itr->get()))
                        itr =   _isolated.erase(itr);
                    else
                        ++itr;
                }
                if (_isolated.empty())
                    _isolate    =   false;
                for (const Node& node : _isolated)
                    collectRows(Object(const_cast<FENode*>(node.get())),0);
            }
            else
            {
                for (const auto& node : _nodeTree->topLevelNodes())
                    collectRows(Object(const_cast<FENode*>(node.get())),0);
            }
        }
        /// <summary>
        /// 拖拽改层级时节点树会先移除再添加,不能按移除事件直接删选中;
        /// 重建行列表时统一剔除已真正脱离节点树的选中节点。
        /// 网格/材质/组件等非节点对象不在节点树里,不能用 isInTree 判断,
        /// 否则会被误删出选中列表而 FLAG_SELECTED 残留,导致后续点击出现多行同时高亮
        /// </summary>
        for (auto itr = _selection.begin(); itr != _selection.end(); )
        {
            if (itr->get() == nullptr)
            {
                itr =   _selection.erase(itr);
                continue;
            }
            const   Node    node    =   nodeOf(*itr);
            if (!node || isInTree(node.get()))
            {
                ++itr;
                continue;
            }
            itr->get()->flags().removeFlag(FLAG_SELECTED);
            itr =   _selection.erase(itr);
        }
        if (_selected)
        {
            const   Node    sel =   nodeOf(_selected);
            if (sel && !isInTree(sel.get()))
                _selected   =   _selection.empty() ? Object() : _selection.back();
        }
        if (_anchor)
        {
            const   Node    anc =   nodeOf(_anchor);
            if (anc && !isInTree(anc.get()))
                _anchor     =   Object();
        }
        _rowsDirty  =   false;
    }

    void    FEModelTree::onNodeAdded(const FENode*)
    {
        _rowsDirty  =   true;
    }

    void    FEModelTree::onNodeRemoved(const FENode* node)
    {
        _rowsDirty  =   true;
        const   FEObject*   removed =   node;
        /// <summary>
        /// 选中项只在重建行列表时按"是否仍在树上"剔除:改层级会触发移除+添加事件,
        /// 此处直接删选中会让移动后的节点残留高亮
        /// </summary>
        if (_context && _context.get() == removed)
            _context    =   Object();
        if (_dragSource && _dragSource.get() == removed)
        {
            _dragSource =   Object();
            _dragging   =   false;
        }
        if (_dropTarget && _dropTarget.get() == removed)
        {
            _dropTarget =   Object();
            _dropMode   =   0;
        }
    }

    void    FEModelTree::onNodeChanged(const FENode*)
    {
        _rowsDirty  =   true;
    }

    void    FEModelTree::onTreeCleared()
    {
        _rowsDirty  =   true;
        /// <summary>
        /// 节点树清空后非节点对象(网格/材质)无法通过 isInTree 剔除,这里直接清空选中
        /// </summary>
        clearSelection();
        _selected   =   Object();
        _anchor     =   Object();
        _context    =   Object();
        _dragSource =   Object();
        _dropTarget =   Object();
        _dragging   =   false;
        _dropMode   =   0;
        /// <summary>
        /// 节点树清空后隔离节点全部失效,直接退出隔离
        /// </summary>
        _isolate    =   false;
        _isolated.clear();
    }

    void    FEModelTree::draw(FENodeTree* nodeTree,ImFont* iconFont)
    {
        bindNodeTree(nodeTree);
        _iconFont   =   iconFont;

        if (_rowsDirty)
            rebuildRows();

        drawToolbar();

        /// <summary>
        /// 行距 = 字体大小 × 2,扣除留白后即为行高
        /// </summary>
        const   float   rowStep     =   ImGui::GetFontSize() * kRowLineScale;
        const   float   rowHeight   =   std::max(1.0f,rowStep - kRowGap);
        std::vector<float>  rowTops;
        rowTops.reserve(_rows.size());

        ImGui::BeginChild("##model_tree_rows",ImVec2(0,0),false);
        if (_rows.empty())
        {
            ImGui::TextDisabled("场景为空");
        }
        const   float   width   =   std::max(1.0f,ImGui::GetContentRegionAvail().x);
        for (size_t i = 0; i < _rows.size(); ++i)
        {
            rowTops.push_back(ImGui::GetCursorScreenPos().y);
            drawRow(_rows[i],int(i),width,rowHeight);
        }

        /// <summary>
        /// 点击行下方空白处清空所有选中项:行按钮覆盖整行宽度,空白只可能出现在最后一行之下
        /// </summary>
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0))
        {
            const   ImVec2  mouse   =   ImGui::GetIO().MousePos;
            const   bool    blank   =   rowTops.empty() || mouse.y >= rowTops.back() + rowHeight;
            if (blank)
            {
                clearSelection();
                _selected   =   Object();
                _anchor     =   Object();
            }
        }

        /// <summary>
        /// 拖拽:按行矩形反查投放目标(鼠标按住时行按钮处于 active,不能用 hovered 判定)。
        /// 鼠标落在行中间 60% 区域时嵌入为该行子节点,落在上下缘 20% 时插入到该行前/后
        /// </summary>
        if (_dragging)
        {
            const   ImVec2  mouse   =   ImGui::GetIO().MousePos;
            const   Nodes   sources =   dragNodes();
            int             hit     =   -1;
            for (size_t i = 0; i < rowTops.size(); ++i)
            {
                if (mouse.y >= rowTops[i] && mouse.y < rowTops[i] + rowHeight)
                {
                    hit =   int(i);
                    break;
                }
            }

            _dropTarget =   (hit >= 0) ? _rows[size_t(hit)]._object : Object();
            _dropMode   =   0;
            if (hit >= 0)
            {
                const   float   ratio   =   (mouse.y - rowTops[size_t(hit)]) / rowHeight;
                const   int     mode    =   (ratio < 0.2f) ? 2 : ((ratio > 0.8f) ? 3 : 1);
                FENode*         target  =   _dropTarget ? _dropTarget->cast<FENode>() : nullptr;
                /// <summary>
                /// 多选拖拽时只要有一个源节点可投放就显示反馈,逐个源在 moveNode 内再校验
                /// </summary>
                for (const Node& source : sources)
                {
                    if (canDrop(source.get(),target,mode))
                    {
                        _dropMode   =   mode;
                        break;
                    }
                }
            }
            else if (ImGui::IsWindowHovered())
            {
                /// <summary>
                /// 没落在任何行上(最后一行之下的空白区):挂到根节点,成为新的顶层节点
                /// </summary>
                const   bool    blank   =   rowTops.empty() || mouse.y >= rowTops.back() + rowHeight;
                if (blank)
                {
                    for (const Node& source : sources)
                    {
                        if (canDrop(source.get(),nullptr,4))
                        {
                            _dropMode   =   4;
                            break;
                        }
                    }
                }
            }

            ImDrawList*     dl      =   ImGui::GetWindowDrawList();
            const   ImVec2  winPos  =   ImGui::GetWindowPos();
            const   float   x0      =   winPos.x + 4.0f;
            const   float   x1      =   winPos.x + ImGui::GetWindowWidth() - 4.0f;
            if (hit >= 0 && _dropMode == 1)
            {
                /// <summary>
                /// 行中间 60%:整行高亮,表示放入该节点内部
                /// </summary>
                dl->AddRectFilled(ImVec2(x0,rowTops[size_t(hit)] + 1.0f),
                                  ImVec2(x1,rowTops[size_t(hit)] + rowHeight - 1.0f),
                                  kDropFill,2.0f);
                dl->AddRect(ImVec2(x0,rowTops[size_t(hit)] + 1.0f),
                            ImVec2(x1,rowTops[size_t(hit)] + rowHeight - 1.0f),
                            kAccent,2.0f,0,2.0f);
            }
            else if (hit >= 0 && (_dropMode == 2 || _dropMode == 3))
            {
                /// <summary>
                /// 上下缘 20%:在两行之间画高亮线,表示插入到该行前/后
                /// </summary>
                const   float   y   =   (_dropMode == 2)
                                        ? rowTops[size_t(hit)]
                                        : rowTops[size_t(hit)] + rowHeight;
                dl->AddLine(ImVec2(x0,y),ImVec2(x1,y),kAccent,2.0f);
            }
            else if (_dropMode == 4)
            {
                /// <summary>
                /// 空白处投放:在行列表末尾画高亮线,表示追加到根节点
                /// </summary>
                const   float   y   =   rowTops.empty()
                                        ? ImGui::GetCursorScreenPos().y
                                        : rowTops.back() + rowHeight;
                dl->AddLine(ImVec2(x0,y),ImVec2(x1,y),kAccent,2.0f);
            }

            /// <summary>
            /// 拖拽预览:跟随鼠标显示被拖拽节点名(多选时附带数量),
            /// 用前景层绘制,保证鼠标移出子窗口后依旧可见
            /// </summary>
            if (_dragSource)
            {
                String          label   =   objectName(_dragSource);
                if (sources.size() > 1)
                    label   +=  " (+" + std::to_string(sources.size() - 1) + ")";
                const   float   pad     =   6.0f;
                const   ImVec2  textSize=   ImGui::CalcTextSize(label.c_str());
                const   ImVec2  boxMin  =   ImVec2(mouse.x + 16.0f,mouse.y + 16.0f);
                const   ImVec2  boxMax  =   ImVec2(boxMin.x + textSize.x + pad * 2.0f,
                                                  boxMin.y + textSize.y + pad * 2.0f);
                ImDrawList*     fg      =   ImGui::GetOverlayDrawList();
                fg->AddRectFilled(boxMin,boxMax,IM_COL32(30,34,42,225),4.0f);
                fg->AddRect(boxMin,boxMax,kAccent,4.0f,0,1.5f);
                fg->AddText(ImVec2(boxMin.x + pad,boxMin.y + pad),kTextSelected,label.c_str());
            }

            if (ImGui::IsMouseReleased(0))
            {
                if (_dropMode == 4)
                    moveNodes(sources,nullptr,4);
                else if (_dropMode != 0 && _dropTarget)
                    moveNodes(sources,_dropTarget->cast<FENode>(),_dropMode);
                _dragging   =   false;
                _dragSource =   Object();
                _dropTarget =   Object();
                _dropMode   =   0;
            }
        }
        ImGui::EndChild();

        handleContextMenu();
        drawRenamePopup();
    }

    void    FEModelTree::drawToolbar()
    {
        /// <summary>
        /// 隔离按钮(位于"展开全部"之前):隔离状态下按钮显示绿色,再次点击取消隔离。
        /// Push/Pop 必须使用点击前的状态,否则首次点击时按钮翻转 _isolate 后
        /// 会弹出从未 Push 过的颜色栈(ImGuiColorMod::back 断言崩溃)
        /// </summary>
        const   bool    isolating   =   _isolate;
        if (isolating)
        {
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.13f,0.52f,0.24f,1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.17f,0.64f,0.30f,1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.10f,0.42f,0.19f,1.0f));
        }
        if (ImGui::Button("隔离"))
            toggleIsolation();
        if (isolating)
            ImGui::PopStyleColor(3);
        ImGui::SameLine();
        if (ImGui::Button("展开全部"))
        {
            /// 只处理当前显示在树上的根(隔离时为被隔离节点)
            for (const RowData& row : _rows)
            {
                if (row._depth == 0)
                    setExpanded(row._object,true,true);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("折叠全部"))
        {
            for (const RowData& row : _rows)
            {
                if (row._depth == 0)
                    setExpanded(row._object,false,true);
            }
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%d 项",int(_rows.size()));
        ImGui::Separator();
    }

    void    FEModelTree::toggleIsolation()
    {
        if (_isolate)
        {
            _isolate    =   false;
            _isolated.clear();
            _rowsDirty  =   true;
            return;
        }
        /// <summary>
        /// 隔离当前选中的节点(可多个);选中项里被其他选中节点覆盖的子孙不再单独隔离
        /// </summary>
        Nodes   nodes;
        for (const Object& object : _selection)
        {
            const   Node    node    =   nodeOf(object);
            if (!node || !isInTree(node.get()))
                continue;
            bool    redundant   =   false;
            for (const Node& other : nodes)
            {
                if (other.get() != node.get() && isAncestor(other.get(),node.get()))
                {
                    redundant   =   true;
                    break;
                }
            }
            if (!redundant)
                nodes.push_back(node);
        }
        if (nodes.empty())
            return;
        _isolated   =   nodes;
        _isolate    =   true;
        /// <summary>
        /// 保持节点原有状态:不改动展开标记,隔离前后的展开/折叠状态一致
        /// </summary>
        _rowsDirty  =   true;
    }

    void    FEModelTree::collectRows(const Object& object,int depth)
    {
        if (!object)
            return;
        RowData row;
        row._object     =   object;
        row._depth      =   depth;
        row._hasChild   =   object->objectCount() > 0;
        row._expanded   =   row._hasChild && object->flags().hasFlag(FLAG_EXPAND);
        _rows.push_back(row);
        if (!row._expanded)
            return;
        object->traverseObject([this,depth](const FEObject& child,const FEObject&,
                                            const FEObject::FETrvsCtx&,uint) -> bool
        {
            collectRows(Object(const_cast<FEObject*>(&child)),depth + 1);
            return  true;
        });
    }

    void    FEModelTree::drawRow(const RowData& row,int index,float width,float rowHeight)
    {
        ImGui::PushID(index);

        const   ImVec2  pos     =   ImGui::GetCursorScreenPos();
        const   ImVec2  end     =   ImVec2(pos.x + width,pos.y + rowHeight);
        /// <summary>
        /// 整行按钮最后提交,否则会抢走子按钮的悬停/点击(ImGui 同一帧只认一个 HoveredId)
        /// </summary>
        const   bool    rowHover=   ImGui::IsMouseHoveringRect(pos,end);

        ImDrawList*     dl      =   ImGui::GetWindowDrawList();
        const   bool    selected=   row._object->flags().hasFlag(FLAG_SELECTED);
        /// <summary>
        /// 隔行背景:奇数行绘制淡白底色,偶数行保留窗口底色
        /// </summary>
        if ((index & 1) != 0)
            dl->AddRectFilled(pos,end,kRowAlt);
        /// <summary>
        /// 选中行整行高亮:矩形填充 + 矩形线框,同时文字使用高亮前景色
        /// </summary>
        if (selected)
        {
            dl->AddRectFilled(pos,end,kRowSelected);
            dl->AddRect(ImVec2(pos.x + 0.5f,pos.y + 0.5f),
                        ImVec2(end.x - 0.5f,end.y - 0.5f),
                        kRowSelBorder,0.0f,0,1.0f);
        }
        else if (rowHover)
            dl->AddRectFilled(pos,end,kRowHover);

        /// <summary>
        /// 图标尺寸取行高的 60%,展开图标与类型图标之间留出图标宽度 20% 的空隙
        /// </summary>
        const   float   iconSize    =   rowHeight * 0.6f;
        const   float   iconGap     =   iconSize * 0.2f;
        const   float   indent  =   14.0f;
        const   float   arrowX  =   pos.x + 2.0f + indent * float(row._depth);
        const   float   iconX   =   arrowX + iconSize + iconGap;
        const   float   textX   =   iconX + iconSize + iconGap;
        const   float   textY   =   pos.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f;
        const   float   iconY   =   pos.y + (rowHeight - iconSize) * 0.5f;

        /// <summary>
        /// 展开箭头(必须落在子窗口裁剪区内,否则点不到)
        /// </summary>
        if (row._hasChild)
        {
            ImGui::SetCursorScreenPos(ImVec2(arrowX,pos.y));
            ImGui::InvisibleButton("##expand",ImVec2(iconSize,rowHeight));
            if (ImGui::IsItemClicked(0))
                setExpanded(row._object,!row._expanded,false);

            const   float   half    =   iconSize * 0.3f;
            const   ImVec2  c   =   ImVec2(arrowX + iconSize * 0.5f,pos.y + rowHeight * 0.5f);
            if (row._expanded)
                dl->AddTriangleFilled(ImVec2(c.x - half,c.y - half * 0.75f),
                                      ImVec2(c.x + half,c.y - half * 0.75f),
                                      ImVec2(c.x,c.y + half * 0.75f),kArrowColor);
            else
                dl->AddTriangleFilled(ImVec2(c.x - half * 0.75f,c.y - half),
                                      ImVec2(c.x - half * 0.75f,c.y + half),
                                      ImVec2(c.x + half * 0.75f,c.y),kArrowColor);
        }

        /// <summary>
        /// 类型图标
        /// </summary>
        if (_iconFont != nullptr)
            dl->AddText(_iconFont,iconSize,ImVec2(iconX,iconY),kArrowColor,kGlyphCube);
        else
            dl->AddRectFilled(ImVec2(iconX,iconY),ImVec2(iconX + iconSize,iconY + iconSize),kArrowColor,2.0f);

        /// <summary>
        /// 显隐开关
        /// </summary>
        const   float   eyeX    =   end.x - iconSize - 4.0f;
        ImGui::SetCursorScreenPos(ImVec2(eyeX,pos.y));
        ImGui::InvisibleButton("##visible",ImVec2(iconSize,rowHeight));
        if (ImGui::IsItemClicked(0))
            setVisible(row._object,!row._object->flags().hasFlag(FLAG_VISIBLE),false);

        /// <summary>
        /// 整行:选中/右键菜单/拖拽
        /// </summary>
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("##row",ImVec2(width,rowHeight));
        if (ImGui::IsItemClicked(0))
        {
            const   ImGuiIO&    io      =   ImGui::GetIO();
            /// <summary>
            /// Shift 范围选择优先于 Ctrl 单项切换,与 Windows 资源管理器一致
            /// </summary>
            if (io.KeyShift)
                selectRangeTo(row._object);
            else if (io.KeyCtrl)
                toggleSelect(row._object);
            else
                selectSingle(row._object);
        }
        if (ImGui::IsItemClicked(1))
        {
            _context    =   row._object;
            /// <summary>
            /// 弹出菜单在父窗口作用域打开(此处位于子窗口,ID 作用域不同)
            /// </summary>
            _contextOpen    =   true;
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0,4.0f) && nodeOf(row._object))
        {
            _dragging   =   true;
            _dragSource =   row._object;
        }

        /// <summary>
        /// 名称(按显隐上色)
        /// </summary>
        const   bool            visible =   row._object->flags().hasFlag(FLAG_VISIBLE);
        const   String          name    =   objectName(row._object);
        ImGui::PushClipRect(ImVec2(textX,pos.y),ImVec2(eyeX - 4.0f,pos.y + rowHeight),true);
        dl->AddText(ImVec2(textX,textY),
                    selected ? kTextSelected : (visible ? kTextColor : kTextHidden),
                    name.c_str());
        ImGui::PopClipRect();

        if (_iconFont != nullptr)
            dl->AddText(_iconFont,iconSize,ImVec2(eyeX,iconY),
                        visible ? kArrowColor : kIconHidden,
                        visible ? kGlyphEye : kGlyphEyeOff);

        ImGui::SetCursorScreenPos(ImVec2(pos.x,pos.y + rowHeight + kRowGap));
        ImGui::PopID();
    }

    void    FEModelTree::handleContextMenu()
    {
        if (_contextOpen)
        {
            ImGui::OpenPopup("##tree_ctx");
            _contextOpen    =   false;
        }
        if (!ImGui::BeginPopup("##tree_ctx"))
            return;

        /// <summary>
        /// 菜单项高度与模型树行高保持一致。本版本 ImGui 的 MenuItem/BeginMenu 行高
        /// 等于文本行高(不含 FramePadding),行高由 ItemSpacing.y 撑开;
        /// Selectable 的高亮框上下各外扩 ItemSpacing.y/2,故把 ItemSpacing.y
        /// 设为 (行高 - 字号) 即可让菜单项高度与模型树 item 完全一致
        /// </summary>
        const   ImGuiStyle& style       =   ImGui::GetStyle();
        const   float   rowHeight       =   std::max(1.0f,ImGui::GetFontSize() * kRowLineScale - kRowGap);
        const   float   spacingY        =   std::max(0.0f,rowHeight - ImGui::GetFontSize());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing
                            ,ImVec2(style.ItemSpacing.x,spacingY));

        const   Object  object  =   _context;
        const   Node    node    =   nodeOf(object);
        if (!object || !node || _scene == nullptr)
        {
            ImGui::PopStyleVar();
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }

        /// <summary>
        /// 节点项:重命名 / 添加子节点 / 移除节点(均走撤销栈)
        /// </summary>
        if (ImGui::MenuItem("重命名..."))
        {
            _renameTarget   =   object;
            std::snprintf(_renameBuf,sizeof(_renameBuf),"%s",objectName(object).c_str());
            _renamePopup    =   true;
            _renameFocus    =   true;
        }
        if (ImGui::MenuItem("添加子节点"))
            addChildNode(node);
        if (ImGui::MenuItem("移除节点"))
            removeNode(node);

        ImGui::Separator();

        /// <summary>
        /// 动画子菜单:创建动画 / 添加到动画树 / 删除动画
        /// </summary>
        FEAnimationSys*     animSys     =   _scene->animationSystem();
        const   auto        nodeAnims   =   node->objects<FEAnimation>();
        bool                allRegistered   =   !nodeAnims.empty();
        for (const FEAnimation* anim : nodeAnims)
        {
            if (!animationRegistered(animSys,anim))
            {
                allRegistered   =   false;
                break;
            }
        }
        if (ImGui::BeginMenu("动画"))
        {
            if (ImGui::MenuItem("创建动画"))
                createAnimation(node);
            if (ImGui::MenuItem("添加到动画树",nullptr,false,
                                animSys != nullptr && !nodeAnims.empty() && !allRegistered))
                addAnimationsToTree(node);
            if (ImGui::MenuItem("删除动画",nullptr,false,!nodeAnims.empty()))
                removeAnimations(node);
            ImGui::EndMenu();
        }

        /// <summary>
        /// 约束子菜单:勾选状态 = 节点是否已挂该类型约束,点击切换添加/移除
        /// </summary>
        if (ImGui::BeginMenu("约束"))
        {
            static  const   char*   kConstraintNames[]  =
            {
                "平移约束","跟随约束","朝向约束","路径约束","驱动约束"
            };
            for (int i = 0; i < CK_COUNT; ++i)
            {
                const   bool    checked =   hasConstraint(node,ConstraintKind(i));
                if (ImGui::MenuItem(kConstraintNames[i],nullptr,checked))
                    toggleConstraint(node,ConstraintKind(i));
            }
            ImGui::EndMenu();
        }

        ImGui::PopStyleVar();
        ImGui::EndPopup();
    }

    void    FEModelTree::drawRenamePopup()
    {
        if (_renamePopup)
        {
            ImGui::OpenPopup("重命名节点");
            _renamePopup    =   false;
        }
        if (!ImGui::BeginPopupModal("重命名节点",nullptr,ImGuiWindowFlags_AlwaysAutoResize))
            return;

        ImGui::TextUnformatted("节点名称");
        ImGui::PushItemWidth(240.0f);
        /// <summary>
        /// 弹窗刚打开时把键盘焦点交给输入框,可直接输入
        /// </summary>
        if (_renameFocus)
        {
            ImGui::SetKeyboardFocusHere();
            _renameFocus    =   false;
        }
        const   bool    enter   =   ImGui::InputText("##rename",_renameBuf,
                                                     sizeof(_renameBuf),
                                                     ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();
        const   bool    commit  =   ImGui::Button("确定") || enter;
        ImGui::SameLine();
        const   bool    cancel  =   ImGui::Button("取消");
        if (commit)
        {
            Node    node;
            if (_renameTarget)
                node    =   _renameTarget->cast<FENode>();
            if (node && _renameBuf[0] != '\0')
                renameNode(node,_renameBuf);
            _renameTarget   =   Object();
            ImGui::CloseCurrentPopup();
        }
        else if (cancel)
        {
            _renameTarget   =   Object();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    void    FEModelTree::renameNode(Node node,const String& name)
    {
        if (node == nullptr || name.empty())
            return;
        if (_undoStack != nullptr)
        {
            _undoStack->push(new FEEditorRenameNodeCmd(node,name,"重命名节点"));
            return;
        }
        node->setName(name);
        node->fireChanged();
    }

    void    FEModelTree::addChildNode(Node parent)
    {
        if (_nodeTree == nullptr || _scene == nullptr)
            return;
        Node    node    =   new FENode(_nodeTree->ctx());
        /// <summary>
        /// 与场景内新建节点一致:名称先在同级唯一化,保证重做时不会因重名而改名
        /// </summary>
        const   Nodes&  siblings    =   (parent != nullptr) ? parent->children()
                                                            : _nodeTree->topLevelNodes();
        node->setName(uniqueNameUnder(siblings,"子节点"));

        if (_undoStack != nullptr)
        {
            _undoStack->push(new FEEditorAddNodeCmd(*_scene,node,parent,"添加子节点"));
        }
        else
        {
            if (parent != nullptr)
                _nodeTree->setParent(node,parent.get());
            else
                _scene->addNodesToTree({node});
            _scene->dispatchNodesToSystem({node});
        }
        /// <summary>
        /// 展开父节点,便于看到新建的子节点
        /// </summary>
        if (parent != nullptr)
            parent->flags().addFlag(FLAG_EXPAND);
        _rowsDirty  =   true;
    }

    void    FEModelTree::removeNode(Node node)
    {
        if (_scene == nullptr || node == nullptr)
            return;
        const   Nodes   nodes   =   {node};
        if (_undoStack != nullptr)
        {
            _undoStack->push(new FEEditorRemoveNodeCmd(*_scene,nodes,"移除节点"));
        }
        else
        {
            _scene->nodeTree().removeNode(node);
            _scene->removeNodesFromSystem(nodes);
        }
        /// <summary>
        /// 选中项与隔离列表在重建行列表时自动剔除已脱离节点树的节点
        /// </summary>
        _rowsDirty  =   true;
    }

    void    FEModelTree::createAnimation(Node node)
    {
        if (node == nullptr || _scene == nullptr)
            return;
        /// <summary>
        /// 新建带默认(位置/缩放/旋转)轨道的动画,只挂到节点上;
        /// 需要在动画树中编辑时,再右键「添加到动画树」
        /// </summary>
        Animation   anim    =   FEAnimationHelper::createNodeAnimtion(node->ctx());
        if (anim == nullptr)
            return;
        if (_undoStack != nullptr)
        {
            _undoStack->push(new FEEditorAnimationCmd(*_scene,node,{anim}
                                                      ,FEEditorAnimationCmd::AM_Attach,"创建动画"));
            return;
        }
        node->addComponent(anim.get());
    }

    void    FEModelTree::removeAnimations(Node node)
    {
        if (node == nullptr || _scene == nullptr)
            return;
        Animations  anims;
        for (FEAnimation* anim : node->objects<FEAnimation>())
        {
            if (anim != nullptr)
                anims.emplace_back(anim);
        }
        if (anims.empty())
            return;
        if (_undoStack != nullptr)
        {
            _undoStack->push(new FEEditorAnimationCmd(*_scene,node,anims
                                                      ,FEEditorAnimationCmd::AM_Detach,"删除动画"));
            return;
        }
        FEAnimationSys* sys =   _scene->animationSystem();
        if (sys != nullptr)
        {
            for (Animation& anim : anims)
                sys->removeObject(anim.get());
        }
        for (Animation& anim : anims)
            node->removeComponent(anim.get());
    }

    void    FEModelTree::addAnimationsToTree(Node node)
    {
        if (node == nullptr || _scene == nullptr)
            return;
        FEAnimationSys* sys =   _scene->animationSystem();
        Animations      anims;
        for (FEAnimation* anim : node->objects<FEAnimation>())
        {
            if (anim != nullptr && !animationRegistered(sys,anim))
                anims.emplace_back(anim);
        }
        if (anims.empty())
            return;
        if (_undoStack != nullptr)
        {
            _undoStack->push(new FEEditorAnimationCmd(*_scene,node,anims
                                                      ,FEEditorAnimationCmd::AM_Register,"添加到动画树"));
            return;
        }
        if (sys != nullptr)
        {
            for (Animation& anim : anims)
                sys->addObject(anim.get());
        }
    }

    bool    FEModelTree::hasConstraint(Node node,ConstraintKind kind) const
    {
        if (node == nullptr)
            return  false;
        switch (kind)
        {
        case    CK_TRANS:   return  !node->objects<FEConstraintTrans>().empty();
        case    CK_FOLLOW:  return  !node->objects<FEConstraintFollow>().empty();
        case    CK_LOOKAT:  return  !node->objects<FEConstraintLookAt>().empty();
        case    CK_PATH:    return  !node->objects<FEConstraintPath>().empty();
        case    CK_DRIVE:   return  !node->objects<FEConstraintDrive>().empty();
        default:            return  false;
        }
    }

    void    FEModelTree::toggleConstraint(Node node,ConstraintKind kind)
    {
        if (node == nullptr || _scene == nullptr)
            return;

        /// <summary>
        /// 已挂该类型约束时取现有实例做移除,否则新建一个做添加
        /// </summary>
        Constraint  constraint;
        switch (kind)
        {
        case    CK_TRANS:
        {
            auto    objs    =   node->objects<FEConstraintTrans>();
            if (!objs.empty())
                constraint  =   objs.front();
            else
                constraint  =   new FEConstraintTrans(node->ctx());
            break;
        }
        case    CK_FOLLOW:
        {
            auto    objs    =   node->objects<FEConstraintFollow>();
            if (!objs.empty())
                constraint  =   objs.front();
            else
                constraint  =   new FEConstraintFollow(node->ctx());
            break;
        }
        case    CK_LOOKAT:
        {
            auto    objs    =   node->objects<FEConstraintLookAt>();
            if (!objs.empty())
                constraint  =   objs.front();
            else
                constraint  =   new FEConstraintLookAt(node->ctx());
            break;
        }
        case    CK_PATH:
        {
            auto    objs    =   node->objects<FEConstraintPath>();
            if (!objs.empty())
                constraint  =   objs.front();
            else
                constraint  =   new FEConstraintPath(node->ctx());
            break;
        }
        case    CK_DRIVE:
        {
            auto    objs    =   node->objects<FEConstraintDrive>();
            if (!objs.empty())
                constraint  =   objs.front();
            else
                constraint  =   new FEConstraintDrive(node->ctx());
            break;
        }
        default:
            return;
        }
        if (constraint == nullptr)
            return;

        const   bool    add =   !hasConstraint(node,kind);
        if (_undoStack != nullptr)
        {
            _undoStack->push(new FEEditorConstraintCmd(*_scene,node,constraint,add
                                                      ,add ? "添加约束" : "移除约束"));
            return;
        }
        FEConstraintSys*    sys =   _scene->constraintSystem();
        if (add)
        {
            node->addComponent(constraint.get());
            if (sys != nullptr)
                sys->addObject(constraint.get());
        }
        else
        {
            if (sys != nullptr)
                sys->removeObject(constraint.get());
            node->removeComponent(constraint.get());
        }
    }

    void    FEModelTree::selectSingle(const Object& object)
    {
        if (!object)
            return;
        clearSelection();
        _selection.push_back(object);
        _selected   =   object;
        _anchor     =   object;
        setSelectedFlag(_selected,true);
    }

    void    FEModelTree::toggleSelect(const Object& object)
    {
        if (!object)
            return;
        /// <summary>
        /// Ctrl 点击已选中项时取消选中;取消的是主选中项时主选中改回选区末尾
        /// </summary>
        if (isSelected(object))
        {
            for (auto itr = _selection.begin(); itr != _selection.end(); ++itr)
            {
                if (itr->get() == object.get())
                {
                    _selection.erase(itr);
                    break;
                }
            }
            setSelectedFlag(object,false);
            _anchor     =   object;
            if (_selected && _selected.get() == object.get())
                _selected   =   _selection.empty() ? Object() : _selection.back();
            return;
        }
        _selected   =   object;
        _anchor     =   object;
        _selection.push_back(object);
        setSelectedFlag(_selected,true);
    }

    void    FEModelTree::selectRangeTo(const Object& object)
    {
        if (!object)
            return;
        const   int from    =   rowIndexOf(_anchor ? _anchor : _selected);
        const   int to      =   rowIndexOf(object);
        if (from < 0 || to < 0)
        {
            selectSingle(object);
            return;
        }
        /// <summary>
        /// 重置选择后选中锚点到目标之间的所有行,并把目标设为主选中项(锚点不变)
        /// </summary>
        clearSelection();
        const   int lo  =   std::min(from,to);
        const   int hi  =   std::max(from,to);
        for (int i = lo; i <= hi; ++i)
            addToSelection(_rows[size_t(i)]._object);
        _selected   =   object;
    }

    void    FEModelTree::clearSelection()
    {
        for (const Object& object : _selection)
            setSelectedFlag(object,false);
        _selection.clear();
    }

    void    FEModelTree::addToSelection(const Object& object)
    {
        if (!object)
            return;
        for (const Object& item : _selection)
        {
            if (item.get() == object.get())
                return;
        }
        _selection.push_back(object);
        setSelectedFlag(object,true);
    }

    bool    FEModelTree::isSelected(const Object& object) const
    {
        if (!object)
            return  false;
        for (const Object& item : _selection)
        {
            if (item.get() == object.get())
                return  true;
        }
        return  false;
    }

    int     FEModelTree::rowIndexOf(const Object& object) const
    {
        if (!object)
            return  -1;
        for (size_t i = 0; i < _rows.size(); ++i)
        {
            if (_rows[i]._object.get() == object.get())
                return  int(i);
        }
        return  -1;
    }

    Nodes   FEModelTree::dragNodes() const
    {
        Nodes   nodes;
        if (!_dragSource)
            return  nodes;
        const   bool    multi   =   isSelected(_dragSource);
        for (size_t i = 0; i < _rows.size(); ++i)
        {
            const   Object& item    =   _rows[i]._object;
            if (multi && !isSelected(item))
                continue;
            const   Node    node    =   nodeOf(item);
            if (!node)
                continue;
            if (!multi)
            {
                /// 单选:只拖自身
                if (static_cast<const FEObject*>(node.get()) == _dragSource.get())
                    nodes.push_back(node);
                continue;
            }
            /// <summary>
            /// 多选:跳过被其他选中节点的子孙(父节点移动后子孙随之移动,避免重复)
            /// </summary>
            bool    redundant   =   false;
            for (size_t j = 0; j < _rows.size(); ++j)
            {
                const   Object& other  =   _rows[j]._object;
                if (other.get() == item.get() || !isSelected(other))
                    continue;
                const   Node    otherRef =   nodeOf(other);
                if (otherRef && isAncestor(otherRef.get(),node.get()))
                {
                    redundant   =   true;
                    break;
                }
            }
            if (!redundant)
                nodes.push_back(node);
        }
        return  nodes;
    }

    void    FEModelTree::setSelectedFlag(const Object& object,bool selected)
    {
        if (!object)
            return;
        /// flags() 的 const 版返回只读引用,这里统一取非 const 指针后再读写
        FEObject*   raw =   const_cast<FEObject*>(object.get());
        if (selected)
            raw->flags().addFlag(FLAG_SELECTED);
        else
            raw->flags().removeFlag(FLAG_SELECTED);
        /// <summary>
        /// 选中状态影响渲染,节点需要打 FLAG_PROP_STATE 并通知刷新
        /// </summary>
        Node    node    =   nodeOf(object);
        if (node)
        {
            node->flags().addFlag(FENode::FLAG_PROP_STATE);
            node->fireChanged();
        }
    }

    void    FEModelTree::setVisible(const Object& object,bool visible,bool recursive)
    {
        if (!object)
            return;
        /// <summary>
        /// flags() 的 const 版返回只读引用,这里统一取非 const 指针后再读写
        /// </summary>
        FEObject*   raw =   const_cast<FEObject*>(object.get());
        if (visible)
            raw->flags().addFlag(FLAG_VISIBLE | FLAG_UPDATE);
        else
            raw->flags().removeFlag(FLAG_VISIBLE);
        /// <summary>
        /// 显隐后节点需要通知渲染刷新
        /// </summary>
        Node    node    =   nodeOf(object);
        if (node)
        {
            node->flags().addFlag(FENode::FLAG_PROP_STATE);
            node->fireChanged();
        }
        if (!recursive)
            return;
        raw->traverseObject([this,visible,recursive](const FEObject& child,const FEObject&,
                                                     const FEObject::FETrvsCtx&,uint) -> bool
        {
            setVisible(Object(const_cast<FEObject*>(&child)),visible,recursive);
            return  true;
        });
    }

    void    FEModelTree::setExpanded(const Object& object,bool expanded,bool recursive)
    {
        if (!object)
            return;
        /// 展开/折叠会改变行列表结构,需重建
        _rowsDirty  =   true;
        FEObject*   raw =   const_cast<FEObject*>(object.get());
        if (expanded)
            raw->flags().addFlag(FLAG_EXPAND);
        else
            raw->flags().removeFlag(FLAG_EXPAND);
        if (!recursive)
            return;
        raw->traverseObject([this,expanded,recursive](const FEObject& child,const FEObject&,
                                                      const FEObject::FETrvsCtx&,uint) -> bool
        {
            setExpanded(Object(const_cast<FEObject*>(&child)),expanded,recursive);
            return  true;
        });
    }

    bool    FEModelTree::isAncestor(const FENode* ancestor,const FENode* node) const
    {
        if (ancestor == nullptr || node == nullptr)
            return  false;
        for (const FENode* p = node; p != nullptr; p = p->parent().get())
        {
            if (p == ancestor)
                return  true;
        }
        return  false;
    }

    bool    FEModelTree::isInTree(const FENode* node) const
    {
        if (_nodeTree == nullptr || node == nullptr)
            return  false;
        /// 沿父链找到子树根,根在顶层集合中才算仍属于当前节点树
        const   FENode* root    =   node;
        while (root->parent() != nullptr)
            root    =   root->parent().get();
        for (const Node& top : _nodeTree->topLevelNodes())
        {
            if (top.get() == root)
                return  true;
        }
        return  false;
    }

    bool    FEModelTree::canDrop(const FENode* source,const FENode* target,int mode) const
    {
        if (source == nullptr)
            return  false;
        if (mode < 1 || mode > 4)
            return  false;
        /// <summary>
        /// 4=挂到根节点,不需要目标
        /// </summary>
        if (mode == 4)
            return  true;
        if (target == nullptr || source == target)
            return  false;
        /// <summary>
        /// 不能把节点拖进自己的子孙(防成环)
        /// </summary>
        if (isAncestor(source,target))
            return  false;
        return  true;
    }

    Node    FEModelTree::topLevelNode(const FENode* node) const
    {
        if (_nodeTree == nullptr || node == nullptr)
            return  Node();
        for (const auto& top : _nodeTree->topLevelNodes())
        {
            if (top.get() == node)
                return  top;
        }
        return  Node();
    }

    void    FEModelTree::moveNode(FENode* source,FENode* target,int mode,FEEditorMoveNodeCmd* cmd)
    {
        if (_nodeTree == nullptr || source == nullptr)
            return;
        if (!canDrop(source,target,mode))
            return;

        /// 计算投放位置:parent 为空表示顶层列表
        FENode* parent  =   nullptr;
        size_t  index   =   SIZE_MAX;
        if  (mode == 1)
        {
            /// <summary>
            /// 嵌入目标内部:追加为目标子节点列表的末尾
            /// </summary>
            parent  =   target;
        }
        else if (mode != 4)
        {
            /// <summary>
            /// 插到目标前/后:与目标同父,下标按同级列表计算
            /// </summary>
            if (target == nullptr)
                return;
            parent  =   target->parent().get();
            if (!siblingIndex(source,target,parent,mode,index))
                return;
        }

        /// 移动前的父节点与下标:撤销命令据此还原
        const   Node    node(source);
        Node            oldParent;
        size_t          oldIndex    =   SIZE_MAX;
        if  (cmd != nullptr)
            placementOf(node,oldParent,oldIndex);

        if  (!applyPlacement(node,parent,index))
            return;

        if  (cmd != nullptr)
        {
            Node    newParent;
            size_t  newIndex    =   SIZE_MAX;
            placementOf(node,newParent,newIndex);
            cmd->add(node,{oldParent,oldIndex},{newParent,newIndex});
        }

        /// <summary>
        /// 嵌入目标内部时展开目标行,便于看到新子节点
        /// </summary>
        if  (mode == 1 && target != nullptr)
            target->flags().addFlag(FLAG_EXPAND);
        /// 拖拽会改变父子结构,需重建行列表
        _rowsDirty  =   true;
    }

    bool    FEModelTree::siblingIndex(const FENode* source,const FENode* target,const FENode* parent,int mode,size_t& index) const
    {
        if (_nodeTree == nullptr || target == nullptr)
            return  false;
        const   Nodes&  kids    =   (parent != nullptr) ? parent->children()
                                                        : _nodeTree->topLevelNodes();
        index   =   kids.size();
        for (size_t i = 0; i < kids.size(); ++i)
        {
            if (kids[i].get() == target)
            {
                index   =   i;
                break;
            }
        }
        if (index == kids.size())
            return  false;
        if (mode == 3)
            ++ index;
        /// <summary>
        /// 源在同一列表中且位于插入点之前时,摘除会让下标前移一位
        /// </summary>
        for (size_t i = 0; i < kids.size(); ++i)
        {
            if (kids[i].get() == source)
            {
                if (i < index)
                    --index;
                break;
            }
        }
        return  true;
    }

    void    FEModelTree::placementOf(const Node& node,Node& parent,size_t& index) const
    {
        parent  =   Node();
        index   =   SIZE_MAX;
        if (_nodeTree == nullptr || node == nullptr)
            return;
        parent  =   node->parent();
        if (parent != nullptr)
        {
            const   auto&   kids    =   parent->children();
            for (size_t i = 0; i < kids.size(); ++i)
            {
                if (kids[i].get() == node.get())
                {
                    index   =   i;
                    break;
                }
            }
            return;
        }
        const   auto&   tops    =   _nodeTree->topLevelNodes();
        for (size_t i = 0; i < tops.size(); ++i)
        {
            if (tops[i].get() == node.get())
            {
                index   =   i;
                break;
            }
        }
    }

    bool    FEModelTree::applyPlacement(const Node& node,FENode* parent,size_t index)
    {
        if (_nodeTree == nullptr || node == nullptr)
            return  false;
        if (parent != nullptr)
            return  _nodeTree->setParent(node,parent,index);
        /// <summary>
        /// 挂到根节点(SIZE_MAX):已是顶层时保持原位,子节点提升并追加到末尾
        /// </summary>
        if (index == SIZE_MAX)
            return  _nodeTree->setParent(node,nullptr);
        /// <summary>
        /// 顶层重排:已是顶层时直接调整列表位置,子节点先提升为顶层再重排
        /// </summary>
        if (topLevelNode(node.get()) == nullptr && !_nodeTree->setParent(node,nullptr))
            return  false;
        return  _nodeTree->moveToplevelNode(node,index);
    }

    void    FEModelTree::moveNodes(const Nodes& sources,FENode* target,int mode)
    {
        if (_nodeTree == nullptr || sources.empty())
            return;
        /// 拖拽改变层级/顺序,生成撤销命令以便 undo/redo
        const   char*   text    =   "调整顺序";
        if  (mode == 1)
            text    =   "调整层级";
        else if (mode == 4)
            text    =   "移到根节点";
        FEEditorMoveNodeCmd*    cmd =   (_undoStack != nullptr)
                                    ?   new FEEditorMoveNodeCmd(*_nodeTree,text,true,[this]() {   _rowsDirty = true;  })
                                    :   nullptr;
        if (mode == 3)
        {
            /// <summary>
            /// 插到目标之后:倒序处理,让多个节点保持拖拽前的先后顺序
            /// </summary>
            for (auto itr = sources.rbegin(); itr != sources.rend(); ++itr)
            {
                Node    source  =   *itr;
                moveNode(source.get(),target,mode,cmd);
            }
        }
        else
        {
            for (size_t i = 0; i < sources.size(); ++i)
            {
                Node    source  =   sources[i];
                moveNode(source.get(),target,mode,cmd);
            }
        }
        if (cmd == nullptr)
            return;
        if (cmd->empty())
            delete  cmd;
        else
            _undoStack->push(cmd);
    }

    String  FEModelTree::objectName(const Object& object) const
    {
        if (!object)
            return  String();
        Node    node    =   nodeOf(object);
        if (node && node->name() != nullptr && node->name()[0] != '\0')
            return  node->name();
        return  object->className();
    }
}
