#pragma     once
#include    "../../FEFileFormat.hpp"

namespace   FE
{
    /// <summary>
    /// PCO 点云八叉树格式(由 tool.las2pco 生成)
    /// </summary>
    class   FEPCOFormat
    {
    public:
        static  Formats formats(const CLSId& id)
        {
            FEFileFormat    fmtPco(".pco","1.0.0.0","PCO Point Cloud!");
            fmtPco._loaderId    =   id;
            fmtPco._type        =   FEFileFormat::DT_Model;
            fmtPco._mode        =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            FEFileFormat    fmtPcoUpper(".PCO","1.0.0.0","PCO Point Cloud!");
            fmtPcoUpper._loaderId   =   id;
            fmtPcoUpper._type       =   FEFileFormat::DT_Model;
            fmtPcoUpper._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;

            return  {fmtPco,fmtPcoUpper};
        }
    };
}
