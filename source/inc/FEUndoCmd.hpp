#pragma     once

#include    "FEDefine.h"
#include    "FEUnused.hpp"

#include    <memory>
#include    <vector>

namespace   FE
{
    class   FEUndoStack;

    /// <summary>
    /// 撤销/重做命令基类,接口参考 Qt 的 QUndoCommand,配合 FEUndoStack 使用。
    /// - 派生类实现 undo()/redo();命令入栈时会立即执行一次 redo()
    /// - 需要与上一条同类命令合并时重写 id()(返回相同的非负标识)与 mergeWith()
    /// - 生命周期由 FEUndoStack 接管:push() 之后不要自行 delete
    /// - 宏(beginMacro/endMacro)由本类的子命令列表实现:默认 undo()/redo()
    ///   就是逆序/顺序执行子命令,与 Qt 一致
    /// 与 Qt 的差异:QString 换成 FE::String,其余语义保持一致
    /// </summary>
    class   FEUndoCmd
    {
        friend  class   FEUndoStack;
    public:
        /// <summary>
        /// 子命令集合:本类独占所有权,析构时随容器一起释放
        /// </summary>
        using   UndoCmds    =   std::vector<std::unique_ptr<FEUndoCmd>>;
    public:
        explicit    FEUndoCmd(const String& text = String())
            :_text(text)
        {}
        virtual     ~FEUndoCmd()                    =   default;
        FEUndoCmd(const FEUndoCmd&)                 =   delete;
        FEUndoCmd&  operator=(const FEUndoCmd&)     =   delete;

        /// <summary>
        /// 撤销本条命令:宏命令默认逆序执行所有子命令
        /// </summary>
        virtual void    undo()
        {
            for (auto itr = _children.rbegin(); itr != _children.rend(); ++itr)
            {
                (*itr)->undo();
            }
        }
        /// <summary>
        /// 重做本条命令:宏命令默认顺序执行所有子命令。
        /// FEUndoStack::push/redo 会调用本函数,子类在这里真正修改数据
        /// </summary>
        virtual void    redo()
        {
            for (auto& var : _children)
            {
                var->redo();
            }
        }
        /// <summary>
        /// 合并标识:返回负值表示不参与合并;只有与栈顶命令 id() 相同且非负时才尝试 mergeWith()
        /// </summary>
        virtual int     id() const
        {
            return  -1;
        }
        /// <summary>
        /// 尝试把 other 合并进本条命令,返回 true 表示合并成功(other 随后被丢弃)。
        /// other 不会执行 redo(),合并实现需自行保证数据已处于 other 的最终状态
        /// </summary>
        virtual bool    mergeWith(const FEUndoCmd* other)
        {
            UNUSED(other);
            return  false;
        }
        /// <summary>
        /// 命令描述,用于撤销/重做菜单项的文本
        /// </summary>
        const   String& text() const
        {
            return  _text;
        }
        void    setText(const String& text)
        {
            _text   =   text;
        }
        /// <summary>
        /// 菜单项文字(actionText),未单独设置时回退为 text()
        /// </summary>
        const   String& actionText() const
        {
            return  _actionText.empty() ? _text : _actionText;
        }
        void    setActionText(const String& text)
        {
            _actionText =   text;
        }
        /// <summary>
        /// 命令是否已失效:undo()/redo() 执行后若为 true,FEUndoStack 会把它从栈中移除
        /// (Qt 6.2 引入的同类机制,用于"操作被后续操作抵消"的场景)
        /// </summary>
        bool    isObsolete() const
        {
            return  _obsolete;
        }
        void    setObsolete(bool obsolete = true)
        {
            _obsolete   =   obsolete;
        }
        /// <summary>
        /// 子命令个数(宏命令内包含的命令数)
        /// </summary>
        int     childCount() const
        {
            return  int(_children.size());
        }
        /// <summary>
        /// 指定下标的子命令,越界返回 nullptr
        /// </summary>
        const   FEUndoCmd*  child(int index) const
        {
            if (index < 0 || index >= childCount())
                return  nullptr;
            return  _children[size_t(index)].get();
        }
        FEUndoCmd*          child(int index)
        {
            if (index < 0 || index >= childCount())
                return  nullptr;
            return  _children[size_t(index)].get();
        }

    protected:
        /// <summary>
        /// 把子命令挂到本命令下,本命令接管所有权(宏容器使用)
        /// </summary>
        void    addChild(FEUndoCmd* cmd)
        {
            if (cmd != nullptr)
                _children.emplace_back(cmd);
        }
        /// <summary>
        /// 摘除并释放指定子命令(空的嵌套宏使用)
        /// </summary>
        void    removeChild(FEUndoCmd* cmd)
        {
            auto    itr =   std::remove_if(_children.begin(),_children.end()
                                            ,[cmd](const std::unique_ptr<FEUndoCmd>& var)
            {
                return  var.get() == cmd;
            });
            _children.erase(itr,_children.end());
        }

    protected:
        /// <summary>
        /// 命令描述文本
        /// </summary>
        String      _text;
        /// <summary>
        /// 菜单项文本(为空时使用 _text)
        /// </summary>
        String      _actionText;
        /// <summary>
        /// 已失效标记
        /// </summary>
        bool        _obsolete   =   false;
        /// <summary>
        /// 子命令(宏命令);unique_ptr 保证析构时递归释放
        /// </summary>
        UndoCmds    _children;
    };
}
