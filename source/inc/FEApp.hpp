#pragma     once
#include    "FEObject.h"
#include    "FEPlatform.h"
#include    "FEInput.hpp"
#include    "FEMessage.hpp"
#include    "FEWindow.hpp"

namespace   FE
{
    class   FEApp :public FEWindow
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
        CreateInfo  _cInfo;
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
            if (_cInfo._notify)
                _cInfo._notify(msg);
        }
    };
    using   App      =   SharedPtr<FEApp>;
}
