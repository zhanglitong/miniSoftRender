
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
        /// 梅森旋转算法（「Mersenne twister」）是一个伪随机数生成算法
        /// 由松本真和西村拓士在1997年提出来的,可以快速产生高质量的伪随机数
        /// 修正了古典随机数生成算法当中的很多缺陷。19937这个名字来源于周期长度为梅森素数
        /// 目前还有待进一步做算法验证
        thread_local  std::random_device rd;
        thread_local  std::mt19937_64 e2(rd());
        // 生成两个 64 位随机数
        uint64_t    ab  =   e2();
        uint64_t    cd  =   e2();

        // --- 修复 1: 严格遵循 RFC 4122 V4 规范 ---
        // 设置版本号 (Version 4): 将 Data3 的高 4 位设为 0100 (0x4)
        // 在大端序视角下，这是第 7 个字节
        uint16_t data3 = static_cast<uint16_t>((ab >> 16) & 0xFFFF);
        data3 = (data3 & 0x0FFF) | 0x4000; 

        // 设置变体号 (Variant): 将 Data4[0] 的高 2 位设为 10 (0x8)
        uint8_t data4_0 = static_cast<uint8_t>((cd >> 56) & 0xFF);
        data4_0 = (data4_0 & 0x3F) | 0x80;

        // --- 修复 2: 显式处理字节序 (使用位移代替 memcpy) ---
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
        auto    srcRef  =   pStr;
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