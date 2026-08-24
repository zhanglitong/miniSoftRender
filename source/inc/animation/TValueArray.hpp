#pragma     once
#include    "../FEMath.hpp"
#include    "../FEObject.h"

namespace FE
{   
    /// <summary>
    /// 关键帧
    /// </summary>
    struct  KeyFrame
    {
        real    _time;
        KFValue _value;
    };

    struct  FrameValue
    {
        real    _t  =   0;
        KFValue _v  =   {};
    };
    using   FrameValues     =   std::vector<FrameValue>;
    /// <summary>
    /// 封装数组对象
    /// </summary>
    /// <typeparam name="TValue"></typeparam>
    template<typename TValue>
    class  TValueArray 
        : public FEObject
    {
    public:
        using   ValueType   =   TValue;
        using   Values      =   std::vector<TValue>;
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

    using   RealsObject     =   SharedPtr<TValueArray<real>>;
    using   Real2sObject    =   SharedPtr<TValueArray<real2>>;
    using   Real3sObject    =   SharedPtr<TValueArray<real3>>;
    using   Real4sObject    =   SharedPtr<TValueArray<real4>>;
    using   QuatrsObject    =   SharedPtr<TValueArray<quatr>>;

    using   FloatsObject    =   SharedPtr<TValueArray<float>>;
    using   Float2sObject   =   SharedPtr<TValueArray<float2>>;
    using   Float3sObject   =   SharedPtr<TValueArray<float3>>;
    using   Float4sObject   =   SharedPtr<TValueArray<float4>>;
    using   QuatfsObject    =   SharedPtr<TValueArray<quatf>>;

    using   BoolsObject     =   SharedPtr<TValueArray<uint8>>;    

    /// <summary>
    /// 这里不能随意更改
    /// </summary>
    using   ValueObject     =   std::variant<std::monostate,RealsObject,Real2sObject,Real3sObject,Real4sObject,QuatrsObject,
                                            FloatsObject,Float2sObject,Float3sObject,Float4sObject,QuatfsObject,
                                            BoolsObject>;
}
