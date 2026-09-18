#pragma     once

#include    "../FEInputCom.hpp"
#include    "../FECamera.hpp"

namespace   FE
{
    class   FE_API  FESceneBrowse:public FEInputCom
    {
    public:
        FESceneBrowse(FEContext& ctx);

        FESceneBrowse(const FESceneBrowse& other);

        virtual ~FESceneBrowse();
    protected:
        /// <summary>
        /// 需要被其他子类重写，处理消息
        /// </summary>
        /// <param name=""></param>
        void    onMessage(const FEMessage& ) override;
    protected:
        void    onLButtonDown(const MsgLButtonDown& msg);
        void    onRButtonDown(const MsgRButtonDown& msg);
        void    onMouseMove(const MsgMouseMove& msg);
        void    onMouseWheel(const MsgMouseWheel&);
        void    onKeyDown(const MsgKeyDown&);
        void    onKeyUp(const MsgKeyUp&);
    };


    using   SceneBrowse  =    SharedPtr<FESceneBrowse>;
}
