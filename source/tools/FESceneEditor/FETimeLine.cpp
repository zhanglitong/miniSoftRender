#include    "FETimeLine.hpp"

#include    "FEModelTree.hpp"

#include    "../../../inc/animation/FEAnimationHelper.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../../inc/animation/FEValueArrayObjects.hpp"
#include    "../../../inc/graphic/FEScene.h"

#include    <algorithm>
#include    <cmath>
#include    <cstdint>
#include    <cstdio>
#include    <functional>
#include    <type_traits>
#include    <utility>
#include    <variant>

namespace   FE
{
namespace
{
    /// <summary>
    /// 命令执行/撤销后刷新场景的回调
    /// </summary>
    using   RefreshFn   =   std::function<void()>;

    /// <summary>
    /// 一条轨道在某个时刻的完整数据(用于撤销重做)
    /// </summary>
    struct  TrackState
    {
        KeyFrameTrack           _track;
        std::vector<real>       _times;
        std::vector<KFValue>    _values;
    };

    /// <summary>
    /// 一次轨道编辑:修改前后的状态,以及需要重建轨道时的 clip
    /// </summary>
    struct  TrackEdit
    {
        TrackState      _old;
        TrackState      _new;
        AnimClip        _clip;
        bool            _recreated  =   false;
    };

    /// <summary>
    /// 读取 value 数组的元素个数
    /// </summary>
    size_t  valueCount(const ValueObject& obj)
    {
        return  std::visit([](auto&& ptr) -> size_t
        {
            using   P   =   std::decay_t<decltype(ptr)>;
            if constexpr (std::is_same_v<P,std::monostate>)
                return  0;
            else
                return  ptr ? ptr->values().size() : 0;
        },obj);
    }
    /// <summary>
    /// 读取 value 数组指定下标的值,类型不匹配时返回空值
    /// </summary>
    KFValue readValue(const ValueObject& obj,size_t index)
    {
        return  std::visit([&](auto&& ptr) -> KFValue
        {
            using   P   =   std::decay_t<decltype(ptr)>;
            if constexpr (std::is_same_v<P,std::monostate>)
                return  KFValue{};
            else
            {
                if (!ptr)
                    return  KFValue{};
                auto&   arr =   ptr->values();
                if (index >= arr.size())
                    return  KFValue{};
                return  KFValue{arr[index]};
            }
        },obj);
    }
    /// <summary>
    /// 把 value 写入数组指定下标,类型不匹配返回 false
    /// </summary>
    bool    writeValue(ValueObject& obj,size_t index,const KFValue& value)
    {
        return  std::visit([&](auto&& ptr) -> bool
        {
            using   P   =   std::decay_t<decltype(ptr)>;
            if constexpr (std::is_same_v<P,std::monostate>)
                return  false;
            else
            {
                if (!ptr)
                    return  false;
                auto&   arr =   ptr->values();
                if (index >= arr.size())
                    return  false;
                using   Elem    =   std::decay_t<decltype(arr[0])>;
                if (auto* pv =  std::get_if<Elem>(&value))
                {
                    arr[index]  =   *pv;
                    return  true;
                }
                return  false;
            }
        },obj);
    }
    /// <summary>
    /// 调整 value 数组长度
    /// </summary>
    void    resizeValues(ValueObject& obj,size_t size)
    {
        std::visit([&](auto&& ptr)
        {
            using   P   =   std::decay_t<decltype(ptr)>;
            if constexpr (!std::is_same_v<P,std::monostate>)
            {
                if (ptr)
                    ptr->values().resize(size);
            }
        },obj);
    }
    /// <summary>
    /// 记录轨道的完整状态
    /// </summary>
    TrackState  snapshotTrack(KeyFrameTrack track)
    {
        TrackState  state;
        state._track    =   track;
        if (!track || !track->_times)
            return  state;
        state._times    =   track->_times->values();
        for (size_t i = 0; i < state._times.size(); ++i)
            state._values.push_back(readValue(track->values(),i));
        return  state;
    }
    /// <summary>
    /// 把完整状态写回轨道
    /// </summary>
    void    applyTrackState(const TrackState& state)
    {
        if (!state._track)
            return;
        /// <summary>
        /// const 的 FERef 解引用得到 const 指针,这里取一份可变引用再写回
        /// </summary>
        KeyFrameTrack  track   =   state._track;
        if (!track->_times)
            track->setTimeObject(RealsObject(new FERealsObject(track->ctx())));
        track->_times->values()  =   state._times;
        ValueObject&    values          =   track->values();
        resizeValues(values,state._times.size());
        for (size_t i = 0; i < state._values.size() && i < state._times.size(); ++i)
            writeValue(values,i,state._values[i]);
        track->flags().addFlag(FEKeyFrameTrack::TrackChanged);
    }
    /// <summary>
    /// 设置一组轨道的状态(关键帧移动/删除/粘贴共用)
    /// </summary>
    class   SetTrackStatesCmd :public FEUndoCmd
    {
    public:
        SetTrackStatesCmd(std::vector<TrackEdit> edits,const String& text,RefreshFn refresh)
            :FEUndoCmd(text)
            ,_edits(std::move(edits))
            ,_refresh(std::move(refresh))
        {}
    public:
        virtual void    redo()  override
        {
            for (auto& edit : _edits)
            {
                /// 由空 clip 新建的轨道被 undo 移除后,redo 需要重新挂回 clip
                if (edit._recreated && edit._clip && edit._new._track)
                    edit._clip->addTrack(edit._new._track);
                applyTrackState(edit._new);
            }
            if (_refresh)
                _refresh();
        }
        virtual void    undo()  override
        {
            for (auto& edit : _edits)
            {
                applyTrackState(edit._old);
                if (edit._recreated && edit._clip)
                    edit._clip->clearObjects();
            }
            if (_refresh)
                _refresh();
        }
    protected:
        std::vector<TrackEdit>  _edits;
        RefreshFn               _refresh;
    };

    /// <summary>
    /// 动画 offset 变更命令
    /// </summary>
    struct  AnimOffset
    {
        Animation   _anim;
        real        _old    =   0;
        real        _new    =   0;
    };
    class   SetOffsetCmd :public FEUndoCmd
    {
    public:
        using   Offsets =   std::vector<AnimOffset>;
    public:
        SetOffsetCmd(Offsets offsets,const String& text,RefreshFn refresh)
            :FEUndoCmd(text)
            ,_offsets(std::move(offsets))
            ,_refresh(std::move(refresh))
        {}
    public:
        virtual void    redo()  override
        {
            for (auto& item : _offsets)
            {
                if (item._anim)
                    item._anim->setOffset(item._new);
            }
            if (_refresh)
                _refresh();
        }
        virtual void    undo()  override
        {
            for (auto& item : _offsets)
            {
                if (item._anim)
                    item._anim->setOffset(item._old);
            }
            if (_refresh)
                _refresh();
        }
    protected:
        Offsets     _offsets;
        RefreshFn   _refresh;
    };

    /// <summary>
    /// 轨道插值方式变更命令
    /// </summary>
    struct  InterpEdit
    {
        KeyFrameTrack   _track;
        uint8           _old    =   0;
        uint8           _new    =   0;
    };
    class   SetInterpCmd :public FEUndoCmd
    {
    public:
        SetInterpCmd(std::vector<InterpEdit> edits,const String& text,RefreshFn refresh)
            :FEUndoCmd(text)
            ,_edits(std::move(edits))
            ,_refresh(std::move(refresh))
        {}
    public:
        virtual void    redo()  override
        {
            for (auto& edit : _edits)
            {
                if (edit._track)
                    edit._track->_type = (FEKeyFrameTrack::InterpolateType)edit._new;
            }
            if (_refresh)
                _refresh();
        }
        virtual void    undo()  override
        {
            for (auto& edit : _edits)
            {
                if (edit._track)
                    edit._track->_type = (FEKeyFrameTrack::InterpolateType)edit._old;
            }
            if (_refresh)
                _refresh();
        }
    protected:
        std::vector<InterpEdit>     _edits;
        RefreshFn                   _refresh;
    };

    /// <summary>
    /// 创建动画组件命令
    /// </summary>
    class   CreateAnimCmd :public FEUndoCmd
    {
    public:
        CreateAnimCmd(FEScene& scene,Node node,Animation anim,RefreshFn refresh)
            :FEUndoCmd("创建动画")
            ,_scene(&scene)
            ,_node(node)
            ,_anim(anim)
            ,_refresh(std::move(refresh))
        {}
    public:
        virtual void    redo()  override
        {
            if (!_node || !_anim)
                return;
            _node->addComponent(_anim.get());
            if (_scene != nullptr && _scene->animationSystem() != nullptr)
                _scene->animationSystem()->addObject(_anim.get());
            if (_refresh)
                _refresh();
        }
        virtual void    undo()  override
        {
            if (!_node || !_anim)
                return;
            if (_scene != nullptr && _scene->animationSystem() != nullptr)
                _scene->animationSystem()->removeObject(_anim.get());
            _node->removeComponent(_anim.get());
            if (_refresh)
                _refresh();
        }
    protected:
        FEScene*    _scene  =   nullptr;
        Node        _node;
        Animation   _anim;
        RefreshFn   _refresh;
    };
}

    void    FETimeLine::bind(FEScene* scene,FEUndoStack* undoStack,FEModelTree* modelTree)
    {
        _scene      =   scene;
        _undoStack  =   undoStack;
        _modelTree  =   modelTree;
        reset();
    }

    void    FETimeLine::unbind()
    {
        reset();
        _scene      =   nullptr;
        _undoStack  =   nullptr;
        _modelTree  =   nullptr;
        _open       =   false;
    }

    void    FETimeLine::reset()
    {
        _node           =   nullptr;
        _anims.clear();
        _rows.clear();
        _collapsed.clear();
        _activeAnim     =   nullptr;
        _selKeys.clear();
        _clipboard.clear();
        _clipBaseTime   =   0;
        _dragTracks.clear();
        _dragAnims.clear();
        _dragOffsets.clear();
        _dragMode       =   DM_None;
        _panning        =   false;
        _playing        =   false;
        _curTime        =   0;
        _viewLeftFrame  =   0;
        _scrollY        =   0;
        _status.clear();
    }

    void    FETimeLine::toggle()
    {
        setOpen(!_open);
    }

    void    FETimeLine::setOpen(bool open)
    {
        if (_open == open)
            return;
        _open   =   open;
        if (_open)
            refreshScene();
    }

    void    FETimeLine::setFps(float fps)
    {
        if (fps < 1.0f)
            fps =   1.0f;
        if (fps > 240.0f)
            fps =   240.0f;
        _fps    =   fps;
    }

    void    FETimeLine::setCurFrame(int frame,bool applyToAnim)
    {
        if (frame < 0)
            frame   =   0;
        _curTime    =   real(frame) / real(_fps);
        if (applyToAnim)
            applyTime();
    }

    int     FETimeLine::curFrame() const
    {
        return  int(std::lround(double(_curTime) * double(_fps)));
    }

    void    FETimeLine::ensureNode()
    {
        FENode* node    =   nullptr;
        if (_modelTree != nullptr)
        {
            Object  selected    =   _modelTree->selected();
            if (selected)
                node            =   selected->cast<FENode>();
        }
        if (node != _node.get())
        {
            _node           =   Node(node);
            _anims.clear();
            _rows.clear();
            _collapsed.clear();
            _selKeys.clear();
            _activeAnim     =   nullptr;
            _dragTracks.clear();
            _dragAnims.clear();
            _dragOffsets.clear();
            _dragMode       =   DM_None;
            _panning        =   false;
            _scrollY        =   0.0f;
            _viewLeftFrame  =   0.0;
            _status.clear();
        }
    }

    void    FETimeLine::reloadAnimations()
    {
        std::vector<Animation>  anims;
        if (_node)
        {
            for (auto* ptr : _node->objects<FEAnimation>())
            {
                if (ptr != nullptr)
                    anims.emplace_back(Animation(ptr));
            }
        }
        _anims.swap(anims);
    }

    void    FETimeLine::rebuildRows()
    {
        _rows.clear();
        for (auto& anim : _anims)
        {
            if (!anim)
                continue;
            Row header;
            header._anim    =   anim;
            header._header  =   true;
            _rows.push_back(header);

            auto    itr =   _collapsed.find(anim);
            if (itr != _collapsed.end() && itr->second)
                continue;
            auto    clip    =   anim->clip();
            if (!clip)
                continue;
            for (auto& track : clip->tracks())
            {
                if (!track)
                    continue;
                Row row;
                row._anim   =   anim;
                row._track  =   track;
                row._header =   false;
                _rows.push_back(row);
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

    void    FETimeLine::contentRange(double& firstFrame,double& lastFrame) const
    {
        bool    any =   false;
        double  lo  =   0.0;
        double  hi  =   150.0;
        for (auto& anim : _anims)
        {
            if (!anim || !anim->clip())
                continue;
            auto    rng =   anim->range();
            if (rng.y < rng.x)
                continue;
            const   double  f0  =   double(rng.x) * double(_fps);
            const   double  f1  =   double(rng.y) * double(_fps);
            if (!any)
            {
                lo  =   f0;
                hi  =   f1;
                any =   true;
            }
            else
            {
                lo  =   std::min(lo,f0);
                hi  =   std::max(hi,f1);
            }
        }
        if (!any)
        {
            lo  =   0.0;
            hi  =   150.0;
        }
        firstFrame  =   std::min(lo,0.0);
        lastFrame   =   std::max(hi,150.0);
    }

    float   FETimeLine::xForFrame(double frame) const
    {
        return  _timelineX + float((frame - _viewLeftFrame) * double(_framePixel));
    }

    double  FETimeLine::frameForX(float x) const
    {
        return  _viewLeftFrame + (double(x) - double(_timelineX)) / double(_framePixel);
    }

    float   FETimeLine::rowTop(int index) const
    {
        return  _canvasY + _rulerH + float(index) * _rowH - _scrollY;
    }

    int     FETimeLine::rowAtY(float y) const
    {
        if (y < _canvasY + _rulerH || y > _canvasY + _canvasH)
            return  -1;
        const   float   offset  =   y - (_canvasY + _rulerH) + _scrollY;
        if (offset < 0.0f)
            return  -1;
        const   int     index   =   int(offset / _rowH);
        if (index < 0 || index >= int(_rows.size()))
            return  -1;
        return  index;
    }

    int     FETimeLine::keyAt(int row,float x,float y) const
    {
        if (row < 0 || row >= int(_rows.size()))
            return  -1;
        const   auto&   item    =   _rows[size_t(row)];
        if (item._header || !item._track || !item._track->_times)
            return  -1;
        const   float   centerY =   rowTop(row) + _rowH * 0.5f;
        if (std::fabs(y - centerY) > _rowH * 0.5f)
            return  -1;

        const   real    offset  =   item._anim ? item._anim->offset() : 0;
        const   auto&   times   =   item._track->_times->values();
        const   float   radius  =   6.0f;
        for (size_t i = 0; i < times.size(); ++i)
        {
            const   float   cx  =   xForFrame(double(times[i] + offset) * double(_fps));
            if (std::fabs(x - cx) <= radius)
                return  int(i);
        }
        return  -1;
    }

    Animation   FETimeLine::animationOf(KeyFrameTrack track) const
    {
        if (!track)
            return  nullptr;
        for (auto& anim : _anims)
        {
            if (!anim || !anim->clip())
                continue;
            for (auto& item : anim->clip()->tracks())
            {
                if (item == track)
                    return  anim;
            }
        }
        return  nullptr;
    }

    bool    FETimeLine::isKeySelected(KeyFrameTrack track,size_t index) const
    {
        auto    itr =   _selKeys.find(track);
        if (itr == _selKeys.end())
            return  false;
        return  itr->second.find(index) != itr->second.end();
    }

    void    FETimeLine::addKeySelection(KeyFrameTrack track,size_t index)
    {
        if (!track)
            return;
        _selKeys[track].insert(index);
    }

    void    FETimeLine::removeKeySelection(KeyFrameTrack track,size_t index)
    {
        auto    itr =   _selKeys.find(track);
        if (itr == _selKeys.end())
            return;
        itr->second.erase(index);
        if (itr->second.empty())
            _selKeys.erase(itr);
    }

    void    FETimeLine::clearSelection()
    {
        _selKeys.clear();
    }

    bool    FETimeLine::allKeysSelected(Animation anim) const
    {
        if (!anim || !anim->clip())
            return  false;
        size_t  total   =   0;
        size_t  selected=   0;
        for (auto& track : anim->clip()->tracks())
        {
            if (!track || !track->_times)
                continue;
            total   +=  track->_times->values().size();
            auto    itr =   _selKeys.find(track);
            if (itr != _selKeys.end())
                selected    +=  itr->second.size();
        }
        return  total > 0 && total == selected;
    }

    void    FETimeLine::collectKeyFrames(std::vector<double>& frames) const
    {
        for (auto& anim : _anims)
        {
            if (!anim || !anim->clip())
                continue;
            const   real    offset  =   anim->offset();
            for (auto& track : anim->clip()->tracks())
            {
                if (!track || !track->_times)
                    continue;
                for (auto& tm : track->_times->values())
                    frames.push_back(double(tm + offset) * double(_fps));
            }
        }
        std::sort(frames.begin(),frames.end());
        frames.erase(std::unique(frames.begin(),frames.end()),frames.end());
    }

    void    FETimeLine::gotoKeyframe(int mode)
    {
        std::vector<double> frames;
        collectKeyFrames(frames);
        if (frames.empty())
            return;

        const   double  cur =   double(_curTime) * double(_fps);
        double          target  =   cur;
        if (mode == 0)
            target  =   frames.front();
        else if (mode == 3)
            target  =   frames.back();
        else if (mode == 1)
        {
            auto    itr =   std::lower_bound(frames.begin(),frames.end(),cur - 0.001);
            if (itr != frames.begin())
                target  =   *(itr - 1);
            else
                target  =   frames.front();
        }
        else if (mode == 2)
        {
            auto    itr =   std::upper_bound(frames.begin(),frames.end(),cur + 0.001);
            target  =   (itr != frames.end()) ? *itr : frames.back();
        }
        setCurFrame(int(std::lround(target)));
    }

    void    FETimeLine::applyTime()
    {
        if (_scene == nullptr)
            return;
        FEAnimationSys* system  =   _scene->animationSystem();
        if (system == nullptr)
            return;
        system->setClipTime(_curTime);
        for (auto& anim : _anims)
        {
            if (anim)
                anim->applyTransform();
        }
    }

    void    FETimeLine::refreshScene()
    {
        if (_scene == nullptr)
            return;
        FEAnimationSys* system  =   _scene->animationSystem();
        if (system != nullptr)
        {
            system->refreshRange();
            system->setClipTime(_curTime);
        }
        for (auto& anim : _anims)
        {
            if (anim)
                anim->applyTransform();
        }
    }

    void    FETimeLine::draw(ImFont* /*iconFont*/)
    {
        if (!_open || _scene == nullptr)
            return;

        ensureNode();
        reloadAnimations();
        rebuildRows();

        ImGuiIO&    io          =   ImGui::GetIO();
        const   float   displayW    =   io.DisplaySize.x;
        const   float   displayH    =   io.DisplaySize.y;
        const   float   panelH      =   std::max(190.0f,std::min(displayH * 0.34f,340.0f));

        ImGui::SetNextWindowPos(ImVec2(0.0f,displayH - panelH),ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(displayW,panelH),ImGuiCond_Always);
        const   ImGuiWindowFlags    flags   =   ImGuiWindowFlags_NoTitleBar
                                            |   ImGuiWindowFlags_NoResize
                                            |   ImGuiWindowFlags_NoMove
                                            |   ImGuiWindowFlags_NoSavedSettings
                                            |   ImGuiWindowFlags_NoScrollbar
                                            |   ImGuiWindowFlags_NoScrollWithMouse
                                            |   ImGuiWindowFlags_NoCollapse;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(6.0f,4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,0.0f);
        ImGui::Begin("##FETimeLine",nullptr,flags);
        ImGui::PopStyleVar(2);

        /// <summary>
        /// 播放:由时间线自身推进时间,再让动画系统按该时间采样
        /// </summary>
        if (_playing)
        {
            double  firstFrame  =   0.0;
            double  lastFrame   =   0.0;
            contentRange(firstFrame,lastFrame);
            const   double  lastTime    =   lastFrame / double(_fps);
            _curTime    +=  real(io.DeltaTime);
            if (_curTime > lastTime)
            {
                if (_loop)
                    _curTime    =   0;
                else
                {
                    _curTime    =   real(lastTime);
                    _playing    =   false;
                }
            }
            applyTime();
        }

        drawToolbar();
        ImGui::Separator();

        /// <summary>
        /// 画布区域:整块不可见按钮承载鼠标交互
        /// </summary>
        const   ImVec2  canvasPos   =   ImGui::GetCursorScreenPos();
        const   ImVec2  avail       =   ImGui::GetContentRegionAvail();
        const   float   canvasW     =   std::max(avail.x,64.0f);
        const   float   canvasH     =   std::max(avail.y,48.0f);
        ImGui::InvisibleButton("##tl_canvas",ImVec2(canvasW,canvasH));
        const   bool    hovered     =   ImGui::IsItemHovered();

        _canvasX    =   canvasPos.x;
        _canvasY    =   canvasPos.y;
        _canvasW    =   canvasW;
        _canvasH    =   canvasH;
        _rulerH     =   ImGui::GetTextLineHeight() + 8.0f;
        _rowH       =   std::max(20.0f,ImGui::GetTextLineHeight() + 8.0f);
        _nameW      =   std::max(110.0f,std::min(canvasW * 0.22f,210.0f));
        _timelineX  =   _canvasX + _nameW;

        handleInput(hovered);
        drawCanvas(hovered);
        handleContextMenu();

        ImGui::End();
    }

    void    FETimeLine::drawToolbar()
    {
        if (ImGui::Button(_playing ? "暂停" : "播放"))
            _playing    =   !_playing;
        ImGui::SameLine();
        if (ImGui::Button("首帧"))
            gotoKeyframe(0);
        ImGui::SameLine();
        if (ImGui::Button("上一帧"))
            gotoKeyframe(1);
        ImGui::SameLine();
        if (ImGui::Button("下一帧"))
            gotoKeyframe(2);
        ImGui::SameLine();
        if (ImGui::Button("末帧"))
            gotoKeyframe(3);

        ImGui::SameLine();
        ImGui::Checkbox("循环",&_loop);
        ImGui::SameLine();
        ImGui::Text("帧 %d   时间 %.2f s",curFrame(),double(_curTime));

        ImGui::SameLine();
        if (ImGui::Button("添加关键帧"))
            addKeyframe();
        ImGui::SameLine();
        if (ImGui::Button("创建动画"))
            createAnimation();

        if (!_status.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.95f,0.75f,0.35f,1.0f),"%s",_status.c_str());
        }

        /// <summary>
        /// 关闭按钮右对齐
        /// </summary>
        const   ImVec2  labelSize   =   ImGui::CalcTextSize("关闭");
        const   float   closeW      =   labelSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
        const   float   maxX        =   ImGui::GetWindowContentRegionMax().x;
        const   float   closeX      =   maxX - closeW;
        if (closeX > ImGui::GetCursorPosX())
            ImGui::SetCursorPosX(closeX);
        ImGui::SameLine();
        if (ImGui::Button("关闭",ImVec2(closeW,0.0f)))
            _open   =   false;
    }

    void    FETimeLine::drawCanvas(bool hovered)
    {
        ImDrawList* drawList    =   ImGui::GetWindowDrawList();
        const   ImU32   colBg       =   IM_COL32(38,41,48,255);
        const   ImU32   colRuler    =   IM_COL32(58,62,72,255);
        const   ImU32   colNameBg   =   IM_COL32(33,36,43,255);
        const   ImU32   colRowA     =   IM_COL32(44,47,55,255);
        const   ImU32   colRowB     =   IM_COL32(40,43,50,255);
        const   ImU32   colLine     =   IM_COL32(76,81,92,255);
        const   ImU32   colOut      =   IM_COL32(27,29,34,255);
        const   ImU32   colHover    =   IM_COL32(58,64,78,255);

        const   ImVec2  topLeft(_canvasX,_canvasY);
        const   ImVec2  bottomRight(_canvasX + _canvasW,_canvasY + _canvasH);
        const   float   rowsTop =   _canvasY + _rulerH;

        drawList->AddRectFilled(topLeft,bottomRight,colBg);

        double  firstFrame  =   0.0;
        double  lastFrame   =   0.0;
        contentRange(firstFrame,lastFrame);

        /// <summary>
        /// 行背景
        /// </summary>
        const   ImVec2  mouse   =   ImGui::GetMousePos();
        const   int     hoverRow=   hovered ? rowAtY(mouse.y) : -1;
        for (int i = 0; i < int(_rows.size()); ++i)
        {
            const   float   y0  =   std::max(rowTop(i),rowsTop);
            const   float   y1  =   std::min(rowTop(i) + _rowH,bottomRight.y);
            if (y1 <= y0)
                continue;
            ImU32   bg  =   (i % 2 == 0) ? colRowA : colRowB;
            if (i == hoverRow)
                bg  =   colHover;
            drawList->AddRectFilled(ImVec2(_timelineX,y0),ImVec2(bottomRight.x,y1),bg);
            drawList->AddLine(ImVec2(_timelineX,y0),ImVec2(bottomRight.x,y0),IM_COL32(52,55,63,255));
        }
        /// <summary>
        /// 名称列背景
        /// </summary>
        drawList->AddRectFilled(ImVec2(topLeft.x,rowsTop),ImVec2(_timelineX,bottomRight.y),colNameBg);

        /// <summary>
        /// 超出动画时间范围的部分压暗
        /// </summary>
        const   float   xFirst  =   xForFrame(firstFrame);
        const   float   xLast   =   xForFrame(lastFrame);
        if (xFirst > _timelineX)
            drawList->AddRectFilled(ImVec2(_timelineX,rowsTop),ImVec2(std::min(xFirst,bottomRight.x),bottomRight.y),colOut);
        if (xLast < bottomRight.x)
            drawList->AddRectFilled(ImVec2(std::max(xLast,_timelineX),rowsTop),bottomRight, colOut);

        /// <summary>
        /// 轨道内容:范围块 + 关键帧
        /// </summary>
        drawList->PushClipRect(ImVec2(_timelineX,rowsTop),bottomRight,true);
        for (int i = 0; i < int(_rows.size()); ++i)
        {
            const   float   y0  =   rowTop(i);
            if (y0 + _rowH < rowsTop || y0 > bottomRight.y)
                continue;
            const   auto&   item    =   _rows[size_t(i)];
            const   auto&   anim    =   item._anim;
            const   real    offset  =   anim ? anim->offset() : 0;

            if (item._header)
            {
                if (!anim || !anim->clip())
                    continue;
                const   auto    rng =   anim->range();
                const   float   x0  =   xForFrame(double(rng.x) * double(_fps));
                const   float   x1  =   xForFrame(double(rng.y) * double(_fps));
                if (x1 > x0)
                {
                    const   ImVec2  p0(std::max(x0,_timelineX),y0 + 4.0f);
                    const   ImVec2  p1(std::min(x1,bottomRight.x),y0 + _rowH - 4.0f);
                    if (p1.x > p0.x && p1.y > p0.y)
                    {
                        const   bool    active  =   (anim == _activeAnim);
                        drawList->AddRectFilled(p0,p1,active ? IM_COL32(0,120,215,90) : IM_COL32(128,128,128,90),3.0f);
                        drawList->AddRect(p0,p1,active ? IM_COL32(70,165,255,220) : IM_COL32(150,152,160,200),3.0f);
                    }
                }

                /// <summary>
                /// 动画头行:所有轨道的关键帧用小块汇总显示
                /// </summary>
                const   float   centerY =   y0 + _rowH * 0.5f;
                for (auto& track : anim->clip()->tracks())
                {
                    if (!track || !track->_times)
                        continue;
                    for (auto& tm : track->_times->values())
                    {
                        const   float   cx  =   xForFrame(double(tm + offset) * double(_fps));
                        if (cx < _timelineX || cx > bottomRight.x)
                            continue;
                        drawList->AddCircleFilled(ImVec2(cx,centerY),3.0f,IM_COL32(120,125,135,220));
                    }
                }
            }
            else
            {
                const   auto&   track   =   item._track;
                if (!track || !track->_times)
                    continue;
                const   float   centerY =   y0 + _rowH * 0.5f;
                const   auto&   times   =   track->_times->values();
                for (size_t k = 0; k < times.size(); ++k)
                {
                    const   float   cx  =   xForFrame(double(times[k] + offset) * double(_fps));
                    if (cx < _timelineX || cx > bottomRight.x)
                        continue;
                    if (isKeySelected(track,k))
                    {
                        drawList->AddCircleFilled(ImVec2(cx,centerY),5.0f,IM_COL32(0,150,220,220));
                        drawList->AddCircle(ImVec2(cx,centerY),5.0f,IM_COL32(255,255,255,230));
                    }
                    else
                    {
                        drawList->AddCircleFilled(ImVec2(cx,centerY),5.0f,IM_COL32(95,100,110,255));
                        drawList->AddCircle(ImVec2(cx,centerY),5.0f,IM_COL32(155,160,170,255));
                    }
                }
            }
        }
        drawList->PopClipRect();

        /// <summary>
        /// 名称列文本
        /// </summary>
        drawList->PushClipRect(ImVec2(topLeft.x,rowsTop),ImVec2(_timelineX,bottomRight.y),true);
        for (int i = 0; i < int(_rows.size()); ++i)
        {
            const   float   y0  =   rowTop(i);
            if (y0 + _rowH < rowsTop || y0 > bottomRight.y)
                continue;
            const   auto&   item    =   _rows[size_t(i)];
            const   float   centerY =   y0 + _rowH * 0.5f;
            if (item._header)
            {
                bool    collapsed   =   false;
                auto    itr         =   _collapsed.find(item._anim);
                if (itr != _collapsed.end())
                    collapsed   =   itr->second;
                const   ImU32   triCol  =   IM_COL32(190,195,205,255);
                if (collapsed)
                {
                    drawList->AddTriangleFilled(ImVec2(topLeft.x + 6.0f,centerY - 4.0f),
                                                ImVec2(topLeft.x + 6.0f,centerY + 4.0f),
                                                ImVec2(topLeft.x + 11.0f,centerY),triCol);
                }
                else
                {
                    drawList->AddTriangleFilled(ImVec2(topLeft.x + 4.0f,centerY - 2.0f),
                                                ImVec2(topLeft.x + 12.0f,centerY - 2.0f),
                                                ImVec2(topLeft.x + 8.0f,centerY + 3.0f),triCol);
                }
                const   char*   name    =   item._anim ? item._anim->name().c_str() : "Animation";
                drawList->AddText(ImVec2(topLeft.x + 18.0f,y0 + 3.0f),
                                  (item._anim == _activeAnim) ? IM_COL32(120,185,255,255) : IM_COL32(215,220,230,255),
                                  name);
            }
            else
            {
                const   auto&   track   =   item._track;
                const   char*   name    =   (track && !track->name().empty()) ? track->name().c_str() : "Track";
                drawList->AddText(ImVec2(topLeft.x + 28.0f,y0 + 3.0f),IM_COL32(160,165,175,255),name);
            }
        }
        drawList->PopClipRect();

        /// <summary>
        /// 标尺背景与刻度
        /// </summary>
        drawList->AddRectFilled(ImVec2(topLeft.x,_canvasY),ImVec2(_timelineX,_canvasY + _rulerH),colNameBg);
        drawList->AddRectFilled(ImVec2(_timelineX,_canvasY),ImVec2(bottomRight.x,_canvasY + _rulerH),colRuler);

        double  step    =   1.0;
        static  const   double  kSteps[]    =   {   1,2,5,10,15,30,60,120,300,600,1800,3600 };
        for (double s : kSteps)
        {
            step    =   s;
            if (s * double(_framePixel) >= 64.0)
                break;
        }
        const   double  viewLeft    =   _viewLeftFrame;
        const   double  viewRight   =   frameForX(bottomRight.x);
        double  startFrame  =   std::floor(viewLeft / step) * step;
        for (double f = startFrame; f <= viewRight; f += step)
        {
            const   float   x   =   xForFrame(f);
            if (x < _timelineX || x > bottomRight.x)
                continue;
            drawList->AddLine(ImVec2(x,_canvasY + _rulerH - 5.0f),ImVec2(x,bottomRight.y),colLine);
            char    buffer[32]  =   {   0   };
            std::snprintf(buffer,sizeof(buffer),"%d",int(std::lround(f)));
            const   ImVec2  textSize    =   ImGui::CalcTextSize(buffer);
            drawList->AddText(ImVec2(x - textSize.x * 0.5f,_canvasY + 2.0f),IM_COL32(205,210,220,255),buffer);
        }

        /// <summary>
        /// 播放头
        /// </summary>
        const   float   playX   =   xForFrame(double(_curTime) * double(_fps));
        if (playX >= _timelineX && playX <= bottomRight.x)
        {
            drawList->AddLine(ImVec2(playX,rowsTop),ImVec2(playX,bottomRight.y),IM_COL32(255,92,82,230),2.0f);
            const   float   triY    =   _canvasY + 3.0f;
            drawList->AddTriangleFilled(ImVec2(playX - 6.0f,triY),
                                        ImVec2(playX + 6.0f,triY),
                                        ImVec2(playX,triY + 8.0f),IM_COL32(255,92,82,255));
            char    buffer[32]  =   {   0   };
            std::snprintf(buffer,sizeof(buffer),"%d",curFrame());
            const   ImVec2  textSize    =   ImGui::CalcTextSize(buffer);
            const   ImVec2  textPos(playX + 4.0f,_canvasY + 1.0f);
            drawList->AddRectFilled(ImVec2(textPos.x - 2.0f,textPos.y - 1.0f),
                                    ImVec2(textPos.x + textSize.x + 2.0f,textPos.y + ImGui::GetTextLineHeight() + 1.0f),
                                    IM_COL32(20,22,26,220));
            drawList->AddText(textPos,IM_COL32(255,225,215,255),buffer);
        }

        /// <summary>
        /// 边框
        /// </summary>
        drawList->AddRect(topLeft,bottomRight,IM_COL32(70,74,84,255));

        /// <summary>
        /// 无数据提示
        /// </summary>
        if (_node == nullptr || _anims.empty())
        {
            const   char*   message =   (_node == nullptr)
                                    ?   "请先在模型树中选择一个节点"
                                    :   "该节点还没有动画,点击「创建动画」";
            const   ImVec2  textSize    =   ImGui::CalcTextSize(message);
            drawList->AddText(ImVec2(_canvasX + (_canvasW - textSize.x) * 0.5f,
                                     _canvasY + _canvasH * 0.5f - textSize.y * 0.5f),
                              IM_COL32(150,155,165,255),message);
        }
    }

    void    FETimeLine::handleInput(bool hovered)
    {
        ImGuiIO&    io      =   ImGui::GetIO();
        const   ImVec2  mouse   =   ImGui::GetMousePos();

        const   bool    inTimeline  =   hovered && mouse.x >= _timelineX && mouse.x <= _canvasX + _canvasW;
        const   bool    inRuler     =   hovered && mouse.y >= _canvasY && mouse.y < _canvasY + _rulerH;
        const   bool    inRows      =   hovered && mouse.y >= _canvasY + _rulerH && mouse.y <= _canvasY + _canvasH;

        /// <summary>
        /// 滚轮: Ctrl 缩放 / Shift 横向滚动 / 普通纵向滚动
        /// </summary>
        if (hovered && io.MouseWheel != 0.0f)
        {
            if (io.KeyCtrl)
            {
                const   double  anchor  =   frameForX(mouse.x);
                float   newPixel        =   _framePixel * (io.MouseWheel > 0.0f ? 1.2f : 0.8f);
                newPixel                =   std::max(1.0f,std::min(newPixel,100.0f));
                _framePixel             =   newPixel;
                _viewLeftFrame          =   anchor - (double(mouse.x) - double(_timelineX)) / double(_framePixel);
                if (_viewLeftFrame < 0.0)
                    _viewLeftFrame      =   0.0;
            }
            else if (io.KeyShift)
            {
                _viewLeftFrame  -=  double(io.MouseWheel) * 20.0;
                if (_viewLeftFrame < 0.0)
                    _viewLeftFrame  =   0.0;
            }
            else
            {
                _scrollY    -=  io.MouseWheel * _rowH * 2.0f;
                if (_scrollY < 0.0f)
                    _scrollY    =   0.0f;
                const   float   rowsH   =   float(_rows.size()) * _rowH;
                const   float   maxScroll   =   std::max(0.0f,rowsH - (_canvasH - _rulerH));
                if (_scrollY > maxScroll)
                    _scrollY    =   maxScroll;
            }
        }

        /// <summary>
        /// 中键平移
        /// </summary>
        if (hovered && ImGui::IsMouseClicked(2))
        {
            _panning        =   true;
            _panStartX      =   mouse.x;
            _panStartView   =   _viewLeftFrame;
        }
        if (_panning)
        {
            if (ImGui::IsMouseDown(2))
            {
                _viewLeftFrame  =   _panStartView - (double(mouse.x) - double(_panStartX)) / double(_framePixel);
                if (_viewLeftFrame < 0.0)
                    _viewLeftFrame  =   0.0;
            }
            else
            {
                _panning    =   false;
            }
        }

        /// <summary>
        /// 左键按下
        /// </summary>
        if (hovered && ImGui::IsMouseClicked(0))
        {
            _pressX =   mouse.x;
            _pressY =   mouse.y;
            _dragMode   =   DM_None;
            _dragTracks.clear();
            _dragAnims.clear();
            _dragOffsets.clear();

            if (inRuler && inTimeline)
            {
                _dragMode   =   DM_Playhead;
                setCurFrame(int(std::lround(frameForX(mouse.x))));
            }
            else if (inRows)
            {
                const   int row =   rowAtY(mouse.y);
                if (mouse.x < _timelineX)
                {
                    /// <summary>
                    /// 名称列:点在动画头行切换折叠
                    /// </summary>
                    if (row >= 0 && _rows[size_t(row)]._header)
                    {
                        Animation   anim    =   _rows[size_t(row)]._anim;
                        bool&       collapsed   =   _collapsed[anim];
                        collapsed   =   !collapsed;
                        _activeAnim =   anim;
                    }
                }
                else if (row >= 0)
                {
                    const   auto&   item    =   _rows[size_t(row)];
                    if (item._header)
                    {
                        _activeAnim =   item._anim;
                        /// <summary>
                        /// 点中范围块 → 拖动动画 offset
                        /// </summary>
                        if (item._anim && item._anim->clip())
                        {
                            const   auto    rng =   item._anim->range();
                            const   float   x0  =   xForFrame(double(rng.x) * double(_fps));
                            const   float   x1  =   xForFrame(double(rng.y) * double(_fps));
                            if (mouse.x >= x0 && mouse.x <= x1)
                            {
                                _dragMode   =   DM_Offset;
                                _dragAnims.push_back(item._anim);
                                _dragOffsets.push_back(item._anim->offset());
                            }
                        }
                    }
                    else
                    {
                        _activeAnim =   item._anim;
                        const   int key =   keyAt(row,mouse.x,mouse.y);
                        if (key >= 0)
                        {
                            const   bool    modify  =   io.KeyCtrl || io.KeyShift;
                            const   bool    selected=   isKeySelected(item._track,size_t(key));
                            if (!modify && !selected)
                                clearSelection();
                            if (modify && selected)
                                removeKeySelection(item._track,size_t(key));
                            else if (!selected)
                                addKeySelection(item._track,size_t(key));

                            _dragMode   =   DM_Keys;
                            /// <summary>
                            /// 记录所有选中轨道的原始时间,供释放时计算新时间
                            /// </summary>
                            for (auto& pair : _selKeys)
                            {
                                KeyFrameTrack   track   =   pair.first;
                                if (!track || !track->_times)
                                    continue;
                                DragTrack   drag;
                                drag._track =   track;
                                drag._times =   track->_times->values();
                                drag._offset=   item._anim ? item._anim->offset() : 0;
                                for (size_t index : pair.second)
                                    drag._indices.push_back(index);
                                _dragTracks.push_back(std::move(drag));
                            }
                        }
                        else
                        {
                            clearSelection();
                            _dragMode   =   DM_Box;
                        }
                    }
                }
            }
        }

        /// <summary>
        /// 拖动播放头
        /// </summary>
        if (_dragMode == DM_Playhead && ImGui::IsMouseDown(0))
            setCurFrame(int(std::lround(frameForX(mouse.x))));

        /// <summary>
        /// 释放左键
        /// </summary>
        if (ImGui::IsMouseReleased(0))
        {
            if (_dragMode == DM_Keys)
                applyKeyDrag(mouse.x - _pressX);
            else if (_dragMode == DM_Offset)
                applyOffsetDrag(mouse.x - _pressX);
            else if (_dragMode == DM_Box)
                boxSelect(_pressX,_pressY,mouse.x,mouse.y);

            _dragMode   =   DM_None;
            _dragTracks.clear();
            _dragAnims.clear();
            _dragOffsets.clear();
        }

        /// <summary>
        /// 右键菜单
        /// </summary>
        if (hovered && inTimeline && ImGui::IsMouseClicked(1))
        {
            _ctxFrame   =   frameForX(mouse.x);
            _ctxRow     =   rowAtY(mouse.y);
            _ctxKey     =   0;
            _ctxOnKey   =   false;
            if (_ctxRow >= 0 && !_rows[size_t(_ctxRow)]._header)
            {
                const   int key =   keyAt(_ctxRow,mouse.x,mouse.y);
                if (key >= 0)
                {
                    _ctxOnKey   =   true;
                    _ctxKey     =   size_t(key);
                }
            }
            ImGui::OpenPopup("##tl_context");
        }

        /// <summary>
        /// 快捷键(仅鼠标在时间线窗口内时生效)
        /// </summary>
        if (hovered && !io.WantTextInput)
        {
            if (ImGui::IsKeyPressed(0x2E,false))
                deleteSelection();
            if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt && ImGui::IsKeyPressed(0x43,false))
                copySelection();
            if (io.KeyCtrl && !io.KeyShift && !io.KeyAlt && ImGui::IsKeyPressed(0x56,false))
                pasteClipboard();
            if (ImGui::IsKeyPressed(0x20,false))
                _playing    =   !_playing;
        }
    }

    void    FETimeLine::handleContextMenu()
    {
        if (!ImGui::BeginPopup("##tl_context"))
            return;

        if (ImGui::MenuItem("在此处添加关键帧"))
        {
            setCurFrame(int(std::lround(_ctxFrame)));
            addKeyframe();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("复制","Ctrl+C",false,!_selKeys.empty()))
            copySelection();
        if (ImGui::MenuItem("粘贴","Ctrl+V",false,!_clipboard.empty()))
            pasteClipboard();
        if (ImGui::MenuItem("删除","Delete",false,!_selKeys.empty()))
            deleteSelection();
        ImGui::Separator();
        if (ImGui::BeginMenu("插值"))
        {
            if (ImGui::MenuItem("贝塞尔"))
                setInterpolation(int(FEKeyFrameTrack::IT_Bezier));
            if (ImGui::MenuItem("线性"))
                setInterpolation(int(FEKeyFrameTrack::IT_Linear));
            if (ImGui::MenuItem("离散"))
                setInterpolation(int(FEKeyFrameTrack::IT_Constant));
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    void    FETimeLine::applyKeyDrag(float deltaPixel)
    {
        if (std::fabs(deltaPixel) < 0.5f || _selKeys.empty())
            return;

        const   real    deltaTime   =   real(double(deltaPixel) / double(_framePixel) / double(_fps));

        /// <summary>
        /// 动画的全部关键帧都被选中时,拖动等同修改动画 offset
        /// </summary>
        if (_activeAnim && allKeysSelected(_activeAnim))
        {
            SetOffsetCmd::Offsets    offsets;
            offsets.push_back({_activeAnim,_activeAnim->offset(),_activeAnim->offset() + deltaTime});
            if (_undoStack != nullptr)
                _undoStack->push(new SetOffsetCmd(std::move(offsets),"拖动动画范围",[this]() {   refreshScene();  }));
            else
            {
                _activeAnim->setOffset(_activeAnim->offset() + deltaTime);
                refreshScene();
            }
            clearSelection();
            return;
        }

        std::vector<TrackEdit>  edits;
        for (auto& drag : _dragTracks)
        {
            KeyFrameTrack   track   =   drag._track;
            if (!track || !track->_times)
                continue;
            auto&   times   =   track->_times->values();
            if (times.size() != drag._times.size())
                continue;

            TrackState  oldState    =   snapshotTrack(track);
            for (size_t index : drag._indices)
            {
                if (index >= times.size())
                    continue;
                real    newTime =   drag._times[index] + deltaTime;
                /// <summary>
                /// 时间线时间(局部时间 + offset)不能小于 0
                /// </summary>
                if (newTime + drag._offset < 0)
                    newTime =   -drag._offset;
                times[index]    =   newTime;
            }
            track->sortKeyFames();

            TrackState  newState    =   snapshotTrack(track);
            TrackEdit   edit;
            edit._old   =   oldState;
            edit._new   =   newState;
            edits.push_back(std::move(edit));
        }
        clearSelection();
        if (edits.empty())
            return;

        if (_undoStack != nullptr)
            _undoStack->push(new SetTrackStatesCmd(std::move(edits),"移动关键帧",[this]() { refreshScene(); }));
        else
        {
            for (auto& edit : edits)
                applyTrackState(edit._new);
            refreshScene();
        }
    }

    void    FETimeLine::applyOffsetDrag(float deltaPixel)
    {
        if (std::fabs(deltaPixel) < 0.5f || _dragAnims.empty())
            return;

        const   real    deltaTime   =   real(double(deltaPixel) / double(_framePixel) / double(_fps));
        SetOffsetCmd::Offsets    offsets;
        for (size_t i = 0; i < _dragAnims.size(); ++i)
        {
            if (!_dragAnims[i])
                continue;
            const   real    oldOffset   =   (i < _dragOffsets.size()) ? _dragOffsets[i] : _dragAnims[i]->offset();
            offsets.push_back({_dragAnims[i],oldOffset,oldOffset + deltaTime});
        }
        if (offsets.empty())
            return;

        if (_undoStack != nullptr)
            _undoStack->push(new SetOffsetCmd(std::move(offsets),"拖动动画范围",[this]() {  refreshScene();  }));
        else
        {
            for (auto& item : offsets)
                item._anim->setOffset(item._new);
            refreshScene();
        }
    }

    void    FETimeLine::boxSelect(float x0,float y0,float x1,float y1)
    {
        clearSelection();
        const   float   left    =   std::min(x0,x1);
        const   float   right   =   std::max(x0,x1);
        const   float   top     =   std::min(y0,y1);
        const   float   bottom  =   std::max(y0,y1);

        for (int row = 0; row < int(_rows.size()); ++row)
        {
            const   auto&   item    =   _rows[size_t(row)];
            if (item._header || !item._track || !item._track->_times)
                continue;
            const   float   centerY =   rowTop(row) + _rowH * 0.5f;
            if (centerY < top || centerY > bottom)
                continue;

            const   real    offset  =   item._anim ? item._anim->offset() : 0;
            const   auto&   times   =   item._track->_times->values();
            for (size_t i = 0; i < times.size(); ++i)
            {
                const   float   cx  =   xForFrame(double(times[i] + offset) * double(_fps));
                if (cx >= left && cx <= right)
                    addKeySelection(item._track,i);
            }
        }
    }

    void    FETimeLine::copySelection()
    {
        _clipboard.clear();
        bool    hasBase =   false;

        for (auto& pair : _selKeys)
        {
            KeyFrameTrack   track   =   pair.first;
            if (!track || !track->_times)
                continue;
            Animation   anim    =   animationOf(track);
            if (!anim)
                continue;
            const   real    offset  =   anim->offset();
            const   auto&   times   =   track->_times->values();

            ClipTrack   clipTrack;
            clipTrack._prop =   track->propertyIndex();
            for (size_t index : pair.second)
            {
                if (index >= times.size())
                    continue;
                const   real    timelineTime    =   times[index] + offset;
                ClipKey key;
                key._time   =   timelineTime;
                key._value  =   readValue(track->values(),index);
                clipTrack._keys.push_back(key);

                if (!hasBase || timelineTime < _clipBaseTime)
                {
                    _clipBaseTime   =   timelineTime;
                    hasBase         =   true;
                }
            }
            if (!clipTrack._keys.empty())
                _clipboard.push_back(std::move(clipTrack));
        }
        _status =   _clipboard.empty() ? "没有可复制的关键帧" : "已复制关键帧";
    }

    void    FETimeLine::pasteClipboard()
    {
        if (_clipboard.empty())
            return;

        std::vector<TrackEdit>  edits;
        for (auto& anim : _anims)
        {
            if (!anim || !anim->clip())
                continue;
            const   real    offset  =   anim->offset();
            for (auto& clipTrack : _clipboard)
            {
                for (auto& trackRef : anim->clip()->tracks())
                {
                    /// <summary>
                    /// tracks() 是 const 接口,拷贝一份可变引用再编辑
                    /// </summary>
                    KeyFrameTrack   track   =   trackRef;
                    if (!track || track->propertyIndex() != clipTrack._prop)
                        continue;

                    TrackState  oldState    =   snapshotTrack(track);
                    bool        changed     =   false;
                    for (auto& key : clipTrack._keys)
                    {
                        const   real    newInternal =   (key._time - _clipBaseTime + _curTime) - offset;
                        if (newInternal < 0)
                            continue;
                        if (!track->_times)
                            track->setTimeObject(RealsObject(new FERealsObject(track->ctx())));
                        auto&   times   =   track->_times->values();
                        auto    itr     =   std::lower_bound(times.begin(),times.end(),newInternal);
                        if (itr != times.end() && *itr == newInternal)
                            continue;
                        const   size_t  index   =   size_t(std::distance(times.begin(),itr));
                        if (track->insertKeyFrame(index,newInternal,key._value))
                            changed =   true;
                    }
                    if (!changed)
                        continue;

                    TrackState  newState    =   snapshotTrack(track);
                    TrackEdit   edit;
                    edit._old   =   oldState;
                    edit._new   =   newState;
                    edits.push_back(std::move(edit));
                }
            }
        }
        if (edits.empty())
        {
            _status =   "粘贴失败:目标时间点已有关键帧";
            return;
        }

        if (_undoStack != nullptr)
            _undoStack->push(new SetTrackStatesCmd(std::move(edits),"粘贴关键帧",[this]() {   refreshScene();  }));
        else
        {
            for (auto& edit : edits)
                applyTrackState(edit._new);
            refreshScene();
        }
        clearSelection();
        _status =   "已粘贴关键帧";
    }

    void    FETimeLine::deleteSelection()
    {
        if (_selKeys.empty())
            return;

        std::vector<TrackEdit>  edits;
        for (auto& pair : _selKeys)
        {
            KeyFrameTrack   track   =   pair.first;
            if (!track || !track->_times)
                continue;

            TrackState  oldState    =   snapshotTrack(track);
            std::vector<size_t> indices(pair.second.begin(),pair.second.end());
            std::sort(indices.begin(),indices.end(),std::greater<size_t>());
            bool    changed =   false;
            for (size_t index : indices)
            {
                if (track->removeKeyFrame(index))
                    changed =   true;
            }
            if (!changed)
                continue;

            TrackState  newState    =   snapshotTrack(track);
            TrackEdit   edit;
            edit._old   =   oldState;
            edit._new   =   newState;
            edits.push_back(std::move(edit));
        }
        clearSelection();
        if (edits.empty())
            return;

        if (_undoStack != nullptr)
            _undoStack->push(new SetTrackStatesCmd(std::move(edits),"删除关键帧",[this]() {   refreshScene();  }));
        else
        {
            for (auto& edit : edits)
                applyTrackState(edit._new);
            refreshScene();
        }
        _status =   "已删除关键帧";
    }

    void    FETimeLine::setInterpolation(int type)
    {
        std::vector<InterpEdit> edits;
        auto    addTrack    =   [&](KeyFrameTrack track)
        {
            if (!track)
                return;
            const   uint8   oldType =   uint8(track->_type);
            const   uint8   newType =   uint8(type);
            if (oldType == newType)
                return;
            edits.push_back({track,oldType,newType});
        };

        if (!_selKeys.empty())
        {
            for (auto& pair : _selKeys)
                addTrack(pair.first);
        }
        else if (_activeAnim && _activeAnim->clip())
        {
            for (auto& track : _activeAnim->clip()->tracks())
                addTrack(track);
        }
        if (edits.empty())
            return;

        if (_undoStack != nullptr)
            _undoStack->push(new SetInterpCmd(std::move(edits),"设置插值",[this]() {  refreshScene();  }));
        else
        {
            for (auto& edit : edits)
                edit._track->_type = (FEKeyFrameTrack::InterpolateType)edit._new;
            refreshScene();
        }
    }

    void    FETimeLine::createAnimation()
    {
        if (!_node)
        {
            _status =   "请先在模型树中选择节点";
            return;
        }
        if (!_node->objects<FEAnimation>().empty())
        {
            _status =   "该节点已有动画";
            return;
        }

        Animation   anim    =   FEAnimationHelper::createNodeAnimtion(_node->ctx());
        if (!anim)
        {
            _status =   "创建动画失败";
            return;
        }
        if (_scene != nullptr && _undoStack != nullptr)
        {
            _undoStack->push(new CreateAnimCmd(*_scene,_node,anim,[this]() {    refreshScene();  }));
        }
        else if (_scene != nullptr)
        {
            _node->addComponent(anim.get());
            if (_scene->animationSystem() != nullptr)
                _scene->animationSystem()->addObject(anim.get());
            refreshScene();
        }
        reloadAnimations();
        _activeAnim =   anim;
        _status     =   "已创建动画";
    }

    void    FETimeLine::addKeyframe()
    {
        if (!_node)
        {
            _status =   "请先在模型树中选择节点";
            return;
        }

        Animation   anim    =   _activeAnim;
        if (!anim && !_anims.empty())
            anim    =   _anims.front();
        if (!anim)
        {
            _status =   "请先创建动画";
            return;
        }
        if (!anim->clip())
        {
            _status =   "动画没有可用的 clip";
            return;
        }

        auto    clip        =   anim->clip();
        const   bool    emptyClip   =   clip->tracks().empty();
        std::vector<TrackState> oldStates;
        for (auto& track : clip->tracks())
            oldStates.push_back(snapshotTrack(track));

        /// <summary>
        /// 空 clip 先补默认轨道(位置/缩放/旋转)
        /// </summary>
        if (emptyClip)
            FEAnimationHelper::addDefaultTracks(clip,_node->ctx());

        bool    changed =   FEAnimationHelper::addNodeKeyFrame(anim,_curTime,_node);
        if (!changed)
            changed =   FEAnimationHelper::updateNodeKeyFrame(anim,_curTime,_node);
        if (!changed)
        {
            _status =   "当前帧没有产生关键帧变化";
            return;
        }

        std::vector<TrackEdit>  edits;
        for (auto& track : clip->tracks())
        {
            TrackEdit   edit;
            edit._old   =   snapshotTrack(track);
            for (auto& oldState : oldStates)
            {
                if (oldState._track == track)
                {
                    edit._old   =   oldState;
                    break;
                }
            }
            edit._new   =   snapshotTrack(track);
            edit._clip  =   clip;
            edit._recreated =   emptyClip;
            edits.push_back(std::move(edit));
        }
        if (edits.empty())
            return;

        if (_undoStack != nullptr)
            _undoStack->push(new SetTrackStatesCmd(std::move(edits),"添加关键帧",[this]() {    refreshScene();  }));
        else
        {
            for (auto& edit : edits)
                applyTrackState(edit._new);
            refreshScene();
        }
        _status =   "已添加关键帧";
    }
}
