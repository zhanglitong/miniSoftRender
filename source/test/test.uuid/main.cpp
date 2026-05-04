
#include    <iostream>
#include    <stdio.h>
#include    <unordered_set>
#include    <assert.h>
#include    "../../inc/FEUuid.h"

namespace   std
{
    template<>
    class   hash<FE::FEUuid>
    {
    public:
        inline  size_t operator()(const FE::FEUuid& key) const noexcept
        {
            constexpr size_t _FNV_offset_basis = 14695981039346656037ULL;
            constexpr size_t _FNV_prime        = 1099511628211ULL;
            size_t  val    =   _FNV_offset_basis;
            for (size_t i = 0; i < 4; ++i)
            {
                val ^=  static_cast<size_t>(key._32[i]);
                val *=  _FNV_prime;
            }
            return val;
        }
    };
}


using   namespace   FE;
int     main(int argc,char** argv)
{
    std::unordered_set<FEUuid>  datas;

    for (size_t i = 0; i < 1000000; i++)
    {
        auto    uuid    =   FEUuid::create();
        auto    itr     =   datas.find(uuid);
        if (itr != datas.end())
            assert(itr != datas.end());
        else
            datas.emplace(uuid);
    }
    return  0;
}
