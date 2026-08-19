#pragma     once

#include    "FEGraphicEnums.h"

namespace   FE
{
    class   FEInputSlotHelper
    {
    public:
        static  auto    enumOfName(FEInputSlot slot)
        {
            switch(slot)
            {
            case IS_VERTEX_POS           :  return  "IS_VERTEX_POS";
            case IS_VERTEX_NOR           :  return  "IS_VERTEX_NOR";
            case IS_VERTEX_COLOR0        :  return  "IS_VERTEX_COLOR0";
            case IS_VERTEX_COLOR1        :  return  "IS_VERTEX_COLOR1";
            case IS_VERTEX_LOD           :  return  "IS_VERTEX_LOD";
            case IS_VERTEX_TEXCOORD0     :  return  "IS_VERTEX_TEXCOORD0";
            case IS_VERTEX_TEXCOORD1     :  return  "IS_VERTEX_TEXCOORD1";
            case IS_VERTEX_TEXCOORD2     :  return  "IS_VERTEX_TEXCOORD2";
            case IS_VERTEX_TEXCOORD3     :  return  "IS_VERTEX_TEXCOORD3";
            case IS_VERTEX_TANGENT       :  return  "IS_VERTEX_TANGENT";
            case IS_VERTEX_BONE_INDEX    :  return  "IS_VERTEX_BONE_INDEX";
            case IS_VERTEX_BONE_WEIGHT   :  return  "IS_VERTEX_BONE_WEIGHT";
            case IS_VERTEX_TARGET0       :  return  "IS_VERTEX_TARGET0";
            case IS_VERTEX_TARGET1       :  return  "IS_VERTEX_TARGET1";
            case IS_VERTEX_TARGET2       :  return  "IS_VERTEX_TARGET2";
            case IS_VERTEX_TARGET3       :  return  "IS_VERTEX_TARGET3";
            case IS_INSTANCE_TRANSLATE   :  return  "IS_INSTANCE_TRANSLATE";
            case IS_INSTANCE_ITRANSLATE  :  return  "IS_INSTANCE_ITRANSLATE";
            case IS_INSTANCE_SCALE       :  return  "IS_INSTANCE_SCALE";
            case IS_INSTANCE_ROTATE      :  return  "IS_INSTANCE_ROTATE";
            case IS_INSTANCE_BOUNDSPHERE :  return  "IS_INSTANCE_BOUNDSPHERE";
            case IS_INSTANCE_INSTANCE    :  return  "IS_INSTANCE_INSTANCE";
            case IS_INSTANCE_LOD_INDEX   :  return  "IS_INSTANCE_LOD_INDEX";
            case IS_INSTANCE_FLAG        :  return  "IS_INSTANCE_FLAG";
            case IS_INSTANCE_MAT_C0      :  return  "IS_INSTANCE_MAT_C0";
            case IS_INSTANCE_MAT_C1      :  return  "IS_INSTANCE_MAT_C1";
            case IS_INSTANCE_MAT_C2      :  return  "IS_INSTANCE_MAT_C2";
            case IS_INSTANCE_MAT_C3      :  return  "IS_INSTANCE_MAT_C3";
            case IS_INSTANCE_COLOR       :  return  "IS_INSTANCE_COLOR";
            default:
                assert(0!=0);
                return  "";
            }
        }
        static  auto    enumFromName(const char* name)
        {
            if (_stricmp(name,enumOfName(IS_VERTEX_POS               )) == 0)   return  IS_VERTEX_POS           ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_NOR          )) == 0)   return  IS_VERTEX_NOR           ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_COLOR0       )) == 0)   return  IS_VERTEX_COLOR0        ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_COLOR1       )) == 0)   return  IS_VERTEX_COLOR1        ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_LOD          )) == 0)   return  IS_VERTEX_LOD           ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_TEXCOORD0    )) == 0)   return  IS_VERTEX_TEXCOORD0     ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_TEXCOORD1    )) == 0)   return  IS_VERTEX_TEXCOORD1     ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_TEXCOORD2    )) == 0)   return  IS_VERTEX_TEXCOORD2     ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_TEXCOORD3    )) == 0)   return  IS_VERTEX_TEXCOORD3     ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_TANGENT      )) == 0)   return  IS_VERTEX_TANGENT       ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_BONE_INDEX   )) == 0)   return  IS_VERTEX_BONE_INDEX    ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_BONE_WEIGHT  )) == 0)   return  IS_VERTEX_BONE_WEIGHT   ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_TARGET0      )) == 0)   return  IS_VERTEX_TARGET0       ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_TARGET1      )) == 0)   return  IS_VERTEX_TARGET1       ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_TARGET2      )) == 0)   return  IS_VERTEX_TARGET2       ;
            else if (_stricmp(name,enumOfName(IS_VERTEX_TARGET3      )) == 0)   return  IS_VERTEX_TARGET3       ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_TRANSLATE  )) == 0)   return  IS_INSTANCE_TRANSLATE   ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_ITRANSLATE )) == 0)   return  IS_INSTANCE_ITRANSLATE  ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_SCALE      )) == 0)   return  IS_INSTANCE_SCALE       ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_ROTATE     )) == 0)   return  IS_INSTANCE_ROTATE      ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_BOUNDSPHERE)) == 0)   return  IS_INSTANCE_BOUNDSPHERE ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_INSTANCE   )) == 0)   return  IS_INSTANCE_INSTANCE    ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_LOD_INDEX  )) == 0)   return  IS_INSTANCE_LOD_INDEX   ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_FLAG       )) == 0)   return  IS_INSTANCE_FLAG        ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_MAT_C0     )) == 0)   return  IS_INSTANCE_MAT_C0      ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_MAT_C1     )) == 0)   return  IS_INSTANCE_MAT_C1      ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_MAT_C2     )) == 0)   return  IS_INSTANCE_MAT_C2      ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_MAT_C3     )) == 0)   return  IS_INSTANCE_MAT_C3      ;
            else if (_stricmp(name,enumOfName(IS_INSTANCE_COLOR      )) == 0)   return  IS_INSTANCE_COLOR      ;
            else
                assert(0!=0);
            return  IS_VERTEX_POS;
            
        }

        static  auto    enumsOfNames(InputSlotBits slots)
        {       
            Strings     results;
            if (slots.hasFlag(IS_VERTEX_POS          )) results.push_back(enumOfName(IS_VERTEX_POS          ));
            if (slots.hasFlag(IS_VERTEX_NOR          )) results.push_back(enumOfName(IS_VERTEX_NOR          ));
            if (slots.hasFlag(IS_VERTEX_COLOR0       )) results.push_back(enumOfName(IS_VERTEX_COLOR0       ));
            if (slots.hasFlag(IS_VERTEX_COLOR1       )) results.push_back(enumOfName(IS_VERTEX_COLOR1       ));
            if (slots.hasFlag(IS_VERTEX_LOD         )) results.push_back(enumOfName(IS_VERTEX_LOD           ));
            if (slots.hasFlag(IS_VERTEX_TEXCOORD0    )) results.push_back(enumOfName(IS_VERTEX_TEXCOORD0    ));
            if (slots.hasFlag(IS_VERTEX_TEXCOORD1    )) results.push_back(enumOfName(IS_VERTEX_TEXCOORD1    ));
            if (slots.hasFlag(IS_VERTEX_TEXCOORD2    )) results.push_back(enumOfName(IS_VERTEX_TEXCOORD2    ));
            if (slots.hasFlag(IS_VERTEX_TEXCOORD3    )) results.push_back(enumOfName(IS_VERTEX_TEXCOORD3    ));
            if (slots.hasFlag(IS_VERTEX_BONE_INDEX   )) results.push_back(enumOfName(IS_VERTEX_BONE_INDEX   ));
            if (slots.hasFlag(IS_VERTEX_BONE_WEIGHT  )) results.push_back(enumOfName(IS_VERTEX_BONE_WEIGHT  ));
            if (slots.hasFlag(IS_VERTEX_TARGET0      )) results.push_back(enumOfName(IS_VERTEX_TARGET0      ));
            if (slots.hasFlag(IS_VERTEX_TARGET1      )) results.push_back(enumOfName(IS_VERTEX_TARGET1      ));
            if (slots.hasFlag(IS_VERTEX_TARGET2      )) results.push_back(enumOfName(IS_VERTEX_TARGET2      ));
            if (slots.hasFlag(IS_VERTEX_TARGET3      )) results.push_back(enumOfName(IS_VERTEX_TARGET3      ));
            if (slots.hasFlag(IS_INSTANCE_TRANSLATE  )) results.push_back(enumOfName(IS_INSTANCE_TRANSLATE  ));
            if (slots.hasFlag(IS_INSTANCE_ITRANSLATE )) results.push_back(enumOfName(IS_INSTANCE_ITRANSLATE ));
            if (slots.hasFlag(IS_INSTANCE_SCALE      )) results.push_back(enumOfName(IS_INSTANCE_SCALE      ));
            if (slots.hasFlag(IS_INSTANCE_ROTATE     )) results.push_back(enumOfName(IS_INSTANCE_ROTATE     ));
            if (slots.hasFlag(IS_INSTANCE_BOUNDSPHERE)) results.push_back(enumOfName(IS_INSTANCE_BOUNDSPHERE));
            if (slots.hasFlag(IS_INSTANCE_INSTANCE   )) results.push_back(enumOfName(IS_INSTANCE_INSTANCE   ));
            if (slots.hasFlag(IS_INSTANCE_LOD_INDEX  )) results.push_back(enumOfName(IS_INSTANCE_LOD_INDEX  ));
            if (slots.hasFlag(IS_INSTANCE_FLAG       )) results.push_back(enumOfName(IS_INSTANCE_FLAG       ));
            if (slots.hasFlag(IS_INSTANCE_MAT_C0     )) results.push_back(enumOfName(IS_INSTANCE_MAT_C0     ));
            if (slots.hasFlag(IS_INSTANCE_MAT_C1     )) results.push_back(enumOfName(IS_INSTANCE_MAT_C1     ));
            if (slots.hasFlag(IS_INSTANCE_MAT_C2     )) results.push_back(enumOfName(IS_INSTANCE_MAT_C2     ));
            if (slots.hasFlag(IS_INSTANCE_MAT_C3     )) results.push_back(enumOfName(IS_INSTANCE_MAT_C3     ));

            return  results;
        }
        static  auto    enumsFromNames(const Strings& names)
        {   
            InputSlotBits    slots;
            for (auto& var : names)
            {
                auto    flag    =   enumFromName(var.c_str());
                slots.addFlag(flag);
            }
            return  slots;
        }
    };
    
}
