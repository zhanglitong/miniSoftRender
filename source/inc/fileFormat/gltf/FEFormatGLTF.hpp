#pragma     once
#include    "../../FEFileFormat.hpp"
#include    "../../FEWriterHelper.hpp"
#include    "../../FEReaderHelper.hpp"

namespace   FE
{
    class   FEGLTFFormat
    {
    public:
        static  Formats formats(const CLSId& id)
        {
            FEFileFormat    fmtText(".gltf","1.0.0.0","GLTF text Format!");
            fmtText._loaderId   =   id;
            fmtText._type       =   FEFileFormat::DT_Model;
            fmtText._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            FEFileFormat    fmtBin(".glb","1.0.0.0","GLTF binary Format!");
            fmtBin._loaderId    =   id;
            fmtBin._type        =   FEFileFormat::DT_Model;
            fmtBin._mode        =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            return  {fmtText,fmtBin};
        }
    };
}
