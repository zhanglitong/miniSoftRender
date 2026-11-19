#pragma     once
#include    "../../FEFileFormat.hpp"

namespace   FE
{
    class   FELASFormat
    {
    public:
        static  Formats formats(const CLSId& id)
        {
            FEFileFormat    fmtLas(".las","1.0.0.0","LAS Point Cloud!");
            fmtLas._loaderId    =   id;
            fmtLas._type        =   FEFileFormat::DT_Model;
            fmtLas._mode        =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            FEFileFormat    fmtLasUpper(".LAS","1.0.0.0","LAS Point Cloud!");
            fmtLasUpper._loaderId   =   id;
            fmtLasUpper._type       =   FEFileFormat::DT_Model;
            fmtLasUpper._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            return  {fmtLas,fmtLasUpper};
        }
    };
}
