#pragma     once
#include    "FEFormat.hpp"

namespace   FE
{
    class   FEFormatHelper
    {
    protected:
        FEFormat    _format;
    public:
        FEFormatHelper(FEFormat fmt)
            :_format(fmt)
        {}
    public:
        /// <summary>
        /// 获取格式需要的内存大小，单位字节
        /// </summary>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  uint8_t     sizeOf(const FEFormat& fmt)
        {
            switch(fmt)
            {
            case FMT_R8_UNORM       :
            case FMT_R8_SNORM       :
            case FMT_R8_USCALED     :
            case FMT_R8_SSCALED     :
            case FMT_R8_UINT        :
            case FMT_R8_SINT        :
            case FMT_R4G4_UNORM     :
                return  1 * sizeof(int8_t);

            case FMT_R8G8_UNORM      :
            case FMT_R8G8_SNORM      :
            case FMT_R8G8_USCALED    :
            case FMT_R8G8_SSCALED    :
            case FMT_R8G8_UINT       :
            case FMT_R8G8_SINT       :
            case FMT_R4G4B4A4_UNORM :
            case FMT_B4G4R4A4_UNORM :
            case FMT_R5G6B5_UNORM   :
            case FMT_B5G6R5_UNORM   :
            case FMT_R5G5B5A1_UNORM :
            case FMT_B5G5R5A1_UNORM :
            case FMT_A1R5G5B5_UNORM :
                return  2 * sizeof(int8_t);

            case FMT_R8G8B8_UNORM   :
            case FMT_R8G8B8_SNORM   :
            case FMT_R8G8B8_USCALED :
            case FMT_R8G8B8_SSCALED :
            case FMT_R8G8B8_UINT    :
            case FMT_R8G8B8_SINT    :
                return  3 * sizeof(int8_t);

            case FMT_R8G8B8A8_UNORM    :  
            case FMT_R8G8B8A8_SNORM    :  
            case FMT_R8G8B8A8_USCALED  :  
            case FMT_R8G8B8A8_SSCALED  :  
            case FMT_R8G8B8A8_UINT     :  
            case FMT_R8G8B8A8_SINT     :  
                return  4 * sizeof(int8_t);
            case FMT_A2B10G10R10_UNORM   :
            case FMT_A2B10G10R10_SNORM   :
            case FMT_A2B10G10R10_USCALED :
            case FMT_A2B10G10R10_SSCALED :
            case FMT_A2B10G10R10_UINT    :
            case FMT_A2B10G10R10_SINT    :
                return  sizeof(int32_t);

            case FMT_R16_UNORM  :
            case FMT_R16_SNORM  :
            case FMT_R16_USCALED:
            case FMT_R16_SSCALED:
            case FMT_R16_UINT   :
            case FMT_R16_SINT   :
                return  sizeof(int16_t);
            case FMT_R16G16_UNORM  :
            case FMT_R16G16_SNORM  :
            case FMT_R16G16_USCALED:
            case FMT_R16G16_SSCALED:
            case FMT_R16G16_UINT   :
            case FMT_R16G16_SINT   :
                return  2 * sizeof(int16_t);
            case FMT_R16G16B16_UNORM   :  
            case FMT_R16G16B16_SNORM   :  
            case FMT_R16G16B16_USCALED :  
            case FMT_R16G16B16_SSCALED :  
            case FMT_R16G16B16_UINT    :  
            case FMT_R16G16B16_SINT    :  
                return 3 * sizeof(int16_t);
            case FMT_R16G16B16A16_UNORM   :  
            case FMT_R16G16B16A16_SNORM   :  
            case FMT_R16G16B16A16_USCALED :  
            case FMT_R16G16B16A16_SSCALED :  
            case FMT_R16G16B16A16_UINT    :  
            case FMT_R16G16B16A16_SINT    :  
                return 4 * sizeof(int16_t);

            case FMT_R32_UINT   :
            case FMT_R32_SINT   :
                return 1 * sizeof(int32_t);
            case FMT_R32G32_UINT  :
            case FMT_R32G32_SINT  :
                return 2 * sizeof(int32_t); 
            case FMT_R32G32B32_UINT :
            case FMT_R32G32B32_SINT :
                return 3 * sizeof(int32_t); 
            case FMT_R32G32B32A32_UINT:
            case FMT_R32G32B32A32_SINT:
                return  4 * sizeof(int32_t);
            case FMT_R16_FLOAT   :
                return  1 * sizeof(float16);
            case FMT_R16G16_FLOAT  :
                return  2 * sizeof(float16);
            case FMT_R16G16B16_FLOAT :
                return  3 * sizeof(float16);
            case FMT_R16G16B16A16_FLOAT:
                return  4 * sizeof(float16);
            case FMT_R32_FLOAT:
                return  1 * sizeof(float);
            case FMT_R32G32_FLOAT:
                return  2 * sizeof(float);
            case FMT_R32G32B32_FLOAT :
                return  3 * sizeof(float);
            case FMT_R32G32B32A32_FLOAT:
                return  4 * sizeof(float);
            case FMT_D16_UNORM  :
                return  1 * sizeof(float16);
            case FMT_D32_UNORM :
                return  1 * sizeof(float32);
            }
            return  1;
        }
        /// <summary>
        /// 获取颜色的通道数，R:1,RG:2,RGB:3,RGBA:4;
        /// </summary>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  uint8_t     channelOf(const FEFormat& fmt)
        {
            switch(fmt)
            {
            case FMT_R8_UNORM       :
            case FMT_R8_SNORM       :
            case FMT_R8_USCALED     :
            case FMT_R8_SSCALED     :
            case FMT_R8_UINT        :
            case FMT_R8_SINT        :
                return  1;
            case FMT_R4G4_UNORM     :
                return  2;

            case FMT_R8G8_UNORM         :
            case FMT_R8G8_SNORM         :
            case FMT_R8G8_USCALED       :
            case FMT_R8G8_SSCALED       :
            case FMT_R8G8_UINT          :
            case FMT_R8G8_SINT          :
            case FMT_R4G4B4A4_UNORM     :
            case FMT_B4G4R4A4_UNORM     :
            case FMT_R5G6B5_UNORM       :
            case FMT_B5G6R5_UNORM       :
            case FMT_R5G5B5A1_UNORM     :
            case FMT_B5G5R5A1_UNORM     :
            case FMT_A1R5G5B5_UNORM     :
                return  2;

            case FMT_R8G8B8_UNORM       :
            case FMT_R8G8B8_SNORM       :
            case FMT_R8G8B8_USCALED     :
            case FMT_R8G8B8_SSCALED     :
            case FMT_R8G8B8_UINT        :
            case FMT_R8G8B8_SINT        :
                return  3;

            case FMT_R8G8B8A8_UNORM     :  
            case FMT_R8G8B8A8_SNORM     :  
            case FMT_R8G8B8A8_USCALED   :  
            case FMT_R8G8B8A8_SSCALED   :  
            case FMT_R8G8B8A8_UINT      :  
            case FMT_R8G8B8A8_SINT      :  
                return  4;
            case FMT_A2B10G10R10_UNORM   :
            case FMT_A2B10G10R10_SNORM   :
            case FMT_A2B10G10R10_USCALED :
            case FMT_A2B10G10R10_SSCALED :
            case FMT_A2B10G10R10_UINT    :
            case FMT_A2B10G10R10_SINT    :
                return  4;

            case FMT_R16_UNORM  :
            case FMT_R16_SNORM  :
            case FMT_R16_USCALED:
            case FMT_R16_SSCALED:
            case FMT_R16_UINT   :
            case FMT_R16_SINT   :
                return  1;
            case FMT_R16G16_UNORM  :
            case FMT_R16G16_SNORM  :
            case FMT_R16G16_USCALED:
            case FMT_R16G16_SSCALED:
            case FMT_R16G16_UINT   :
            case FMT_R16G16_SINT   :
                return  2;
            case FMT_R16G16B16_UNORM   :  
            case FMT_R16G16B16_SNORM   :  
            case FMT_R16G16B16_USCALED :  
            case FMT_R16G16B16_SSCALED :  
            case FMT_R16G16B16_UINT    :  
            case FMT_R16G16B16_SINT    :  
                return 3;
            case FMT_R16G16B16A16_UNORM   :  
            case FMT_R16G16B16A16_SNORM   :  
            case FMT_R16G16B16A16_USCALED :  
            case FMT_R16G16B16A16_SSCALED :  
            case FMT_R16G16B16A16_UINT    :  
            case FMT_R16G16B16A16_SINT    :  
                return 4;

            case FMT_R32_UINT   :
            case FMT_R32_SINT   :
                return 1;
            case FMT_R32G32_UINT  :
            case FMT_R32G32_SINT  :
                return 2; 
            case FMT_R32G32B32_UINT :
            case FMT_R32G32B32_SINT :
                return 3; 
            case FMT_R32G32B32A32_UINT:
            case FMT_R32G32B32A32_SINT:
                return  4;
            case FMT_R16_FLOAT   :
                return  1;
            case FMT_R16G16_FLOAT  :
                return  2;
            case FMT_R16G16B16_FLOAT :
                return  3;
            case FMT_R16G16B16A16_FLOAT:
                return  4;
            case FMT_R32_FLOAT:
                return  1;
            case FMT_R32G32_FLOAT:
                return  2;
            case FMT_R32G32B32_FLOAT :
                return  3;
            case FMT_R32G32B32A32_FLOAT:
                return  4;
            case FMT_D16_UNORM  :
                return  1;
            case FMT_D32_UNORM :
                return  1;
            }
            return  1;
        }
        /// <summary>
        /// 获取名称
        /// </summary>
        /// <param name="fmt"></param>
        /// <returns></returns>
        static  auto        enumName(FEFormat fmt)
        {
            switch(fmt)
            {
            case FMT_R8_UNORM               :  return  "R8_UNORM";
            case FMT_R8_SNORM               :  return  "R8_SNORM";
            case FMT_R8_USCALED             :  return  "R8_USCALED";
            case FMT_R8_SSCALED             :  return  "R8_SSCALED";
            case FMT_R8_UINT                :  return  "R8_UINT";
            case FMT_R8_SINT                :  return  "R8_SINT";
            case FMT_R4G4_UNORM             :  return  "R4G4_UNORM";

            case FMT_R8G8_UNORM             :  return  "RG8_UNORM";
            case FMT_R8G8_SNORM             :  return  "RG8_SNORM";
            case FMT_R8G8_USCALED           :  return  "RG8_USCALED";
            case FMT_R8G8_SSCALED           :  return  "RG8_SSCALED";
            case FMT_R8G8_UINT              :  return  "RG8_UINT";
            case FMT_R8G8_SINT              :  return  "RG8_SINT";

            case FMT_R4G4B4A4_UNORM         :  return  "R4G4B4A4_UNORM";
            case FMT_B4G4R4A4_UNORM         :  return  "B4G4R4A4_UNORM";
            case FMT_R5G6B5_UNORM           :  return  "R5G6B5_UNORM";
            case FMT_B5G6R5_UNORM           :  return  "B5G6R5_UNORM";
            case FMT_R5G5B5A1_UNORM         :  return  "R5G5B5A1_UNORM";
            case FMT_B5G5R5A1_UNORM         :  return  "B5G5R5A1_UNORM";
            case FMT_A1R5G5B5_UNORM         :  return  "A1R5G5B5_UNORM";

            case FMT_R8G8B8_UNORM           :  return  "R8G8B8_UNORM";
            case FMT_R8G8B8_SNORM           :  return  "R8G8B8_SNORM";
            case FMT_R8G8B8_USCALED         :  return  "R8G8B8_USCALED";
            case FMT_R8G8B8_SSCALED         :  return  "R8G8B8_SSCALED";
            case FMT_R8G8B8_UINT            :  return  "R8G8B8_UINT";
            case FMT_R8G8B8_SINT            :  return  "R8G8B8_SINT";

            case FMT_R8G8B8A8_UNORM         :  return  "R8G8B8A8_UNORM";
            case FMT_R8G8B8A8_SNORM         :  return  "R8G8B8A8_SNORM";
            case FMT_R8G8B8A8_USCALED       :  return  "R8G8B8A8_USCALED";
            case FMT_R8G8B8A8_SSCALED       :  return  "R8G8B8A8_SSCALED";
            case FMT_R8G8B8A8_UINT          :  return  "R8G8B8A8_UINT";
            case FMT_R8G8B8A8_SINT          :  return  "R8G8B8A8_SINT";

            case FMT_A2B10G10R10_UNORM      :  return  "A2B10G10R10_UNORM";
            case FMT_A2B10G10R10_SNORM      :  return  "A2B10G10R10_SNORM";
            case FMT_A2B10G10R10_USCALED    :  return  "A2B10G10R10_USCALED";
            case FMT_A2B10G10R10_SSCALED    :  return  "A2B10G10R10_SSCALED";
            case FMT_A2B10G10R10_UINT       :  return  "A2B10G10R10_UINT";
            case FMT_A2B10G10R10_SINT       :  return  "A2B10G10R10_SINT";

            case FMT_R16_UNORM              :  return  "R16_UNORM";
            case FMT_R16_SNORM              :  return  "R16_SNORM";
            case FMT_R16_USCALED            :  return  "R16_USCALED";
            case FMT_R16_SSCALED            :  return  "R16_SSCALED";
            case FMT_R16_UINT               :  return  "R16_UINT";
            case FMT_R16_SINT               :  return  "R16_SINT";

            case FMT_R16G16_UNORM           :  return  "R16G16_UNORM";
            case FMT_R16G16_SNORM           :  return  "R16G16_SNORM";
            case FMT_R16G16_USCALED         :  return  "R16G16_USCALED";
            case FMT_R16G16_SSCALED         :  return  "R16G16_SSCALED";
            case FMT_R16G16_UINT            :  return  "R16G16_UINT";
            case FMT_R16G16_SINT            :  return  "R16G16_SINT";

            case FMT_R16G16B16_UNORM        :  return  "R16G16B16_UNORM";
            case FMT_R16G16B16_SNORM        :  return  "R16G16B16_SNORM";
            case FMT_R16G16B16_USCALED      :  return  "R16G16B16_USCALED";
            case FMT_R16G16B16_SSCALED      :  return  "R16G16B16_SSCALED";
            case FMT_R16G16B16_UINT         :  return  "R16G16B16_UINT";
            case FMT_R16G16B16_SINT         :  return  "R16G16B16_SINT";

            case  FMT_R16G16B16A16_UNORM    :  return  "R16G16B16A16_UNORM";
            case  FMT_R16G16B16A16_SNORM    :  return  "R16G16B16A16_SNORM";
            case  FMT_R16G16B16A16_USCALED  :  return  "R16G16B16A16_USCALED";
            case  FMT_R16G16B16A16_SSCALED  :  return  "R16G16B16A16_SSCALED";
            case  FMT_R16G16B16A16_UINT     :  return  "R16G16B16A16_UINT";
            case  FMT_R16G16B16A16_SINT     :  return  "R16G16B16A16_SINT";

            case FMT_R32_UINT               :  return  "R32_UINT";
            case FMT_R32_SINT               :  return  "R32_SINT";

            case FMT_R32G32_UINT            :  return  "R32G32_UINT";
            case FMT_R32G32_SINT            :  return  "R32G32_SINT";

            case FMT_R32G32B32_UINT         :  return  "R32G32B32_UINT";
            case FMT_R32G32B32_SINT         :  return  "R32G32B32_SINT";

            case FMT_R32G32B32A32_UINT      :  return  "R32G32B32A32_UINT";
            case FMT_R32G32B32A32_SINT      :  return  "R32G32B32A32_SINT";

            case FMT_R16_FLOAT              :  return  "R16_FLOAT";
            case FMT_R16G16_FLOAT           :  return  "R16G16_FLOAT";
            case FMT_R16G16B16_FLOAT        :  return  "R16G16B16_FLOAT";
            case FMT_R16G16B16A16_FLOAT     :  return  "R16G16B16A16_FLOAT";

            case FMT_R32_FLOAT              :  return  "R32_FLOAT";
            case FMT_R32G32_FLOAT           :  return  "R32G32_FLOAT";
            case FMT_R32G32B32_FLOAT        :  return  "R32G32B32_FLOAT";
            case FMT_R32G32B32A32_FLOAT     :  return  "R32G32B32A32_FLOAT";

            case FMT_D16_UNORM              :  return  "D16_UNORM";
            case FMT_D32_UNORM              :  return  "D32_UNORM";
            }
            assert(0!=0);
            return  "NONE";
        }
        /// <summary>
        /// 根据名称获取格式
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        static  FEFormat    formatFromName(const char* name)
        {
            if (_stricmp(enumName(      FMT_R8_UNORM            ),    name) == 0)   return  FMT_R8_UNORM            ;
            else if (_stricmp(enumName( FMT_R8_SNORM            ),    name) == 0)   return  FMT_R8_SNORM            ;
            else if (_stricmp(enumName( FMT_R8_USCALED          ),    name) == 0)   return  FMT_R8_USCALED          ;
            else if (_stricmp(enumName( FMT_R8_SSCALED          ),    name) == 0)   return  FMT_R8_SSCALED          ;
            else if (_stricmp(enumName( FMT_R8_UINT             ),    name) == 0)   return  FMT_R8_UINT             ;
            else if (_stricmp(enumName( FMT_R8_SINT             ),    name) == 0)   return  FMT_R8_SINT             ;
            else if (_stricmp(enumName( FMT_R4G4_UNORM          ),    name) == 0)   return  FMT_R4G4_UNORM          ;
            else if (_stricmp(enumName( FMT_R8G8_UNORM          ),    name) == 0)   return  FMT_R8G8_UNORM          ;
            else if (_stricmp(enumName( FMT_R8G8_SNORM          ),    name) == 0)   return  FMT_R8G8_SNORM          ;
            else if (_stricmp(enumName( FMT_R8G8_USCALED        ),    name) == 0)   return  FMT_R8G8_USCALED        ;
            else if (_stricmp(enumName( FMT_R8G8_SSCALED        ),    name) == 0)   return  FMT_R8G8_SSCALED        ;
            else if (_stricmp(enumName( FMT_R8G8_UINT           ),    name) == 0)   return  FMT_R8G8_UINT           ;
            else if (_stricmp(enumName( FMT_R8G8_SINT           ),    name) == 0)   return  FMT_R8G8_SINT           ;
            else if (_stricmp(enumName( FMT_R4G4B4A4_UNORM      ),    name) == 0)   return  FMT_R4G4B4A4_UNORM      ;
            else if (_stricmp(enumName( FMT_B4G4R4A4_UNORM      ),    name) == 0)   return  FMT_B4G4R4A4_UNORM      ;
            else if (_stricmp(enumName( FMT_R5G6B5_UNORM        ),    name) == 0)   return  FMT_R5G6B5_UNORM        ;
            else if (_stricmp(enumName( FMT_B5G6R5_UNORM        ),    name) == 0)   return  FMT_B5G6R5_UNORM        ;
            else if (_stricmp(enumName( FMT_R5G5B5A1_UNORM      ),    name) == 0)   return  FMT_R5G5B5A1_UNORM      ;
            else if (_stricmp(enumName( FMT_B5G5R5A1_UNORM      ),    name) == 0)   return  FMT_B5G5R5A1_UNORM      ;
            else if (_stricmp(enumName( FMT_A1R5G5B5_UNORM      ),    name) == 0)   return  FMT_A1R5G5B5_UNORM      ;
            else if (_stricmp(enumName( FMT_R8G8B8_UNORM        ),    name) == 0)   return  FMT_R8G8B8_UNORM        ;
            else if (_stricmp(enumName( FMT_R8G8B8_SNORM        ),    name) == 0)   return  FMT_R8G8B8_SNORM        ;
            else if (_stricmp(enumName( FMT_R8G8B8_USCALED      ),    name) == 0)   return  FMT_R8G8B8_USCALED      ;
            else if (_stricmp(enumName( FMT_R8G8B8_SSCALED      ),    name) == 0)   return  FMT_R8G8B8_SSCALED      ;
            else if (_stricmp(enumName( FMT_R8G8B8_UINT         ),    name) == 0)   return  FMT_R8G8B8_UINT         ;
            else if (_stricmp(enumName( FMT_R8G8B8_SINT         ),    name) == 0)   return  FMT_R8G8B8_SINT         ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_UNORM      ),    name) == 0)   return  FMT_R8G8B8A8_UNORM      ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_SNORM      ),    name) == 0)   return  FMT_R8G8B8A8_SNORM      ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_USCALED    ),    name) == 0)   return  FMT_R8G8B8A8_USCALED    ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_SSCALED    ),    name) == 0)   return  FMT_R8G8B8A8_SSCALED    ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_UINT       ),    name) == 0)   return  FMT_R8G8B8A8_UINT       ;
            else if (_stricmp(enumName( FMT_R8G8B8A8_SINT       ),    name) == 0)   return  FMT_R8G8B8A8_SINT       ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_UNORM   ),    name) == 0)   return  FMT_A2B10G10R10_UNORM   ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_SNORM   ),    name) == 0)   return  FMT_A2B10G10R10_SNORM   ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_USCALED ),    name) == 0)   return  FMT_A2B10G10R10_USCALED ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_SSCALED ),    name) == 0)   return  FMT_A2B10G10R10_SSCALED ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_UINT    ),    name) == 0)   return  FMT_A2B10G10R10_UINT    ;
            else if (_stricmp(enumName( FMT_A2B10G10R10_SINT    ),    name) == 0)   return  FMT_A2B10G10R10_SINT    ;
            else if (_stricmp(enumName( FMT_R16_UNORM           ),    name) == 0)   return  FMT_R16_UNORM           ;
            else if (_stricmp(enumName( FMT_R16_SNORM           ),    name) == 0)   return  FMT_R16_SNORM           ;
            else if (_stricmp(enumName( FMT_R16_USCALED         ),    name) == 0)   return  FMT_R16_USCALED         ;
            else if (_stricmp(enumName( FMT_R16_SSCALED         ),    name) == 0)   return  FMT_R16_SSCALED         ;
            else if (_stricmp(enumName( FMT_R16_UINT            ),    name) == 0)   return  FMT_R16_UINT            ;
            else if (_stricmp(enumName( FMT_R16_SINT            ),    name) == 0)   return  FMT_R16_SINT            ;
            else if (_stricmp(enumName( FMT_R16G16_UNORM        ),    name) == 0)   return  FMT_R16G16_UNORM        ;
            else if (_stricmp(enumName( FMT_R16G16_SNORM        ),    name) == 0)   return  FMT_R16G16_SNORM        ;
            else if (_stricmp(enumName( FMT_R16G16_USCALED      ),    name) == 0)   return  FMT_R16G16_USCALED      ;
            else if (_stricmp(enumName( FMT_R16G16_SSCALED      ),    name) == 0)   return  FMT_R16G16_SSCALED      ;
            else if (_stricmp(enumName( FMT_R16G16_UINT         ),    name) == 0)   return  FMT_R16G16_UINT         ;
            else if (_stricmp(enumName( FMT_R16G16_SINT         ),    name) == 0)   return  FMT_R16G16_SINT         ;
            else if (_stricmp(enumName( FMT_R16G16B16_UNORM     ),    name) == 0)   return  FMT_R16G16B16_UNORM     ;
            else if (_stricmp(enumName( FMT_R16G16B16_SNORM     ),    name) == 0)   return  FMT_R16G16B16_SNORM     ;
            else if (_stricmp(enumName( FMT_R16G16B16_USCALED   ),    name) == 0)   return  FMT_R16G16B16_USCALED   ;
            else if (_stricmp(enumName( FMT_R16G16B16_SSCALED   ),    name) == 0)   return  FMT_R16G16B16_SSCALED   ;
            else if (_stricmp(enumName( FMT_R16G16B16_UINT      ),    name) == 0)   return  FMT_R16G16B16_UINT      ;
            else if (_stricmp(enumName( FMT_R16G16B16_SINT      ),    name) == 0)   return  FMT_R16G16B16_SINT      ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_UNORM  ),    name) == 0)   return  FMT_R16G16B16A16_UNORM  ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_SNORM  ),    name) == 0)   return  FMT_R16G16B16A16_SNORM  ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_USCALED),    name) == 0)   return  FMT_R16G16B16A16_USCALED;
            else if (_stricmp(enumName( FMT_R16G16B16A16_SSCALED),    name) == 0)   return  FMT_R16G16B16A16_SSCALED;
            else if (_stricmp(enumName( FMT_R16G16B16A16_UINT   ),    name) == 0)   return  FMT_R16G16B16A16_UINT   ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_SINT   ),    name) == 0)   return  FMT_R16G16B16A16_SINT   ;
            else if (_stricmp(enumName( FMT_R32_UINT            ),    name) == 0)   return  FMT_R32_UINT            ;
            else if (_stricmp(enumName( FMT_R32_SINT            ),    name) == 0)   return  FMT_R32_SINT            ;
            else if (_stricmp(enumName( FMT_R32G32_UINT         ),    name) == 0)   return  FMT_R32G32_UINT         ;
            else if (_stricmp(enumName( FMT_R32G32_SINT         ),    name) == 0)   return  FMT_R32G32_SINT         ;
            else if (_stricmp(enumName( FMT_R32G32B32_UINT      ),    name) == 0)   return  FMT_R32G32B32_UINT      ;
            else if (_stricmp(enumName( FMT_R32G32B32_SINT      ),    name) == 0)   return  FMT_R32G32B32_SINT      ;
            else if (_stricmp(enumName( FMT_R32G32B32A32_UINT   ),    name) == 0)   return  FMT_R32G32B32A32_UINT   ;
            else if (_stricmp(enumName( FMT_R32G32B32A32_SINT   ),    name) == 0)   return  FMT_R32G32B32A32_SINT   ;
            else if (_stricmp(enumName( FMT_R16_FLOAT           ),    name) == 0)   return  FMT_R16_FLOAT           ;
            else if (_stricmp(enumName( FMT_R16G16_FLOAT        ),    name) == 0)   return  FMT_R16G16_FLOAT        ;
            else if (_stricmp(enumName( FMT_R16G16B16_FLOAT     ),    name) == 0)   return  FMT_R16G16B16_FLOAT     ;
            else if (_stricmp(enumName( FMT_R16G16B16A16_FLOAT  ),    name) == 0)   return  FMT_R16G16B16A16_FLOAT  ;
            else if (_stricmp(enumName( FMT_R32_FLOAT           ),    name) == 0)   return  FMT_R32_FLOAT           ;
            else if (_stricmp(enumName( FMT_R32G32_FLOAT        ),    name) == 0)   return  FMT_R32G32_FLOAT        ;
            else if (_stricmp(enumName( FMT_R32G32B32_FLOAT     ),    name) == 0)   return  FMT_R32G32B32_FLOAT     ;
            else if (_stricmp(enumName( FMT_R32G32B32A32_FLOAT  ),    name) == 0)   return  FMT_R32G32B32A32_FLOAT  ;
            else if (_stricmp(enumName( FMT_D16_UNORM           ),    name) == 0)   return  FMT_D16_UNORM           ;
            else if (_stricmp(enumName( FMT_D32_UNORM           ),    name) == 0)   return  FMT_D32_UNORM           ;
            assert(0!=0);
            return  FMT_NONE                ;
        }
        
        /// <summary>
        /// 计算一行数据(w个像素)占用的内存空间大小(单位字节)
        /// </summary>
        /// <param name="w">宽度</param>
        /// <param name="align">对齐字节数1,2,4,8,16,32,64...</param>
        /// <param name="fmt">像素格式</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32_t    pitch(uint32_t w,uint16_t align,FEFormat fmt)
        {
            return  (w * FEFormatHelper::sizeOf(fmt) + (align - 1)) / align * align;
        }
        /// <summary>
        /// 计算一行数据(w个像素)占用的内存空间大小(单位字节)
        /// </summary>
        /// <param name="w"></param>
        /// <param name="align">对齐字节数1,2,4,8,16,32,64...</param>
        /// <param name="pixelByte">每个像素的字节数</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32_t    pitch(uint32_t w,uint16_t align,uint16_t pixelByte)
        {
            return  (w * pixelByte + (align - 1)) / align * align;
        }
        /// <summary>
        /// 根据图像的大小,mipmap,对齐方式,数据格式,计算需要的空间字节数
        /// </summary>
        /// <param name="extent">长宽高</param>
        /// <param name="levels">mipmap</param>
        /// <param name="align">对齐1,2,4,8,16,32,64...</param>
        /// <param name="fmt">格式</param>
        /// <returns>所需空间大小，单位字节</returns>
        static  uint32_t    bytesOfLayer(const uint3& extent,uint32_t levels,uint32_t align,FEFormat fmt)
        {
            uint32_t    mips    =   (std::max<uint32_t>)(1,levels);
            uint32_t    nByte   =   0;
            for (uint32_t m = 0; m < mips; ++m)
            {
                const auto    w     =   (std::max<uint32_t>)(extent.x >> m,1);
                const auto    h     =   (std::max<uint32_t>)(extent.y >> m,1);
                const auto    d     =   (std::max<uint32_t>)(extent.z >> m,1);
                nByte               +=   pitch(w,align,fmt) * h * d;
            }
            return  nByte;
        }
    };
    template<FEFormat fmt>
    struct  TypeOfFormat
    {};

    template<>  struct TypeOfFormat<FMT_R8_UNORM            >   { using type    =   uint8_t;    };
    template<>  struct TypeOfFormat<FMT_R8_SNORM            >   { using type    =   int8_t;     };
    template<>  struct TypeOfFormat<FMT_R8_USCALED          >   { using type    =   uint8_t;    };
    template<>  struct TypeOfFormat<FMT_R8_SSCALED          >   { using type    =   int8_t;     };
    template<>  struct TypeOfFormat<FMT_R8_UINT             >   { using type    =   uint8_t;    };
    template<>  struct TypeOfFormat<FMT_R8_SINT             >   { using type    =   int8_t;     };
    template<>  struct TypeOfFormat<FMT_R4G4_UNORM          >   { using type    =   uint8_t;    };

    template<>  struct TypeOfFormat<FMT_R8G8_UNORM          >   { using type    =   uint8x2;    };
    template<>  struct TypeOfFormat<FMT_R8G8_SNORM          >   { using type    =   int8x2;     };
    template<>  struct TypeOfFormat<FMT_R8G8_USCALED        >   { using type    =   uint8x2;    };
    template<>  struct TypeOfFormat<FMT_R8G8_SSCALED        >   { using type    =   int8x2;     };
    template<>  struct TypeOfFormat<FMT_R8G8_UINT           >   { using type    =   uint8x2;    };
    template<>  struct TypeOfFormat<FMT_R8G8_SINT           >   { using type    =   int8x2;     };

    template<>  struct TypeOfFormat<FMT_R4G4B4A4_UNORM      >   { using type    =   uint16_t;   };
    template<>  struct TypeOfFormat<FMT_B4G4R4A4_UNORM      >   { using type    =   uint16_t;   };
    template<>  struct TypeOfFormat<FMT_R5G6B5_UNORM        >   { using type    =   uint16_t;   };
    template<>  struct TypeOfFormat<FMT_B5G6R5_UNORM        >   { using type    =   uint16_t;   };
    template<>  struct TypeOfFormat<FMT_R5G5B5A1_UNORM      >   { using type    =   uint16_t;   };
    template<>  struct TypeOfFormat<FMT_B5G5R5A1_UNORM      >   { using type    =   uint16_t;   };
    template<>  struct TypeOfFormat<FMT_A1R5G5B5_UNORM      >   { using type    =   uint16_t;   };

    template<>  struct TypeOfFormat<FMT_R8G8B8_UNORM        >   { using type    =   uint8x3;    };
    template<>  struct TypeOfFormat<FMT_R8G8B8_SNORM        >   { using type    =   int8x3;     };
    template<>  struct TypeOfFormat<FMT_R8G8B8_USCALED      >   { using type    =   uint8x3;    };
    template<>  struct TypeOfFormat<FMT_R8G8B8_SSCALED      >   { using type    =   int8x3;     };
    template<>  struct TypeOfFormat<FMT_R8G8B8_UINT         >   { using type    =   uint8x3;    };
    template<>  struct TypeOfFormat<FMT_R8G8B8_SINT         >   { using type    =   int8x3;     };

    template<>  struct TypeOfFormat<FMT_R8G8B8A8_UNORM      >   { using type    =   uint8x4;    };
    template<>  struct TypeOfFormat<FMT_R8G8B8A8_SNORM      >   { using type    =   int8x4;     };
    template<>  struct TypeOfFormat<FMT_R8G8B8A8_USCALED    >   { using type    =   uint8x4;    };
    template<>  struct TypeOfFormat<FMT_R8G8B8A8_SSCALED    >   { using type    =   int8x4;     };
    template<>  struct TypeOfFormat<FMT_R8G8B8A8_UINT       >   { using type    =   uint8x4;    };
    template<>  struct TypeOfFormat<FMT_R8G8B8A8_SINT       >   { using type    =   int8x4;     };

    template<>  struct TypeOfFormat<FMT_A2B10G10R10_UNORM   >   { using type    =   uint32_t;   };
    template<>  struct TypeOfFormat<FMT_A2B10G10R10_SNORM   >   { using type    =   uint32_t;   };
    template<>  struct TypeOfFormat<FMT_A2B10G10R10_USCALED >   { using type    =   uint32_t;   };
    template<>  struct TypeOfFormat<FMT_A2B10G10R10_SSCALED >   { using type    =   uint32_t;   };
    template<>  struct TypeOfFormat<FMT_A2B10G10R10_UINT    >   { using type    =   uint32_t;   };
    template<>  struct TypeOfFormat<FMT_A2B10G10R10_SINT    >   { using type    =   uint32_t;   };

    template<>  struct TypeOfFormat<FMT_R16_UNORM           >   { using type    =   uint16_t;   };
    template<>  struct TypeOfFormat<FMT_R16_SNORM           >   { using type    =   int16_t ;   };
    template<>  struct TypeOfFormat<FMT_R16_USCALED         >   { using type    =   uint16_t;   };
    template<>  struct TypeOfFormat<FMT_R16_SSCALED         >   { using type    =   int16_t ;   };
    template<>  struct TypeOfFormat<FMT_R16_UINT            >   { using type    =   uint16_t;   };
    template<>  struct TypeOfFormat<FMT_R16_SINT            >   { using type    =   int16_t ;   };

    template<>  struct TypeOfFormat<FMT_R16G16_UNORM        >   { using type    =   uint16x2;   };
    template<>  struct TypeOfFormat<FMT_R16G16_SNORM        >   { using type    =   int16x2 ;   };
    template<>  struct TypeOfFormat<FMT_R16G16_USCALED      >   { using type    =   uint16x2;   };
    template<>  struct TypeOfFormat<FMT_R16G16_SSCALED      >   { using type    =   int16x2 ;   };
    template<>  struct TypeOfFormat<FMT_R16G16_UINT         >   { using type    =   uint16x2;   };
    template<>  struct TypeOfFormat<FMT_R16G16_SINT         >   { using type    =   int16x2 ;   };

    template<>  struct TypeOfFormat<FMT_R16G16B16_UNORM     >   { using type    =   uint16x3;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16_SNORM     >   { using type    =   int16x3 ;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16_USCALED   >   { using type    =   uint16x3;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16_SSCALED   >   { using type    =   int16x3 ;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16_UINT      >   { using type    =   uint16x3;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16_SINT      >   { using type    =   int16x3 ;   };

    template<>  struct TypeOfFormat<FMT_R16G16B16A16_UNORM  >   { using type    =   uint16x4;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16A16_SNORM  >   { using type    =   int16x4 ;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16A16_USCALED>   { using type    =   uint16x4;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16A16_SSCALED>   { using type    =   int16x4 ;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16A16_UINT   >   { using type    =   uint16x4;   };
    template<>  struct TypeOfFormat<FMT_R16G16B16A16_SINT   >   { using type    =   int16x4 ;   };

    template<>  struct TypeOfFormat<FMT_R32_UINT            >   { using type    =   uint32_t;   };
    template<>  struct TypeOfFormat<FMT_R32_SINT            >   { using type    =   int32_t;    };

    template<>  struct TypeOfFormat<FMT_R32G32_UINT         >   { using type    =   uint32x2;   };
    template<>  struct TypeOfFormat<FMT_R32G32_SINT         >   { using type    =   int32x2 ;   };

    template<>  struct TypeOfFormat<FMT_R32G32B32_UINT      >   { using type    =   uint32x3;   };
    template<>  struct TypeOfFormat<FMT_R32G32B32_SINT      >   { using type    =   int32x3 ;   };

    template<>  struct TypeOfFormat<FMT_R32G32B32A32_UINT   >   { using type    =   uint32x4;   };
    template<>  struct TypeOfFormat<FMT_R32G32B32A32_SINT   >   { using type    =   int32x4 ;   };

    template<>  struct TypeOfFormat<FMT_R16_FLOAT           >   { using type    =   half;       };
    template<>  struct TypeOfFormat<FMT_R16G16_FLOAT        >   { using type    =   half2;      };
    template<>  struct TypeOfFormat<FMT_R16G16B16_FLOAT     >   { using type    =   half3;      };
    template<>  struct TypeOfFormat<FMT_R16G16B16A16_FLOAT  >   { using type    =   half4;      };

    template<>  struct TypeOfFormat<FMT_R32_FLOAT           >   { using type    =   float;      };
    template<>  struct TypeOfFormat<FMT_R32G32_FLOAT        >   { using type    =   float2;     };
    template<>  struct TypeOfFormat<FMT_R32G32B32_FLOAT     >   { using type    =   float3;     };
    template<>  struct TypeOfFormat<FMT_R32G32B32A32_FLOAT  >   { using type    =   float4;     }; 

    template<>  struct TypeOfFormat<FMT_D16_UNORM           >   { using type    =   float16;    };
    template<>  struct TypeOfFormat<FMT_D32_UNORM           >   { using type    =   float;      };

}