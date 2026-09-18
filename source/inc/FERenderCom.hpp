#pragma     once

#include    "graphic/FEFactoryRender.hpp"
#include    "material/FEMaterialLibrary.hpp"
#include    "FEInputCom.hpp"


namespace   FE
{
    /// <summary>
    /// 该实现逻辑简单，开发者可以自由发挥更新与渲染函数
    /// </summary>
    class   FERenderCom :public FEFactoryRender
    {
    public:
        FERenderCom(FEContext& ctx)
            :FEFactoryRender(ctx)
        {}
        FERenderCom(const FERenderCom& other)
            :FEFactoryRender(other)
        {}
        /// <summary>
        /// 如果需要
        /// </summary>
        /// <returns></returns>
        virtual Input   inputComponent() 
        {
            return  nullptr;
        }
        /// <summary>
        /// 每一帧调用
        /// </summary>
        /// <param name="cmd"></param>
        virtual void    update(CMDPtr ) override
        {}
        /// <summary>
        /// 每一帧调用
        /// </summary>
        /// <param name="cmd"></param>
        virtual void    render(CMDPtr cmd) override
        {}
        /// <summary>
        /// 销毁前一定调用该函数，解除数据对工厂的引用计数
        /// 才能正确的释放数据
        /// </summary>
        virtual void    destroy() override
        {}
    };
    using   FactorySimple   =   SharedPtr<FERenderCom>;
}

