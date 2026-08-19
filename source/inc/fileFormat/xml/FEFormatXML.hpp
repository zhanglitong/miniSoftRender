#pragma     once
#include    "../../FEFileFormat.hpp"

namespace   FE
{
    class   FEFormatXML
    {
    public:
        static  Formats formats(const CLSId& id)
        {
            FEFileFormat    fmt(".xml","1.0.0.0","FE XML Format!");
            fmt._loaderId   =   id;
            fmt._type       =   FEFileFormat::DT_Model;
            fmt._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;
            return  {fmt};
        }
    };
}
