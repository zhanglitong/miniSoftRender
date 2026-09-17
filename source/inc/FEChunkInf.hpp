#pragma     once

#include    <stdint.h>
#include    "FEDefine.h"
namespace   FE
{
    /// <summary>
    /// 该接口务必不要擅自修复，会影响文件序列化
    /// </summary>
    class   FE_API  FEChunkInf
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
                /// 标记是否有子对象,同时表达个数范围，0: 无,1,uint8,2,uint16,3,uint32
                /// </summary>
                uint64_t    _hasChild:2;
                /// <summary>
                /// 子类使用，慎重!!!!，默认值都是1
                /// </summary>
                uint64_t    _flags:13;
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
            _type       =   type;
            _flags      =   flags;
            _length     =   len;  
            _hasChild   =   0;
            _hasVersion =   0;
        }
    };

    /// <summary>
    /// 默认可以满足大部分场景，用来控制是存在某个状态/成员/变量
    /// </summary>
    struct  FEChunkBits
    {
        union
        {
            struct 
            {
                uint16  _bit0   :1;
                uint16  _bit1   :1;
                uint16  _bit2   :1;
                uint16  _bit3   :1;
                uint16  _bit4   :1;
                uint16  _bit5   :1;
                uint16  _bit6   :1;
                uint16  _bit7   :1;
                uint16  _bit8   :1;
                uint16  _bit9   :1;
                uint16  _bit10  :1;
                uint16  _bit11  :1;
                uint16  _bit12  :1;
                uint16  _bit13  :1;
            };
            uint16  _value;
        };
        FEChunkBits(uint16 flags = 0)
        {
            _bit0       =   flags>>0 ;
            _bit1       =   flags>>1 ;
            _bit2       =   flags>>2 ;
            _bit3       =   flags>>3 ;
            _bit4       =   flags>>4 ;
            _bit5       =   flags>>5 ;
            _bit6       =   flags>>6 ;
            _bit7       =   flags>>7 ;
            _bit8       =   flags>>8 ;
            _bit9       =   flags>>9 ;
            _bit10      =   flags>>10;
            _bit11      =   flags>>11;
            _bit12      =   flags>>12;
            _bit13      =   flags>>13;
        }
    };
}
