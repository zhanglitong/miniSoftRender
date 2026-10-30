#include    "FETimeLine.hpp"

#include    "FEEditorUndoCmds.hpp"
#include    "FEModelTree.hpp"
#include    "FEUiStyle.hpp"

#include    "../../../inc/animation/FEAnimationHelper.hpp"
#include    "../../../inc/animation/FEAnimationSys.hpp"
#include    "../../../inc/animation/FEValueArrayObjects.hpp"
#include    "../../../inc/graphic/FEScene.h"

#include    <algorithm>
#include    <cctype>
#include    <cmath>
#include    <cstdint>
#include    <cstdio>
#include    <cstdlib>
#include    <functional>
#include    <type_traits>
#include    <utility>
#include    <variant>

namespace   FE
{
    /// <summary>
    /// 行高/缩进/行底色与文字颜色统一取自 FEUiStyle(与模型树、动画树共用)
    /// </summary>
    using   namespace   ui;

namespace
{
    /// <summary>
    /// 工具栏图标(Font Awesome 5 Solid 码位)
    /// </summary>
    const   char*   kGlyphStepBack     =   u8"\uf048";
    const   char*   kGlyphStepForward  =   u8"\uf051";
    const   char*   kGlyphPlay         =   u8"\uf04b";
    const   char*   kGlyphPause        =   u8"\uf04c";
    /// <summary>
    /// 添加关键帧(加号)
    /// </summary>
    const   char*   kGlyphAddKey       =   u8"\uf067";

    /// <summary>
    /// 动画树行缩进层级:播放器 0 / 节点 1 / 动画 2 / 轨道 3
    /// </summary>
    int animRowDepth(FEAnimItem::Type type)
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
        reals                   _times;
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
                /// 本次新建的轨道从 clip 移除:只能移除该轨道,不能清空整个 clip
                /// (补充缺失轨道的打帧只会新建缺失的那一条)
                if (edit._recreated && edit._clip && edit._new._track)
                    edit._clip->removeObject(edit._new._track);
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
    /// 创建动画组件命令:只把动画挂到节点上,不登记到动画树
    /// (要让动画出现在动画树,需在模型树右键「动画」子菜单「添加到播放器」)
    /// </summary>
    class   CreateAnimCmd :public FEUndoCmd
    {
    public:
        CreateAnimCmd(Node node,Animation anim,RefreshFn refresh)
            :FEUndoCmd("创建动画")
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
            if (_refresh)
                _refresh();
        }
        virtual void    undo()  override
        {
            if (!_node || !_anim)
                return;
            _node->removeComponent(_anim.get());
            if (_refresh)
                _refresh();
        }
    protected:
        Node        _node;
        Animation   _anim;
        RefreshFn   _refresh;
    };

    /// <summary>
    /// 轨道属性是否覆盖属性面板上的行:整组轨道(TRANSFORM_XYZ 等)覆盖其各轴
    /// </summary>
    bool    trackCoversProperty(PropIndex trackProp,PropIndex prop)
    {
        if (trackProp == prop)
            return  true;
        switch (prop)
        {
        case    PROP_TRANSFORM_X:
        case    PROP_TRANSFORM_Y:
        case    PROP_TRANSFORM_Z:
            return  trackProp == PROP_TRANSFORM_XYZ;
        case    PROP_SCALE_X:
        case    PROP_SCALE_Y:
        case    PROP_SCALE_Z:
            return  trackProp == PROP_SCALE_XYZ;
        case    PROP_ROTATE_X:
        case    PROP_ROTATE_Y:
        case    PROP_ROTATE_Z:
            return  trackProp == PROP_ROTATE_XYZ;
        default:
            return  false;
        }
    }
    /// <summary>
    /// 属性行 → 添加关键帧时要尝试的属性位(轴轨道与整组轨道一起尝试,存在哪个就写哪个)
    /// </summary>
    uint32  keyBitsForProperty(PropIndex prop)
    {
        switch (prop)
        {
        case    PROP_TRANSFORM_X:   return  NP_TRANSFORM_X  | NP_TRANSFORM_XYZ;
        case    PROP_TRANSFORM_Y:   return  NP_TRANSFORM_Y  | NP_TRANSFORM_XYZ;
        case    PROP_TRANSFORM_Z:   return  NP_TRANSFORM_Z  | NP_TRANSFORM_XYZ;
        case    PROP_SCALE_X:       return  NP_SCALE_X      | NP_SCALE_XYZ;
        case    PROP_SCALE_Y:       return  NP_SCALE_Y      | NP_SCALE_XYZ;
        case    PROP_SCALE_Z:       return  NP_SCALE_Z      | NP_SCALE_XYZ;
        case    PROP_ROTATE_X:      return  NP_ROTATE_X     | NP_ROTATE_XYZ;
        case    PROP_ROTATE_Y:      return  NP_ROTATE_Y     | NP_ROTATE_XYZ;
        case    PROP_ROTATE_Z:      return  NP_ROTATE_Z     | NP_ROTATE_XYZ;
        default:                    return  0;
        }
    }
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
        _tree.reset();
        _selKeys.clear();
        _clipboard.clear();
        _clipBaseTime   =   0;
        _dragTracks.clear();
        _dragAnims.clear();
        _dragOffsets.clear();
        _dragMode       =   DM_None;
        _panning        =   false;
        _ctxAnim        =   Animation();
        _ctxAnimOpen    =   false;
        _ctxPlayback    =   Playback();
        _ctxPlaybackOpen        =   false;
        _ctxCreatePlaybackOpen  =   false;
        _renamePlayback         =   Playback();
        _renamePlaybackOpen     =   false;
        _renamePlaybackFocus    =   false;
        _playDir        =   0;
        _curTime        =   0;
        _viewLeftFrame  =   0;
        _scrollY        =   0;
        _totalFrames    =   150;
        _totalManual    =   false;
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
        /// <summary>
        /// 只记录模型树当前节点,供"创建动画"使用;动画树内容由动画系统决定,
        /// 不跟随模型树选择变化
        /// </summary>
        _node   =   Node(node);
    }

    void    FETimeLine::startPlay(int dir)
    {
        if (_playDir == dir)
        {
            _playDir    =   0;
            return;
        }
        double  firstFrame  =   0.0;
        double  lastFrame   =   0.0;
        contentRange(firstFrame,lastFrame);
        const   double  cur =   double(_curTime) * double(_fps);
        /// <summary>
        /// 已经停在端点时,换向播放从另一端重新开始
        /// </summary>
        if (dir > 0 && cur >= lastFrame - 0.001)
            _curTime    =   real(firstFrame / double(_fps));
        else if (dir < 0 && cur <= firstFrame + 0.001)
            _curTime    =   real(lastFrame / double(_fps));
        _playDir    =   dir;
    }

    void    FETimeLine::contentRange(double& firstFrame,double& lastFrame) const
    {
        bool    any =   false;
        double  lo  =   0.0;
        double  hi  =   150.0;
        for (auto& anim : _tree.animations())
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
            hi  =   0.0;
        }
        firstFrame  =   std::min(lo,0.0);
        /// <summary>
        /// 未手动设过总帧数时,末尾自动覆盖动画内容;手动设定后以设定值为准
        /// </summary>
        const   double  total   =   double(std::max(1,_totalFrames));
        lastFrame   =   _totalManual ? total : std::max(hi,total);
    }

    void    FETimeLine::updateScrollMetrics()
    {
        const   float   sb      =   kScrollBarSize;
        const   float   viewH0  =   std::max(1.0f,_canvasH - _rulerH);
        const   float   rowsH   =   float(_tree.rows().size()) * _rowStep;

        double  firstFrame  =   0.0;
        double  lastFrame   =   0.0;
        contentRange(firstFrame,lastFrame);

        /// <summary>
        /// 垂直/水平滚动条互相挤占空间,迭代两次即可收敛
        /// </summary>
        bool    vScroll =   rowsH > viewH0;
        bool    hScroll =   false;
        float   trackW  =   1.0f;
        for (int iter = 0; iter < 2; ++iter)
        {
            trackW  =   std::max(1.0f,_canvasW - _nameW - _splitterW - (vScroll ? sb : 0.0f));
            const   double  visibleFrames   =   double(trackW) / double(_framePixel);
            const   double  total           =   std::max(lastFrame,visibleFrames);
            _viewMax    =   std::max(0.0,total - visibleFrames);
            hScroll     =   _viewMax > 0.0;
            const   float   viewH   =   std::max(1.0f,viewH0 - (hScroll ? sb : 0.0f));
            _scrollMax  =   std::max(0.0f,rowsH - viewH);
            vScroll     =   _scrollMax > 0.0f;
        }
        _vScroll    =   vScroll;
        _hScroll    =   hScroll;

        /// <summary>
        /// 偏移量始终夹在有效范围内:行增删/缩放后自动回到可见区域
        /// </summary>
        _scrollY        =   std::max(0.0f,std::min(_scrollY,_scrollMax));
        _viewLeftFrame  =   std::max(0.0,std::min(_viewLeftFrame,_viewMax));

        /// <summary>
        /// 垂直滚动条:位于时间线右侧,底部让出水平滚动条
        /// </summary>
        const   float   hBarH   =   _hScroll ? sb : 0.0f;
        _vTrackTop  =   _canvasY + _rulerH;
        _vTrackH    =   std::max(1.0f,_canvasH - _rulerH - hBarH);
        if (_vScroll)
        {
            const   float   ratio       =   _vTrackH / (_vTrackH + _scrollMax);
            _vThumbH    =   std::max(24.0f,_vTrackH * ratio);
            _vThumbTop  =   _vTrackTop + (_vTrackH - _vThumbH) * (_scrollY / _scrollMax);
        }
        else
        {
            _vThumbH    =   _vTrackH;
            _vThumbTop  =   _vTrackTop;
        }

        /// <summary>
        /// 水平滚动条:位于时间线底部,右侧让出垂直滚动条
        /// </summary>
        _hTrackLeft =   _timelineX;
        _hTrackW    =   std::max(1.0f,_canvasW - _nameW - _splitterW - (_vScroll ? sb : 0.0f));
        if (_hScroll)
        {
            const   double  visibleFrames   =   double(_hTrackW) / double(_framePixel);
            _hThumbW    =   std::max(24.0f,_hTrackW * float(visibleFrames / (visibleFrames + _viewMax)));
            _hThumbLeft =   _hTrackLeft + (_hTrackW - _hThumbW) * float(_viewLeftFrame / _viewMax);
        }
        else
        {
            _hThumbW    =   _hTrackW;
            _hThumbLeft =   _hTrackLeft;
        }
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
        return  _canvasY + _rulerH + float(index) * _rowStep - _scrollY;
    }

    int     FETimeLine::rowAtY(float y) const
    {
        if (y < _canvasY + _rulerH || y > _canvasY + _canvasH)
            return  -1;
        const   float   offset  =   y - (_canvasY + _rulerH) + _scrollY;
        if (offset < 0.0f)
            return  -1;
        const   int     index   =   int(offset / _rowStep);
        if (index < 0 || index >= int(_tree.rows().size()))
            return  -1;
        return  index;
    }

    int     FETimeLine::keyAt(int row,float x,float y) const
    {
        if (row < 0 || row >= int(_tree.rows().size()))
            return  -1;
        const   auto&   item    =   _tree.rows()[size_t(row)];
        const   KeyFrameTrack   track   =   item.track();
        if (item.type() != FEAnimItem::Type::Track || !track || !track->_times)
            return  -1;
        const   float   centerY =   rowTop(row) + _rowH * 0.5f;
        if (std::fabs(y - centerY) > _rowH * 0.5f)
            return  -1;

        const   Animation   anim    =   _tree.rowAnim(row);
        const   real    offset  =   anim ? anim->offset() : 0;
        const   auto&   times   =   track->_times->values();
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
        for (auto& anim : _tree.animations())
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
        for (auto& anim : _tree.animations())
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
        for (auto& anim : _tree.animations())
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
        for (auto& anim : _tree.animations())
        {
            if (anim)
                anim->applyTransform();
        }
    }

    void    FETimeLine::draw(ImFont* iconFont)
    {
        if (!_open || _scene == nullptr)
            return;

        _iconFont   =   iconFont;
        ensureNode();
        /// <summary>
        /// 动画树数据来自动画系统:只有"添加到播放器"登记的动画才会显示
        /// </summary>
        _tree.reload(_scene->animationSystem());
        _tree.rebuild();

        ImGuiIO&    io          =   ImGui::GetIO();
        const   float   displayW    =   io.DisplaySize.x;
        const   float   displayH    =   io.DisplaySize.y;
        /// <summary>
        /// 面板高度:上边缘可拖动调整;未调整过时按显示高度自适应
        /// </summary>
        const   float   minPanelH   =   190.0f;
        const   float   maxPanelH   =   std::max(minPanelH,displayH - 80.0f);
        float   panelH  =   (_panelH > 0.0f) ? _panelH : displayH * 0.34f;
        panelH  =   std::max(minPanelH,std::min(panelH,maxPanelH));

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
        /// 播放:由时间线自身推进时间,再让动画系统按该时间采样。
        /// 速率作用于时间增量,正/逆向由 _playDir 决定
        /// </summary>
        if (_playDir != 0)
        {
            double  firstFrame  =   0.0;
            double  lastFrame   =   0.0;
            contentRange(firstFrame,lastFrame);
            const   double  firstTime   =   firstFrame / double(_fps);
            const   double  lastTime    =   lastFrame / double(_fps);
            _curTime    +=  real(double(io.DeltaTime) * double(_speed) * double(_playDir));
            if (_curTime > real(lastTime))
            {
                if (_loop)
                    _curTime    =   real(firstTime);
                else
                {
                    _curTime    =   real(lastTime);
                    _playDir    =   0;
                }
            }
            else if (_curTime < real(firstTime))
            {
                if (_loop)
                    _curTime    =   real(lastTime);
                else
                {
                    _curTime    =   real(firstTime);
                    _playDir    =   0;
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
        /// <summary>
        /// 行高与模型树一致:行距 = 字体大小 × 2.4,扣除行间距后为行高
        /// </summary>
        _rowStep    =   ui::rowStep();
        _rowH       =   ui::rowHeight();
        /// <summary>
        /// 名称列宽度由分割条拖动决定,这里只保证上下限
        /// </summary>
        const   float   minNameW    =   80.0f;
        const   float   maxNameW    =   std::max(minNameW,canvasW - 120.0f - _splitterW);
        _nameW      =   std::max(minNameW,std::min(_nameW,maxNameW));
        _timelineX  =   _canvasX + _nameW + _splitterW;

        /// <summary>
        /// 滚动范围与滚动条几何:内容超出可视区时才显示。
        /// 垂直偏移 _scrollY 由动画树与时间线共用,滚动天然联动
        /// </summary>
        updateScrollMetrics();

        handleInput(hovered);
        _timelineX  =   _canvasX + _nameW + _splitterW;
        drawCanvas(hovered);
        handleContextMenu();

        /// <summary>
        /// 上边缘拖动调整面板高度:整条上边缘都是拖动区。
        /// 用最后提交的不可见按钮覆盖窗口顶部留白,避免与内部控件抢点击
        /// </summary>
        const   ImVec2  winPos  =   ImGui::GetWindowPos();
        const   ImVec2  winSize =   ImGui::GetWindowSize();
        ImGui::SetCursorScreenPos(winPos);
        ImGui::InvisibleButton("##tl_resize_top",ImVec2(winSize.x,4.0f));
        const   bool    edgeHot =   ImGui::IsItemHovered() || ImGui::IsItemActive();
        if (edgeHot)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
        if (ImGui::IsItemClicked(0))
        {
            _resizeStartY   =   io.MousePos.y;
            _resizeStartH   =   panelH;
        }
        if (ImGui::IsItemActive())
        {
            _panelH =   std::max(minPanelH,
                                std::min(_resizeStartH + (_resizeStartY - io.MousePos.y),maxPanelH));
        }
        if (edgeHot)
        {
            ImGui::GetWindowDrawList()->AddLine(ImVec2(winPos.x,winPos.y + 0.5f),
                                                ImVec2(winPos.x + winSize.x,winPos.y + 0.5f),
                                                IM_COL32(70,165,255,255),2.0f);
        }

        ImGui::End();
    }

    bool    FETimeLine::iconButton(const char* glyph,const char* text,const char* tip,bool active)
    {
        return  iconButtonImpl(glyph,text,tip,active,false);
    }

    bool    FETimeLine::iconButtonRotated(const char* glyph,const char* text,const char* tip,bool active)
    {
        return  iconButtonImpl(glyph,text,tip,active,true);
    }

    bool    FETimeLine::iconButtonImpl(const char* glyph,const char* text,const char* tip,bool active,bool rotate)
    {
        const   ImGuiStyle& style   =   ImGui::GetStyle();
        const   bool    useIcon     =   (_iconFont != nullptr);
        const   ImFont* font        =   useIcon ? _iconFont : ImGui::GetFont();
        /// <summary>
        /// 图标按正文字号绘制,按钮高度取标准控件高度,使整行控件上下居中且不撑高功能栏
        /// </summary>
        const   float   iconSize    =   ImGui::GetFontSize();
        const   ImVec2  label       =   useIcon
                                    ?   font->CalcTextSizeA(iconSize,FLT_MAX,0.0f,glyph,nullptr,nullptr)
                                    :   ImGui::CalcTextSize(text);
        const   float   btnH        =   ImGui::GetFrameHeight();
        const   ImVec2  size        =   ImVec2(std::max(label.x + style.FramePadding.x * 2.0f,btnH),btnH);
        const   ImVec2  pos         =   ImGui::GetCursorScreenPos();

        ImGui::PushID(glyph);
        ImGui::PushID(tip);
        if (active)
            ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(0.24f,0.52f,0.86f,1.0f));
        const   bool    clicked =   ImGui::Button("##tl_icon",size);
        if (active)
            ImGui::PopStyleColor();
        ImGui::PopID();
        ImGui::PopID();

        /// <summary>
        /// 自绘图标:文字左上角对齐到按钮中心;rotate 时顶点绕文字中心翻转 180°
        /// </summary>
        ImDrawList*     drawList    =   ImGui::GetWindowDrawList();
        const   ImVec2  textPos(pos.x + (size.x - label.x) * 0.5f,pos.y + (size.y - label.y) * 0.5f);
        const   int     vtxBegin    =   drawList->VtxBuffer.Size;
        drawList->AddText(font,iconSize,textPos,ImGui::GetColorU32(ImGuiCol_Text),
                          useIcon ? glyph : text);
        if (rotate)
        {
            const   ImVec2  center(textPos.x + label.x * 0.5f,textPos.y + label.y * 0.5f);
            for (int i = vtxBegin; i < drawList->VtxBuffer.Size; ++i)
            {
                ImDrawVert& v   =   drawList->VtxBuffer[i];
                v.pos.x =   center.x * 2.0f - v.pos.x;
                v.pos.y =   center.y * 2.0f - v.pos.y;
            }
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s",tip);
        return  clicked;
    }

    void    FETimeLine::drawToolbar()
    {
        const   ImGuiStyle& style       =   ImGui::GetStyle();
        const   float   winX            =   ImGui::GetWindowPos().x;
        const   float   localLeft       =   ImGui::GetCursorScreenPos().x - winX;
        /// <summary>
        /// 两组的边界:动画树(名称列)右缘,与画布分割位置一致
        /// </summary>
        const   float   group1Right     =   localLeft + _nameW + _splitterW;
        const   float   group1Width     =   group1Right - localLeft - style.WindowPadding.x;

        /// <summary>
        /// 第一组(动画树上方):交给动画树绘制搜索栏,回车执行搜索
        /// </summary>
        _tree.drawSearchBar([this](const char* glyph,const char* text,const char* tip,bool active)
        {
            return  iconButton(glyph,text,tip,active);
        },group1Width);

        /// <summary>
        /// 第二组(时间线上方):播放控制,以图标按钮呈现。
        /// 第一组文字过宽时右移到组尾之后,避免重叠
        /// </summary>
        const   float   group1End   =   ImGui::GetItemRectMax().x - winX;
        float   group2X =   group1Right;
        if (group2X < group1End + style.ItemSpacing.x)
            group2X =   group1End + style.ItemSpacing.x;
        ImGui::SameLine(group2X);

        ImGui::Checkbox("循环",&_loop);
        ImGui::SameLine();
        if (iconButton(kGlyphStepBack,"上一帧","上一个关键帧"))
            gotoKeyframe(1);
        ImGui::SameLine();
        if (iconButton(_playDir > 0 ? kGlyphPause : kGlyphPlay,"播放",
                       _playDir > 0 ? "暂停" : "正向播放",_playDir > 0))
            startPlay(1);
        ImGui::SameLine();
        if (iconButtonRotated(_playDir < 0 ? kGlyphPause : kGlyphPlay,"逆向",
                              _playDir < 0 ? "暂停" : "逆向播放",_playDir < 0))
            startPlay(-1);
        ImGui::SameLine();
        if (iconButton(kGlyphStepForward,"下一帧","下一个关键帧"))
            gotoKeyframe(2);

        /// <summary>
        /// 播放速率:0.1 ~ 60 倍
        /// </summary>
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("速率");
        ImGui::SameLine();
        ImGui::PushItemWidth(64.0f);
        ImGui::DragFloat("##tl_speed",&_speed,0.05f,0.1f,60.0f,"%.2f");
        ImGui::PopItemWidth();

        /// <summary>
        /// 当前帧:可拖动调整,双击/Ctrl+点击输入帧号
        /// </summary>
        double  firstFrame  =   0.0;
        double  lastFrame   =   0.0;
        contentRange(firstFrame,lastFrame);
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("帧");
        ImGui::SameLine();
        ImGui::PushItemWidth(64.0f);
        int     frame   =   curFrame();
        if (ImGui::DragInt("##tl_curframe",&frame,1.0f,0,int(std::lround(lastFrame))))
            setCurFrame(frame);
        ImGui::PopItemWidth();

        /// <summary>
        /// 总帧数:可拖动调整,双击/Ctrl+点击输入数值
        /// </summary>
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("总帧");
        ImGui::SameLine();
        ImGui::PushItemWidth(64.0f);
        int     total   =   int(std::lround(lastFrame));
        if (ImGui::DragInt("##tl_totalframes",&total,1.0f,1,100000))
        {
            _totalFrames    =   std::max(1,total);
            _totalManual    =   true;
            if (curFrame() > _totalFrames)
                setCurFrame(_totalFrames);
        }
        ImGui::PopItemWidth();

        /// <summary>
        /// 当前帧对应的时间(帧号 / 帧率)
        /// </summary>
        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::Text("时间 %.2f s",double(curFrame()) / double(_fps));

        ImGui::SameLine();
        if (iconButton(kGlyphAddKey,"添加关键帧","添加关键帧"))
            addKeyframe();

        if (!_status.empty())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.95f,0.75f,0.35f,1.0f),"%s",_status.c_str());
        }

        /// <summary>
        /// 关闭按钮右对齐
        /// </summary>
        const   ImVec2  labelSize   =   ImGui::CalcTextSize("关闭");
        const   float   closeW      =   labelSize.x + style.FramePadding.x * 2.0f;
        const   float   maxX        =   ImGui::GetWindowContentRegionMax().x;
        const   float   lastRight   =   ImGui::GetItemRectMax().x - winX;
        const   float   closeX      =   std::max(maxX - closeW,lastRight + style.ItemSpacing.x);
        ImGui::SameLine(closeX);
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
        /// <summary>
        /// 滚动条占用的边缘宽度;内容区相应收窄,只在不滚动方向上保留原尺寸
        /// </summary>
        const   float   sb              =   kScrollBarSize;
        const   float   contentRight    =   _vScroll ? bottomRight.x - sb : bottomRight.x;
        const   float   contentBottom   =   _hScroll ? bottomRight.y - sb : bottomRight.y;
        /// <summary>
        /// 名称列右边界;名称列与 _timelineX 之间的 6px 区域是留给分割条的间隙
        /// </summary>
        const   float   nameRight   =   _timelineX - _splitterW;

        drawList->AddRectFilled(topLeft,bottomRight,colBg);

        double  firstFrame  =   0.0;
        double  lastFrame   =   0.0;
        contentRange(firstFrame,lastFrame);

        /// <summary>
        /// 行背景
        /// </summary>
        const   ImVec2  mouse   =   ImGui::GetMousePos();
        const   int     hoverRow=   hovered ? rowAtY(mouse.y) : -1;
        for (int i = 0; i < int(_tree.rows().size()); ++i)
        {
            const   float   y0  =   std::max(rowTop(i),rowsTop);
            const   float   y1  =   std::min(rowTop(i) + _rowH,contentBottom);
            if (y1 <= y0)
                continue;
            ImU32   bg  =   (i % 2 == 0) ? colRowA : colRowB;
            if (i == hoverRow)
                bg  =   colHover;
            else if (_tree.isSelected(_tree.rows()[size_t(i)].object()))
                bg  =   kRowSelected;
            drawList->AddRectFilled(ImVec2(_timelineX,y0),ImVec2(contentRight,y1),bg);
            drawList->AddLine(ImVec2(_timelineX,y0),ImVec2(contentRight,y0),IM_COL32(52,55,63,255));
        }

        /// <summary>
        /// 超出动画时间范围的部分压暗
        /// </summary>
        const   float   xFirst  =   xForFrame(firstFrame);
        const   float   xLast   =   xForFrame(lastFrame);
        if (xFirst > _timelineX)
            drawList->AddRectFilled(ImVec2(_timelineX,rowsTop),ImVec2(std::min(xFirst,contentRight),contentBottom),colOut);
        if (xLast < contentRight)
            drawList->AddRectFilled(ImVec2(std::max(xLast,_timelineX),rowsTop),ImVec2(contentRight,contentBottom),colOut);

        /// <summary>
        /// 轨道内容:范围块 + 关键帧
        /// </summary>
        drawList->PushClipRect(ImVec2(_timelineX,rowsTop),ImVec2(contentRight,contentBottom),true);
        for (int i = 0; i < int(_tree.rows().size()); ++i)
        {
            const   float   y0  =   rowTop(i);
            if (y0 + _rowH < rowsTop || y0 > contentBottom)
                continue;
            const   auto&   item    =   _tree.rows()[size_t(i)];
            const   Animation   anim    =   _tree.rowAnim(i);
            const   real    offset  =   anim ? anim->offset() : 0;

            if (item.type() == FEAnimItem::Type::Animation)
            {
                if (!anim || !anim->clip())
                    continue;
                const   auto    rng =   anim->range();
                const   float   x0  =   xForFrame(double(rng.x) * double(_fps));
                const   float   x1  =   xForFrame(double(rng.y) * double(_fps));
                if (x1 > x0)
                {
                    const   ImVec2  p0(std::max(x0,_timelineX),y0 + 4.0f);
                    const   ImVec2  p1(std::min(x1,contentRight),y0 + _rowH - 4.0f);
                    if (p1.x > p0.x && p1.y > p0.y)
                    {
                        const   bool    active  =   (anim == _tree.activeAnim());
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
                        if (cx < _timelineX || cx > contentRight)
                            continue;
                        drawList->AddCircleFilled(ImVec2(cx,centerY),3.0f,IM_COL32(120,125,135,220));
                    }
                }
            }
            else if (item.type() == FEAnimItem::Type::Track)
            {
                const   auto&   track   =   item.track();
                if (!track || !track->_times)
                    continue;
                const   float   centerY =   y0 + _rowH * 0.5f;
                const   auto&   times   =   track->_times->values();
                for (size_t k = 0; k < times.size(); ++k)
                {
                    const   float   cx  =   xForFrame(double(times[k] + offset) * double(_fps));
                    if (cx < _timelineX || cx > contentRight)
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
        /// 名称列(动画树):底色、折叠三角与名称
        /// </summary>
        FEAnimationTree::Layout  treeLayout;
        treeLayout._drawList =   drawList;
        treeLayout._left     =   topLeft.x;
        treeLayout._right    =   nameRight;
        treeLayout._top      =   rowsTop;
        treeLayout._bottom   =   contentBottom;
        treeLayout._rowTop0  =   _canvasY + _rulerH - _scrollY;
        treeLayout._rowH     =   _rowH;
        treeLayout._rowStep  =   _rowStep;
        treeLayout._hoverRow =   hoverRow;
        _tree.drawNames(treeLayout);

        /// <summary>
        /// 标尺背景与刻度
        /// </summary>
        drawList->AddRectFilled(ImVec2(topLeft.x,_canvasY),ImVec2(nameRight,_canvasY + _rulerH),colNameBg);
        /// <summary>
        /// 动画树标题(与右侧标尺同高)
        /// </summary>
        {
            const   char*   title       =   "动画树";
            const   ImVec2  titleSize   =   ImGui::CalcTextSize(title);
            drawList->AddText(ImVec2(_canvasX + 12.0f,
                                     _canvasY + (_rulerH - titleSize.y) * 0.5f),
                              IM_COL32(225,230,240,255),title);
        }
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
            drawList->AddLine(ImVec2(x,_canvasY + _rulerH - 5.0f),ImVec2(x,contentBottom),colLine);
            char    buffer[32]  =   {   0   };
            std::snprintf(buffer,sizeof(buffer),"%d",int(std::lround(f)));
            const   ImVec2  textSize    =   ImGui::CalcTextSize(buffer);
            drawList->AddText(ImVec2(x - textSize.x * 0.5f,_canvasY + 2.0f),IM_COL32(205,210,220,255),buffer);
        }

        /// <summary>
        /// 播放头
        /// </summary>
        const   float   playX   =   xForFrame(double(_curTime) * double(_fps));
        if (playX >= _timelineX && playX <= contentRight)
        {
            drawList->AddLine(ImVec2(playX,rowsTop),ImVec2(playX,contentBottom),IM_COL32(255,92,82,230),2.0f);
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
        /// 动画树与时间线之间的间隙与分割条
        /// 间隙内绘制深色底和竖条,分割条完全落在间隙中,不遮挡第 0/1 帧
        /// </summary>
        const   bool    splitterActive  =   (_dragMode == DM_Splitter);
        const   float   splitterX       =   _timelineX - _splitterW * 0.5f;
        const   bool    splitterHot     =   splitterActive
                                        ||  (hovered && std::fabs(mouse.x - splitterX) <= _splitterW * 0.5f + 1.0f);
        /// <summary>
        /// 间隙底色
        /// </summary>
        drawList->AddRectFilled(ImVec2(nameRight,_canvasY),
                                ImVec2(_timelineX,bottomRight.y),
                                IM_COL32(22,24,29,255));
        const   float   splitW      =   splitterHot ? _splitterW : 2.0f;
        const   ImU32   colStrip    =   splitterActive ? IM_COL32(70,165,255,255)
                                      : splitterHot    ? IM_COL32(120,170,235,255)
                                                       : IM_COL32(110,117,132,255);
        drawList->AddRectFilled(ImVec2(splitterX - splitW * 0.5f,_canvasY),
                                ImVec2(splitterX + splitW * 0.5f,bottomRight.y),
                                colStrip);
        /// <summary>
        /// 三横线拖动手柄(常驻显示,不超时间隙)
        /// </summary>
        const   float   gripY   =   _canvasY + _canvasH * 0.5f;
        const   ImU32   gripCol =   splitterHot ? IM_COL32(255,255,255,255)
                                                : IM_COL32(190,198,214,255);
        for (int i = -1; i <= 1; ++i)
        {
            const   float   gy  =   gripY + float(i) * 4.0f;
            drawList->AddLine(ImVec2(splitterX - _splitterW * 0.5f + 1.0f,gy),
                              ImVec2(splitterX + _splitterW * 0.5f - 1.0f,gy),
                              gripCol,1.0f);
        }
        if (splitterHot)
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        /// <summary>
        /// 滚动条(仅内容超出可视区时显示;垂直偏移由动画树与时间线共用)
        /// </summary>
        const   ImU32   colSbTrack  =   IM_COL32(30,33,39,255);
        const   ImU32   colSbThumb  =   IM_COL32(110,117,132,255);
        const   ImU32   colSbHot    =   IM_COL32(150,158,172,255);
        if (_vScroll)
        {
            const   float   x0  =   bottomRight.x - sb;
            drawList->AddRectFilled(ImVec2(x0,_vTrackTop),
                                    ImVec2(bottomRight.x,_vTrackTop + _vTrackH),colSbTrack);
            const   bool    hot =   (_dragMode == DM_VScroll)
                                ||  (hovered && mouse.x >= x0
                                &&  mouse.y >= _vThumbTop
                                &&  mouse.y <= _vThumbTop + _vThumbH);
            if (hot)
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            drawList->AddRectFilled(ImVec2(x0 + 2.0f,_vThumbTop),
                                    ImVec2(bottomRight.x - 2.0f,_vThumbTop + _vThumbH),
                                    hot ? colSbHot : colSbThumb,3.0f);
        }
        if (_hScroll)
        {
            const   float   y0  =   bottomRight.y - sb;
            drawList->AddRectFilled(ImVec2(_hTrackLeft,y0),
                                    ImVec2(contentRight,bottomRight.y),colSbTrack);
            const   bool    hot =   (_dragMode == DM_HScroll)
                                ||  (hovered && mouse.y >= y0
                                &&  mouse.x >= _hThumbLeft
                                &&  mouse.x <= _hThumbLeft + _hThumbW);
            if (hot)
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            drawList->AddRectFilled(ImVec2(_hThumbLeft,y0 + 2.0f),
                                    ImVec2(_hThumbLeft + _hThumbW,bottomRight.y - 2.0f),
                                    hot ? colSbHot : colSbThumb,3.0f);
        }

        /// <summary>
        /// 边框
        /// </summary>
        drawList->AddRect(topLeft,bottomRight,IM_COL32(70,74,84,255));

        /// <summary>
        /// 无数据提示
        /// </summary>
        if (_tree.rows().empty())
        {
            const   char*   message =   "动画树为空,右键空白处「创建播放器」";
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
        /// 分割条位于名称列与时间线之间的间隙中央,命中范围覆盖整个间隙
        /// </summary>
        const   float   splitterX   =   _timelineX - _splitterW * 0.5f;
        const   bool    inSplitter  =   mouse.y >= _canvasY && mouse.y <= _canvasY + _canvasH
                                    &&  std::fabs(mouse.x - splitterX) <= _splitterW * 0.5f + 1.0f;
        if (hovered && inSplitter && ImGui::IsMouseClicked(0))
            _dragMode   =   DM_Splitter;
        if (_dragMode == DM_Splitter)
        {
            if (ImGui::IsMouseDown(0))
            {
                const   float   minNameW    =   80.0f;
                const   float   maxNameW    =   std::max(minNameW,_canvasW - 120.0f - _splitterW);
                _nameW      =   std::max(minNameW,
                                         std::min(mouse.x - _canvasX - _splitterW * 0.5f,maxNameW));
                _timelineX  =   _canvasX + _nameW + _splitterW;
            }
            else
            {
                _dragMode   =   DM_None;
            }
        }

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
                _viewLeftFrame  =   std::max(0.0,std::min(_viewLeftFrame,_viewMax));
            }
            else
            {
                _scrollY    -=  io.MouseWheel * _rowStep * 2.0f;
                _scrollY    =   std::max(0.0f,std::min(_scrollY,_scrollMax));
            }
        }

        /// <summary>
        /// 滚动条:点击滑块开始拖动,点击轨道空白按比例跳转。
        /// 垂直偏移 _scrollY 由动画树与时间线共用,两端始终同步
        /// </summary>
        const   float   sb          =   kScrollBarSize;
        const   float   vBarX0      =   _canvasX + _canvasW - sb;
        const   float   hBarY0      =   _canvasY + _canvasH - sb;
        const   bool    inVBar      =   _vScroll && mouse.x >= vBarX0
                                    &&  mouse.y >= _vTrackTop
                                    &&  mouse.y <= _vTrackTop + _vTrackH;
        const   bool    inHBar      =   _hScroll && mouse.y >= hBarY0
                                    &&  mouse.x >= _hTrackLeft
                                    &&  mouse.x <= _hTrackLeft + _hTrackW;
        if (hovered && (inVBar || inHBar) && ImGui::IsMouseClicked(0))
        {
            if (inVBar)
            {
                _dragMode   =   DM_VScroll;
                if (mouse.y >= _vThumbTop && mouse.y <= _vThumbTop + _vThumbH)
                {
                    _sbGrab =   mouse.y - _vThumbTop;
                }
                else
                {
                    /// <summary>
                    /// 点轨道空白:滑块中心跳到鼠标处
                    /// </summary>
                    _sbGrab =   _vThumbH * 0.5f;
                    const   float   ratio   =   (mouse.y - _vTrackTop) / _vTrackH;
                    _scrollY    =   std::max(0.0f,std::min(ratio * _scrollMax,_scrollMax));
                }
            }
            else
            {
                _dragMode   =   DM_HScroll;
                if (mouse.x >= _hThumbLeft && mouse.x <= _hThumbLeft + _hThumbW)
                {
                    _sbGrab =   mouse.x - _hThumbLeft;
                }
                else
                {
                    _sbGrab =   _hThumbW * 0.5f;
                    const   float   ratio   =   (mouse.x - _hTrackLeft) / _hTrackW;
                    _viewLeftFrame  =   double(std::max(0.0f,std::min(ratio,1.0f))) * _viewMax;
                }
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }
        if (_dragMode == DM_VScroll)
        {
            if (ImGui::IsMouseDown(0))
            {
                const   float   travel  =   std::max(1.0f,_vTrackH - _vThumbH);
                const   float   top     =   mouse.y - _sbGrab;
                const   float   ratio   =   std::max(0.0f,std::min(1.0f,(top - _vTrackTop) / travel));
                _scrollY    =   ratio * _scrollMax;
            }
            else
            {
                _dragMode   =   DM_None;
            }
        }
        if (_dragMode == DM_HScroll)
        {
            if (ImGui::IsMouseDown(0))
            {
                const   float   travel  =   std::max(1.0f,_hTrackW - _hThumbW);
                const   float   left    =   mouse.x - _sbGrab;
                const   float   ratio   =   std::max(0.0f,std::min(1.0f,(left - _hTrackLeft) / travel));
                _viewLeftFrame  =   double(ratio) * _viewMax;
            }
            else
            {
                _dragMode   =   DM_None;
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
        if (hovered && ImGui::IsMouseClicked(0) && _dragMode != DM_Splitter
            && _dragMode != DM_VScroll && _dragMode != DM_HScroll)
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
                    /// 名称列:点中折叠箭头切换折叠;点行选中(Shift 范围/Ctrl 切换);
                    /// 点空白处清空选中,与模型树一致
                    /// </summary>
                    if (row >= 0)
                    {
                        const   auto&   item    =   _tree.rows()[size_t(row)];
                        const   float   iconSize=   _rowH * 0.6f;
                        const   float   arrowX  =   _canvasX + 2.0f
                                                +   kIndent * float(animRowDepth(item.type()));
                        const   bool    onArrow =   item.type() != FEAnimItem::Type::Track
                                                &&  mouse.x >= arrowX
                                                &&  mouse.x <= arrowX + iconSize;
                        if (onArrow)
                        {
                            _tree.onClickNameColumn(row);
                        }
                        else
                        {
                            const   Object  object  =   item.object();
                            if (io.KeyShift)
                                _tree.selectRangeTo(object);
                            else if (io.KeyCtrl)
                                _tree.toggleSelect(object);
                            else
                                _tree.selectSingle(object);
                            if (item.type() == FEAnimItem::Type::Animation && item.anim())
                                _tree.setActiveAnim(item.anim());
                        }
                    }
                    else
                    {
                        _tree.clearSelection();
                    }
                }
                else if (row >= 0)
                {
                    const   auto&   item    =   _tree.rows()[size_t(row)];
                    const   Animation   anim    =   _tree.rowAnim(row);
                    if (item.type() == FEAnimItem::Type::Animation)
                    {
                        _tree.setActiveAnim(anim);
                        /// <summary>
                        /// 点中范围块 → 拖动动画 offset
                        /// </summary>
                        if (anim && anim->clip())
                        {
                            const   auto    rng =   anim->range();
                            const   float   x0  =   xForFrame(double(rng.x) * double(_fps));
                            const   float   x1  =   xForFrame(double(rng.y) * double(_fps));
                            if (mouse.x >= x0 && mouse.x <= x1)
                            {
                                _dragMode   =   DM_Offset;
                                _dragAnims.push_back(anim);
                                _dragOffsets.push_back(anim->offset());
                            }
                        }
                    }
                    else if (item.type() == FEAnimItem::Type::Track)
                    {
                        const   KeyFrameTrack   track   =   item.track();
                        _tree.setActiveAnim(anim);
                        const   int key =   keyAt(row,mouse.x,mouse.y);
                        if (key >= 0 && track)
                        {
                            const   bool    modify  =   io.KeyCtrl || io.KeyShift;
                            const   bool    selected=   isKeySelected(track,size_t(key));
                            if (!modify && !selected)
                                clearSelection();
                            if (modify && selected)
                                removeKeySelection(track,size_t(key));
                            else if (!selected)
                                addKeySelection(track,size_t(key));

                            _dragMode   =   DM_Keys;
                            /// <summary>
                            /// 记录所有选中轨道的原始时间,供释放时计算新时间
                            /// </summary>
                            for (auto& pair : _selKeys)
                            {
                                KeyFrameTrack   selTrack    =   pair.first;
                                if (!selTrack || !selTrack->_times)
                                    continue;
                                DragTrack   drag;
                                drag._track =   selTrack;
                                drag._times =   selTrack->_times->values();
                                drag._offset=   anim ? anim->offset() : 0;
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
        /// 右键菜单(时间线区域):播放器行弹出播放器菜单,其余位置弹出关键帧菜单
        /// </summary>
        if (hovered && inTimeline && ImGui::IsMouseClicked(1))
        {
            _ctxFrame   =   frameForX(mouse.x);
            _ctxRow     =   rowAtY(mouse.y);
            _ctxKey     =   0;
            _ctxOnKey   =   false;
            if (_ctxRow >= 0 && _tree.rows()[size_t(_ctxRow)].type() == FEAnimItem::Type::Playback)
            {
                _ctxPlayback        =   _tree.rows()[size_t(_ctxRow)].playback();
                _ctxPlaybackOpen    =   true;
            }
            else
            {
                if (_ctxRow >= 0 && _tree.rows()[size_t(_ctxRow)].type() == FEAnimItem::Type::Track)
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
        }

        /// <summary>
        /// 名称列右键:播放器行 → 删除/重命名播放器;动画行 → 删除动画;
        /// 其它位置(含空白处)→ 创建播放器
        /// </summary>
        if (hovered && inRows && mouse.x < _timelineX && ImGui::IsMouseClicked(1))
        {
            const   int row =   rowAtY(mouse.y);
            if (row >= 0 && _tree.rows()[size_t(row)].type() == FEAnimItem::Type::Playback)
            {
                _ctxPlayback        =   _tree.rows()[size_t(row)].playback();
                _ctxPlaybackOpen    =   true;
            }
            else if (row >= 0 && _tree.rows()[size_t(row)].type() == FEAnimItem::Type::Animation)
            {
                _ctxAnim    =   _tree.rows()[size_t(row)].anim();
                _ctxAnimOpen=   true;
            }
            else
            {
                _ctxCreatePlaybackOpen  =   true;
            }
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
                startPlay(1);
        }
    }

    void    FETimeLine::handleContextMenu()
    {
        /// <summary>
        /// 动画行右键菜单:只删除当前动画
        /// </summary>
        if (_ctxAnimOpen)
        {
            ImGui::OpenPopup("##tl_anim_context");
            _ctxAnimOpen    =   false;
        }
        if (ImGui::BeginPopup("##tl_anim_context"))
        {
            const   Animation   anim    =   _ctxAnim;
            if (ImGui::MenuItem("删除动画",nullptr,false,anim != nullptr))
                removeAnimation(anim);
            ImGui::EndPopup();
        }
        else if (!ImGui::IsPopupOpen("##tl_anim_context"))
        {
            /// <summary>
            /// 菜单真正关闭后才释放动画引用:菜单刚打开的那一帧 BeginPopup 可能返回 false,
            /// 此时清空会导致菜单项为禁用状态,点了也不删除
            /// </summary>
            _ctxAnim    =   Animation();
        }

        /// <summary>
        /// 播放器行右键菜单:重命名 / 删除播放器
        /// </summary>
        if (_ctxPlaybackOpen)
        {
            ImGui::OpenPopup("##tl_playback_context");
            _ctxPlaybackOpen    =   false;
        }
        if (ImGui::BeginPopup("##tl_playback_context"))
        {
            const   Playback    playback    =   _ctxPlayback;
            if (ImGui::MenuItem("重命名播放器",nullptr,false,playback != nullptr))
            {
                _renamePlayback         =   playback;
                _renamePlaybackOpen     =   true;
                _renamePlaybackFocus    =   true;
                std::snprintf(_renamePlaybackBuf,sizeof(_renamePlaybackBuf),"%s",playback->name().c_str());
            }
            if (ImGui::MenuItem("删除播放器",nullptr,false,playback != nullptr))
                removePlayback(playback);
            ImGui::EndPopup();
        }
        else if (!ImGui::IsPopupOpen("##tl_playback_context"))
        {
            /// <summary>
            /// 菜单真正关闭后才释放播放器引用(理由同动画行菜单)
            /// </summary>
            _ctxPlayback    =   Playback();
        }

        /// <summary>
        /// 动画树空白处右键菜单:创建播放器
        /// </summary>
        if (_ctxCreatePlaybackOpen)
        {
            ImGui::OpenPopup("##tl_playback_create");
            _ctxCreatePlaybackOpen  =   false;
        }
        if (ImGui::BeginPopup("##tl_playback_create"))
        {
            if (ImGui::MenuItem("创建播放器"))
                createPlayback();
            ImGui::EndPopup();
        }

        /// <summary>
        /// 重命名播放器弹窗
        /// </summary>
        if (_renamePlaybackOpen)
        {
            ImGui::OpenPopup("重命名播放器");
            _renamePlaybackOpen =   false;
        }
        if (ImGui::BeginPopupModal("重命名播放器",nullptr,ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted("播放器名称");
            ImGui::PushItemWidth(240.0f);
            if (_renamePlaybackFocus)
            {
                ImGui::SetKeyboardFocusHere();
                _renamePlaybackFocus    =   false;
            }
            const   bool    enter   =   ImGui::InputText("##rename_playback",_renamePlaybackBuf,
                                                         sizeof(_renamePlaybackBuf),
                                                         ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopItemWidth();
            const   bool    commit  =   ImGui::Button("确定") || enter;
            ImGui::SameLine();
            const   bool    cancel  =   ImGui::Button("取消");
            if (commit)
            {
                if (_renamePlaybackBuf[0] != '\0')
                    renamePlayback(_renamePlayback,_renamePlaybackBuf);
                _renamePlayback         =   Playback();
                ImGui::CloseCurrentPopup();
            }
            else if (cancel)
            {
                _renamePlayback         =   Playback();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

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
        Animation   active  =   _tree.activeAnim();
        if (active && allKeysSelected(active))
        {
            SetOffsetCmd::Offsets    offsets;
            offsets.push_back({active,active->offset(),active->offset() + deltaTime});
            if (_undoStack != nullptr)
                _undoStack->push(new SetOffsetCmd(std::move(offsets),"拖动动画范围",[this]() {   refreshScene();  }));
            else
            {
                active->setOffset(active->offset() + deltaTime);
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

        for (int row = 0; row < int(_tree.rows().size()); ++row)
        {
            const   auto&   item    =   _tree.rows()[size_t(row)];
            const   KeyFrameTrack   track   =   item.track();
            if (item.type() != FEAnimItem::Type::Track || !track || !track->_times)
                continue;
            const   float   centerY =   rowTop(row) + _rowH * 0.5f;
            if (centerY < top || centerY > bottom)
                continue;

            const   Animation   anim    =   _tree.rowAnim(row);
            const   real    offset  =   anim ? anim->offset() : 0;
            const   auto&   times   =   track->_times->values();
            for (size_t i = 0; i < times.size(); ++i)
            {
                const   float   cx  =   xForFrame(double(times[i] + offset) * double(_fps));
                if (cx >= left && cx <= right)
                    addKeySelection(track,i);
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
        for (auto& anim : _tree.animations())
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
        else if (_tree.activeAnim() && _tree.activeAnim()->clip())
        {
            for (auto& track : _tree.activeAnim()->clip()->tracks())
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
            _undoStack->push(new CreateAnimCmd(_node,anim,[this]() {    refreshScene();  }));
        }
        else if (_scene != nullptr)
        {
            _node->addComponent(anim.get());
            refreshScene();
        }
        _tree.reload(_scene != nullptr ? _scene->animationSystem() : nullptr);
        _tree.setActiveAnim(anim);
        /// <summary>
        /// 动画是节点上的组件,模型树行列表不会因组件增删自动重建
        /// </summary>
        if (_modelTree != nullptr)
            _modelTree->refresh();
        _status     =   "已创建动画,可在模型树右键「动画」子菜单「添加到播放器」";
    }

    void    FETimeLine::removeAnimation(Animation anim)
    {
        if (!anim || _scene == nullptr)
            return;

        /// <summary>
        /// 动画必须知道所属节点,才能把组件一并摘除
        /// </summary>
        Node    node;
        if (anim->owner())
            node    =   Node(anim->owner()->cast<FENode>());
        if (!node)
        {
            /// <summary>
            /// 取不到所属节点时至少从动画系统(动画树)摘除,保证动画树能刷新
            /// </summary>
            if (FEAnimationSys* sys = _scene->animationSystem())
                sys->removeObject(anim.get());
        }
        else if (_undoStack != nullptr)
        {
            _undoStack->push(new FEEditorAnimationCmd(*_scene,node,Animations{anim}
                                                      ,FEEditorAnimationCmd::AM_Detach,"删除动画"));
        }
        else
        {
            if (FEAnimationSys* sys = _scene->animationSystem())
                sys->removeObject(anim.get());
            node->removeComponent(anim.get());
        }

        /// <summary>
        /// 丢弃该动画的关键帧选择,避免后续操作引用已删除的轨道
        /// </summary>
        if (auto clip = anim->clip())
        {
            for (auto& track : clip->tracks())
                _selKeys.erase(track);
        }
        if (_tree.activeAnim() == anim)
            _tree.setActiveAnim(Animation());
        if (_ctxAnim == anim)
            _ctxAnim    =   Animation();

        /// <summary>
        /// 立即重建动画树,删除当帧即从列表消失,不再等下一帧的 draw
        /// </summary>
        _tree.reload(_scene->animationSystem());
        _tree.rebuild();

        /// <summary>
        /// 动画是节点上的组件,模型树行列表不会因组件增删自动重建
        /// </summary>
        if (_modelTree != nullptr)
            _modelTree->refresh();

        refreshScene();
        _status =   "已删除动画";
    }

    void    FETimeLine::createPlayback()
    {
        if (_scene == nullptr)
            return;
        FEAnimationSys* sys =   _scene->animationSystem();
        if (sys == nullptr)
            return;

        /// <summary>
        /// 播放器在动画系统中以名字为键,这里自动取不重复的名字
        /// </summary>
        auto&   playbacks   =   sys->playbacks();
        String  name        =   "播放器";
        int     index       =   1;
        while (playbacks.find(name) != playbacks.end())
        {
            name    =   String("播放器") + std::to_string(index);
            ++index;
        }
        sys->getOrCreate(name);

        _tree.reload(sys);
        _tree.rebuild();
        _status =   "已创建播放器:" + name;
    }

    void    FETimeLine::removePlayback(Playback playback)
    {
        if (_scene == nullptr || !playback)
            return;
        FEAnimationSys* sys =   _scene->animationSystem();
        if (sys == nullptr)
            return;

        auto&   playbacks   =   sys->playbacks();
        auto    itr =   playbacks.find(playback->name());
        if (itr == playbacks.end() || itr->second != playback)
        {
            /// <summary>
            /// 名字对不上时按对象查找,避免名字与索引键不同步时删不掉
            /// </summary>
            itr =   playbacks.begin();
            while (itr != playbacks.end() && itr->second != playback)
                ++itr;
        }
        if (itr == playbacks.end())
            return;
        const   String  name    =   playback->name();
        playbacks.erase(itr);

        /// <summary>
        /// 播放器删除后其动画不再登记在动画树;节点上的动画组件保留,
        /// 活动动画与关键帧选择由 reload/下面两步统一清理
        /// </summary>
        _tree.reload(sys);
        _tree.rebuild();
        for (auto sel = _selKeys.begin(); sel != _selKeys.end(); )
        {
            bool    alive   =   false;
            for (const Animation& anim : _tree.animations())
            {
                auto    clip    =   anim ? anim->clip() : AnimClip();
                if (!clip)
                    continue;
                for (const KeyFrameTrack& track : clip->tracks())
                {
                    if (track == sel->first)
                    {
                        alive   =   true;
                        break;
                    }
                }
                if (alive)
                    break;
            }
            if (alive)
                ++sel;
            else
                sel =   _selKeys.erase(sel);
        }
        _status =   "已删除播放器:" + name;
    }

    void    FETimeLine::renamePlayback(Playback playback,const String& newName)
    {
        if (_scene == nullptr || !playback)
            return;
        const   String  oldName =   playback->name();
        if (newName.empty() || newName == oldName)
            return;
        FEAnimationSys* sys =   _scene->animationSystem();
        if (sys == nullptr)
            return;

        auto&   playbacks   =   sys->playbacks();
        if (playbacks.find(newName) != playbacks.end())
        {
            _status =   "已有同名播放器";
            return;
        }
        auto    itr =   playbacks.find(oldName);
        if (itr == playbacks.end() || itr->second != playback)
        {
            itr =   playbacks.begin();
            while (itr != playbacks.end() && itr->second != playback)
                ++itr;
        }
        if (itr == playbacks.end())
            return;

        playbacks.erase(itr);
        playbacks[newName]  =   playback;
        playback->setName(newName);

        _tree.reload(sys);
        _tree.rebuild();
        _status =   "已重命名播放器:" + newName;
    }

    void    FETimeLine::addKeyframe()
    {
        Animation   anim    =   _tree.activeAnim();
        if (!anim && !_tree.animations().empty())
            anim    =   _tree.animations().front();
        if (!anim)
        {
            _status =   "请先在模型树右键「动画」子菜单「添加到播放器」";
            return;
        }

        /// <summary>
        /// 关键帧作用于动画的所属节点,不依赖模型树当前选择
        /// </summary>
        Node    node;
        if (anim->owner())
            node    =   Node(anim->owner()->cast<FENode>());
        if (!node)
            node    =   _node;
        if (!node)
        {
            _status =   "动画没有所属节点";
            return;
        }
        if (!anim->clip())
        {
            _status =   "动画没有可用的 clip";
            return;
        }
        /// 打帧时间吸附到帧,避免关键帧落在两帧之间
        _curTime    =   real(curFrame()) / real(_fps);

        auto    clip        =   anim->clip();
        std::vector<TrackState> oldStates;
        for (auto& track : clip->tracks())
            oldStates.push_back(snapshotTrack(track));
        /// 记录已有轨道:自动补齐的新轨道要单独标记,撤销时只移除它们
        std::vector<KeyFrameTrack>  existingTracks  =   clip->tracks();

        /// <summary>
        /// 补齐缺失的默认变换轨道(位置/缩放/旋转),空 clip 与只有部分轨道的 clip 都适用
        /// </summary>
        FEAnimationHelper::ensureTracks(clip,node->ctx(),NP_Default);

        bool    changed =   FEAnimationHelper::addNodeKeyFrame(anim,_curTime,node);
        if (!changed)
            changed =   FEAnimationHelper::updateNodeKeyFrame(anim,_curTime,node);
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
            edit._recreated =   std::find(existingTracks.begin(),existingTracks.end(),track) == existingTracks.end();
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

    bool    FETimeLine::hasKeyframe(PropIndex prop) const
    {
        Animation   anim    =   _tree.activeAnim();
        if (!anim && !_tree.animations().empty())
            anim    =   _tree.animations().front();
        if (!anim || !anim->clip())
            return  false;

        /// <summary>
        /// 时间线时间转内部时间(与添加关键帧时一致)
        /// </summary>
        const   real    internalTime    =   _curTime - anim->offset();
        AnimClip        clip            =   anim->clip();
        for (auto& track : clip->tracks())
        {
            if (!track || !track->_times)
                continue;
            if (!trackCoversProperty(track->propertyIndex(),prop))
                continue;
            const   reals&  times   =   track->_times->values();
            if (std::find(times.begin(),times.end(),internalTime) != times.end())
                return  true;
        }
        return  false;
    }

    bool    FETimeLine::toggleKeyframe(PropIndex prop)
    {
        Animation   anim    =   _tree.activeAnim();
        if (!anim && !_tree.animations().empty())
            anim    =   _tree.animations().front();
        if (!anim)
        {
            _status =   "请先在模型树右键「动画」子菜单「添加到播放器」";
            return  false;
        }

        /// <summary>
        /// 关键帧作用于动画的所属节点,不依赖模型树当前选择
        /// </summary>
        Node    node;
        if (anim->owner())
            node    =   Node(anim->owner()->cast<FENode>());
        if (!node)
            node    =   _node;
        if (!node)
        {
            _status =   "动画没有所属节点";
            return  false;
        }
        AnimClip    clip    =   anim->clip();
        if (!clip)
        {
            _status =   "动画没有可用的 clip";
            return  false;
        }
        /// 打帧时间吸附到帧,避免关键帧落在两帧之间
        _curTime    =   real(curFrame()) / real(_fps);

        /// <summary>
        /// 当前时间已有关键帧:按属性移除
        /// </summary>
        if (hasKeyframe(prop))
        {
            std::vector<TrackEdit>  edits;
            for (auto& trackRef : clip->tracks())
            {
                /// <summary>
                /// tracks() 是 const 接口,拷贝一份可变引用再编辑
                /// </summary>
                KeyFrameTrack   track   =   trackRef;
                if (!track || !track->_times)
                    continue;
                if (!trackCoversProperty(track->propertyIndex(),prop))
                    continue;
                TrackState  oldState    =   snapshotTrack(track);
                if (!track->removeKeyFrame(_curTime - anim->offset()))
                    continue;
                TrackEdit   edit;
                edit._old   =   oldState;
                edit._new   =   snapshotTrack(track);
                edit._clip  =   clip;
                edits.push_back(std::move(edit));
            }
            if (edits.empty())
                return  true;
            if (_undoStack != nullptr)
                _undoStack->push(new SetTrackStatesCmd(std::move(edits),"删除关键帧",[this]() {   refreshScene();    }));
            else
            {
                for (auto& edit : edits)
                    applyTrackState(edit._new);
                refreshScene();
            }
            _status =   "已删除关键帧";
            return  false;
        }

        /// <summary>
        /// 没有关键帧:按属性添加
        /// </summary>
        std::vector<TrackState> oldStates;
        for (auto& track : clip->tracks())
            oldStates.push_back(snapshotTrack(track));
        /// 记录已有轨道:自动补齐的新轨道要单独标记,撤销时只移除它们
        std::vector<KeyFrameTrack>  existingTracks  =   clip->tracks();

        const   NodeProperyBits bits(keyBitsForProperty(prop));
        /// <summary>
        /// 补齐该属性对应的缺失轨道(clip 只有位置轨道时,旋转也能落帧)
        /// </summary>
        FEAnimationHelper::ensureTracks(clip,node->ctx(),bits);

        bool    changed =   FEAnimationHelper::addNodeKeyFrame(anim,_curTime,node,bits);
        if (!changed)
            changed =   FEAnimationHelper::updateNodeKeyFrame(anim,_curTime,node,bits);
        if (!changed)
        {
            _status =   "当前帧没有产生关键帧变化";
            return  false;
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
            edit._recreated =   std::find(existingTracks.begin(),existingTracks.end(),track) == existingTracks.end();
            edits.push_back(std::move(edit));
        }
        if (edits.empty())
            return  false;

        if (_undoStack != nullptr)
            _undoStack->push(new SetTrackStatesCmd(std::move(edits),"添加关键帧",[this]() {    refreshScene();  }));
        else
        {
            for (auto& edit : edits)
                applyTrackState(edit._new);
            refreshScene();
        }
        _status =   "已添加关键帧";
        return  true;
    }
}
