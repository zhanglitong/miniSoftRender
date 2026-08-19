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
        /// 璁＄畻鑾峰彇甯х殑鑼冨洿
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
        /// 鑾峰彇涓抚鏁?
        /// </summary>
        /// <returns></returns>
        uint        frames() const
        {
            auto    temp   =   range();
            return  (temp.y - temp.x) + 1;
        }
        /// <summary>
        /// 鏄惁鏈夋晥锛屽鏋滄湁鍏抽敭甯э紝鏈夋晥锛屽惁鍒欐棤鏁?
        /// </summary>
        /// <returns></returns>
        bool        isValid() const
        {
            return !_keyframes.empty();
        }
        /// <summary>
        /// 璁剧疆灞炴€х储寮?
        /// </summary>
        /// <param name="index"></param>
        void        setPropertyIndex(const PropIndex& index)
        {
            _propIndex  =   index;
        }
        /// <summary>
        /// 鑾峰彇鎵€鏈夊叧閿抚鏁版嵁
        /// </summary>
        /// <returns></returns>
        auto&       keyframes() const
        {
            return _keyframes;
        }
        /// <summary>
        /// 绗竴甯?
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
        /// 鏈€鍚庝竴甯?
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
        /// 娣诲姞鍏抽敭甯?
        /// </summary>
        /// <param name="keyFrame"></param>
        /// <returns></returns>
        bool        addKeyFrame(KeyFrame keyFrame)
        {
            /// 浜屽垎娉曟煡鎵炬彃鍏ヤ綅缃?
            auto    itr =   std::lower_bound(_keyframes.begin(), _keyframes.end(), keyFrame, [](const KeyFrame& l, const KeyFrame& r)
            {
                return l->frame() < r->frame();
            });
            /// 娌℃湁鎵惧埌
            if (itr == _keyframes.end())
                _keyframes.push_back(keyFrame);
            /// 鎵惧埌
            else if((*itr)->frame() != keyFrame->frame())
                _keyframes.insert(itr,keyFrame);
            /// 宸茬粡瀛樺湪
            else
                return  false;
            return  true;
        }
        /// <summary>
        /// 璋冪敤瀹屾垚璇ュ嚱鏁板悗锛屽鏋渇rames鏄棤搴忕殑锛岄渶瑕佸啀娆¤皟鐢?@ref  sortKeyFames();
        /// 淇濊瘉鏁版嵁鏈夊簭
        /// </summary>
        /// <param name="frames"></param>
        void        appendKeyFrames(const KeyFrames& frames)
        {
            _keyframes.insert(_keyframes.end(),frames.begin(),frames.end());
        }
        /// <summary>
        /// 鏍规嵁绱㈠紩鍙疯幏鍙栧叧閿抚
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
        /// 绉婚櫎鍏抽敭甯у璞?
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
        /// 鏍规嵁鍏抽敭甯у簭鍙风Щ闄?
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
        /// 鏍规嵁鍏抽敭甯у簭鍙凤紝鑾峰彇鍏抽敭甯ф暟缁勪笅鏍?
        /// </summary>
        /// <param name="keyFrame"></param>
        /// <returns>size_t(-1) 娌℃湁鎵惧埌</returns>
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
        /// 閲嶆柊鏍规嵁鏃堕棿鎺掑簭锛屽綋track鍐呯殑keyframe琚洿鎺ヤ慨鏀规椂闂存椂璋冪敤
        /// </summary>
        void        sortKeyFames()
        {
            std::sort(_keyframes.begin(),_keyframes.end(),[](const KeyFrame& l, const KeyFrame& r)
            {
                return l->frame() < r->frame();
            });
        }
        /// <summary>
        /// 鏇存柊閫昏緫
        /// </summary>
        /// <param name="frame"></param>
        bool        update(const real& frame,FETrackResult& result)
        {
            if (!isValid())
                return  false;
            auto    rng =   range();
            /// 涓嶈兘 <= 鍚庣画闇€瑕佽繘琛岃绠楋紝瑙﹀彂鏉′欢璁＄畻
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
