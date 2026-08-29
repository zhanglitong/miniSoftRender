#pragma     once
#include    "FEKeyFrameTrack.hpp"
#include    "FETrackResult.hpp"
#include    "FEObjectsTemplate.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEAnimClip, "{4DC5CFDD-59F2-4715-A852-10C8A76D06E6}");

    /// lambda
    constexpr   auto    TrackLessFunc = [](const KeyFrameTrack& l, const KeyFrameTrack& r)
    {
        return  l->times().get() < r->times().get();
    };
    /// 推导出来类型
    using   TrackLess    =   decltype(TrackLessFunc);

    class   FEAnimtion;

    class   FEAnimClip 
        : public FEObject
        , public FEObjectsTemplate<KeyFrameTrack, TrackLess>
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEAnimClip)
    public:
        FEAnimClip(FEContext& ctx)  
            :FEObject(ctx)
            ,FEObjectsTemplate<KeyFrameTrack, TrackLess>(TrackLessFunc)
        {
        }
        FEAnimClip(const FEAnimClip& other)
            :FEObject(other)
            ,FEObjectsTemplate<KeyFrameTrack, TrackLess>(other)
        {
            _objects     =   other._objects;
        }
        ~FEAnimClip()   =   default;
    public:
        /// <summary>
        /// 鑾峰彇甯ц寖鍥?
        /// </summary>
        /// <returns></returns>
        real2       range() const
        {
            real2   result(-1,-1);
            if (_objects.empty())
                return  result;
            else
                result  =   _objects.front()->range();
            for (size_t i = 1; i < _objects.size(); ++i)
            {
                auto    tmp =   _objects[i]->range();
                result.x    =   (std::min)(result.x,tmp.x);
                result.y    =   (std::max)(result.y,tmp.y);
            }
            return  result;
        }
        /// <summary>
        /// 获取所有track,只读
        /// </summary>
        /// <returns></returns>
        auto&       tracks() const
        {
            return  _objects;
        }
        /// <summary>
        /// 获取所有track,读写
        /// </summary>
        /// <returns></returns>
        auto&       tracks()
        {
            return  _objects;
        }
        /// <summary>
        /// 添加track
        /// </summary>
        /// <param name="track"></param>
        /// <returns>添加的track对象数</returns>
        size_t      addTrack(KeyFrameTrack track)
        {
            return  addObject(track);
        }
        /// <summary>
        /// 添加track
        /// </summary>
        /// <param name="track"></param>
        /// <returns>添加的track对象数</returns>
        size_t      addTracks(const KeyFrameTracks& tracks)
        {
            return  addObjects(tracks);
        }
        /// <summary>
        /// 传统流程,速度慢
        /// </summary>
        /// <param name="frame"></param>
        void        update0(const real& frame,TrackResults& results)
        {
            results.resize(_objects.size());
            /// 注意这里
            for (size_t i = 0 ;i < _objects.size() ; ++ i)
            {
                auto        track   =   _objects[i];
                results[i]._track   =   track;
                results[i]._prop    =   track->propertyIndex();
                results[i]._valid   =   track->update(frame,results[i]);
            }
        }
        /// <summary>
        /// update() 用 calcFrameOffset() 缓存了二分查找结果（针对共享 timeline 的优化），然后走 3 参数重载。
        /// 相同时间线的track更新计算中，减少了二分查找开销
        /// </summary>
        /// <param name="frame"></param>
        void        update(const real& frame,TrackResults& results)
        {
            results.resize(_objects.size());
            /// _objects 已经按照times 对象排序，即相同的timeline 会在一起
            RealsObject             timeLine    =   nullptr;
            FEKeyFrameTrack::KFOff  kfValue     =   {};
            for (size_t i = 0 ;i < _objects.size() ; ++ i)
            {
                auto        track   =   _objects[i];
                if (track->times() != timeLine)
                {
                    timeLine    =   track->times();
                    kfValue     =   track->calcFrameOffset(frame);
                }
                results[i]._track   =   track;
                results[i]._prop    =   track->propertyIndex();
                results[i]._valid   =   track->update(kfValue.index,frame,results[i]);
            }
        }
    };


    using   AnimClip    =   SharedPtr<FEAnimClip>;
    using   AnimClips   =   std::vector<AnimClip>;
}
