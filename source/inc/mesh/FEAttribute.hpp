#pragma     once

#include    <type_traits>
#include    <functional>
#include    "../FEObject.h"
#include    "../FEFormat.hpp"
#include    "../FEFormatHelper.hpp"
#include    "graphic/FEGraphicEnums.h"
namespace   FE
{
    class   FEAttribute
    {
    public:
        FEAttribute(FEInputSlot slot = IS_VERTEX_POS,FEFormat fmt = FMT_R32G32B32_FLOAT)
        {
            this->_slot     =   slot;
            this->_format   =   fmt;
        }
        /// <summary>
        /// 业务层命名，POS,NORMAL,UV,COLOR...
        /// </summary>
        FEInputSlot _slot   =   IS_VERTEX_POS;
        /// <summary>
        /// 属性数据的数据格式
        /// </summary>
        FEFormat    _format =   FMT_R8G8B8A8_UNORM;
    public:
        auto        slot() const
        {
            return  _slot;
        }
        auto        format() const
        {
            return  _format;
        }
        uint16      stride() const
        {
            return  FEFormatHelper::sizeOf(_format);
        }
    };

    using   Attrs   =   std::vector<FEAttribute>;
}

