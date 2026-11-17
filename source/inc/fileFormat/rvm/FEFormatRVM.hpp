#pragma     once
#include    "../../FEFileFormat.hpp"

namespace   FE
{
    class   FERVMFomat
    {
    public:
        static  Formats formats(const CLSId& id)
        {
            FEFileFormat    fmtRvm(".rvm","1.0.0.0","AVEVA RVM Model!");
            fmtRvm._loaderId    =   id;
            fmtRvm._type        =   FEFileFormat::DT_Model;
            fmtRvm._mode        =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            FEFileFormat    fmtRvmUpper(".RVM","1.0.0.0","AVEVA RVM Model!");
            fmtRvmUpper._loaderId   =   id;
            fmtRvmUpper._type       =   FEFileFormat::DT_Model;
            fmtRvmUpper._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            return  {fmtRvm,fmtRvmUpper};
        }
    };
}
