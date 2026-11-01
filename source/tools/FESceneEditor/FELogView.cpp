#include    "FELogView.hpp"

#include    "FEUiStrings.hpp"
#include    "FEUiStyle.hpp"

#include    "../../../inc/FEContext.hpp"

namespace   FE
{
namespace
{
    /// <summary>
    /// 级别下拉框文字与宽度、正文行颜色(暗色主题)
    /// </summary>
    const   char*   kLevelNames[]   =
    {
        "全部","调试","事件","信息","警告","错误","异常"
    };
    constexpr   float   kLevelComboW    =   96.0f;
    constexpr   ImU32   kEventColor     =   IM_COL32(140,190,255,255);
    constexpr   ImU32   kWarningColor   =   IM_COL32(255,190,90,255);
    constexpr   ImU32   kErrorColor     =   IM_COL32(255,110,110,255);
    constexpr   ImU32   kExceptionColor =   IM_COL32(255,95,170,255);
    /// <summary>
    /// 计数文字颜色
    /// </summary>
    constexpr   ImU32   kCountColor     =   ui::kTextMuted;
}

    FELogView::~FELogView()
    {
        unbind();
    }

    void    FELogView::bind(FEContext& ctx)
    {
        unbind();
        FELog&  log =   ctx.log();
        _log        =   &log;
        _prevNotify =   log.notify();
        log.notify()=   [this](const char* module,const char* type,const char* text,const FEDateTime& dt)
        {
            append(module,type,text,dt);
        };
    }

    void    FELogView::unbind()
    {
        if (_log != nullptr)
        {
            _log->notify()  =   _prevNotify;
            _log            =   nullptr;
        }
        _prevNotify =   FELog::NotifyEvent();
        clear();
    }

    void    FELogView::clear()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _entries.clear();
        _visible.clear();
        _scrollToBottom =   true;
    }

    void    FELogView::append(const char* module,const char* type,const char* text,const FEDateTime& dt)
    {
        Entry   entry;
        char    clock[32]   =   {   0   };
        dt.toTimeString(clock);
        entry._clock    =   clock;
        entry._type     =   (type   != nullptr) ? type   : "";
        entry._level    =   FELog::enumFromName(entry._type.c_str());
        entry._module   =   (module != nullptr) ? module : "";
        entry._text     =   (text   != nullptr) ? text   : "";

        std::lock_guard<std::mutex> lock(_mutex);
        const   bool    visible =   matches(entry._level);
        _entries.push_back(std::move(entry));
        if (visible)
            _visible.push_back(&_entries.back());
        /// <summary>
        /// 超出上限时丢最旧;对应可见项同步移除(丢弃顺序与加入顺序一致)
        /// </summary>
        while (_entries.size() > _maxEntries)
        {
            if (matches(_entries.front()._level) && !_visible.empty())
                _visible.erase(_visible.begin());
            _entries.pop_front();
        }
    }

    bool    FELogView::matches(FELog::LEVEL level) const
    {
        if (_levelIndex <= 0)
            return  true;
        return  int(level) >= _levelIndex - 1;
    }

    void    FELogView::setLevel(int index)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_levelIndex == index)
            return;
        _levelIndex =   index;
        _visible.clear();
        _visible.reserve(_entries.size());
        for (const auto& entry : _entries)
        {
            if (matches(entry._level))
                _visible.push_back(&entry);
        }
        _scrollToBottom =   true;
    }

    ImU32   FELogView::colorOf(FELog::LEVEL level)
    {
        switch (level)
        {
        case FELog::LEVEL_DEBUG:        return  ui::kTextMuted;
        case FELog::LEVEL_EVENT:        return  kEventColor;
        case FELog::LEVEL_WARNING:      return  kWarningColor;
        case FELog::LEVEL_ERROR:        return  kErrorColor;
        case FELog::LEVEL_EXCEPTION:    return  kExceptionColor;
        case FELog::LEVEL_INFOR:
        default:                        return  ui::kTextColor;
        }
    }

    std::string FELogView::compose(const Entry& entry)
    {
        std::string line;
        line.reserve(entry._clock.size() + entry._type.size() + entry._module.size() + entry._text.size() + 8);
        line    +=  entry._clock;
        line    +=  "  ";
        line    +=  entry._type;
        line    +=  "  [";
        line    +=  entry._module;
        line    +=  "] ";
        line    +=  entry._text;
        return  line;
    }

    void    FELogView::drawContent()
    {
        /// <summary>
        /// 工具栏:级别过滤 / 自动滚动 / 清空 / 总条数
        /// </summary>
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("级别");
        ImGui::SameLine();
        ImGui::PushItemWidth(kLevelComboW);
        int levelIndex  =   _levelIndex;
        if (ImGui::Combo("##log_level",&levelIndex,kLevelNames,IM_ARRAYSIZE(kLevelNames)))
            setLevel(levelIndex);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Checkbox("自动滚动",&_autoScroll);
        ImGui::SameLine();
        if (ImGui::Button(ui::text::kClear))
            clear();
        /// <summary>
        /// 复制可见项指针后在锁外绘制,避免长时间阻塞写日志线程
        /// </summary>
        std::vector<const Entry*>   rows;
        size_t                      total   =   0;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            rows    =   _visible;
            total   =   _entries.size();
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text,kCountColor);
        ImGui::Text("共 %zu 条",total);
        ImGui::PopStyleColor();
        ImGui::Separator();
        ImGui::BeginChild("##log_body",ImVec2(0.0f,0.0f),false,ImGuiWindowFlags_HorizontalScrollbar);
        const   bool    atBottom    =   ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
        if (!rows.empty())
        {
            ImGuiListClipper    clipper(int(rows.size()),ImGui::GetTextLineHeightWithSpacing());
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart;i < clipper.DisplayEnd;++i)
                {
                    const   Entry&  entry   =   *rows[size_t(i)];
                    ImGui::PushStyleColor(ImGuiCol_Text,colorOf(entry._level));
                    ImGui::TextUnformatted(compose(entry).c_str());
                    ImGui::PopStyleColor();
                }
            }
        }
        /// <summary>
        /// 自动滚动开启时跟随新日志;用户向上滚动后保持当前位置。
        /// 清空/切换过滤强制回到底部
        /// </summary>
        const   bool    follow  =   _autoScroll ? (atBottom || _scrollToBottom) : _scrollToBottom;
        if (!rows.empty() && follow)
            ImGui::SetScrollHere(1.0f);
        _scrollToBottom =   false;
        ImGui::EndChild();
    }
}
