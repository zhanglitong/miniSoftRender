
#include    "../inc/graphic/RSObject.h"

namespace   FE
{

    RSObject::RSObject(FEContext& ctx)
        :FENotify(ctx)
    {
    }

    RSObject::RSObject(const RSObject& other)
        :FENotify(other)
    {
    }
    RSObject::~RSObject()
    {
    }

}
