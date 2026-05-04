#pragma     once

#include    "FEDefine.h"
#include    "FEUuid.h"

namespace   FE
{
    template<class CLASS>
    struct UUIDTraits {};

#define DEFINE_CLASS_UUID(CLASS, uuid)                                      \
    class CLASS;                                                            \
    template <> struct UUIDTraits<CLASS>                                    \
    {                                                                       \
    public:                                                                 \
        static const FEUuid&    ClassGUID()                                 \
        {                                                                   \
            static  FEUuid  sGuid   =   FEUuid::from(uuid);                 \
            return  sGuid;                                                  \
        }                                                                   \
    }

#define UUIDOF(CLASS)       FE::UUIDTraits<CLASS>::ClassGUID()

    DEFINE_CLASS_UUID(FEObject,"{00000000-1111-2222-3333-0123456789AB}");

    class   FE_API  FEObject :public std::enable_shared_from_this<FEObject>
    {
    public:
        FEObject()  =   default;
        virtual ~FEObject()   =   default;
    public:   
        inline  const FEUuid&   objectId() const
        {
            return  _id;
        }
        virtual const FEUuid&   classId() const;
        virtual const char*     className() const;
    protected:
        FEUuid  _id;
    };

    using   Object  =   SharedPtr<FEObject>;
    using   Objects =   std::vector<Object>;
}

