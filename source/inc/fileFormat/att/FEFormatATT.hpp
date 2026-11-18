#pragma     once
#include    "../../FEFileFormat.hpp"

namespace   FE
{
    class   FEATTFomat
    {
    public:
        static  Formats formats(const CLSId& id)
        {
            FEFileFormat    fmtAtt(".att","1.0.0.0","AVEVA RVM Attribute File!");
            fmtAtt._loaderId    =   id;
            fmtAtt._type        =   FEFileFormat::DT_Attribute;
            fmtAtt._mode        =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            FEFileFormat    fmtAttUpper(".ATT","1.0.0.0","AVEVA RVM Attribute File!");
            fmtAttUpper._loaderId   =   id;
            fmtAttUpper._type       =   FEFileFormat::DT_Attribute;
            fmtAttUpper._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            return  {fmtAtt,fmtAttUpper};
        }
    };
}
