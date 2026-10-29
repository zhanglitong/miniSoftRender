/// <summary>
/// FEModelTree
/// ImGui 版模型树,参考 QtTree 的行为:
///   - 行内容:展开箭头 + 类型图标 + 显隐开关 + 名称;
///   - 单击选中, Ctrl 点击切换单项选中, Shift 点击按锚点范围选中(参考 Windows 资源管理器);
///   - 箭头展开/折叠(FLAG_EXPAND),眼睛图标切换显隐;
///   - 右键菜单:重命名、显示/隐藏子树、展开/折叠子树;
///   - 拖拽挂接:落在目标行中间 60% 成为其子节点(高亮该行),
///     落在行上/下缘 20% 插入到目标前/后(行间绘制高亮线);拖动时绘制拖拽预览。
/// 数据直接来自 FENodeTree 的顶层节点,读写 FEObject 的 flags 与节点树;
/// 选中项列表与 FLAG_SELECTED 保持同步,主选中项供时间线等面板使用。
/// </summary>

#pragma     once

#include    "../../../inc/node/FENodeTree.hpp"

#include    <imgui.h>

#include    <string>
#include    <vector>

namespace   FE
{
    class   FEUndoStack;
    class   FEEditorMoveNodeCmd;
    class   FEScene;

    class   FEModelTree
    {
    public:
        FEModelTree() = default;
        /// <summary>
        /// 析构时解除对节点树事件的订阅
        /// </summary>
        ~FEModelTree();
        /// <summary>
        /// 在调用方已 Begin 的窗口内绘制模型树
        /// </summary>
        /// <param name="nodeTree">节点树,可为 nullptr</param>
        /// <param name="iconFont">图标字体,可为 nullptr</param>
        void    draw(FENodeTree* nodeTree,ImFont* iconFont);
        /// <summary>
        /// 解除对当前节点树的订阅(场景释放前调用)
        /// </summary>
        void    unbind();
        /// <summary>
        /// 绑定撤销栈:拖拽改变层级/顺序会生成撤销命令,可为 nullptr(不记录历史)
        /// </summary>
        /// <param name="undoStack">撤销栈</param>
        void    setUndoStack(FEUndoStack* undoStack)
        {
            _undoStack  =   undoStack;
        }
        /// <summary>
        /// 绑定场景:右键菜单增删节点/动画/约束需要访问节点树与各组件系统
        /// </summary>
        /// <param name="scene">场景,可为 nullptr</param>
        void    setScene(FEScene* scene)
        {
            _scene  =   scene;
        }
        /// <summary>
        /// 当前选中对象
        /// </summary>
        Object  selected() const    {   return  _selected;  }
        /// <summary>
        /// 当前选中对象列表(Ctrl/Shift 多选,主选中项见 selected())
        /// </summary>
        const   std::vector<Object>&    selection() const   {   return  _selection;  }
    protected:
        /// <summary>
        /// 绑定节点树,并订阅 add/remove 事件用于驱动刷新
        /// </summary>
        /// <param name="nodeTree">节点树,可为 nullptr</param>
        void    bindNodeTree(FENodeTree* nodeTree);
        /// <summary>
        /// 按当前节点树重建行列表
        /// </summary>
        void    rebuildRows();
        /// <summary>
        /// 节点新增通知:标记行列表需要重建
        /// </summary>
        /// <param name="node">新增节点</param>
        void    onNodeAdded(const FENode* node);
        /// <summary>
        /// 节点移除通知:标记行列表需要重建
        /// </summary>
        /// <param name="node">移除节点</param>
        void    onNodeRemoved(const FENode* node);
        /// <summary>
        /// 节点变更通知:标记行列表需要重建
        /// </summary>
        /// <param name="node">变更节点</param>
        void    onNodeChanged(const FENode* node);
        /// <summary>
        /// 节点树清空通知:标记行列表需要重建
        /// </summary>
        void    onTreeCleared();
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
        /// 约束类型(右键约束子菜单条目顺序)
        /// </summary>
        enum    ConstraintKind
        {
            CK_TRANS,
            CK_FOLLOW,
            CK_LOOKAT,
            CK_PATH,
            CK_DRIVE,
            CK_COUNT,
        };
        /// <summary>
        /// 重命名节点(走撤销栈)
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <param name="name">新名称</param>
        void    renameNode(Node node,const String& name);
        /// <summary>
        /// 新建空子节点(父节点为空表示加入顶层),走撤销栈
        /// </summary>
        /// <param name="parent">父节点</param>
        void    addChildNode(Node parent);
        /// <summary>
        /// 移除节点(连同子树),走撤销栈
        /// </summary>
        /// <param name="node">目标节点</param>
        void    removeNode(Node node);
        /// <summary>
        /// 创建动画:挂载新的动画组件并登记到动画系统,走撤销栈
        /// </summary>
        /// <param name="node">目标节点</param>
        void    createAnimation(Node node);
        /// <summary>
        /// 删除动画:从动画系统摘除并删除节点上的动画组件,走撤销栈
        /// </summary>
        /// <param name="node">目标节点</param>
        void    removeAnimations(Node node);
        /// <summary>
        /// 添加到动画树:把节点上尚未登记的动画登记到动画系统,走撤销栈
        /// </summary>
        /// <param name="node">目标节点</param>
        void    addAnimationsToTree(Node node);
        /// <summary>
        /// 添加/移除指定类型的约束(已存在则移除),走撤销栈
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <param name="kind">约束类型</param>
        void    toggleConstraint(Node node,ConstraintKind kind);
        /// <summary>
        /// 节点是否已挂指定类型的约束
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <param name="kind">约束类型</param>
        /// <returns>true 已挂 / false 未挂</returns>
        bool    hasConstraint(Node node,ConstraintKind kind) const;
        /// <summary>
        /// 选中/取消选中(与 QtTree 一致:节点额外打 FLAG_PROP_STATE 并通知刷新)
        /// </summary>
        /// <param name="object">目标对象</param>
        void    setSelectedFlag(const Object& object,bool selected);
        /// <summary>
        /// 单选:清空原有选择后选中目标,并把目标设为范围选择的锚点
        /// </summary>
        /// <param name="object">目标对象</param>
        void    selectSingle(const Object& object);
        /// <summary>
        /// Ctrl 点击:目标已选中则取消选中,否则加入选中列表并成为主选中项
        /// </summary>
        /// <param name="object">目标对象</param>
        void    toggleSelect(const Object& object);
        /// <summary>
        /// Shift 点击:选中锚点到目标之间的全部行(参考 Windows 资源管理器)
        /// </summary>
        /// <param name="object">目标对象</param>
        void    selectRangeTo(const Object& object);
        /// <summary>
        /// 清空所有选中项
        /// </summary>
        void    clearSelection();
        /// <summary>
        /// 加入选中列表(已在列表中则跳过)
        /// </summary>
        /// <param name="object">目标对象</param>
        void    addToSelection(const Object& object);
        /// <summary>
        /// 判断对象是否在选中列表中
        /// </summary>
        /// <param name="object">目标对象</param>
        /// <returns>true 已选中 / false 未选中</returns>
        bool    isSelected(const Object& object) const;
        /// <summary>
        /// 对象在当前行列表中的序号
        /// </summary>
        /// <param name="object">目标对象</param>
        /// <returns>行号;不在列表中返回 -1</returns>
        int     rowIndexOf(const Object& object) const;
        /// <summary>
        /// 收集一次拖拽要移动的节点:拖动源在选区内时移动整个选区,
        /// 并去掉被其他选中节点包含的子孙,避免重复移动
        /// </summary>
        /// <returns>按行顺序排列的节点列表</returns>
        Nodes   dragNodes() const;
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
        /// 隔离/取消隔离:开启时模型树只显示当前选中节点及其子树(可多个);
        /// 已处于隔离状态时再次调用则恢复显示整棵树
        /// </summary>
        void    toggleIsolation();
        /// <summary>
        /// 拖拽投放(1=嵌入目标,2=插到目标前,3=插到目标后,4=挂到根节点)
        /// </summary>
        /// <param name="source">拖拽源节点</param>
        /// <param name="target">投放目标节点(mode=4 时可为 nullptr)</param>
        /// <param name="mode">投放模式</param>
        /// <param name="cmd">撤销命令,传入时记录本条移动前后的位置(可为 nullptr)</param>
        void    moveNode(FENode* source,FENode* target,int mode,FEEditorMoveNodeCmd* cmd = nullptr);
        /// <summary>
        /// 批量投放:按行顺序移动多个节点,插到目标后时倒序处理以保持原有先后
        /// </summary>
        /// <param name="sources">拖拽源节点列表</param>
        /// <param name="target">投放目标节点</param>
        /// <param name="mode">投放模式</param>
        void    moveNodes(const Nodes& sources,FENode* target,int mode);
        /// <summary>
        /// 计算"插到目标前/后"时相对同级列表的下标
        /// </summary>
        /// <param name="source">拖拽源节点</param>
        /// <param name="target">投放目标节点</param>
        /// <param name="parent">目标的父节点(nullptr=顶层列表)</param>
        /// <param name="mode">2=目标前 3=目标后</param>
        /// <param name="index">输出下标(按移除源节点后的列表计算)</param>
        /// <returns>目标不在同级列表中时返回 false</returns>
        bool    siblingIndex(const FENode* source,const FENode* target,const FENode* parent,int mode,size_t& index) const;
        /// <summary>
        /// 取节点当前的父节点与同级下标
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <param name="parent">输出父节点(nullptr=顶层)</param>
        /// <param name="index">输出下标(节点不在此列表中时为 SIZE_MAX)</param>
        void    placementOf(const Node& node,Node& parent,size_t& index) const;
        /// <summary>
        /// 应用一次层级/顺序变更:父节点为空时提升到顶层并按顶层列表重排
        /// </summary>
        /// <param name="node">要移动的节点</param>
        /// <param name="parent">目标父节点(nullptr=顶层)</param>
        /// <param name="index">下标(按移除该节点后的列表计算;SIZE_MAX=末尾)</param>
        /// <returns>移动成功返回 true</returns>
        bool    applyPlacement(const Node& node,FENode* parent,size_t index);
        bool    isAncestor(const FENode* ancestor,const FENode* node) const;
        /// <summary>
        /// 节点是否仍挂在当前节点树上(用于剔除已删除节点的选中状态)
        /// </summary>
        /// <param name="node">目标节点</param>
        /// <returns>true 仍在树上 / false 已脱离</returns>
        bool    isInTree(const FENode* node) const;
        bool    canDrop(const FENode* source,const FENode* target,int mode) const;
        String  objectName(const Object& object) const;
        /// <summary>
        /// 把顶层节点包装成 Node(用于 nodeTree 的移除/添加)
        /// </summary>
        /// <param name="node">节点指针</param>
        Node    topLevelNode(const FENode* node) const;
    protected:
        /// <summary>
        /// 数据来源:节点树
        /// </summary>
        FENodeTree*             _nodeTree   =   nullptr;
        /// <summary>
        /// 撤销栈:拖拽改变层级/顺序时生成撤销命令
        /// </summary>
        FEUndoStack*            _undoStack  =   nullptr;
        /// <summary>
        /// 场景:右键菜单增删节点/动画/约束的数据来源
        /// </summary>
        FEScene*                _scene      =   nullptr;
        /// <summary>
        /// 隔离状态:开启时模型树只显示 _isolated 中的节点及其子树
        /// </summary>
        bool                    _isolate    =   false;
        /// <summary>
        /// 被隔离的节点(支持多个,已在节点树中被删除的会被自动剔除)
        /// </summary>
        Nodes                   _isolated;
        Rows                    _rows;
        /// <summary>
        /// 行列表是否需要重建(由 add/remove 事件与本地结构变更置位)
        /// </summary>
        bool                    _rowsDirty  =   true;
        ImFont*                 _iconFont   =   nullptr;
        /// <summary>
        /// 主选中项(选区中最后点击的对象,供时间线等面板使用)
        /// </summary>
        Object                  _selected;
        /// <summary>
        /// 选中对象列表,与 FLAG_SELECTED 同步
        /// </summary>
        std::vector<Object>     _selection;
        /// <summary>
        /// Shift 范围选择的锚点
        /// </summary>
        Object                  _anchor;
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
        /// 0 无 / 1 嵌入 / 2 前 / 3 后 / 4 挂到根节点
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
