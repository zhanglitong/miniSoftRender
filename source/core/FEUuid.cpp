
#include    "../inc/FEUuid.h"
#include    <assert.h>
#include    <chrono>
#include    <map>
#include    <vector>
#include    <sstream>
#include    <random>
#include    <string>

namespace   FE
{
    inline  auto    genUUID()
    {
        GUID  guid;
        /// 
        thread_local  std::random_device rd;
        thread_local  std::mt19937_64 e2(rd());
        // 
        uint64_t    ab  =   e2();
        uint64_t    cd  =   e2();

        // 
        uint16_t data3 = static_cast<uint16_t>((ab >> 16) & 0xFFFF);
        data3 = (data3 & 0x0FFF) | 0x4000; 

        // 
        uint8_t data4_0 = static_cast<uint8_t>((cd >> 56) & 0xFF);
        data4_0 = (data4_0 & 0x3F) | 0x80;

        // 
        guid.Data1      =   static_cast<uint32_t>(ab >> 32);
        guid.Data2      =   static_cast<uint16_t>(ab >> 16);
        guid.Data3      =   data3;

        guid.Data4[0]   =   data4_0;
        guid.Data4[1]   =   static_cast<uint8_t>(cd >> 48);
        guid.Data4[2]   =   static_cast<uint8_t>(cd >> 40);
        guid.Data4[3]   =   static_cast<uint8_t>(cd >> 32);
        guid.Data4[4]   =   static_cast<uint8_t>(cd >> 24);
        guid.Data4[5]   =   static_cast<uint8_t>(cd >> 16);
        guid.Data4[6]   =   static_cast<uint8_t>(cd >> 8);
        guid.Data4[7]   =   static_cast<uint8_t>(cd);
        return  guid;
    }

    FEUuid  FEUuid::create()
    {
        return  FEUuid(genUUID());
    }
    FEUuid  FEUuid::from(const char* pStr,bool* pResult)
    {
        FEUuid  result;
        
        int     iResult =   sscanf_s( pStr
                                    , "{%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}"
                                    , &result._guid.Data1
                                    , &result._guid.Data2
                                    , &result._guid.Data3
                                    , &result._guid.Data4[0] 
                                    , &result._guid.Data4[1]
                                    , &result._guid.Data4[2]
                                    , &result._guid.Data4[3]
                                    , &result._guid.Data4[4]
                                    , &result._guid.Data4[5]
                                    , &result._guid.Data4[6]
                                    , &result._guid.Data4[7]);
        if (pResult)
            *pResult    =   iResult == 11;
        assert(iResult == 11);
        if (iResult == 11)
            return  result;
        else
            return  {};
    }
}
