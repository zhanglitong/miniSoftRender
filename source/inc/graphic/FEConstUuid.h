#pragma     once
#include    "../FEUuid.h"

namespace   FE
{
   
    class    FEConstUuid
    {
    public:
        inline  static  FEUuid  CameraUBOId     =   FEUuid::from("{D942F85B-D252-4238-92C0-7A13A3E02F12}");
        inline  static  FEUuid  LightsId        =   FEUuid::from("{DFA35753-3D7C-4579-B3E1-536B3EAC9ACD}");
        inline  static  FEUuid  ClipUBOId       =   FEUuid::from("{C0A5D268-6339-4CCF-9B36-2F32A48CF171}");
        inline  static  FEUuid  SkyUBOId        =   FEUuid::from("{BEAF0423-A4DF-461D-84DE-106C1B16AC5F}");
    };

}