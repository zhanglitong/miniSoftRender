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
        /// <summary>
        /// 场景默认进度条(常驻、默认隐藏)的对象 Id:
        /// 通过 FEScene::progressBar() 或 FEFactoryMgr::findObject(id) 查回后设置状态
        /// </summary>
        inline  static  FEUuid  ProgressBarId   =   FEUuid::from("{C41B7E5A-8D36-4F92-B0A7-6E9D2C58F1B3}");
        /// <summary>
        /// 系统默认纹理(设备缓存共享,全引擎只创建一份,只读)
        /// </summary>
        inline  static  FEUuid  DefaultWhiteTexId   =   FEUuid::from("{7B2E1C4A-5D3F-4A61-9C8E-1F2D3B4C5E70}");
        inline  static  FEUuid  DefaultNormalTexId  =   FEUuid::from("{8C3F2D5B-6E4A-4B72-AD9F-2A3E4C5D6F81}");
        inline  static  FEUuid  DefaultBlackTexId   =   FEUuid::from("{9D4A3E6C-7F5B-4C83-BEA0-3B4F5D6E7A92}");
        inline  static  FEUuid  DefaultBRDFTexId    =   FEUuid::from("{AE5B4F7D-8A6C-4D94-CFB1-4C5A6E7F8BA3}");
        inline  static  FEUuid  DefaultBlackCubeId  =   FEUuid::from("{BF6C5A8E-9B7D-4EA5-DAC2-5D6B7F8A9CB4}");
    };

}
