#pragma     once
#include    "../FEMath.hpp"
#include    "../FEObject.h"

namespace FE
{
    class   FEKeyFrameTrack;
    class   FETrackResult
    {
    public:
        FEKeyFrameTrack*    _track  =   nullptr;
        CLSVar              _value  =   {};
        int                 _prop   =   {-1};
        bool                _valid  =   false;
    };

    using   TrackResults    =   std::vector<FETrackResult>;
}
