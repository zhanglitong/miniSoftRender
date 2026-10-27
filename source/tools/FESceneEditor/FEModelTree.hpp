/// <summary>
/// FEModelTree
/// ImGui 版模型树,参考 QtTree 的行为:
///   - 行内容:展开箭头 + 类型图标 + 显隐开关 + 名称;
///   - 单击选中(设置 FLAG_SELECTED,并通知节点刷新渲染状态);
///   - 箭头展开/折叠(FLAG_EXPAND),眼睛图标切换显隐;
///   - 右键菜单:重命名、显示/隐藏子树、展开/折叠子树;
///   - 拖拽挂接:拖到目标行中部成为子节点,拖到上/下缘插入到目标前/后。
/// 数据直接读写 FEObject 的 flags 与场景节点树,不额外保存树状态。
/// </summary>

#pragma     once

#include    "../../../inc/node/FENode.hpp"
#include    "../../../inc/graphic/FEScene.h"

#include    <imgui.h>

#include    <string>
#include    <vector>

namespace   FE
{
    class   FEModelTree
    {
    public:
        /// <summary>
        /// 在调用方已 Begin 的窗口内绘制模型树
        /// </summary>
        /// <param name="scene">场景</param>
        /// <param name="iconFont">图标字体,可为 nullptr</param>
        void    draw(Scene scene,ImFont* iconFont);
        /// <summary>
        /// 当前选中对象
        /// </summary>
        Object  selected() const    {   return  _selected;  }
    protected:
        /// <summary>
        /// 一行树节点的数据
        /// </summary>
        struct  RowData
        {
            Object      _object;
            int         _depth      =   0;
            bool        _hasChild   =   false;
            bool        _expanded   =   false;
        };
        /// <summary>
        /// 行列表
        /// </summary>
        using   Rows    =   std::vector<RowData>;
    protected:
        void    collectRows(const Object& object,int depth);
        void    drawRow(const RowData& row,int index,float width,float rowHeight);
        void    drawToolbar();
        void    handleContextMenu();
        void    drawRenamePopup();
        /// <summary>
        /// 选中/取消选中(与 QtTree 一致:节点额外打 FLAG_PROP_STATE 并通知刷新)
        /// </summary>
        /// <param name="object">目标对象</param>
        void    select(const Object& object);
        void    setSelectedFlag(Object& object,bool selected);
        /// <summary>
        /// 显隐;recursive 时连同子孙一起处理
        /// </summary>
        /// <param name="object">目标对象</param>
        /// <param name="visible">true 显示 / false 隐藏</param>
        /// <param name="recursive">是否连同子树一起处理</param>
        void    setVisible(const Object& object,bool visible,bool recursive);
        /// <summary>
        /// 展开/折叠;recursive 时连同子孙一起处理
        /// </summary>
        /// <param name="object">目标对象</param>
        /// <param name="expanded">true 展开 / false 折叠</param>
        /// <param name="recursive">是否连同子树一起处理</param>
        void    setExpanded(const Object& object,bool expanded,bool recursive);
        /// <summary>
        /// 拖拽投放(1=嵌入目标,2=插到目标前,3=插到目标后)
        /// </summary>
        /// <param name="source">拖拽源节点</param>
        /// <param name="target">投放目标节点</param>
        /// <param name="mode">投放模式</param>
        void    moveNode(FENode* source,FENode* target,int mode);
        bool    isAncestor(const FENode* ancestor,const FENode* node) const;
        bool    canDrop(const FENode* source,const FENode* target,int mode) const;
        std::string objectName(const Object& object) const;
        /// <summary>
        /// 把顶层节点包装成 Node(用于 nodeTree 的移除/添加)
        /// </summary>
        /// <param name="node">节点指针</param>
        Node    topLevelNode(FENode* node) const;
    protected:
        Scene                   _scene;
        Rows                    _rows;
        ImFont*                 _iconFont   =   nullptr;
        Object                  _selected;
        /// <summary>
        /// 右键菜单目标
        /// </summary>
        Object                  _context;
        /// <summary>
        /// 下一帧在父窗口打开弹出菜单
        /// </summary>
        bool                    _contextOpen    =   false;
        /// <summary>
        /// 拖拽源
        /// </summary>
        Object                  _dragSource;
        bool                    _dragging   =   false;
        /// <summary>
        /// 当前投放目标行
        /// </summary>
        Object                  _dropTarget;
        /// <summary>
        /// 0 无 / 1 嵌入 / 2 前 / 3 后
        /// </summary>
        int                     _dropMode   =   0;
        Object                  _renameTarget;
        char                    _renameBuf[128] =   {   0   };
        bool                    _renamePopup    =   false;
        /// <summary>
        /// 下一帧把焦点交给输入框
        /// </summary>
        bool                    _renameFocus    =   false;
    };
}
