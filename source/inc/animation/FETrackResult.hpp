#pragma     once
#include    "../FEMath.hpp"
#include    "../FEObject.h"

#include    "FEValueArrayObjects.hpp"


namespace FE
{
    /// <summary>
    /// 这里不能随意更改
    /// </summary>
    using   ValueObject     =   std::variant<std::monostate,
                                            RealsObject,Real2sObject,Real3sObject,Real4sObject,QuatrsObject,
                                            FloatsObject,Float2sObject,Float3sObject,Float4sObject,QuatfsObject,
                                            BoolsObject,RgbaObject,AlphaObject,
                                            IntsObject,Int2sObject,Int3sObject,Int4sObject,
                                            UintsObject,Uint2sObject,Uint3sObject,Uint4sObject>;
    class   FEKeyFrameTrack;
    class   FETrackResult
    {
    public:
        FEKeyFrameTrack*    _track  =   nullptr;
        KFValue             _value  =   {};
        int                 _prop   =   {-1};
        bool                _valid  =   false;
    };

    using   TrackResults    =   std::vector<FETrackResult>;
}
