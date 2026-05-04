
#include    "../inc/FEObject.h"

namespace   FE
{
    const char*     FEObject::className() const          
    {                                                  
        return  "FEObject";                           
    }                                                  
    const FEUuid&   FEObject::classId() const
    {                                                  
        return  UUIDOF(FEObject);                    
    }
}