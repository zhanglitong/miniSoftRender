#pragma     once
#include    "FEObject.h"
#include    "FEPlatform.h"
#include    "FEInput.hpp"
#include    "FEMessage.hpp"

namespace   FE
{
    class   FEApp
        :public FEObject
        ,public FEInput
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
            :FEObject(ctx)
        {}
        FEApp(const FEApp& other)
            :FEObject(other)
        {
        }
        virtual ~FEApp()
        {}
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
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
        /// 
        /// </summary>
        virtual void    destroy()   =   0;
        virtual void    renderOneFrame()    =   0;
        virtual void    onMessage(const FEMessage& msg) override 
        {
            if (_cInfo._notify)
                _cInfo._notify(msg);
        }

    };
    using   App      =   SharedPtr<FEApp>;
}