/// tool.resource.cc Compiler V1.0.0.0
/// tool.resource.cc Copyright 2018-2024 PR Inc All rights reserved !
/// Warning! All changes made in this file will be lost!
/// Build Time 2026-10-24 22:38:36.600

#include "../inc/FEAssetsMgr.h"
#pragma warning(push)
#pragma warning(disable:4828)




/// E:/miniSoftRender/bin/data/buildIn/readme.txt
namespace  FE
{
   unsigned int    _E6vminiSoftRendervbinvdatavbuildInvreadmeutxt_len      =   0;
   unsigned char   _E6vminiSoftRendervbinvdatavbuildInvreadmeutxt[]    =   
   { 
       0x00,
   };
};

/// helper function !
/// 
/// comments
namespace FE
{
   class   CLASS_coreufers
   {
   public:
       CLASS_coreufers()
       {
            FE::FEAssetsMgr::instance().addBuffer("buildIn/readme.txt",_E6vminiSoftRendervbinvdatavbuildInvreadmeutxt,_E6vminiSoftRendervbinvdatavbuildInvreadmeutxt_len,false);

       }
   };
   static  CLASS_coreufers    _coreufers;
}
#pragma warning(pop)
