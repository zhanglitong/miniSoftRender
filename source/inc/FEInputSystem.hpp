#pragma     once

#include    "FEInputCom.hpp"
#include    "FEComponentSys.hpp"
#include    "FEObjectsTemplate.hpp"
namespace   FE
{
    /// 按照优先级排序,优先级相同的按照order排序
    constexpr   auto    InputComLessFunc = [](const InputCom& l, const InputCom& r)
    {
        if( l->priority().priority() == r->priority().priority())
            return  l->priority().order() < r->priority().order();
        else
            return  l->priority().priority() < r->priority().priority();
    };
    /// 推导出来类型
    using   InputComLess    =   decltype(InputComLessFunc);

    DEFINE_CLASS_UUID(FEInputSystem, "{FB707EC2-26C6-4C33-9CBC-825389F650FB}");

    class   FEInputSystem :public FEComponentSys
    {
        IMPLEMENT_CLASS_REFLECT(FEInputSystem)
    public:
        using InputComs = FEObjectsTemplate<InputCom, InputComLess>;
    public:
        FEInputSystem(FEContext& ctx)
            :FEComponentSys(ctx)
            ,_input(InputComLessFunc)
        {}

        FEInputSystem(const FEInputSystem& other)
            :FEComponentSys(other)
            ,_input(InputComLessFunc)
        {}

        virtual size_t  addObject(Component  com) override
        {
            if (com->cast<FEInputCom>())
                return  _input.addObject(com->cast<FEInputCom>());
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
            if (com->cast<FEInputCom>())
                return  _input.removeObject(com->cast<FEInputCom>());
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
        virtual void    onMessage(const FEMessage& msg)  override
        {
            for (auto var : _input.objects())
            {
                var->onMessage(msg);
            }
        }
    protected:
        InputComs   _input;
    };

    using   InputSys    =   SharedPtr<FEInputSystem>;
}
