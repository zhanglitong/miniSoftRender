
#include    <iostream>
#include    <stdio.h>
#include    <unordered_set>
#include    <assert.h>
#include    "../../inc/FESmallVector.h"
#include    "../../inc/FEUuid.h"


using   namespace   FE;
int     main(int argc,char** argv)
{
    TSmallVector<FEUuid>    tt;

    for (size_t i = 0; i < 6; i++)
    {
        tt.push_back(FEUuid::create());
    }
    tt.resize(100);
    return  0;
}
