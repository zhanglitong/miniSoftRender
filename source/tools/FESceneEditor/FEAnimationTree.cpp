#include    "FEAnimationTree.hpp"

#include    "../../../inc/animation/FEAnimationSys.hpp"

#include    <algorithm>
#include    <cctype>

namespace   FE
{
namespace
{
    /// <summary>
    /// 搜索选项图标:大小写匹配(字母 A)/全匹配(等号)
    /// </summary>
    const   char*   kGlyphCaseMatch    =   u8"\uf031";
    const   char*   kGlyphWholeMatch   =   u8"\uf52c";

    /// <summary>
    /// 绘制折叠三角
    /// </summary>
    /// <param name="drawList">绘制列表</param>
    /// <param name="x">三角左边界</param>
    /// <param name="centerY">行中心 y</param>
    /// <param name="collapsed">是否折叠(折叠时指向右)</param>
    /// <param name="color">颜色</param>
    void    drawTriangle(ImDrawList* drawList,float x,float centerY,bool collapsed,ImU32 color)
    {
        if (collapsed)
        {
            drawList->AddTriangleFilled(ImVec2(x,centerY - 4.0f),
                                        ImVec2(x,centerY + 4.0f),
                                        ImVec2(x + 5.0f,centerY),color);
        }
        else
        {
            drawList->AddTriangleFilled(ImVec2(x,centerY - 2.0f),
                                        ImVec2(x + 8.0f,centerY - 2.0f),
                                        ImVec2(x + 4.0f,centerY + 3.0f),color);
        }
    }
}

    void    FEAnimationTree::reset()
    {
        _anims.clear();
        _rows.clear();
        _collapsed.clear();
        _collapsedNodes.clear();
        _activeAnim     =   Animation();
        _searchBuf[0]   =   '\0';
        _searchQuery.clear();
    }

    void    FEAnimationTree::reload(FEAnimationSys* sys)
    {
        Animations  anims;
        if (sys != nullptr)
        {
            for (auto& var : sys->playbacks())
            {
                if (!var.second)
                    continue;
                for (const Animation& anim : var.second->objects())
                {
                    if (anim)
                        anims.push_back(anim);
                }
            }
        }
        _anims.swap(anims);
    }

    void    FEAnimationTree::rebuild()
    {
        _rows.clear();
        const   bool    searching   =   !_searchQuery.empty();

        /// <summary>
        /// 按所属节点分组,保持动画在播放器中首次出现的顺序
        /// </summary>
        Nodes                                       nodes;
        std::map<FENode*,Animations>                groups;
        for (auto& anim : _anims)
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
                Entry   entry;
                entry._anim     =   anim;
                entry._tracks   =   std::move(tracks);
                entries.push_back(std::move(entry));
            }
            if (searching && entries.empty())
                continue;

            Row nodeRow;
            nodeRow._type   =   RowType::Node;
            nodeRow._node   =   node;
            _rows.push_back(nodeRow);

            if (!searching && isNodeCollapsed(node))
                continue;

            for (auto& entry : entries)
            {
                Row animRow;
                animRow._type   =   RowType::Animation;
                animRow._node   =   node;
                animRow._anim   =   entry._anim;
                _rows.push_back(animRow);

                /// <summary>
                /// 搜索态下忽略折叠,保证命中的轨道可见
                /// </summary>
                if (!searching && isAnimCollapsed(entry._anim))
                    continue;
                for (auto& track : entry._tracks)
                {
                    Row row;
                    row._type   =   RowType::Track;
                    row._node   =   node;
                    row._anim   =   entry._anim;
                    row._track  =   track;
                    _rows.push_back(row);
                }
            }
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
    }

    bool    FEAnimationTree::isNodeCollapsed(const Node& node) const
    {
        if (!node)
            return  false;
        auto    itr =   _collapsedNodes.find(node.get());
        return  itr != _collapsedNodes.end() && itr->second;
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
        const   ImU32   triCol      =   IM_COL32(190,195,205,255);

        /// <summary>
        /// 名称列底色
        /// </summary>
        drawList->AddRectFilled(ImVec2(layout._left,layout._top),
                                ImVec2(layout._right,layout._bottom),colNameBg);

        /// <summary>
        /// 节点 / 动画 / 轨道名称与折叠三角
        /// </summary>
        drawList->PushClipRect(ImVec2(layout._left,layout._top),
                               ImVec2(layout._right,layout._bottom),true);
        for (int i = 0; i < int(_rows.size()); ++i)
        {
            const   float   y0  =   layout._rowTop0 + float(i) * layout._rowH;
            if (y0 + layout._rowH < layout._top || y0 > layout._bottom)
                continue;
            const   auto&   item    =   _rows[size_t(i)];
            const   float   centerY =   y0 + layout._rowH * 0.5f;
            switch (item._type)
            {
            case RowType::Node:
                drawTriangle(drawList,layout._left + 4.0f,centerY,isNodeCollapsed(item._node),triCol);
                drawList->AddText(ImVec2(layout._left + 16.0f,y0 + 3.0f),
                                  IM_COL32(255,195,110,255),
                                  item._node ? item._node->name() : "Node");
                break;
            case RowType::Animation:
                drawTriangle(drawList,layout._left + 16.0f,centerY,isAnimCollapsed(item._anim),triCol);
                drawList->AddText(ImVec2(layout._left + 28.0f,y0 + 3.0f),
                                  (item._anim == _activeAnim) ? IM_COL32(120,185,255,255)
                                                              : IM_COL32(215,220,230,255),
                                  item._anim ? item._anim->name().c_str() : "Animation");
                break;
            case RowType::Track:
            default:
                drawList->AddText(ImVec2(layout._left + 40.0f,y0 + 3.0f),
                                  IM_COL32(160,165,175,255),
                                  (item._track && !item._track->name().empty())
                                      ? item._track->name().c_str()
                                      : "Track");
                break;
            }
        }
        drawList->PopClipRect();
    }

    bool    FEAnimationTree::onClickNameColumn(int row)
    {
        if (row < 0 || row >= int(_rows.size()))
            return  false;
        const   auto&   item    =   _rows[size_t(row)];
        switch (item._type)
        {
        case RowType::Node:
            /// <summary>
            /// 节点行:切换节点折叠
            /// </summary>
            if (item._node)
            {
                bool&   collapsed   =   _collapsedNodes[item._node.get()];
                collapsed   =   !collapsed;
            }
            return  true;
        case RowType::Animation:
            /// <summary>
            /// 动画行:切换折叠并设为活动动画
            /// </summary>
            {
                bool&   collapsed   =   _collapsed[item._anim];
                collapsed   =   !collapsed;
                _activeAnim =   item._anim;
            }
            return  true;
        case RowType::Track:
        default:
            return  false;
        }
    }
}
