#pragma     once
#include    "../FEMath.hpp"

namespace   FE
{
    struct  FECmdArray
    {
        uint    count;
        uint    primCount;
        uint    first;
        uint    baseInstance;
    };

    using   CmdArrays   =   std::vector<FECmdArray>;
    struct  FECmdIndex
    {
        uint    count;
        uint    primCount;
        uint    firstIndex;
        int     baseVertex;
        uint    baseInstance;
    };
    using   CmdIndexs   =   std::vector<FECmdIndex>;
}