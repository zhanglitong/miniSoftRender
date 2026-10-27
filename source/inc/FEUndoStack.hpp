#pragma     once

#include    "FEDefine.h"
#include    "FEDelegate.hpp"
#include    "FEUndoCmd.hpp"

#include    <memory>
#include    <vector>

namespace   FE
{
    /// <summary>
    /// 撤销/重做栈,接口参考 Qt 的 QUndoStack:
    /// - push() 立即执行命令的 redo(),游标(index)始终指向"已执行的命令数"
    /// - undo()/redo() 只移动游标,不改变栈内容;setIndex() 可以一次移动多步
    /// - beginMacro()/endMacro() 把一段命令合并成一条,undo/redo 时成组生效
    /// - 同 id 的连续命令可通过 mergeWith() 合并(例如连续拖动同一个物体)
    /// - setUndoLimit() 限制保留的命令条数(0 表示不限制,超出时丢弃最旧的命令)
    /// - setClean()/isClean() 记录"已保存"的位置,供编辑器判断工程是否有未保存修改
    /// 与 Qt 的差异:
    /// - QString 换成 FE::String
    /// - 信号改用 FETMultiDelegate,用法:stack.canUndoChanged += {this,[](bool can){...}};
    /// - 不提供 createUndoAction/createRedoAction(引擎没有 Action 抽象),
    ///   界面用 undoText()/redoText() 自行拼装撤销/重做菜单项
    /// 非线程安全,只应在 UI/主线程使用
    /// </summary>
    class   FEUndoStack
    {
    public:
        /// Qt 的 canUndoChanged / canRedoChanged / cleanChanged 信号对应
        using   SignalBool      =   FETMultiDelegate<void(bool)>;
        /// Qt 的 undoTextChanged / redoTextChanged 信号对应
        using   SignalString    =   FETMultiDelegate<void(const String&)>;
        /// Qt 的 indexChanged 信号对应
        using   SignalInt       =   FETMultiDelegate<void(int)>;
        /// <summary>
        /// 已入栈的命令集合
        /// </summary>
        using   UndoCmds        =   std::vector<std::unique_ptr<FEUndoCmd>>;
        /// <summary>
        /// 进行中的宏容器栈(不持有所有权,所有权在父容器或 _cmds)
        /// </summary>
        using   MacroCmds       =   std::vector<FEUndoCmd*>;
    public:
        FEUndoStack()   =   default;
        ~FEUndoStack()
        {
            /// 宏未结束时,栈内容只由最外层的宏容器持有,释放它会递归释放嵌套的容器
            if (!_macroStack.empty())
                delete _macroStack.front();
        }
        FEUndoStack(const FEUndoStack&)             =   delete;
        FEUndoStack&    operator=(const FEUndoStack&)   =   delete;

        /// <summary>
        /// 压入一条命令,栈接管其所有权。
        /// 与 Qt 一致:
        /// - 若当前存在 redo 分支,先丢弃该分支
        /// - 若上一条命令 id() 相同且 mergeWith() 返回 true,本条被丢弃(不执行 redo())
        /// - 否则立即执行一次 redo(),之后可通过 undo() 撤销
        /// - 宏进行中(beginMacro 之后)只把命令挂到宏容器,执行推迟到 endMacro()
        /// </summary>
        void    push(FEUndoCmd* cmd)
        {
            if (cmd == nullptr)
                return;
            const   State   before  =   state();
            /// 宏进行中:命令成为宏的子命令,由宏容器统一 redo/undo
            if (!_macroStack.empty())
            {
                _macroStack.back()->addChild(cmd);
                notifyChanges(before);
                return;
            }
            truncateRedo();
            /// 合并:与栈顶命令同 id 且合并成功,新命令被丢弃
            if (!_cmds.empty())
            {
                FEUndoCmd*  last    =   _cmds.back().get();
                const   int id      =   last->id();
                if (id >= 0 && id == cmd->id() && last->mergeWith(cmd))
                {
                    delete  cmd;
                    notifyChanges(before);
                    return;
                }
            }
            /// 超出上限:先丢弃最旧的命令(不执行 undo)
            if (_undoLimit > 0 && count() >= _undoLimit)
                dropOldest();
            cmd->redo();
            if (cmd->isObsolete())
            {
                /// redo 之后即失效:不入栈
                delete  cmd;
            }
            else
            {
                _cmds.emplace_back(cmd);
                ++ _index;
            }
            notifyChanges(before);
        }
        /// <summary>
        /// 撤销最近一条命令
        /// </summary>
        void    undo()
        {
            if (canUndo())
                setIndex(_index - 1);
        }
        /// <summary>
        /// 重做被撤销的命令
        /// </summary>
        void    redo()
        {
            if (canRedo())
                setIndex(_index + 1);
        }
        /// <summary>
        /// 把游标移动到指定位置:向后逐个 redo(),向前逐个 undo()
        /// </summary>
        void    setIndex(int index)
        {
            const   State   before  =   state();
            if (index < 0)
                index   =   0;
            if (index > count())
                index   =   count();
            while (_index < index && _index < count())
            {
                FEUndoCmd*  cmd =   _cmds[size_t(_index)].get();
                cmd->redo();
                /// 失效命令直接移出栈,游标不动(下一条补位)
                if (cmd->isObsolete())
                    removeAt(_index);
                else
                    ++ _index;
            }
            while (_index > index && _index > 0)
            {
                FEUndoCmd*  cmd =   _cmds[size_t(_index - 1)].get();
                cmd->undo();
                if (cmd->isObsolete())
                    removeAt(_index - 1);
                else
                    -- _index;
            }
            notifyChanges(before);
        }
        /// <summary>
        /// 是否可以撤销
        /// </summary>
        bool    canUndo() const
        {
            return  _index > 0;
        }
        /// <summary>
        /// 是否可以重做
        /// </summary>
        bool    canRedo() const
        {
            return  _index < count();
        }
        /// <summary>
        /// 栈中命令条数(含被撤销、仍可重做的命令)
        /// </summary>
        int     count() const
        {
            return  int(_cmds.size());
        }
        /// <summary>
        /// 当前游标:0 表示全部处于未执行状态,count() 表示全部已执行
        /// </summary>
        int     index() const
        {
            return  _index;
        }
        /// <summary>
        /// 撤销菜单项文本(将要被撤销的命令)
        /// </summary>
        String  undoText() const
        {
            return  canUndo() ? _cmds[size_t(_index - 1)]->text() : String();
        }
        /// <summary>
        /// 重做菜单项文本(将要被重做的命令)
        /// </summary>
        String  redoText() const
        {
            return  canRedo() ? _cmds[size_t(_index)]->text() : String();
        }
        /// <summary>
        /// 指定下标命令的文本,越界返回空串
        /// </summary>
        const   String& text(int index) const
        {
            static  const   String  sEmpty;
            if (index < 0 || index >= count())
                return  sEmpty;
            return  _cmds[size_t(index)]->text();
        }
        /// <summary>
        /// 限制保留的命令条数,0 表示不限制。超出时丢弃最旧的命令(不执行 undo)
        /// </summary>
        void    setUndoLimit(int limit)
        {
            if (limit < 0)
                limit   =   0;
            if (limit == _undoLimit)
                return;
            _undoLimit  =   limit;
            const   State   before  =   state();
            while (_undoLimit > 0 && count() > _undoLimit)
            {
                dropOldest();
            }
            notifyChanges(before);
        }
        int     undoLimit() const
        {
            return  _undoLimit;
        }
        /// <summary>
        /// 开始一个宏:之后 push 的命令都成为它的子命令,endMacro() 时合成一条命令入栈
        /// </summary>
        void    beginMacro(const String& text)
        {
            /// 只有最外层宏需要丢弃 redo 分支,嵌套宏的命令属于父宏
            if (_macroStack.empty())
                truncateRedo();
            FEUndoCmd*  cmd =   new FEUndoCmd(text);
            /// 嵌套:新容器成为当前宏的子命令
            if (!_macroStack.empty())
                _macroStack.back()->addChild(cmd);
            _macroStack.push_back(cmd);
        }
        /// <summary>
        /// 结束当前宏:命令数为 0 的宏被丢弃;最外层宏入栈并执行一次(容器 redo 会顺序执行子命令)
        /// </summary>
        void    endMacro()
        {
            if (_macroStack.empty())
                return;
            const   State   before  =   state();
            FEUndoCmd*  cmd =   _macroStack.back();
            _macroStack.pop_back();
            if (cmd->childCount() == 0)
            {
                /// 空宏:从父宏摘除后释放
                if (_macroStack.empty())
                    delete  cmd;
                else
                    _macroStack.back()->removeChild(cmd);
            }
            else if (_macroStack.empty())
            {
                /// 最外层宏:入栈,子命令在此刻才真正执行
                cmd->redo();
                if (cmd->isObsolete())
                {
                    delete  cmd;
                }
                else
                {
                    _cmds.emplace_back(cmd);
                    ++ _index;
                }
            }
            notifyChanges(before);
        }
        /// <summary>
        /// 是否处于 clean 状态(游标停在 setClean() 时的位置),编辑器用它判断有无未保存修改
        /// </summary>
        bool    isClean() const
        {
            return  _index == _cleanIndex;
        }
        /// <summary>
        /// 把当前位置标记为 clean
        /// </summary>
        void    setClean()
        {
            const   State   before  =   state();
            _cleanIndex =   _index;
            notifyChanges(before);
        }
        /// <summary>
        /// clean 位置,返回 -1 表示该位置已随 redo 分支一起被丢弃
        /// </summary>
        int     cleanIndex() const
        {
            return  _cleanIndex;
        }
        /// <summary>
        /// 清空历史记录:不执行 undo,直接释放所有命令,游标与 clean 位置归零
        /// </summary>
        void    clear()
        {
            const   State   before  =   state();
            if (!_macroStack.empty())
            {
                /// 释放最外层宏容器会递归释放嵌套容器
                delete  _macroStack.front();
                _macroStack.clear();
            }
            _cmds.clear();
            _index      =   0;
            _cleanIndex =   0;
            notifyChanges(before);
        }

    public:
        /// <summary>
        /// Qt 信号 canUndoChanged(bool):可用撤销状态变化
        /// </summary>
        SignalBool      canUndoChanged;
        /// <summary>
        /// Qt 信号 canRedoChanged(bool):可用重做状态变化
        /// </summary>
        SignalBool      canRedoChanged;
        /// <summary>
        /// Qt 信号 undoTextChanged(const String&):撤销文本变化
        /// </summary>
        SignalString    undoTextChanged;
        /// <summary>
        /// Qt 信号 redoTextChanged(const String&):重做文本变化
        /// </summary>
        SignalString    redoTextChanged;
        /// <summary>
        /// Qt 信号 indexChanged(int):游标变化
        /// </summary>
        SignalInt       indexChanged;
        /// <summary>
        /// Qt 信号 cleanChanged(bool):clean 状态变化
        /// </summary>
        SignalBool      cleanChanged;

    protected:
        /// <summary>
        /// 信号所需的栈状态快照,用于只在真正变化时触发通知(与 Qt 一致)
        /// </summary>
        struct  State
        {
            bool    canUndo     =   false;
            bool    canRedo     =   false;
            bool    clean       =   true;
            int     index       =   0;
            String  undoText;
            String  redoText;
        };
        State   state() const
        {
            State   result;
            result.canUndo  =   canUndo();
            result.canRedo  =   canRedo();
            result.clean    =   isClean();
            result.index    =   _index;
            result.undoText =   undoText();
            result.redoText =   redoText();
            return  result;
        }
        /// <summary>
        /// 与操作前的快照比较,只触发发生变化的信号
        /// </summary>
        void    notifyChanges(const State& before)
        {
            const   State   now =   state();
            if (now.canUndo != before.canUndo)
                canUndoChanged(now.canUndo);
            if (now.canRedo != before.canRedo)
                canRedoChanged(now.canRedo);
            if (now.clean != before.clean)
                cleanChanged(now.clean);
            if (now.index != before.index)
                indexChanged(now.index);
            if (now.undoText != before.undoText)
                undoTextChanged(now.undoText);
            if (now.redoText != before.redoText)
                redoTextChanged(now.redoText);
        }
        /// <summary>
        /// 丢弃游标之后的命令(redo 分支);clean 位置落在其中时失效
        /// </summary>
        void    truncateRedo()
        {
            if (_index >= count())
                return;
            _cmds.erase(_cmds.begin() + _index,_cmds.end());
            if (_cleanIndex > _index)
                _cleanIndex =   -1;
        }
        /// <summary>
        /// 删除指定位置的命令(不执行 undo),同步修正游标与 clean 位置
        /// </summary>
        void    removeAt(int index)
        {
            _cmds.erase(_cmds.begin() + index);
            if (index < _index)
                -- _index;
            if (_cleanIndex < 0)
                return;
            if (index < _cleanIndex)
                -- _cleanIndex;
            else if (index == _cleanIndex)
                _cleanIndex =   -1;
        }
        /// <summary>
        /// 丢弃最旧的命令(undoLimit 与 setUndoLimit 使用)
        /// </summary>
        void    dropOldest()
        {
            if (!_cmds.empty())
                removeAt(0);
        }

    protected:
        /// <summary>
        /// 已入栈的命令
        /// </summary>
        UndoCmds    _cmds;
        /// <summary>
        /// 进行中的宏(beginMacro 之后、endMacro 之前的容器)
        /// </summary>
        MacroCmds   _macroStack;
        /// <summary>
        /// 游标:已执行到第几条命令
        /// </summary>
        int         _index      =   0;
        /// <summary>
        /// clean 位置,-1 表示该位置已被丢弃
        /// </summary>
        int         _cleanIndex =   0;
        /// <summary>
        /// 命令条数上限,0 表示不限制
        /// </summary>
        int         _undoLimit  =   0;
    };
}
