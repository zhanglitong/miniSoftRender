#include    "FEAnimationTree.hpp"
#include    "FEUiStyle.hpp"

#include    "../../../inc/animation/FEAnimationSys.hpp"

#include    <algorithm>
#include    <cctype>

namespace   FE
{
    /// <summary>
    /// 行高/缩进/行底色与文字颜色统一取自 FEUiStyle(与模型树、时间线共用)
    /// </summary>
    using   namespace   ui;

namespace
{
    /// <summary>
    /// 搜索选项图标:大小写匹配(字母 A)/全匹配(等号)
    /// </summary>
    const   char*   kGlyphCaseMatch    =   u8"\uf031";
    const   char*   kGlyphWholeMatch   =   u8"\uf52c";

    /// <summary>
    /// 行缩进层级:播放器 0 / 节点 1 / 动画 2 / 轨道 3
    /// </summary>
    int depthOf(FEAnimItem::Type type)
    {
        switch (type)
        {
        case FEAnimItem::Type::Playback:    return  0;
        case FEAnimItem::Type::Node:        return  1;
        case FEAnimItem::Type::Animation:   return  2;
        case FEAnimItem::Type::Track:
        default:                            return  3;
        }
    }
}

    void    FEAnimationTree::reset()
    {
        _playbackEntries.clear();
        _anims.clear();
        _rows.clear();
        _collapsedPlaybacks.clear();
        _defaultEnsured =   false;
        _collapsed.clear();
        _collapsedNodes.clear();
        _activeAnim     =   Animation();
        _selection.clear();
        _selected       =   Object();
        _anchor         =   Object();
        _searchBuf[0]   =   '\0';
        _searchQuery.clear();
    }

    void    FEAnimationTree::reload(FEAnimationSys* sys)
    {
        PlaybackEntries entries;
        if (sys != nullptr)
        {
            /// <summary>
            /// 动画树默认有一个默认播放器:绑定场景后补建一次;
            /// 用户删除后不再自动重建,避免"删不掉"
            /// </summary>
            if (!_defaultEnsured)
            {
                sys->getOrCreate();
                _defaultEnsured =   true;
            }
            for (auto& var : sys->playbacks())
            {
                if (!var.second)
                    continue;
                PlaybackEntry   entry;
                entry._playback =   var.second;
                for (const Animation& anim : var.second->objects())
                {
                    if (anim)
                        entry._anims.push_back(anim);
                }
                entries.push_back(std::move(entry));
            }
        }
        _playbackEntries.swap(entries);

        /// <summary>
        /// 汇总各播放器的动画,供时间线计算范围与查找轨道
        /// </summary>
        Animations  anims;
        for (const PlaybackEntry& entry : _playbackEntries)
        {
            for (const Animation& anim : entry._anims)
                anims.push_back(anim);
        }
        _anims.swap(anims);
        /// <summary>
        /// 活动动画已不在动画系统中(如在模型树里被删除)时立即释放,
        /// 避免时间线继续显示已删除动画的轨道
        /// </summary>
        if (_activeAnim)
        {
            bool    found   =   false;
            for (const Animation& anim : _anims)
            {
                if (anim == _activeAnim)
                {
                    found   =   true;
                    break;
                }
            }
            if (!found)
                _activeAnim =   Animation();
        }
    }

    void    FEAnimationTree::rebuild()
    {
        _rows.clear();
        const   bool    searching   =   !_searchQuery.empty();

        for (const PlaybackEntry& entry : _playbackEntries)
        {
            if (!entry._playback)
                continue;

            /// <summary>
            /// 播放器行:搜索态下若其下没有任何命中内容,则整段隐藏
            /// </summary>
            const   size_t  startIndex  =   _rows.size();
            _rows.emplace_back(FEAnimItem(FEAnimItem::Type::Playback,entry._playback));

            if (!searching && isPlaybackCollapsed(entry._playback))
                continue;

            /// <summary>
            /// 播放器内的动画按所属节点分组,保持动画在播放器中首次出现的顺序
            /// </summary>
            Nodes                                       nodes;
            std::map<FENode*,Animations>                groups;
            for (auto& anim : entry._anims)
            {
                if (!anim || !anim->owner())
                    continue;
                FENode* node    =   anim->owner()->cast<FENode>();
                if (node == nullptr)
                    continue;
                auto    itr =   groups.find(node);
                if (itr == groups.end())
                {
                    nodes.emplace_back(Node(node));
                    groups[node]    =   Animations{ anim };
                }
                else
                {
                    itr->second.push_back(anim);
                }
            }

            for (auto& node : nodes)
            {
                if (!node)
                    continue;

                const   bool    nodeMatch   =   searching && nameMatches(node->name());

                /// <summary>
                /// 收集该节点下命中搜索条件的动画与轨道
                /// </summary>
                struct  Entry
                {
                    Animation       _anim;
                    KeyFrameTracks  _tracks;
                };
                std::vector<Entry>  entries;
                for (auto& anim : groups[node.get()])
                {
                    const   bool    animMatch   =   !searching || nodeMatch || nameMatches(anim->name());
                    KeyFrameTracks  tracks;
                    if (auto clip = anim->clip())
                    {
                        for (auto& track : clip->tracks())
                        {
                            if (track && (animMatch || nameMatches(track->name())))
                                tracks.push_back(track);
                        }
                    }
                    if (searching && !nodeMatch && tracks.empty())
                        continue;
                    Entry   entryItem;
                    entryItem._anim     =   anim;
                    entryItem._tracks   =   std::move(tracks);
                    entries.push_back(std::move(entryItem));
                }
                if (searching && entries.empty())
                    continue;

                _rows.emplace_back(FEAnimItem(FEAnimItem::Type::Node,node));

                if (!searching && isNodeCollapsed(node))
                    continue;

                for (auto& item : entries)
                {
                    _rows.emplace_back(FEAnimItem(FEAnimItem::Type::Animation,item._anim));

                    /// <summary>
                    /// 搜索态下忽略折叠,保证命中的轨道可见
                    /// </summary>
                    if (!searching && isAnimCollapsed(item._anim))
                        continue;
                    for (auto& track : item._tracks)
                        _rows.emplace_back(FEAnimItem(FEAnimItem::Type::Track,track));
                }
            }

            /// <summary>
            /// 搜索态下该播放器没有任何命中内容时,把播放器行也去掉
            /// </summary>
            if (searching && _rows.size() == startIndex + 1)
                _rows.resize(startIndex);
        }

        bool    found   =   false;
        for (auto& anim : _anims)
        {
            if (anim == _activeAnim)
            {
                found   =   true;
                break;
            }
        }
        if (!found)
            _activeAnim =   _anims.empty() ? Animation() : _anims.front();

        /// <summary>
        /// 重建行列表后剔除已不在行列表中的选中项(播放器/动画被删除、搜索过滤等)
        /// </summary>
        for (auto itr = _selection.begin(); itr != _selection.end(); )
        {
            if (rowIndexOf(*itr) < 0)
                itr =   _selection.erase(itr);
            else
                ++itr;
        }
        if (_selected && !isSelected(_selected))
            _selected   =   _selection.empty() ? Object() : _selection.back();
        if (_anchor && !isSelected(_anchor))
            _anchor     =   Object();
    }

    bool    FEAnimationTree::isNodeCollapsed(const Node& node) const
    {
        if (!node)
            return  false;
        auto    itr =   _collapsedNodes.find(node.get());
        return  itr != _collapsedNodes.end() && itr->second;
    }

    bool    FEAnimationTree::isPlaybackCollapsed(const Playback& playback) const
    {
        if (!playback)
            return  false;
        auto    itr =   _collapsedPlaybacks.find(playback.get());
        return  itr != _collapsedPlaybacks.end() && itr->second;
    }

    bool    FEAnimationTree::isAnimCollapsed(Animation anim) const
    {
        auto    itr =   _collapsed.find(anim);
        return  itr != _collapsed.end() && itr->second;
    }

    bool    FEAnimationTree::nameMatches(const std::string& name) const
    {
        if (_searchQuery.empty())
            return  true;

        std::string hay     =   name;
        std::string needle  =   _searchQuery;
        if (!_searchCase)
        {
            auto    lower   =   [](std::string& s)
            {
                for (auto& c : s)
                    c   =   char(std::tolower(static_cast<unsigned char>(c)));
            };
            lower(hay);
            lower(needle);
        }
        if (_searchWhole)
            return  hay == needle;
        return  hay.find(needle) != std::string::npos;
    }

    void    FEAnimationTree::drawSearchBar(const IconButtonFn& iconButton,float groupWidth)
    {
        const   ImGuiStyle& style       =   ImGui::GetStyle();

        /// <summary>
        /// 搜索栏:搜索框 + 大小写匹配 + 全匹配(图标开关),回车执行搜索
        /// </summary>
        const   float   iconW       =   26.0f;
        const   float   reserve     =   iconW * 2.0f
                                    +   ImGui::CalcTextSize("搜索").x
                                    +   style.ItemSpacing.x * 3.0f;
        float   searchW =   groupWidth - reserve;
        searchW =   std::max(60.0f,std::min(searchW,240.0f));
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("搜索");
        ImGui::SameLine();
        ImGui::PushItemWidth(searchW);
        if (ImGui::InputText("##tl_search",_searchBuf,sizeof(_searchBuf),ImGuiInputTextFlags_EnterReturnsTrue))
            _searchQuery.assign(_searchBuf);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (iconButton(kGlyphCaseMatch,"Aa","大小写匹配",_searchCase))
            _searchCase =   !_searchCase;
        ImGui::SameLine();
        if (iconButton(kGlyphWholeMatch,"全","全匹配",_searchWhole))
            _searchWhole    =   !_searchWhole;
    }

    void    FEAnimationTree::drawNames(const Layout& layout) const
    {
        ImDrawList* drawList    =   layout._drawList;
        if (drawList == nullptr)
            return;

        const   ImU32   colNameBg   =   IM_COL32(33,36,43,255);

        /// <summary>
        /// 名称列底色
        /// </summary>
        drawList->AddRectFilled(ImVec2(layout._left,layout._top),
                                ImVec2(layout._right,layout._bottom),colNameBg);

        /// <summary>
        /// 行高与缩进规则与模型树一致:图标尺寸取行高 60%,每级缩进 14px
        /// </summary>
        const   float   iconSize    =   layout._rowH * 0.6f;
        const   float   iconGap     =   iconSize * 0.2f;
        const   float   textLine    =   ImGui::GetTextLineHeight();

        drawList->PushClipRect(ImVec2(layout._left,layout._top),
                               ImVec2(layout._right,layout._bottom),true);
        for (int i = 0; i < int(_rows.size()); ++i)
        {
            const   float   y0  =   layout._rowTop0 + float(i) * layout._rowStep;
            if (y0 + layout._rowH < layout._top || y0 > layout._bottom)
                continue;
            const   auto&   item    =   _rows[size_t(i)];
            const   Object  object  =   item.object();
            const   bool    selected=   isSelected(object);
            const   bool    rowHover=   (i == layout._hoverRow);

            const   ImVec2  pos(layout._left,y0);
            const   ImVec2  end(layout._right,y0 + layout._rowH);
            /// <summary>
            /// 隔行 / 悬停 / 选中底色,与模型树保持一致
            /// </summary>
            if ((i & 1) != 0)
                drawList->AddRectFilled(pos,end,kRowAlt);
            if (selected)
            {
                drawList->AddRectFilled(pos,end,kRowSelected);
                drawList->AddRect(ImVec2(pos.x + 0.5f,pos.y + 0.5f),
                                  ImVec2(end.x - 0.5f,end.y - 0.5f),
                                  kRowSelBorder,0.0f,0,1.0f);
            }
            else if (rowHover)
                drawList->AddRectFilled(pos,end,kRowHover);

            const   int     depth   =   depthOf(item.type());
            const   float   arrowX  =   layout._left + 2.0f + kIndent * float(depth);
            const   float   textX   =   arrowX + iconSize + iconGap * 2.0f;
            const   float   textY   =   y0 + (layout._rowH - textLine) * 0.5f;

            bool            collapsed   =   false;
            const   char*   name        =   "";
            ImU32           textColor   =   kTextColor;
            switch (item.type())
            {
            case FEAnimItem::Type::Playback:
                {
                    const   Playback    playback    =   item.playback();
                    collapsed   =   isPlaybackCollapsed(playback);
                    name        =   (playback && !playback->name().empty())
                                    ? playback->name().c_str()
                                    : "Playback";
                    textColor   =   IM_COL32(150,205,140,255);
                }
                break;
            case FEAnimItem::Type::Node:
                {
                    const   Node    node    =   item.node();
                    collapsed   =   isNodeCollapsed(node);
                    const   char*   nodeName    =   node ? node->name() : nullptr;
                    name        =   (nodeName != nullptr && nodeName[0] != '\0') ? nodeName : "Node";
                    textColor   =   IM_COL32(255,195,110,255);
                }
                break;
            case FEAnimItem::Type::Animation:
                {
                    const   Animation   anim    =   item.anim();
                    collapsed   =   isAnimCollapsed(anim);
                    name        =   (anim && !anim->name().empty()) ? anim->name().c_str() : "Animation";
                    textColor   =   (anim == _activeAnim) ? IM_COL32(120,185,255,255) : kTextColor;
                }
                break;
            case FEAnimItem::Type::Track:
            default:
                {
                    const   KeyFrameTrack   track   =   item.track();
                    name        =   (track && !track->name().empty()) ? track->name().c_str() : "Track";
                    textColor   =   IM_COL32(160,165,175,255);
                }
                break;
            }
            /// <summary>
            /// 折叠三角(轨道行没有子行,不绘制),形状与模型树一致
            /// </summary>
            if (item.type() != FEAnimItem::Type::Track)
            {
                const   float   half    =   iconSize * 0.3f;
                const   ImVec2  center(arrowX + iconSize * 0.5f,y0 + layout._rowH * 0.5f);
                if (collapsed)
                {
                    drawList->AddTriangleFilled(ImVec2(center.x - half * 0.75f,center.y - half),
                                                ImVec2(center.x - half * 0.75f,center.y + half),
                                                ImVec2(center.x + half * 0.75f,center.y),kArrowColor);
                }
                else
                {
                    drawList->AddTriangleFilled(ImVec2(center.x - half,center.y - half * 0.75f),
                                                ImVec2(center.x + half,center.y - half * 0.75f),
                                                ImVec2(center.x,center.y + half * 0.75f),kArrowColor);
                }
            }

            /// <summary>
            /// 名称:选中行使用高亮前景色,超出名称列右边界时裁剪
            /// </summary>
            drawList->PushClipRect(ImVec2(textX,y0),ImVec2(layout._right,y0 + layout._rowH),true);
            drawList->AddText(ImVec2(textX,textY),selected ? kTextSelected : textColor,name);
            drawList->PopClipRect();
        }
        drawList->PopClipRect();
    }

    bool    FEAnimationTree::onClickNameColumn(int row)
    {
        if (row < 0 || row >= int(_rows.size()))
            return  false;
        const   auto&   item    =   _rows[size_t(row)];
        switch (item.type())
        {
        case FEAnimItem::Type::Playback:
            /// <summary>
            /// 播放器行:切换播放器折叠
            /// </summary>
            if (item.playback())
            {
                bool&   collapsed   =   _collapsedPlaybacks[item.playback().get()];
                collapsed   =   !collapsed;
            }
            return  true;
        case FEAnimItem::Type::Node:
            /// <summary>
            /// 节点行:切换节点折叠
            /// </summary>
            if (item.node())
            {
                bool&   collapsed   =   _collapsedNodes[item.node().get()];
                collapsed   =   !collapsed;
            }
            return  true;
        case FEAnimItem::Type::Animation:
            /// <summary>
            /// 动画行:切换折叠并设为活动动画
            /// </summary>
            {
                const   Animation   anim    =   item.anim();
                bool&   collapsed   =   _collapsed[anim];
                collapsed   =   !collapsed;
                _activeAnim =   anim;
            }
            return  true;
        case FEAnimItem::Type::Track:
        default:
            return  false;
        }
    }

    Animation   FEAnimationTree::rowAnim(int row) const
    {
        if (row < 0 || row >= int(_rows.size()))
            return  Animation();
        /// <summary>
        /// 轨道行紧跟在所属动画行之后,向上找到动画行即可;遇到节点/播放器行说明不在动画下
        /// </summary>
        for (int i = row; i >= 0; --i)
        {
            const   auto&   item    =   _rows[size_t(i)];
            switch (item.type())
            {
            case FEAnimItem::Type::Animation:
                return  item.anim();
            case FEAnimItem::Type::Track:
                continue;
            case FEAnimItem::Type::Node:
            case FEAnimItem::Type::Playback:
            default:
                return  Animation();
            }
        }
        return  Animation();
    }

    bool    FEAnimationTree::isSelected(const Object& object) const
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

    void    FEAnimationTree::selectSingle(const Object& object)
    {
        if (!object)
            return;
        clearSelection();
        _selection.push_back(object);
        _selected   =   object;
        _anchor     =   object;
    }

    void    FEAnimationTree::toggleSelect(const Object& object)
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
            _anchor =   object;
            if (_selected && _selected.get() == object.get())
                _selected   =   _selection.empty() ? Object() : _selection.back();
            return;
        }
        _selected   =   object;
        _anchor     =   object;
        _selection.push_back(object);
    }

    void    FEAnimationTree::selectRangeTo(const Object& object)
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
            addToSelection(_rows[size_t(i)].object());
        _selected   =   object;
    }

    void    FEAnimationTree::clearSelection()
    {
        _selection.clear();
    }

    void    FEAnimationTree::addToSelection(const Object& object)
    {
        if (!object || isSelected(object))
            return;
        _selection.push_back(object);
    }

    int     FEAnimationTree::rowIndexOf(const Object& object) const
    {
        if (!object)
            return  -1;
        for (size_t i = 0; i < _rows.size(); ++i)
        {
            if (_rows[i].object().get() == object.get())
                return  int(i);
        }
        return  -1;
    }
}
