#pragma     once

#include    "FEDefine.h"

#ifndef GUID_DEFINED
#define GUID_DEFINED 1
    typedef struct _GUID
    {
        unsigned int   Data1;
        unsigned short Data2;
        unsigned short Data3;
        unsigned char  Data4[ 8 ];
    } GUID;
    const GUID  GUID_NULL   =   {0,0,0,{0,0,0,0,0,0,0,0}};

#endif

namespace   FE
{
    class   FE_API  FEUuid
    {
    public:
        union
        {
            uint64  _64[2];
            uint32  _32[4];
            uint16  _16[8];
            uint8   _8[16];
            GUID    _guid;
        };
    public:
        FEUuid()
        {
            _64[0]  =   0;
            _64[1]  =   0;
        }
        FEUuid(const GUID& guid)
        {
            _guid   =   guid;
        }
        /// <summary>
        /// 赋值运算
        /// </summary>
        /// <param name="right"></param>
        /// <returns></returns>
        inline  auto&   operator = (const FEUuid& right)
        {
            if (this == &right)
                return *this;
            _64[0]   =   right._64[0];
            _64[1]   =   right._64[1];
            return *this;
        }
        inline  bool    operator ==(const FEUuid& right) const
        {              
            return  _64[0] == right._64[0] && _64[1] == right._64[1] ;
        }
        inline  bool    operator !=(const FEUuid& right) const
        {
            return  _64[0] != right._64[0] || _64[1] != right._64[1] ;
        }
        /// <summary>
        /// 大小比较,大于
        /// </summary>
        /// <param name="right"></param>
        /// <returns></returns>
        inline  bool    operator > (const FEUuid& right) const
        {
            if (_64[0] != right._64[0])
                return  _64[0] > right._64[0];
            else
                return  _64[1] > right._64[1];
        }
        inline  bool    operator < (const FEUuid& right) const
        {
            if (_64[0] != right._64[0])
                return  _64[0] < right._64[0];
            else
                return  _64[1] < right._64[1];
        }
        /// <summary>
        /// 与GUID的赋值运算
        /// </summary>
        /// <param name="guid"></param>
        /// <returns></returns>
        inline  auto&   operator = (const GUID& guid)
        {
            _guid   =   guid;
            return *this;
        }
        inline  bool    operator ==(const GUID& guid) const
        {
            return ((*this) == guid);
        }
        inline  bool    operator !=(const GUID& guid) const
        {
            return !((*this) == guid);
        }
        inline  operator    GUID() const
        {
            return  _guid;
        }
    public:
        static  FEUuid  create();
        static  FEUuid  from(const char* uuid,bool* pResult = nullptr);
    };
}

