#pragma     once

#include    <vector>
#include    <stdio.h>
#include    <memory>
#include    <array>
#include    <map>
#include    <set>
#include    <unordered_map>
#include    <unordered_set>
#include    <string>
#include    <variant>
#include    <type_traits>
#include    <algorithm>
#include    <numeric>

#include    <assert.h>
#include    "FEPlatform.h"
#include    "FEFloat16.hpp"
#include    "FEFloat32.hpp"
#include    "FEReference.hpp"
#include    "FERef.hpp"
#include    "FEConst.h"

#define     FE_API

template<typename T>
using   SharedPtr   =  FE::FERef<T>;

namespace   FE
{
    using   uint8   =   std::uint8_t;
    using   int8    =   std::int8_t;
    using   byte    =   unsigned char;

    using   uint16  =   std::uint16_t;
    using   int16   =   std::int16_t;

    using   uint32  =   std::uint32_t;
    using   uint    =   std::uint32_t;
    using   int32   =   std::int32_t;

    using   uint64  =   std::uint64_t;
    using   int64   =   std::int64_t;
    
    

    using   float64 =   double;
    using   real    =   double;
    using   String  =   std::string;
    using   Strings =   std::vector<String>;
    using   PCSTR   =   const char*;
    using   PCVOID  =   const void*;
    using   PVOID   =   void*;

    template<typename T, typename... Set>
    constexpr bool is_any_type = (std::is_same_v<T, Set> || ...);

    constexpr int8      MinInt8     =   (std::numeric_limits<int8>::min)();
    constexpr int16     MinInt16    =   (std::numeric_limits<int16>::min)();
    constexpr int32     MinInt32    =   (std::numeric_limits<int32>::min)();

    constexpr int8      MaxInt8     =   (std::numeric_limits<int8>::max)();
    constexpr int16     MaxInt16    =   (std::numeric_limits<int16>::max)();
    constexpr int32     MaxInt32    =   (std::numeric_limits<int32>::max)();

    constexpr uint8     MinUint8    =   (std::numeric_limits<uint8>::min)();
    constexpr uint16    MinUint16   =   (std::numeric_limits<uint16>::min)();
    constexpr uint32    MinUint32   =   (std::numeric_limits<uint32>::min)();

    constexpr uint8     MaxUint8    =   (std::numeric_limits<uint8>::max)();
    constexpr uint16    MaxUint16   =   (std::numeric_limits<uint16>::max)();
    constexpr uint32    MaxUint32   =   (std::numeric_limits<uint32>::max)();


}
