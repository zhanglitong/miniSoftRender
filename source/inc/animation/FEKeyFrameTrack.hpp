#pragma     once
#include    "FEKeyFrame.hpp"
#include    "FETrackResult.hpp"

namespace FE
{
    using   PropIndex       =   int;
    DEFINE_CLASS_UUID(FEKeyFrameTrack, "{474DC049-28C0-4F79-AB66-CDCA4707DD8B}");

    class   FEKeyFrameTrack : public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEKeyFrameTrack)
    public:
        FEKeyFrameTrack(FEContext& ctx,PropIndex index = -1)  
            :FEObject(ctx)
        {
            _propIndex  =   index;
        }

        FEKeyFrameTrack(const FEKeyFrameTrack& other)
            :FEObject(other)
        {
            _propIndex  =   other._propIndex;
            _keyframes  =   other._keyframes;
        }
        ~FEKeyFrameTrack() 
        {
        }
    public:
        /// <summary>
        /// 计算获取帧的范围
        /// </summary>
        /// <returns></returns>
        uint2       range() const
        {
            if (_keyframes.empty())
                return  uint2(0,0);
            uint    nMin  =   front()->frame();
            uint    nMax  =   back()->frame();
            return  uint2(nMin,nMax);
        }
        /// <summary>
        /// 获取中帧数
        /// </summary>
        /// <returns></returns>
        uint        frames() const
        {
            auto    temp   =   range();
            return  (temp.y - temp.x) + 1;
        }
        /// <summary>
        /// 是否有效，如果有关键帧，有效，否则无效
        /// </summary>
        /// <returns></returns>
        bool        isValid() const
        {
            return !_keyframes.empty();
        }
        /// <summary>
        /// 设置属性索引
        /// </summary>
        /// <param name="index"></param>
        void        setPropertyIndex(const PropIndex& index)
        {
            _propIndex  =   index;
        }
        /// <summary>
        /// 获取所有关键帧数据
        /// </summary>
        /// <returns></returns>
        auto&       keyframes() const
        {
            return _keyframes;
        }
        /// <summary>
        /// 第一帧
        /// </summary>
        /// <returns></returns>
        KeyFrame    front() const
        {
            if (_keyframes.empty())
                return  nullptr;
            else
                return  _keyframes.front();
        }
        /// <summary>
        /// 最后一帧
        /// </summary>
        /// <returns></returns>
        KeyFrame    back() const
        {
            if (_keyframes.empty())
                return  nullptr;
            else
                return  _keyframes.back();
        }
        /// <summary>
        /// 添加关键帧
        /// </summary>
        /// <param name="keyFrame"></param>
        /// <returns></returns>
        bool        addKeyFrame(KeyFrame keyFrame)
        {
            /// 二分法查找插入位置
            auto    itr =   std::lower_bound(_keyframes.begin(), _keyframes.end(), keyFrame, [](const KeyFrame& l, const KeyFrame& r)
            {
                return l->frame() < r->frame();
            });
            /// 没有找到
            if (itr == _keyframes.end())
                _keyframes.push_back(keyFrame);
            /// 找到
            else if((*itr)->frame() != keyFrame->frame())
                _keyframes.insert(itr,keyFrame);
            /// 已经存在
            else
                return  false;
            return  true;
        }
        /// <summary>
        /// 调用完成该函数后，如果frames是无序的，需要再次调用 @ref  sortKeyFames();
        /// 保证数据有序
        /// </summary>
        /// <param name="frames"></param>
        void        appendKeyFrames(const KeyFrames& frames)
        {
            _keyframes.insert(_keyframes.end(),frames.begin(),frames.end());
        }
        /// <summary>
        /// 根据索引号获取关键帧
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        KeyFrame    keyFrame(size_t index) const
        {
            if (index < _keyframes.size())
                return  _keyframes[index];
            else
                return  nullptr;
        }
        /// <summary>
        /// 移除关键帧对象
        /// </summary>
        /// <param name="keyFrame"></param>
        /// <returns></returns>
        bool        removeKeyframe(KeyFrame keyFrame)
        {
            auto    itr =   std::lower_bound(_keyframes.begin(), _keyframes.end(), keyFrame, [](const KeyFrame& l, const KeyFrame& r)
            {
                return l->frame() < r->frame();
            });
            if (itr != _keyframes.end() && (*itr) == keyFrame)
            {
                _keyframes.erase(itr);
                return  true;
            }
            return  false;
        }
        /// <summary>
        /// 根据关键帧序号移除
        /// </summary>
        /// <param name="keyFrame"></param>
        /// <returns></returns>
        bool        removeKeyFrame(uint keyNo)
        {
            auto    itr =   std::lower_bound(_keyframes.begin(), _keyframes.end(), keyNo, [](const KeyFrame& l, uint keyNo)
            {
                return l->frame() < keyNo;
            });
            if (itr != _keyframes.end() && (*itr)->frame() == keyNo)
            {
                _keyframes.erase(itr);
                return  true;
            }
            return  false;
        }
        /// <summary>
        /// 根据关键帧序号，获取关键帧数组下标
        /// </summary>
        /// <param name="keyFrame"></param>
        /// <returns>size_t(-1) 没有找到</returns>
        size_t      keyFrameIndex(uint keyNo) const
        {
            auto    itr =   std::lower_bound(_keyframes.begin(), _keyframes.end(), keyNo, [](const KeyFrame& l, uint keyNo)
            {
                return l->frame() < keyNo;
            });
            if (itr != _keyframes.end() && (*itr)->frame() == keyNo)
                return  std::distance(_keyframes.begin(),itr);
            else
                return  size_t(-1);
        }
        /// <summary>
        /// 重新根据时间排序，当track内的keyframe被直接修改时间时调用
        /// </summary>
        void        sortKeyFames()
        {
            std::sort(_keyframes.begin(),_keyframes.end(),[](const KeyFrame& l, const KeyFrame& r)
            {
                return l->frame() < r->frame();
            });
        }
        /// <summary>
        /// 更新逻辑
        /// </summary>
        /// <param name="frame"></param>
        bool        update(const real& frame,FETrackResult& result)
        {
            if (!isValid())
                return  false;
            auto    rng =   range();
            /// 不能 <= 后续需要进行计算，触发条件计算
            if (frame < rng.x)
                result._value   =   front()->value();
            else if(frame > rng.y)
                result._value   =   back()->value();
            else
            {
                auto    itr =   std::lower_bound(_keyframes.begin(), _keyframes.end(), frame, [](const KeyFrame& l, real frame)
                {
                    return l->frame() < frame;
                });
                if (itr == _keyframes.end())
                    result._value   =   _keyframes.back()->value();
                if (itr == _keyframes.begin())
                    result._value   =   _keyframes.front()->value();
                else
                    result._value   =   (*(itr - 1))->interpolate(frame, *(*itr));
            } 
            return  true;
        }
    public:
        String      _name;
        KeyFrames   _keyframes;
        PropIndex   _propIndex  =   -1;
    };

    using   KeyFrameTrack       =   SharedPtr<FEKeyFrameTrack>;
    using   KeyFrameTracks      =   std::vector<KeyFrameTrack>;
}
