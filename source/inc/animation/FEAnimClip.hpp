#pragma     once
#include    "FEKeyFrameTrack.hpp"
#include    "FETrackResult.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEAnimClip, "{4DC5CFDD-59F2-4715-A852-10C8A76D06E6}");

    class   FEAnimtion;

    class   FEAnimClip : public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEAnimClip)
    public:
        FEAnimClip(FEContext& ctx)  
            :FEObject(ctx)
        {
        }
        FEAnimClip(const FEAnimClip& other)
            :FEObject(other)
        {
            _tracks     =   other._tracks;
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
            if (_tracks.empty())
                return  result;
            else
                result  =   _tracks.front()->range();
            for (size_t i = 1; i < _tracks.size(); ++i)
            {
                auto    tmp =   _tracks[i]->range();
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
        /// 
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


    using   AnimClip    =   SharedPtr<FEAnimClip>;
    using   AnimClips   =   std::vector<AnimClip>;
}
