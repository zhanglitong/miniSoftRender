#pragma     once
#include    "../inc/FEImage.h"
#include    "image/FEBmpWriter.hpp"
#include    "image/FEBmpReader.hpp"

namespace   FE
{
    class   FEImageHelper
    {
    public:
        static  bool    saveBmp(FEImage& image,const char* fileName)
        {
            return  FEBmpWriter::save(fileName,image);
        }
    };

}
