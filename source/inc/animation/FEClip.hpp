#pragma     once
#include    "FEKeyFrameTrack.hpp"
#include    "FETrackResult.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEClip, "{0AFD2EDB-C83A-49F4-ACC6-52E6060D54E4}");

    class   FEAnimtion;

    class   FEClip : public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEClip)
    public:
        FEClip(FEContext& ctx)  
            :FEObject(ctx)
        {
        }
        FEClip(const FEClip& other)
            :FEObject(other)
        {
            _tracks     =   other._tracks;
        }
        ~FEClip()   =   default;
    public:
        /// <summary>
        /// 鑾峰彇甯ц寖鍥?
        /// </summary>
        /// <returns></returns>
        uint2       range() const
        {
            uint2   result(0,0);
            for (auto& track : _tracks)
            {
                auto    tmp =   track->range();
                result.x    =   (std::min)(result.x,tmp.x);
                result.y    =   (std::max)(result.y,tmp.y);
            }
            return  result;
        }
        /// <summary>
        /// 鍙鑾峰彇
        /// </summary>
        /// <returns></returns>
        auto&       tracks() const
        {
            return  _tracks;
        }
        /// <summary>
        /// 璇诲啓
        /// </summary>
        /// <returns></returns>
        auto&       tracks()
        {
            return  _tracks;
        }
        void        addTrack(KeyFrameTrack track)
        {
            _tracks.push_back(track);
        }
        void        addTracks(const KeyFrameTracks& tracks)
        {
            _tracks.insert(_tracks.end(),tracks.begin(),tracks.end());
        }
        /// <summary>
        /// 鏇存柊鎵€鏈夎建閬撳€?
        /// </summary>
        /// <param name="frame"></param>
        void        update(const real& frame,TrackResults& results)
        {
            results.resize(_tracks.size());
            for (size_t i = 0 ;i < _tracks.size() ; ++ i)
            {
                auto        track   =   _tracks[i];
                results[i]._track   =   track;
                results[i]._valid   =   track->update(frame,results[i]);
            }
        }
    public:
        KeyFrameTracks  _tracks;
    };

    using   ClipPtr     =   SharedPtr<FEClip>;
    using   ClipPtrs    =   std::vector<ClipPtr>;
}
