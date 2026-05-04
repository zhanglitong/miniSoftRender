#pragma     once

#include    <vector>
#include    <stdio.h>
#include    <memory>
#include    <array>
#define     FE_API

#define     FE_API

template<typename T>
using   SharedPtr   =  std::shared_ptr<T>;

namespace   FE
{
    using   uint8   =   std::uint8_t;
    using   int8    =   std::int8_t;

    using   uint16  =   std::uint16_t;
    using   int16   =   std::int16_t;

    using   uint32  =   std::uint32_t;
    using   int32   =   std::int32_t;

    using   uint64  =   std::uint64_t;
    using   int64   =   std::int64_t;
    


    using   float16 =   std::uint16_t;
    using   float32 =   float;
    using   float64 =   double;
    using   real    =   double;
}
