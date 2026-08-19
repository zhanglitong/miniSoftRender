
#include    <iostream>
#include    <stdio.h>
#include    <unordered_set>
#include    <assert.h>
#include    "../../inc/FEUuid.h"
#include    "../../inc/FENodeHelper.hpp"


using   namespace   FE;
int     main(int ,char** )
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
