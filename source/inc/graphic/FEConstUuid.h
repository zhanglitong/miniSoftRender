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
        /// 引擎全局 UBO(EngineState + 天空环境,绑定槽 SB_Global)
        inline  static  FEUuid  GlobalUBOId     =   FEUuid::from("{BEAF0423-A4DF-461D-84DE-106C1B16AC5F}");
        /// <summary>
        /// transmission 场景折射纹理(FEScene 注册,PBR 材质自取)
        /// </summary>
        inline  static  FEUuid  SceneTexId       =  FEUuid::from("{3F8A2C51-9E47-4B1D-A6C8-2D5F7B9E0C34}");
    };

}
