#pragma     once
#include    "FEObject.h"
#include    "FEPlatform.h"
#include    "FEInput.hpp"
#include    "FEMessage.hpp"
#include    "FEWindow.hpp"

namespace   FE
{
    class   FE_API FEApp :public FEWindow
    {
    public:
        struct  CreateInfo
        {
            int         _width      =   1280;
            int         _height     =   720;
            void*       _window     =   nullptr;
            void*       _appInst    =   nullptr;
            bool        _fullscreen =   false;
            MsgNotify   _notify     =   {};
        };
    protected:
        CreateInfo      _cInfo;
        FEMessageBus    _messageBus;
    public:
        FEApp(FEContext& ctx)
            :FEWindow(ctx)
        {}
        FEApp(const FEApp& other)
            :FEWindow(other)
        {
        }
        virtual ~FEApp()
        {}
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        /// <summary>
        /// 消息总线:按 MessageId 订阅通知(多代理),
        /// 订阅者先于 _notify/场景默认处理被派发(见 FEMessageBus 说明)
        /// </summary>
        inline  auto&   messageBus()
        {
            return  _messageBus;
        }
        virtual RectU32 rect() const override   =   0;
        /// <summary>
        /// 
        /// </summary>
        /// <param name="cInf"></param>
        virtual bool    setup(const CreateInfo& cInf)   =   0;
        /// <summary>
        /// 
        /// </summary>
        virtual void    run()   =   0;
        /// <summary>
        /// 获取当前应用的全路径
        /// </summary>
        /// <returns></returns>
        virtual String  filePathName()  =   0;
        virtual String  path()  =   0;
        /// <summary>
        /// 销毁
        /// </summary>
        virtual void    destroy()   =   0;
        /// <summary>
        /// 绘制一帧
        /// </summary>
        virtual void    renderOneFrame()    =   0;
        virtual void    onMessage(const FEMessage& msg) override
        {
            /// 先派发消息总线(订阅者先于默认处理,
            /// 如 MSG_CLOSE 时抢在场景销毁 device 之前释放资源)
            _messageBus.dispatch(msg);
            if (_cInfo._notify)
            {
                switch(msg.msgId())
                {
                case MSG_RESIZE:
                    {
                        _cInfo._width   =   ((const MsgResize&)msg)._info._size.x;
                        _cInfo._height  =   ((const MsgResize&)msg)._info._size.y;
                    }
                    break;
                }
                _cInfo._notify(msg);
            }

        }
    };
    using   App      =   SharedPtr<FEApp>;
}
