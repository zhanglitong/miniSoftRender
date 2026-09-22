#pragma     once
#include    "../FEComponentSys.hpp"
#include    "../FEObjectsTemplate.hpp"
#include    "FEConstraint.hpp"

namespace FE
{
    /// 按照优先级排序,优先级相同的按照order排序
    constexpr   auto    ConstComLessFunc = [](const Constraint& l, const Constraint& r)
    {
        if( l->priority().priority() == r->priority().priority())
            return  l->priority().order() < r->priority().order();
        else
            return  l->priority().priority() < r->priority().priority();
    };
    /// 推导出来类型
    using   ConstComLess    =   decltype(ConstComLessFunc);

    DEFINE_CLASS_UUID(FEConstraintSys, "{5ED9E0B6-D627-4E1A-BAE4-4716FD31052F}");
    class   FEConstraintSys : public FEComponentSys
    {
    public:
        using   ConstComs   =   FEObjectsTemplate<Constraint, ConstComLess>;
    public:
        IMPLEMENT_CLASS_REFLECT(FEConstraintSys)
    public:       
        FEConstraintSys(FEContext& ctx)
            :FEComponentSys(ctx)   
        {}         
        FEConstraintSys(const FEConstraintSys& other)            
            :FEComponentSys(other)         
        {}
        /// <summary>
        /// 根据Id查找
        /// </summary>
        /// <param name="objectId"></param>
        /// <returns></returns>
        inline  auto    query(const FEUuid& objectId)const ->Component 
        {
            return  _objects.query(objectId).get();
        }
        virtual size_t  addObject(Component  com) override
        {
            if (com->cast<FEConstraint>())
                return  _objects.addObject(com->cast<FEConstraint>());
            else
                return  0;
        }
        virtual size_t  addObjects(const Components&  coms) override
        {
            size_t  result = 0;
            for (auto var : coms)
            {
                result += addObject(var);
            }
            return  result;
        }
        virtual size_t  removeObject(Component com) override
        {
            if (com->cast<FEConstraint>())
                return  _objects.removeObject(com->cast<FEConstraint>());
            else
                return  0;
        }
        virtual size_t  removeObjects(const Components& coms) override
        {
            size_t  result  = 0;
            for (auto var : coms)
            {
                result += removeObject(var);
            }
            return  result;
        }
        /// <summary>
        /// 清空
        /// </summary>
        virtual void    clear() override
        {
            _objects.clearObjects();
        }
        virtual void    update(const real&)
        {}
    protected:
        ConstComs   _objects;
    };
}

