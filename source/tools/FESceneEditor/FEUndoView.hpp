/// <summary>
/// FEUndoView
/// 撤销历史窗口,参考 Qt 的 QUndoView:
///   - 按入栈顺序列出 FEUndoStack 中的全部命令,行号为 1..count;
///   - 游标(index)之后的条目表示"已撤销",用暗色文字区分;
///   - 选中态落在最后一条已执行的命令上,与 QUndoView 的当前位置一致;
///   - 单击任意条目调用 setIndex(),一次回滚/重做到该位置。
/// 窗口是浮动的,由 Ribbon「编辑 → 历史 → 撤销历史」开关。
/// </summary>

#pragma     once

#include    "../../../inc/FEUndoStack.hpp"

#include    <functional>
#include    <vector>

namespace   FE
{
    class   FEUndoView
    {
    public:
        /// <summary>
        /// 推迟到下一帧执行的场景操作队列(与 FEEditorUi 的 _pendingActions 同类型)
        /// </summary>
        using   Actions     =   std::vector<std::function<void()>>;
    public:
        FEUndoView()    =   default;
        ~FEUndoView()   =   default;
        FEUndoView(const FEUndoView&)               =   delete;
        FEUndoView& operator=(const FEUndoView&)    =   delete;
    public:
        /// <summary>
        /// 显示/隐藏撤销历史窗口
        /// </summary>
        void    toggle();
        /// <summary>
        /// 直接设置窗口的显示状态
        /// </summary>
        void    setOpen(bool open);
        bool    isOpen()    const   {   return  _open;   }
        /// <summary>
        /// 每帧绘制;未打开时直接返回。
        /// 条目的点击不回滚立即执行,而是压入 pending,由宿主在原定的
        /// "渲染趟之外"时机(setIndex 会增删场景节点)统一执行
        /// </summary>
        /// <param name="stack">被展示的撤销栈</param>
        /// <param name="pending">宿主待执行操作队列</param>
        void    draw(FEUndoStack& stack,Actions& pending);
    protected:
        /// <summary>
        /// 窗口开关
        /// </summary>
        bool                            _open   =   false;
    };
}
