#pragma     once
#include    "../FEMath.hpp"
#include    "../FEObject.h"

#include    "FERealsObject.hpp"
#include    "FEReal2sObject.hpp"
#include    "FEReal3sObject.hpp"
#include    "FEReal4sObject.hpp"
#include    "FEQuatrsObject.hpp"

#include    "FEFloatsObject.hpp"
#include    "FEFloat2sObject.hpp"
#include    "FEFloat3sObject.hpp"
#include    "FEFloat4sObject.hpp"
#include    "FEQuatfsObject.hpp"
#include    "FEBoolsObject.hpp"
#include    "FEColorObject.hpp"
#include    "FEAlphaObject.hpp"


namespace FE
{
    /// <summary>
    /// 这里不能随意更改
    /// </summary>
    using   ValueObject     =   std::variant<std::monostate,
                                            RealsObject,Real2sObject,Real3sObject,Real4sObject,QuatrsObject,
                                            FloatsObject,Float2sObject,Float3sObject,Float4sObject,QuatfsObject,
                                            BoolsObject,RgbaObject,AlphaObject>;
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
