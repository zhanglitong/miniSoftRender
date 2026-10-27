/// <summary>
/// FEModelTree 实现:ImGui 绘制场景节点树并处理选择/展开/显隐/拖拽/重命名。
/// </summary>

#include    "FEModelTree.hpp"

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
    const   ImU32   kTextColor     =   IM_COL32(214,219,229,255);
    const   ImU32   kTextHidden    =   IM_COL32(140,146,158,255);
    const   ImU32   kIconHidden    =   IM_COL32(110,116,128,255);
    const   ImU32   kAccent        =   IM_COL32(74,144,226,255);
    const   ImU32   kArrowColor    =   IM_COL32(150,156,168,255);

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
}

namespace   FE
{
    void    FEModelTree::draw(Scene scene,ImFont* iconFont)
    {
        _scene      =   scene;
        _iconFont   =   iconFont;

        _rows.clear();
        if (_scene != nullptr)
        {
            for (const auto& node : _scene->nodeTree().topLevelNodes())
                collectRows(Object(const_cast<FENode*>(node.get())),0);
        }

        drawToolbar();

        const   float   rowHeight   =   ImGui::GetTextLineHeight() + 8.0f;
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
        /// 拖拽:按行矩形反查投放目标(鼠标按住时行按钮处于 active,不能用 hovered 判定)
        /// </summary>
        if (_dragging)
        {
            const   ImVec2  mouse   =   ImGui::GetIO().MousePos;
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
                _dropMode   =   (ratio < 0.3f) ? 2 : ((ratio > 0.7f) ? 3 : 1);
                FENode*         source  =   _dragSource ? _dragSource->cast<FENode>() : nullptr;
                FENode*         target  =   _dropTarget ? _dropTarget->cast<FENode>() : nullptr;
                if (!canDrop(source,target,_dropMode))
                    _dropMode   =   0;
            }

            ImDrawList*     dl      =   ImGui::GetWindowDrawList();
            const   ImVec2  winPos  =   ImGui::GetWindowPos();
            const   float   x0      =   winPos.x + 4.0f;
            const   float   x1      =   winPos.x + ImGui::GetWindowWidth() - 4.0f;
            if (hit >= 0 && _dropMode == 1)
            {
                dl->AddRect(ImVec2(x0,rowTops[size_t(hit)] + 1.0f),
                            ImVec2(x1,rowTops[size_t(hit)] + rowHeight - 1.0f),kAccent,2.0f);
            }
            else if (hit >= 0 && (_dropMode == 2 || _dropMode == 3))
            {
                const   float   y   =   (_dropMode == 2)
                                        ? rowTops[size_t(hit)]
                                        : rowTops[size_t(hit)] + rowHeight;
                dl->AddLine(ImVec2(x0,y),ImVec2(x1,y),kAccent,2.0f);
            }

            if (ImGui::IsMouseReleased(0))
            {
                if (_dropMode != 0 && _dropTarget && _dragSource)
                {
                    moveNode(_dragSource->cast<FENode>(),
                             _dropTarget->cast<FENode>(),_dropMode);
                }
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
        if (ImGui::Button("展开全部"))
        {
            if (_scene != nullptr)
            {
                for (const auto& node : _scene->nodeTree().topLevelNodes())
                    setExpanded(Object(const_cast<FENode*>(node.get())),true,true);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("折叠全部"))
        {
            if (_scene != nullptr)
            {
                for (const auto& node : _scene->nodeTree().topLevelNodes())
                    setExpanded(Object(const_cast<FENode*>(node.get())),false,true);
            }
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%d 项",int(_rows.size()));
        ImGui::Separator();
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
        if (selected)
            dl->AddRectFilled(pos,end,kRowSelected);
        else if (rowHover)
            dl->AddRectFilled(pos,end,kRowHover);

        const   float   indent  =   14.0f;
        const   float   arrowX  =   pos.x + 2.0f + indent * float(row._depth);
        const   float   iconX   =   arrowX + 14.0f;
        const   float   textX   =   iconX + 18.0f;
        const   float   iconY   =   pos.y + (rowHeight - ImGui::GetTextLineHeight()) * 0.5f;

        /// <summary>
        /// 展开箭头(必须落在子窗口裁剪区内,否则点不到)
        /// </summary>
        if (row._hasChild)
        {
            ImGui::SetCursorScreenPos(ImVec2(arrowX,pos.y));
            ImGui::InvisibleButton("##expand",ImVec2(12.0f,rowHeight));
            if (ImGui::IsItemClicked(0))
                setExpanded(row._object,!row._expanded,false);

            const   ImVec2  c   =   ImVec2(arrowX + 6.0f,pos.y + rowHeight * 0.5f);
            if (row._expanded)
                dl->AddTriangleFilled(ImVec2(c.x - 4.0f,c.y - 3.0f),
                                      ImVec2(c.x + 4.0f,c.y - 3.0f),
                                      ImVec2(c.x,c.y + 4.0f),kArrowColor);
            else
                dl->AddTriangleFilled(ImVec2(c.x - 3.0f,c.y - 4.0f),
                                      ImVec2(c.x - 3.0f,c.y + 4.0f),
                                      ImVec2(c.x + 4.0f,c.y),kArrowColor);
        }

        /// <summary>
        /// 类型图标
        /// </summary>
        if (_iconFont != nullptr)
            dl->AddText(_iconFont,13.0f,ImVec2(iconX,iconY),kArrowColor,kGlyphCube);
        else
            dl->AddRectFilled(ImVec2(iconX,iconY),ImVec2(iconX + 12.0f,iconY + 12.0f),kArrowColor,2.0f);

        /// <summary>
        /// 显隐开关
        /// </summary>
        const   float   eyeX    =   end.x - 20.0f;
        ImGui::SetCursorScreenPos(ImVec2(eyeX,pos.y));
        ImGui::InvisibleButton("##visible",ImVec2(18.0f,rowHeight));
        if (ImGui::IsItemClicked(0))
            setVisible(row._object,!row._object->flags().hasFlag(FLAG_VISIBLE),false);

        /// <summary>
        /// 整行:选中/右键菜单/拖拽
        /// </summary>
        ImGui::SetCursorScreenPos(pos);
        ImGui::InvisibleButton("##row",ImVec2(width,rowHeight));
        if (ImGui::IsItemClicked(0))
            select(row._object);
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
        const   std::string     name    =   objectName(row._object);
        ImGui::PushClipRect(ImVec2(textX,pos.y),ImVec2(eyeX - 4.0f,pos.y + rowHeight),true);
        dl->AddText(ImVec2(textX,iconY),visible ? kTextColor : kTextHidden,name.c_str());
        ImGui::PopClipRect();

        if (_iconFont != nullptr)
            dl->AddText(_iconFont,13.0f,ImVec2(eyeX + 2.0f,iconY),
                        visible ? kArrowColor : kIconHidden,
                        visible ? kGlyphEye : kGlyphEyeOff);

        ImGui::SetCursorScreenPos(ImVec2(pos.x,pos.y + rowHeight));
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
        const   Object  object  =   _context;
        if (!object)
        {
            ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
            return;
        }

        const   bool    visible =   object->flags().hasFlag(FLAG_VISIBLE);
        if (ImGui::MenuItem(visible ? "隐藏" : "显示"))
            setVisible(object,!visible,false);
        if (ImGui::MenuItem("显示子树"))
            setVisible(object,true,true);
        if (ImGui::MenuItem("隐藏子树"))
            setVisible(object,false,true);
        ImGui::Separator();
        if (ImGui::MenuItem("展开子树"))
            setExpanded(object,true,true);
        if (ImGui::MenuItem("折叠子树"))
            setExpanded(object,false,true);
        ImGui::Separator();
        if (nodeOf(object) && ImGui::MenuItem("重命名..."))
        {
            _renameTarget   =   object;
            std::snprintf(_renameBuf,sizeof(_renameBuf),"%s",objectName(object).c_str());
            _renamePopup    =   true;
            _renameFocus    =   true;
        }
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
            {
                node->setName(_renameBuf);
                node->fireChanged();
            }
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

    void    FEModelTree::select(const Object& object)
    {
        if (!object)
            return;
        if (_selected && _selected != object)
            setSelectedFlag(_selected,false);
        _selected   =   object;
        setSelectedFlag(_selected,true);
    }

    void    FEModelTree::setSelectedFlag(Object& object,bool selected)
    {
        if (!object)
            return;
        if (selected)
            object->flags().addFlag(FLAG_SELECTED);
        else
            object->flags().removeFlag(FLAG_SELECTED);
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

    bool    FEModelTree::canDrop(const FENode* source,const FENode* target,int mode) const
    {
        if (source == nullptr || target == nullptr || source == target)
            return  false;
        /// <summary>
        /// 不能把节点拖进自己的子孙(防成环)
        /// </summary>
        if (isAncestor(source,target))
            return  false;
        /// <summary>
        /// 前/后插入需要目标有父节点(顶层排序暂不支持)
        /// </summary>
        if (mode == 2 || mode == 3)
            return  target->parent() != nullptr;
        return  true;
    }

    Node    FEModelTree::topLevelNode(FENode* node) const
    {
        if (_scene == nullptr || node == nullptr)
            return  Node();
        for (const auto& top : _scene->nodeTree().topLevelNodes())
        {
            if (top.get() == node)
                return  top;
        }
        return  Node();
    }

    void    FEModelTree::moveNode(FENode* source,FENode* target,int mode)
    {
        if (_scene == nullptr || source == nullptr || target == nullptr)
            return;
        if (!canDrop(source,target,mode))
            return;

        Node    src =   topLevelNode(source);
        if (source->parent() == nullptr)
        {
            if (src)
                _scene->nodeTree().removeToplevelNode(src);
        }

        if (mode == 1)
        {
            source->reparent(target);
            target->flags().addFlag(FLAG_EXPAND);
        }
        else
        {
            FENode*         parent  =   target->parent().get();
            if (parent == nullptr)
                return;
            size_t          index   =   0;
            auto&           kids    =   parent->children();
            for (; index < kids.size(); ++index)
            {
                if (kids[index].get() == target)
                    break;
            }
            if (mode == 3)
                ++index;
            source->reparent(parent,index);
        }
    }

    std::string FEModelTree::objectName(const Object& object) const
    {
        if (!object)
            return  std::string();
        Node    node    =   nodeOf(object);
        if (node && node->name() != nullptr && node->name()[0] != '\0')
            return  node->name();
        return  object->className();
    }
}
