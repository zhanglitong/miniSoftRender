#pragma     once
#include    "../FEMath.hpp"
#include    "../FEObject.h"

namespace FE
{   
    /// <summary>
    /// 封装数组对象
    /// </summary>
    /// <typeparam name="TValue"></typeparam>
    template<typename TValue>
    class  TValueArray 
        : public FEObject
    {
    public:
        using   Values  =   std::vector<TValue>;
    public:
        TValueArray(FEContext& ctx)
            :FEObject(ctx)
        {}
        TValueArray(const TValueArray & other)
            :FEObject(other)
        {
            _values     =   other._values;
        }
        ~TValueArray()   =   default;

        Values&         values()
        {
            return  _values;
        }
        const Values&   values() const
        {
            return  _values;
        }

    protected:
        Values  _values;
    };

    using   RealsObject =   SharedPtr<TValueArray<real>>;
}
