#pragma     once

#include    "FEDrawArray.hpp"
#include    "FEDrawElementUint8.hpp"
#include    "FEDrawElementUint16.hpp"
#include    "FEDrawElementUint32.hpp"

namespace   FE
{
    class   FEPrimitiveHelper
    {
    public:
        template<class T>
        static  Primitive   createIndex(FEContext& ctx, const std::vector<T>& data)
        {
            auto    nMax    =   maxValue(data);
            //if (nMax < (std::numeric_limits<uint8>::max)())
            //    return  createIndex8(ctx,data);
            if (nMax < (std::numeric_limits<uint16>::max)())
                return  createIndex16(ctx,data);
            else
                return  createIndex32(ctx,data);
        }
        template<class T>
        static  Primitive   createIndex8(FEContext& ctx,const std::vector<T>& data)
        {
            static_assert(is_any_type<T, uint8,  int8x2,  uint8x3, uint8x4
                                        ,uint16, uint16x2,uint16x3,uint16x4
                                        ,int16,  int16x2, int16x3, int16x4
                                        ,uint32, uint32x2,uint32x3,uint32x4
                                        ,int32,  int32x2, int32x3, uint32x4>);
            auto    pri     =   new FEDrawElementUint8(ctx);
            if constexpr(std::is_same_v<T,uint8>)
            {
                pri->index().resize(data.size());
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size());
            }
            else if constexpr(std::is_same_v<T,uint8x2>)
            {
                pri->index().resize(data.size() * 2);
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size() * sizeof(uint8x2));
            }
            else if constexpr(std::is_same_v<T,uint8x3>)
            {
                pri->index().resize(data.size() * 3);
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size() * sizeof(uint8x3));
            }
            else if constexpr(std::is_same_v<T,uint8x4>)
            {
                pri->index().resize(data.size() * 4);
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size() * sizeof(uint8x4));
            }
            else if constexpr(is_any_type<T,uint16,int16,int32,uint32>)
            {
                pri->index().resize(data.size());
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i]    =   uint8(data[i]);
                }
            }
            else if constexpr(is_any_type<T,uint16x2,int16x2,int32x2,uint32x2>)
            {
                pri->index().resize(data.size() * 2);
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i * 2 + 0]    =   uint8(data[i][0]);
                    pData[i * 2 + 1]    =   uint8(data[i][1]);
                }
            }
            else if constexpr(is_any_type<T,uint16x3,int16x3,int32x3,uint32x3>)
            {
                pri->index().resize(data.size() * 3);
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i * 2 + 0]    =   uint8(data[i][0]);
                    pData[i * 2 + 1]    =   uint8(data[i][1]);
                    pData[i * 3 + 2]    =   uint8(data[i][2]);
                }
            }
            else if constexpr(is_any_type<T,uint16x4,int16x4,int32x4,uint32x4>)
            {
                pri->index().resize(data.size() * 3);
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i * 2 + 0]    =   uint8(data[i][0]);
                    pData[i * 2 + 1]    =   uint8(data[i][1]);
                    pData[i * 3 + 2]    =   uint8(data[i][2]);
                    pData[i * 3 + 3]    =   uint8(data[i][3]);
                }
            }
            return  pri;
        }
        template<class T>
        static  Primitive   createIndex16(FEContext& ctx,const std::vector<T>& data)
        {
            static_assert(is_any_type<T, uint8,  int8x2,  uint8x3, uint8x4
                                        ,uint16, uint16x2,uint16x3,uint16x4
                                        ,int16,  int16x2, int16x3, int16x4
                                        ,uint32, uint32x2,uint32x3,uint32x4
                                        ,int32,  int32x2, int32x3, uint32x4>);
            auto    pri     =   new FEDrawElementUint16(ctx);
            if constexpr(std::is_same_v<T,uint16>)
            {
                pri->index().resize(data.size());
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size());
            }
            else if constexpr(std::is_same_v<T,uint16x2>)
            {
                pri->index().resize(data.size() * 2);
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size() * sizeof(uint16x2));
            }
            else if constexpr(std::is_same_v<T,uint16x3>)
            {
                pri->index().resize(data.size() * 3);
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size() * sizeof(uint16x3));
            }
            else if constexpr(std::is_same_v<T,uint16x4>)
            {
                pri->index().resize(data.size() * 4);
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size() * sizeof(uint16x4));
            }
            else if constexpr(is_any_type<T,uint8,int16,int32,uint32>)
            {
                pri->index().resize(data.size());
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i]    =   uint16(data[i]);
                }
            }
            else if constexpr(is_any_type<T,uint16x2,int16x2,int32x2,uint32x2>)
            {
                pri->index().resize(data.size() * 2);
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i * 2 + 0]    =   uint16(data[i][0]);
                    pData[i * 2 + 1]    =   uint16(data[i][1]);
                }
            }
            else if constexpr(is_any_type<T,uint16x3,int16x3,int32x3,uint32x3>)
            {
                pri->index().resize(data.size() * 3);
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i * 2 + 0]    =   uint16(data[i][0]);
                    pData[i * 2 + 1]    =   uint16(data[i][1]);
                    pData[i * 3 + 2]    =   uint16(data[i][2]);
                }
            }
            else if constexpr(is_any_type<T,uint16x4,int16x4,int32x4,uint32x4>)
            {
                pri->index().resize(data.size() * 3);
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i * 2 + 0]    =   uint16(data[i][0]);
                    pData[i * 2 + 1]    =   uint16(data[i][1]);
                    pData[i * 3 + 2]    =   uint16(data[i][2]);
                    pData[i * 3 + 3]    =   uint16(data[i][3]);
                }
            }
            return  pri;
        }
        template<class T>
        static  Primitive   createIndex32(FEContext& ctx,const std::vector<T>& data)
        {
            static_assert(is_any_type<T, uint8,  int8x2,  uint8x3, uint8x4
                                        ,uint16, uint16x2,uint16x3,uint16x4
                                        ,int16,  int16x2, int16x3, int16x4
                                        ,uint32, uint32x2,uint32x3,uint32x4
                                        ,int32,  int32x2, int32x3, uint32x4>);
            auto    pri     =   new FEDrawElementUint32(ctx);
            if constexpr(std::is_same_v<T,uint32>)
            {
                pri->index().resize(data.size());
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size());
            }
            else if constexpr(std::is_same_v<T,uint32x2>)
            {
                pri->index().resize(data.size() * 2);
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size() * sizeof(uint32x2));
            }
            else if constexpr(std::is_same_v<T,uint32x3>)
            {
                pri->index().resize(data.size() * 3);
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size() * sizeof(uint32x3));
            }
            else if constexpr(std::is_same_v<T,uint32x4>)
            {
                pri->index().resize(data.size() * 4);
                auto    pData   =   pri->index().data();
                memcpy(pData,data.data(),data.size() * sizeof(uint32x4));
            }
            else if constexpr(is_any_type<T,uint8,uint16,int16,int32>)
            {
                pri->index().resize(data.size());
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i]    =   uint32(data[i]);
                }
            }
            else if constexpr(is_any_type<T,uint8x2,int16x2,uint16x2,int16x2>)
            {
                pri->index().resize(data.size() * 2);
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i * 2 + 0]    =   uint32(data[i][0]);
                    pData[i * 2 + 1]    =   uint32(data[i][1]);
                }
            }
            else if constexpr(is_any_type<T,uint8x3,uint16x3,int16x3,int32x3>)
            {
                pri->index().resize(data.size() * 3);
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i * 2 + 0]    =   uint32(data[i][0]);
                    pData[i * 2 + 1]    =   uint32(data[i][1]);
                    pData[i * 3 + 2]    =   uint32(data[i][2]);
                }
            }
            else if constexpr(is_any_type<T,uint8x4,int16x4,uint16x4,int16x4>)
            {
                pri->index().resize(data.size() * 3);
                auto    pData   =   pri->index().data();
                for (size_t i = 0; i < data.size(); i++)
                {
                    pData[i * 2 + 0]    =   uint32(data[i][0]);
                    pData[i * 2 + 1]    =   uint32(data[i][1]);
                    pData[i * 3 + 2]    =   uint32(data[i][2]);
                    pData[i * 3 + 3]    =   uint32(data[i][3]);
                }
            }
            return  pri;
        }
    public:
        template<class T>
        static  uint        maxValue(const std::vector<T>& data)
        {
            if (data.empty())
                return  0;
            static_assert(is_any_type<T, uint8,  int8x2,  uint8x3, uint8x4
                                        ,uint16, uint16x2,uint16x3,uint16x4
                                        ,int16,  int16x2, int16x3, int16x4
                                        ,uint32, uint32x2,uint32x3,uint32x4
                                        ,int32,  int32x2, int32x3, uint32x4>);
            if constexpr(is_any_type<T,uint8,uint16,int16,uint32,int32>)
            {
                return  *std::max_element(data.begin(),data.end());
            }
            else
            {
                uint    nMax    =   0;
                for (auto& var : data)
                {
                    nMax    =   std::max((uint)var.maxValue(),nMax);
                }
                return  nMax;
            }
        }
    };
}

