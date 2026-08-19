#pragma     once
#include    "../FEObject.h"
#include    "../FENotify.hpp"
#include    "../FEResult.hpp"
#include    "../FEContext.hpp"
#include    "../FEKeyValues.hpp"


namespace   FE
{
    using   Handle      =   void*;
    using   Handles     =   std::vector<Handle>;
    using   PCSTRs      =   std::vector<const char*>;

    class   FEDevice;
    /// <summary>
    /// Render System Object
    /// </summary>
    class   RSObject : public FENotify
    {
    public:
        RSObject(FEContext& ctx);

        RSObject(const RSObject& other);

        virtual ~RSObject();

        /// <summary>
        /// 对象是否是有效的
        /// </summary>
        virtual bool    isValid() const
        {
            return  false;
        }
        /// <summary>
        /// 如果对象是在引擎创建之前创建，函数中调用渲染相关函数会报错
        /// 所以需要在创建引擎成功后才能在调用渲染相关函数
        /// 该函数在引擎成功创建后调用
        /// </summary>
        virtual void    initialize() 
        {}
        /// <summary>
        /// 如果对象是在引擎销毁之前创建，
        /// </summary>
        virtual void    destroy() 
        {}
        /// <summary>
        /// 对象句柄,具体实现的原始对象
        /// </summary>
        virtual Handle  native()  const 
        {
            return  Handle(nullptr);
        }
    };

    template<typename TNative,typename TInterface>   
    class   TRSObject:public TInterface
    {
    public:
        TNative     _native;
    public:
        template<class ... Args>
        TRSObject(FEContext& ctx,TNative native = {},Args&&... args)
            :TInterface(ctx,std::forward<Args>(args)...)
            ,_native(native)
        {}
        /// <summary>
        /// 对象是否是有效的
        /// </summary>
        virtual bool    isValid() const override
        {
            return  _native != nullptr;
        }
        /// <summary>
        /// 如果对象是在引擎创建之前创建，函数中调用渲染相关函数会报错
        /// 所以需要在创建引擎成功后才能在调用渲染相关函数
        /// 该函数在引擎成功创建后调用
        /// </summary>
        virtual void    initialize() override
        {}
        /// <summary>
        /// 如果对象是在引擎销毁之前创建，
        /// </summary>
        virtual void    destroy() override
        {}
        /// <summary>
        /// 对象句柄,具体实现的原始对象
        /// </summary>
        virtual Handle  native()  const override
        {
            return  Handle(_native);
        }
    };
}
