#pragma     once
#include    "../../FEFileFormat.hpp"
#include    "../../FEWriterHelper.hpp"
#include    "../../FEReaderHelper.hpp"

namespace   FE
{
    struct  FEFepjHeader
    {
        struct  Header
        {
            char    _magic[4]   =   {'f','e','p','j'};
            uint    _version    =   MAKE_VERSION(1,0,0,0);
            uint64  _datetime   =   0;
            uint64  _nodeCnt    =   0;
            uint64  _objectCnt  =   0;
        };
        FEFepjHeader(bool cacheFromSystem )
        {
            UNUSED(cacheFromSystem);
        }
        bool    isValid() const
        {
            return  strncmp(_header._magic,"fepj",4) == 0;
        }
        size_t    write(FEWriter& writer)
        {
            return  writer.write(_header);
        }
        size_t  read(FEReader& reader)
        {
            return  reader.read(_header);
        }
    public:
        Header  _header;
    };

    class   FEFepjFormat
    {
    public:
        static  Formats formats(const CLSId& id)
        {
            FEFileFormat    fmt(".fepj","1.0.0.0","FE Buildin Format!");
            fmt._loaderId   =   id;
            fmt._type       =   FEFileFormat::DT_Model;
            fmt._mode       =   FEFileFormat::SM_FILE|FEFileFormat::SM_MEMORY;
            return  {fmt};
        }
    };
}
