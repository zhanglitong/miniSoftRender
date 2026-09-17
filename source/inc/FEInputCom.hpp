#pragma     once

#include    "FEMessage.hpp"
#include    "FEInput.hpp"
#include    "FEComponent.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEInputCom, "{4AAA639C-889E-4D10-B287-FB9C6F23BD1E}");

    class   FE_API FEInputCom 
        :public FEComponent
        ,public FEInput
    {
        IMPLEMENT_CLASS_REFLECT(FEInputCom) 
    public:
        using   Input   =   SharedPtr<FEInputCom>;
    public:
        FEInputCom(FEContext& ctx);

        FEInputCom(const FEInputCom& other);
        /// <summary>
        /// 获取捕获组件,是哪一个组件捕获了
        /// </summary>
        /// <returns></returns>
        Input   captureObject() const;
        /// <summary>
        /// 捕获消息输入，如果已经被其他系统捕获了，返回失败
        /// </summary>
        bool    setCapture() ;
        /// <summary>
        /// 释放捕获,如果不是当前对象捕获的，释放会失败返回false
        /// </summary>
        bool    releaseCapture();
        /// <summary>
        /// 需要被其他子类重写，处理消息
        /// </summary>
        /// <param name=""></param>
        void    onMessage(const FEMessage& ) override;
    };

    using   InputCom    =   SharedPtr<FEInputCom>;
}
