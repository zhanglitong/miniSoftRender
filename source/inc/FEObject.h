#pragma     once

#include    <type_traits>
#include    "FEDefine.h"
#include    "FEUuid.h"

namespace   FE
{
    template<class CLASS>
    struct UUIDTraits :public std::false_type  
    {
        static const FEUuid&    ClassGUID()
        {
            return  FEUuid::zero();
        }
    };

#define DEFINE_CLASS_UUID(CLASS, uuid)                                      \
    class CLASS;                                                            \
    template <> struct UUIDTraits<CLASS> : public std::true_type            \
    {                                                                       \
    public:                                                                 \
        static const FEUuid&    ClassGUID()                                 \
        {                                                                   \
            static  FEUuid  sGuid   =   FEUuid::from(uuid);                 \
            return  sGuid;                                                  \
        }                                                                   \
    }

    ///
    /// 统一使用该宏定义,继承了FEObject 的类使用
    ///
#define IMPLEMENT_CLASS_REFLECT(class_name)                                 \
    public:                                                                 \
        virtual const char*     className() const override                  \
        {                                                                   \
            return  #class_name;                                            \
        }                                                                   \
        virtual const FEUuid&   classId() const override                    \
        {                                                                   \
            return  UUIDOF(class_name);                                     \
        }                                                                   \

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
    public:
        static  void*   operator    new(size_t size);
        static  void*   operator    new(size_t size, std::align_val_t align);
        static  void    operator    delete(void* ptr) noexcept;
        static  void    operator    delete(void* ptr, std::align_val_t align) noexcept;
    protected:
        FEUuid  _id;
    };

    using   Object  =   SharedPtr<FEObject>;
    using   Objects =   std::vector<Object>;
}

