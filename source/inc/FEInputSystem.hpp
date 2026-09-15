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

        /// <summary>
        /// 根据Id查找
        /// </summary>
        /// <param name="objectId"></param>
        /// <returns></returns>
        inline  auto    query(const FEUuid& objectId)const ->Component 
        {
            return  _input.query(objectId).get();
        }
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
        /// <summary>
        /// 情况
        /// </summary>
        virtual void    clear() override
        {
            _input.clearObjects();
        }
        virtual void    onMessage(const FEMessage& msg)
        {
            /// 组件已经按照优先级排序
            /// 消息首先传递给优先级高的组件
            /// 如果消息被标记消费过，则不处理
            /// 消息是否被标记，由处理消息的组件决定
            for (auto var : _input.objects())
            {
                var->onMessage(msg);
                /// 是否被捕获
                if (var->captureObject() != nullptr)
                    break;
                /// 如果消费过，不在向下分发传递
                if (msg.isConsumed())
                    break;
            }
        }
        /// <summary>
        /// 获取当前的捕获对象，可用来判断是否被捕获
        /// </summary>
        const InputCom  captureObject() const
        {
            return  _capture;
        }
        /// <summary>
        /// 捕获 _capture != nullptr,说明已经被其他的组件捕获
        /// </summary>
        /// <param name="com"></param>
        inline  bool    setCapture(InputCom com)
        {
            if (_capture != nullptr)
                return  false;
            _capture    =   com;
            return  true;
        }
        inline  bool    releaseCapture(InputCom com)
        {
            if (com != _capture)
                return  false;
            _capture    =   nullptr;
            return  true;
        }
    protected:
        InputComs   _input;
        InputCom    _capture;
    };

    using   InputSys    =   SharedPtr<FEInputSystem>;
}
