#pragma     once

#include    <stdint.h>
#include    "FEDefine.h"
namespace   FE
{
    /// <summary>
    /// 该接口务必不要擅自修复，会影响文件序列化
    /// </summary>
    class   FEChunkInf
    {
    public:
        friend  class   FEObject;
    public:
        union
        {
            struct 
            {
                /// <summary>
                ///  类型  0 - 8bit
                /// </summary>
                uint64_t    _type:8;
                /// <summary>
                /// 是否需要写入版本号,如果没有变化过，与基类一致，则可以不写入，减少体积
                /// </summary>
                uint64_t    _hasVersion:1;
                /// <summary>
                /// 标记是否有子对象
                /// </summary>
                uint64_t    _hasChild:1;
                /// <summary>
                /// 子类使用，慎重!!!!，默认值都是1
                /// </summary>
                uint64_t    _flags:14;
                /// <summary>
                /// 块长度信息,包含了 FEChunkInf 长度
                /// </summary>
                uint64_t    _length:40;
            };
            char        _uint8[8];
            uint16_t    _uint16[4];
            uint32_t    _uint32[2];
            uint64_t    _uint64;
        };
    public:  
        FEChunkInf(uint8_t type = 0,uint16_t flags = 0xFFFF,uint64_t len = 0)
        {
            _type       =   0;
            _flags      =   flags;
            _length     =   len;  
            _hasChild   =   0;
            _hasVersion =   0;
        }
    };
}
