/// <summary>
/// FELogView
/// 输出日志面板:订阅 FELog 的写日志通知,把日志缓存到有界环形队列并在 ImGui 中实时显示。
/// 只负责内容绘制,窗口与标签页由 FEBottomDock 提供。
/// 日志可能来自渲染线程,缓存读写均加锁。
/// </summary>

#pragma     once

#include    "../../../inc/FELog.hpp"

#include    <deque>
#include    <mutex>
#include    <string>
#include    <vector>

#include    <imgui.h>

namespace   FE
{
    class   FEContext;

    class   FELogView
    {
    public:
        FELogView() = default;
        ~FELogView();
        FELogView(const FELogView&)              =   delete;
        FELogView& operator=(const FELogView&)   =   delete;
    public:
        /// <summary>
        /// 订阅日志通知;解绑时恢复原回调
        /// </summary>
        /// <param name="ctx">引擎上下文(取其中的 FELog)</param>
        void    bind(FEContext& ctx);
        /// <summary>
        /// 恢复原日志回调并丢弃缓存;可在场景销毁前安全调用
        /// </summary>
        void    unbind();
        /// <summary>
        /// 清空已缓存的日志
        /// </summary>
        void    clear();
        /// <summary>
        /// 每帧绘制标签页内容(工具栏 + 日志列表)
        /// </summary>
        void    drawContent();
    protected:
        /// <summary>
        /// 一条日志:时间、级别、来源模块与正文
        /// </summary>
        struct  Entry
        {
            std::string             _clock;
            FELog::LEVEL            _level  =   FELog::LEVEL_INFOR;
            std::string             _type;
            std::string             _module;
            std::string             _text;
        };
        /// <summary>
        /// 日志回调:追加一条记录(可跨线程调用)
        /// </summary>
        void    append(const char* module,const char* type,const char* text,const FEDateTime& dt);
        /// <summary>
        /// 级别过滤:0 表示全部,其余为最小级别
        /// </summary>
        bool    matches(FELog::LEVEL level) const;
        /// <summary>
        /// 设置级别过滤并重建可见列表(与写日志线程共用锁)
        /// </summary>
        void    setLevel(int index);
        /// <summary>
        /// 级别对应的文字颜色
        /// </summary>
        static  ImU32   colorOf(FELog::LEVEL level);
        /// <summary>
        /// 拼成一行显示文本
        /// </summary>
        static  std::string compose(const Entry& entry);
    protected:
        FELog*                          _log            =   nullptr;
        /// <summary>
        /// 绑定前的回调,解绑时恢复
        /// </summary>
        FELog::NotifyEvent              _prevNotify;
        mutable std::mutex              _mutex;
        /// <summary>
        /// 环形缓存(超出上限丢最旧)
        /// </summary>
        std::deque<Entry>               _entries;
        /// <summary>
        /// 当前级别过滤下的可见项;元素指向 _entries 中的记录
        /// </summary>
        std::vector<const Entry*>       _visible;
        size_t                          _maxEntries     =   4000;
        /// <summary>
        /// 级别下拉框索引:0 全部,1 调试 ... 6 异常
        /// </summary>
        int                             _levelIndex     =   0;
        bool                            _autoScroll     =   true;
        /// <summary>
        /// 清空/重建后强制滚到底部
        /// </summary>
        bool                            _scrollToBottom =   true;
    };
}
